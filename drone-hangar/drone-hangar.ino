/* AUTHORS:
   BOTTEGHI MATTEO     0001129907
   MULARONI MATTIA     0001126065
   MONTANARI NICOLAS   0001128064
*/

#include <Servo.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// --- CONFIGURAZIONE PIN ---
const int greenLedPins[] = { 10, 11 };
const int redLedPin = 12;
const int btn = 4;
const int trigPin = 6;
const int echoPin = 7;
const int pirSensor = 2;
const int temperatureSensor = A0;
const int servoPin = 5;
const int NUM_greenLed = 2;

// Thermistor parameters
#define RT0 1500
#define B 3977
#define R 60000
float VRT, VR, TR, ln, TX, T0;

// --- PARAMETRI ---
#define D1_DIST_EXIT 20
#define T1_TIME_EXIT 3000
#define D2_DIST_LAND 10
#define T2_TIME_LAND 3000

#define TEMP1_PRE_ALARM 25
#define T3_TIME_PRE_ALARM 40
#define TEMP2_ALARM 400
#define T4_TIME_ALARM 3000

// --- STATI ---
enum State {
  DRONE_INSIDE,
  TAKE_OFF,
  DRONE_OUT,
  LANDING,
  ALARM
};

const char* stateNames[] = {
  "DRONE_INSIDE",
  "TAKE_OFF",
  "DRONE_OUT",
  "LANDING",
  "ALARM"
};

State statoCorrente = DRONE_INSIDE;
State statoPrecedente = DRONE_INSIDE;

// --- OGGETTI ---
LiquidCrystal_I2C lcd(0x27, 16, 2);
Servo myservo;

// --- VARIABILI ---
unsigned long currentMillis;
bool cmdTakeOffReceived = false;
bool cmdLandReceived = false;
bool cmdResetReceived = false;

unsigned long timerFSM = 0;
unsigned long timerTemp = 0;
unsigned long timerSerial = 0;

unsigned long persistenceTimer = 0;
unsigned long debugPrintTimer = 0;
unsigned long tempPreAlarmTimer = 0;
unsigned long tempAlarmTimer = 0;
bool isPreAlarm = false;

// --- PROTOTIPI ---
void initHardware();
void taskFSM();
void taskTemperature();
void taskBlink();
void taskSerial();
float getDistance();
float getTemperature();
bool checkPir();
void openDoor();
void closeDoor();
void setState(State next, const char* serialMsg);

// ============================
// SETUP
// ============================

void setup() {
  initHardware();
  Serial.setTimeout(50);
}

// ============================
// LOOP
// ============================

void loop() {
  currentMillis = millis();

  if (currentMillis - timerFSM >= 100) {
    timerFSM = currentMillis;
    taskFSM();
  }

  if (currentMillis - timerTemp >= 500) {
    timerTemp = currentMillis;
    taskTemperature();
  }

  taskBlink();

  if (currentMillis - timerSerial >= 100) {
    timerSerial = currentMillis;
    taskSerial();
  }
}

// ==========================================
// FUNZIONI UTILI
// ==========================================

void setState(State next, const char* serialMsg) {
  statoPrecedente = statoCorrente;
  statoCorrente = next;
  lcd.clear();
  lcd.print(stateNames[next]);
  if (isPreAlarm) lcd.print(" (PRE)");
  Serial.print("MSG: ");
  Serial.println(serialMsg);
}

// ==========================================
// TASK PRINCIPALI
// ==========================================

void taskFSM() {
  switch (statoCorrente) {

    case DRONE_INSIDE:
      digitalWrite(greenLedPins[0], HIGH);
      if (cmdTakeOffReceived) {
        if (!isPreAlarm) {
          setState(TAKE_OFF, "TAKE_OFF");
          digitalWrite(greenLedPins[0], LOW);
          openDoor();
        } else {
          Serial.println("MSG: ERROR TAKE_OFF -> PRE-ALARM");
        }
        cmdTakeOffReceived = false;
      }
      break;

    case TAKE_OFF:
      if (getDistance() <= D1_DIST_EXIT) {
        persistenceTimer = millis();
      } else if (millis() - persistenceTimer >= T1_TIME_EXIT) {
        setState(DRONE_OUT, "DRONE_OUT");
        closeDoor();
      }
      break;

    case DRONE_OUT:
      if (cmdLandReceived) {
        static unsigned long lastPirCheck = 0;
        if (isPreAlarm) {
          Serial.println("MSG: ERROR LANDING -> PRE-ALARM");
          cmdLandReceived = false;
          break;
        } else if (checkPir()) {
          Serial.println("MSG: LANDING");
          setState(LANDING, "LANDING");
          openDoor();
        }
        static unsigned long lastPirPrint = 0;
        if (millis() - lastPirPrint > 500) {
          lastPirPrint = millis();
          Serial.println("MSG WAITING FOR DPD...");
        }
      }
      break;

    case LANDING:
      if (getDistance() > D2_DIST_LAND) {
        persistenceTimer = millis();
      } else if (millis() - persistenceTimer >= T2_TIME_LAND) {
        setState(DRONE_INSIDE, "DRONE_INSIDE");
        closeDoor();
        digitalWrite(greenLedPins[0], HIGH);
      }
      break;

    case ALARM:
      if (digitalRead(btn) == HIGH || cmdResetReceived) {
        if (getTemperature() < TEMP1_PRE_ALARM) {
          setState(statoPrecedente, stateNames[statoPrecedente]);
          digitalWrite(greenLedPins[0], HIGH);
          closeDoor();
        } else {
          Serial.println("Reset fallito - Temperatura ancora troppo alta!");
        }
        cmdResetReceived = false;
      }
      break;
  }
}

