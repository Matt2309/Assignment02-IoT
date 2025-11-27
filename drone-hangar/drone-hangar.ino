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
float TR, VR, ln, TX, T0, VRT;

// --- PARAMETRI ---
#define D1_DIST_EXIT 20
#define T1_TIME_EXIT 3000
#define D2_DIST_LAND 10
#define T2_TIME_LAND 3000

// PARAMETRI TEMPERATURA
#define TEMP1_PRE_ALARM 25
#define T3_TIME_PRE_ALARM 4000
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

// Variabili Comandi (Globali)
bool cmdTakeOffReceived = false;
bool cmdLandReceived = false;
bool cmdResetReceived = false;

// Timers
unsigned long timerFSM = 0;
unsigned long timerTemp = 0;
unsigned long timerBlink = 0;
unsigned long timerSerial = 0;

// Timer logica
unsigned long persistenceTimer = 0;
unsigned long debugPrintTimer = 0;
unsigned long stateEnterTime = 0;

// Timer temperatura
unsigned long tempPreAlarmTimer = 0;  // timer per i 4 secondi
unsigned long tempAlarmTimer = 0;     // timer per i 3 secondi

bool isPreAlarm = false;  // se settata a true blocca nuove operazioni


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

void setup() {
  initHardware();
  // Timeout rapido
  Serial.setTimeout(50);
}

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
//             TASK PRINCIPALI
// ==========================================

