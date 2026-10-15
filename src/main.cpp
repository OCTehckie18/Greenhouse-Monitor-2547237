#include <Arduino.h>

/*********************************************************************
  Smart Greenhouse - Blynk IoT + Keypad Calculator + Servo Vent
  Board : ESP32 DevKit C (Wokwi)

  Blynk Datastreams
    V0 = Temperature (double, 0-60 C)      -> Gauge
    V1 = Humidity    (double, 0-100 %)     -> Gauge
    V2 = Soil moisture (integer, 0-100 %)  -> Gauge
    V3 = Vent angle  (integer, 0-180 deg)  -> Slider (cloud override)

  Keypad layout            Keys
    1 2 3 A      digits 0-9
    4 5 6 B      A = +   B = -   C = *   D = /
    7 8 9 C      #  = compute (equals)
    * 0 # D      *  = clear
  Mode switch : press '#' on an empty/finished screen -> VENT mode
                In VENT mode: 1 = 0 deg, 2 = 90 deg, 3 = 180 deg,
                '#' returns to CALC mode.
*********************************************************************/

#define BLYNK_TEMPLATE_ID "TMPL3CdLAoUU8"
#define BLYNK_TEMPLATE_NAME "Greenhouse Monitor 2547237"
#define BLYNK_AUTH_TOKEN "vHIFPCU9Dk3Kn7FDm_hEap5AaFqsEoXJ"
#define BLYNK_PRINT Serial

#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <DHT.h>
#include <Keypad.h>
#include <LiquidCrystal_I2C.h>
#include <ESP32Servo.h>

// ---- Wi-Fi (Wokwi virtual access point) ----
char ssid[] = "Wokwi-GUEST";
char pass[] = "";

// ---- Pins ----
#define DHT_PIN 4
#define DHT_TYPE DHT22
#define SOIL_PIN 34 // potentiometer simulating soil moisture
#define SERVO_PIN 18

// ---- Objects ----
DHT dht(DHT_PIN, DHT_TYPE);
LiquidCrystal_I2C lcd(0x27, 16, 2);
Servo vent;
BlynkTimer timer;

// ---- Keypad ----
const byte ROWS = 4, COLS = 4;
char keys[ROWS][COLS] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}};
byte rowPins[ROWS] = {13, 14, 27, 26};
byte colPins[COLS] = {25, 33, 32, 19};
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// ---- State ----
enum Mode
{
  MODE_CALC,
  MODE_VENT
};
Mode mode = MODE_CALC;

String num1 = "", num2 = "", resultStr = "";
char op = 0;
bool resultShown = false;
int currentAngle = 0;

const int MAX_DIGITS = 4; // keeps 9999*9999 inside 32-bit long

// ---------------------------------------------------------------
// LCD helpers
// ---------------------------------------------------------------
void lcdLine(uint8_t row, String s)
{
  while (s.length() < 16)
    s += ' ';
  lcd.setCursor(0, row);
  lcd.print(s.substring(0, 16));
}

void showCalc()
{
  String expr = num1;
  if (op)
    expr += op;
  expr += num2;
  lcdLine(0, expr);
  lcdLine(1, resultShown ? ("= " + resultStr) : "");
}

void showVent()
{
  lcdLine(0, "VENT: 1=0 2=90 3=180");
  lcdLine(1, "Angle: " + String(currentAngle) + " deg");
}

void clearCalc()
{
  num1 = num2 = resultStr = "";
  op = 0;
  resultShown = false;
}

// ---------------------------------------------------------------
// Servo control (shared by keypad and cloud)
// ---------------------------------------------------------------
void setVent(int angle, const char *source)
{
  angle = constrain(angle, 0, 180);
  vent.write(angle);
  currentAngle = angle;
  Serial.printf("[VENT] %d deg (from %s)\n", angle, source);
  if (mode == MODE_VENT)
    showVent();
}