void taskTemperature() {
  if (statoCorrente == ALARM) return;

  float temp = getTemperature();

  if (temp > TEMP2_ALARM) {
    if (tempAlarmTimer == 0) tempAlarmTimer = millis();
    else if (millis() - tempAlarmTimer >= T4_TIME_ALARM) {
      setState(ALARM, "ALARM");
      digitalWrite(greenLedPins[0], LOW);
      digitalWrite(greenLedPins[1], LOW);
      digitalWrite(redLedPin, HIGH);
      closeDoor();
      tempAlarmTimer = 0;
    }
  } else {
    tempAlarmTimer = 0;
  }

  if (temp >= TEMP1_PRE_ALARM) {
    if (tempPreAlarmTimer == 0) tempPreAlarmTimer = millis();
    else if (!isPreAlarm && millis() - tempPreAlarmTimer >= T3_TIME_PRE_ALARM) {
      isPreAlarm = true;
      Serial.println("MSG: PRE_ALARM");
      lcd.setCursor(0, 1);
      lcd.print("(PRE)");
    }
  } else {
    tempPreAlarmTimer = 0;
    if (isPreAlarm) {
      isPreAlarm = false;
      setState(statoCorrente, stateNames[statoCorrente]);
    }
  }
}

void taskBlink() {
  static unsigned long lastBlinkTime = 0;

  if (statoCorrente == TAKE_OFF || statoCorrente == LANDING) {
    if (millis() - lastBlinkTime > 500) {
      lastBlinkTime = millis();
      digitalWrite(greenLedPins[1], !digitalRead(greenLedPins[1]));
    }
  } else {
    digitalWrite(greenLedPins[1], LOW);
  }
}

void taskSerial() {
  if (Serial.available() > 0) {
    String msg = Serial.readStringUntil('\n');
    msg.trim();
    msg.toUpperCase();

    if (msg.length() > 0) {
      Serial.print("RX: [");
      Serial.print(msg);
      Serial.println("]");

      if (msg.indexOf("TAKEOFF") >= 0) cmdTakeOffReceived = true;
      else if (msg.indexOf("LAND") >= 0) cmdLandReceived = true;
      else if (msg.indexOf("RESET") >= 0) cmdResetReceived = true;
    }
  }

  if (millis() - debugPrintTimer > 1000) {
    debugPrintTimer = millis();
    Serial.print("Temp: ");
    Serial.println(getTemperature());
    if (statoCorrente == TAKE_OFF || statoCorrente == LANDING) {
      Serial.print("MSG: DIST ");
      Serial.println(getDistance());
    }
  }
}

// ==========================================
// FUNZIONI HARDWARE
// ==========================================

void initHardware() {
  Serial.begin(9600);
  lcd.init();
  lcd.backlight();

  T0 = 25 + 273.15;

  for (int i = 0; i < NUM_greenLed; i++)
    pinMode(greenLedPins[i], OUTPUT);

  pinMode(redLedPin, OUTPUT);
  pinMode(btn, INPUT);
  pinMode(pirSensor, INPUT);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  myservo.attach(servoPin);
  closeDoor();

  lcd.clear();
  lcd.print("DRONE INSIDE");
  Serial.println("MSG: DRONE_INSIDE");
}

float getDistance() {
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  long duration = pulseIn(echoPin, HIGH, 25000);
  return duration == 0 ? 999.0 : duration * 0.034 / 2.0;
}

float getTemperature() {
  VRT = (5.00 / 1023.00) * analogRead(temperatureSensor);
  VR = 5.00 - VRT;
  TR = VRT / (VR / R);
  ln = log(TR / RT0);
  TX = 1 / ((ln / B) + (1 / T0));
  return TX - 273.15;
}

bool checkPir() {
  return digitalRead(pirSensor) == HIGH;
}

void openDoor() {
  Serial.println("Servo: OPENING...");
  myservo.attach(servoPin);
  myservo.write(180);
  delay(2000);
  myservo.write(90);
  myservo.detach();
}

void closeDoor() {
  Serial.println("Servo: CLOSING...");
  myservo.attach(servoPin);
  myservo.write(90);
  delay(2000);
  myservo.write(180);
  myservo.detach();
}
