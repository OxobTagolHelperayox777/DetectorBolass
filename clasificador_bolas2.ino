#include <Wire.h>
#include <Adafruit_TCS34725.h>
#include <Servo.h>
#include <LiquidCrystal_I2C.h>

// ------------------ CONFIG ------------------
#define SERVO_PIN 9
#define LED_PIN 3

LiquidCrystal_I2C lcd(0x27, 16, 2); // Dirección típica

Adafruit_TCS34725 tcs = Adafruit_TCS34725(
  TCS34725_INTEGRATIONTIME_50MS,
  TCS34725_GAIN_4X);

Servo myServo;

// ------------------ POSICIONES SERVO ------------------
int posHome = 0;
int posBlanco = 90;
int posRojo = 0;
int posAzul = 90;
int posNegro = 180;

// ------------------ CONTROL ------------------
bool bolaDetectada = false;
bool esperandoRecogida = false;

unsigned long tiempoInicioRecogida = 0;
const unsigned long TIEMPO_RECOGIDA = 100;

// ------------------ CONTADORES ------------------
int contadorRojo = 0;
int contadorNegro = 0;
int contadorBlanco = 0;

int ultimoColor = -1; // 0=rojo, 1=negro, 2=blanco
unsigned long ultimoTiempoColor = 0;
const unsigned long BLOQUEO_REPETICION = 10000;

// ------------------ SETUP ------------------
void setup() {

  Serial.begin(9600);

  if (!tcs.begin()) {
    Serial.println("ERROR: No se detectó el TCS34725");
    while (1);
  }

  // LCD INIT
  lcd.init();
  lcd.backlight();

  lcd.setCursor(0, 0);
  lcd.print("Sistema listo");
  delay(2000);
  lcd.clear();

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);

  myServo.attach(SERVO_PIN);
  myServo.write(posHome);

  Serial.println("Sistema listo, servo recto esperando bola");
}

// ------------------ LOOP ------------------
void loop() {

  uint16_t r, g, b, c;
  tcs.getRawData(&r, &g, &b, &c);

  Serial.print("R: "); Serial.print(r);
  Serial.print(" G: "); Serial.print(g);
  Serial.print(" B: "); Serial.print(b);
  Serial.print(" C: "); Serial.println(c);

  if (!bolaDetectada && !esperandoRecogida) {

    int targetPos = detectarColor(r, g, b, c);

    if (targetPos != -1) {

      int color = -1;
      if (targetPos == posRojo) color = 0;
      if (targetPos == posNegro) color = 1;
      if (targetPos == posBlanco) color = 2;

      if (color != -1) {
        actualizarContador(color);
        actualizarPantalla();
      }

      myServo.write(targetPos);
      bolaDetectada = true;

      Serial.println("Color detectado, cilindro girado");
      delay(2000);
    }
  }

  if (bolaDetectada) {
    bolaDetectada = false;
    esperandoRecogida = true;
    tiempoInicioRecogida = millis();
  }

  if (esperandoRecogida) {
    if (millis() - tiempoInicioRecogida >= TIEMPO_RECOGIDA) {
      esperandoRecogida = false;
    }
  }
}

// ------------------ DETECCIÓN COLOR ------------------
int detectarColor(uint16_t r, uint16_t g, uint16_t b, uint16_t c) {

  if ((c > 620 && c < 1300) && r < 400 && r > 270 && g < 500 && g > 310 && b < 340) {
    Serial.println("Color: NEGRO");
    return posNegro;
  }
  else if (c > 1200 && r > 400 && g > 400 && b > 400) {
    Serial.println("Color: BLANCO");
    return posBlanco;
  }
  else if (r > g * 1.5 && r > b * 1.5) {
    Serial.println("Color: ROJO");
    return posRojo;
  }
  else if (c < 540 && r < 220 && r > 200 && g < 230 && b < 190) {
    Serial.println("Color: carton");
    return -1;
  }

  return -1;
}

// ------------------ CONTADOR ------------------
void actualizarContador(int color) {

  unsigned long ahora = millis();

  if (color == ultimoColor) {
    if (ahora - ultimoTiempoColor < BLOQUEO_REPETICION) {
      return;
    }
  }

  if (color == 0) {
    contadorRojo++;
    if (contadorRojo > 99) contadorRojo = 0;
  }

  if (color == 1) {
    contadorNegro++;
    if (contadorNegro > 99) contadorNegro = 0;
  }

  if (color == 2) {
    contadorBlanco++;
    if (contadorBlanco > 99) contadorBlanco = 0;
  }

  ultimoColor = color;
  ultimoTiempoColor = ahora;
}

// ------------------ LCD ------------------
void actualizarPantalla() {

  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("R:");
  lcd.print(contadorRojo);
  lcd.print(" N:");
  lcd.print(contadorNegro);

  lcd.setCursor(0, 1);
  lcd.print("B:");
  lcd.print(contadorBlanco);
}