// ---------------------------------------------------------------
// Calculator
// ---------------------------------------------------------------
void computeResult()
{
  long a = num1.toInt();
  long b = num2.toInt();
  double r = 0;
  bool err = false;

  switch (op)
  {
  case '+':
    r = a + b;
    break;
  case '-':
    r = a - b;
    break;
  case '*':
    r = (double)a * b;
    break;
  case '/':
    if (b == 0)
      err = true;
    else
      r = (double)a / b;
    break;
  }

  if (err)
    resultStr = "Error: div by 0";
  else if (r == (long)r)
    resultStr = String((long)r);
  else
    resultStr = String(r, 2);

  resultShown = true;
  showCalc();
  Serial.println(num1 + String(op) + num2 + " = " + resultStr);
}

void handleCalcKey(char k)
{
  if (k >= '0' && k <= '9')
  {
    if (resultShown)
      clearCalc();
    String &target = op ? num2 : num1;
    if (target.length() < MAX_DIGITS)
      target += k;
    showCalc();
  }
  else if (k == 'A' || k == 'B' || k == 'C' || k == 'D')
  {
    if (resultShown)
      clearCalc();
    if (num1.length() == 0)
      return; // need first number
    if (k == 'A')
      op = '+';
    if (k == 'B')
      op = '-';
    if (k == 'C')
      op = '*';
    if (k == 'D')
      op = '/';
    showCalc();
  }
  else if (k == '*')
  { // clear
    clearCalc();
    showCalc();
  }
  else if (k == '#')
  {
    if (num1.length() && op && num2.length() && !resultShown)
    {
      computeResult();
    }
    else if (resultShown || num1.length() == 0)
    {
      clearCalc();
      mode = MODE_VENT; // switch to vent mode
      showVent();
    }
  }
}

void handleVentKey(char k)
{
  if (k == '1')
  {
    setVent(0, "keypad");
    Blynk.virtualWrite(V3, currentAngle);
  }
  else if (k == '2')
  {
    setVent(90, "keypad");
    Blynk.virtualWrite(V3, currentAngle);
  }
  else if (k == '3')
  {
    setVent(180, "keypad");
    Blynk.virtualWrite(V3, currentAngle);
  }
  else if (k == '#')
  {
    mode = MODE_CALC;
    showCalc();
  }
}

// ---------------------------------------------------------------
// Blynk
// ---------------------------------------------------------------
BLYNK_CONNECTED()
{
  Blynk.virtualWrite(V3, currentAngle); // sync slider with real position
}

// Cloud slider / button -> servo
BLYNK_WRITE(V3)
{
  setVent(param.asInt(), "cloud");
}

// Timed telemetry (called by BlynkTimer, never blocks loop)
void sendSensorData()
{
  float t = dht.readTemperature();
  float h = dht.readHumidity();
  int raw = analogRead(SOIL_PIN);
  int soil = map(raw, 0, 4095, 0, 100);

  if (!isnan(t))
    Blynk.virtualWrite(V0, t);
  if (!isnan(h))
    Blynk.virtualWrite(V1, h);
  Blynk.virtualWrite(V2, soil);

  Serial.printf("T=%.1fC  H=%.1f%%  Soil=%d%%\n", t, h, soil);
}

// ---------------------------------------------------------------
void setup()
{
  Serial.begin(115200);

  lcd.init();
  lcd.backlight();
  lcdLine(0, "Greenhouse IoT");
  lcdLine(1, "Connecting...");

  dht.begin();
  vent.setPeriodHertz(50);
  vent.attach(SERVO_PIN, 500, 2400);
  vent.write(0);

  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass); // blocks only until connected

  timer.setInterval(2000L, sendSensorData); // every 2 s, non-blocking

  showCalc();
}

void loop()
{
  Blynk.run(); // cloud commands
  timer.run(); // periodic telemetry

  char k = keypad.getKey(); // non-blocking local input
  if (k)
  {
    if (mode == MODE_CALC)
      handleCalcKey(k);
    else
      handleVentKey(k);
  }
}