void taskFSM() {
  // --- LOGICA CICLICA ---
  switch (statoCorrente) {
    case DRONE_INSIDE:
      digitalWrite(greenLedPins[0], HIGH);
      if (cmdTakeOffReceived) {
        if (!isPreAlarm) {
          statoPrecedente = statoCorrente;
          statoCorrente = TAKE_OFF;
          lcd.clear();
          lcd.print("TAKE OFF");
          digitalWrite(greenLedPins[0], LOW);
          openDoor();
          Serial.println("MSG: TAKE_OFF");
        } else {
          Serial.println("MSG: ERROR TAKE_OFF -> PRE-ALARM");
        }
        cmdTakeOffReceived = false;
      }
      break;

    case TAKE_OFF:
      if (getDistance() <= D1_DIST_EXIT) {
        persistenceTimer = millis();
      } else {
        if (millis() - persistenceTimer >= T1_TIME_EXIT) {
          statoPrecedente = statoCorrente;
          statoCorrente = DRONE_OUT;
          lcd.clear();
          lcd.print("DRONE OUT");
          if (isPreAlarm) lcd.print(" (PRE)");
          closeDoor();
          Serial.println("MSG: DRONE_OUT");
        }
      }
      break;

    case DRONE_OUT:
      if (cmdLandReceived) {

        // Blocco sicurezza pre-allarme
        if (isPreAlarm) {
          Serial.println("MSG: ERROR LANDING -> PRE-ALARM");
          cmdLandReceived = false;
        } else {
          // Logica PIR
          int pirVal = digitalRead(pirSensor);

          // Stampo stato PIR ogni tanto
          static unsigned long lastPirPrint = 0;
          if (millis() - lastPirPrint > 500) {
            lastPirPrint = millis();
            Serial.print(">>> STO ASPETTANDO IL PIR... Valore: ");
            Serial.println(pirVal);
          }

          if (pirVal == HIGH) {
            Serial.println(">>> PIR RILEVATO! Atterraggio in corso...");
            Serial.println("MSG: LANDING");
            statoPrecedente = statoCorrente;
            statoCorrente = LANDING;
            lcd.clear();
            lcd.print("LANDING");
            openDoor();
            cmdLandReceived = false;
          }
        }
      }
      break;

    case LANDING:
      if (getDistance() > D2_DIST_LAND) {
        persistenceTimer = millis();
      } else {
        if (millis() - persistenceTimer >= T2_TIME_LAND) {
          statoPrecedente = statoCorrente;
          statoCorrente = DRONE_INSIDE;
          lcd.clear();
          lcd.print("DRONE INSIDE");
          Serial.println("MSG: DRONE_INSIDE");
          closeDoor();
          if (isPreAlarm) lcd.print(" (PRE)");
          digitalWrite(greenLedPins[0], HIGH);
        }
      }
      break;

    case ALARM:
      if (digitalRead(btn) == HIGH || cmdResetReceived) {
        if (getTemperature() < TEMP1_PRE_ALARM) {
          statoCorrente = statoPrecedente;
          lcd.clear();
          lcd.print(stateNames[statoCorrente]);
          Serial.print("MSG: ");
          Serial.print(stateNames[statoCorrente]);
          Serial.print("\n");
          digitalWrite(greenLedPins[0], HIGH);
          closeDoor();
          Serial.println("Reset eseguito e temperatura OK.");
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

  // --- 1. LOGICA ALLARME CRITICO (> 30°C per 3 sec) ---
  if (temp > TEMP2_ALARM) {
    if (tempAlarmTimer == 0) {
      tempAlarmTimer = millis();
    } else if (millis() - tempAlarmTimer >= T4_TIME_ALARM) {
      if (statoCorrente == DRONE_OUT) {
        Serial.println("MSG: ALARM");
      }
      statoPrecedente = statoCorrente;
      statoCorrente = ALARM;
      lcd.clear();
      lcd.print("ALARM");
      digitalWrite(greenLedPins[0], LOW);
      digitalWrite(greenLedPins[1], LOW);
      digitalWrite(redLedPin, HIGH);
      closeDoor();
      Serial.println("SISTEMA BLOCCATO PER TEMPERATURA! Premi bottone RESET.");
      // Manda messaggio se il drone è fuori
      
      tempAlarmTimer = 0;
    }
  } else {
    tempAlarmTimer = 0;
  }

  // --- 2. LOGICA PRE-ALLARME (>= 25°C per 4 sec) ---
  if (temp >= TEMP1_PRE_ALARM) {
    if (tempPreAlarmTimer == 0) {
      tempPreAlarmTimer = millis();
    } else if (millis() - tempPreAlarmTimer >= T3_TIME_PRE_ALARM) {  // CORRETTO QUI
      if (!isPreAlarm && statoCorrente != ALARM) {
        isPreAlarm = true;
        Serial.println("ATTENZIONE: Pre-Allarme Attivo (Temp Alta)");
        Serial.println("MSG: PRE_ALARM");
        if (statoCorrente == DRONE_INSIDE || statoCorrente == DRONE_OUT) {
          lcd.setCursor(0, 1);
          lcd.print("(PRE)");
        }
      }
    }
  } else {
    tempPreAlarmTimer = 0;  // CORRETTO NOME VARIABILE
    if (isPreAlarm) {
      isPreAlarm = false;
      lcd.setCursor(0, 0);
      lcd.clear();
      lcd.print("DRONE INSIDE");
      Serial.println("MSG: DRONE_INSIDE");
      Serial.println("INFO: Temperatura normalizzata. Pre-Allarme rimosso.");
    }
  }
}

void taskBlink() {
  if (statoCorrente == TAKE_OFF || statoCorrente == LANDING) {
    static unsigned long lastBlinkTime = 0;
    if (millis() - lastBlinkTime > 500) {
      lastBlinkTime = millis();
      int state = digitalRead(greenLedPins[1]);
      digitalWrite(greenLedPins[1], !state);
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

      if (msg.indexOf("TAKEOFF") >= 0) {
        cmdTakeOffReceived = true;
        Serial.println("CMD: TAKEOFF SET to TRUE");
      } else if (msg.indexOf("LAND") >= 0) {
        cmdLandReceived = true;
        Serial.println("CMD: LAND SET to TRUE");
      } else if (msg.indexOf("RESET") >= 0) {
        cmdResetReceived = true;
        Serial.println("CMD: RESET SET to TRUE");
      }
    }
  }

  // Telemetria
  if (millis() - debugPrintTimer > 1000) {
    debugPrintTimer = millis();
    // Debug Temperatura
    Serial.print("Temp: ");
    Serial.println(getTemperature());
    if (statoCorrente == TAKE_OFF || statoCorrente == LANDING) {
      Serial.print("MSG: DIST ");
      Serial.println(getDistance());
    }
  }
}

// ==========================================
//             FUNZIONI HARDWARE
// ==========================================

void initHardware() {
  Serial.begin(9600);
  lcd.init();
  lcd.backlight();

  //temp conversion
  T0 = 25 + 273.15;

  for (int i = 0; i < NUM_greenLed; i++) {
    pinMode(greenLedPins[i], OUTPUT);
  }
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
  if (duration == 0) return 999.0;
  return duration * 0.034 / 2.0;
}

float getTemperature() {
  VRT = (5.00 / 1023.00) * analogRead(temperatureSensor);
  VR = 5.00 - VRT;

  TR = VRT / (VR / R);

  ln = log(TR / RT0);
  TX = (1 / ((ln / B) + (1 / T0)));

  //return in Celsius
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