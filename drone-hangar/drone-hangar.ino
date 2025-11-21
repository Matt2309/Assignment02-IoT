/* AUTHORS:
   BOTTEGHI MATTEO     0001129907
   MULARONI MATTIA     0001126065
   MONTANARI NICOLAS   0001128064
*/

#include <Servo.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// --- CONFIGURAZIONE PIN ---
const int greenLedPins[] = {10, 11}; 
const int redLedPin = 12;            
const int btn = 4;                   
const int trigPin = 6;
const int echoPin = 7;
const int pirSensor = 2;
const int temperatureSensor = A0;
const int servoPin = 5;
const int NUM_greenLed = 2;

// --- PARAMETRI ---
#define D1_DIST_EXIT 20     
#define T1_TIME_EXIT 3000   
#define D2_DIST_LAND 10     
#define T2_TIME_LAND 3000   

// --- DEBUG ---
// Lascialo FALSE per usare il PIR. Se hai problemi HW, metti TRUE.
#define SKIP_PIR_CHECK false  

// --- STATI ---
enum State {
  DRONE_INSIDE,
  TAKE_OFF,
  DRONE_OUT,
  LANDING,
  ALARM
};

State statoCorrente = DRONE_INSIDE;
State statoPrecedente = ALARM; 

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

unsigned long persistenceTimer = 0; 
unsigned long debugPrintTimer = 0; 
unsigned long stateEnterTime = 0; 

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

  // --- CAMBIO STATO ---
  if (statoCorrente != statoPrecedente) {
    lcd.clear();
    stateEnterTime = millis(); 
    persistenceTimer = millis(); 

    Serial.print("STATE CHANGED TO: ");
    Serial.println(statoCorrente);

    switch (statoCorrente) {
      case DRONE_INSIDE:
        lcd.print("DRONE INSIDE");
        digitalWrite(greenLedPins[0], HIGH); 
        closeDoor();
        break;

      case TAKE_OFF:
        lcd.print("TAKE OFF");
        digitalWrite(greenLedPins[0], LOW); 
        openDoor();
        Serial.println("MSG: Attendo uscita (>20cm)...");
        break;

      case DRONE_OUT:
        lcd.print("DRONE OUT");
        closeDoor();
        Serial.println("MSG: Drone fuori. Invia LAND.");
        break;

      case LANDING:
        lcd.print("LANDING");
        openDoor();
        break;

      case ALARM:
        lcd.print("ALARM");
        digitalWrite(greenLedPins[0], LOW);
        digitalWrite(greenLedPins[1], LOW);
        digitalWrite(redLedPin, HIGH); 
        closeDoor();
        break;
    }
    statoPrecedente = statoCorrente;
  }

  // --- LOGICA CICLICA ---
  switch (statoCorrente) {

    case DRONE_INSIDE:
      if (cmdTakeOffReceived) {
        statoCorrente = TAKE_OFF;
        cmdTakeOffReceived = false; 
      }
      break;

    case TAKE_OFF:
      if(getDistance() <= D1_DIST_EXIT){
        persistenceTimer = millis(); 
      } else {
        if(millis() - persistenceTimer >= T1_TIME_EXIT){
          statoCorrente = DRONE_OUT;
        }
      }
      break;

    case DRONE_OUT:
      // --- MODIFICA LOGICA LANDING ---
      
      // Se ho ricevuto il comando LAND (anche in passato), inizio a monitorare il PIR
      if(cmdLandReceived){
        int pirVal = digitalRead(pirSensor);

        // Stampo lo stato del PIR continuamente per debuggare l'hardware
        static unsigned long lastPirPrint = 0;
        if (millis() - lastPirPrint > 500) {
           lastPirPrint = millis();
           Serial.print(">>> STO ASPETTANDO IL PIR... Valore attuale Arduino: "); 
           Serial.println(pirVal);
        }
        
        // Se il PIR scatta ORA, atterro
        if(pirVal == HIGH || SKIP_PIR_CHECK){ 
          Serial.println(">>> PIR RILEVATO! Atterraggio in corso...");
          statoCorrente = LANDING;
          cmdLandReceived = false; // Resetto SOLO ORA che ha funzionato
        } 
        // NOTA: Ho rimosso l'else che resettava il comando.
        // Ora se il PIR è 0, il comando RESTA attivo e riprova al prossimo giro.
      }
      break;

    case LANDING:
      if(getDistance() > D2_DIST_LAND){
        persistenceTimer = millis(); 
      } else {
        if(millis() - persistenceTimer >= T2_TIME_LAND){
          statoCorrente = DRONE_INSIDE;
        }
      }
      break;

    case ALARM:
      if (digitalRead(btn) == HIGH || cmdResetReceived) {
        statoCorrente = DRONE_INSIDE;
        cmdResetReceived = false; 
        Serial.println("MSG: Reset eseguito");
      }
      break;
  }
}

void taskTemperature() {
  if (statoCorrente == ALARM) return; 
  float temp = getTemperature();
  // Logica allarmi futura
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
      Serial.print("RX: ["); Serial.print(msg); Serial.println("]");
      
      if (msg.indexOf("TAKEOFF") >= 0) {
        cmdTakeOffReceived = true;
        Serial.println("CMD: TAKEOFF SET to TRUE");
      } 
      else if (msg.indexOf("LAND") >= 0) {
        cmdLandReceived = true;
        Serial.println("CMD: LAND SET to TRUE (Waiting for PIR...)");
      } 
      else if (msg.indexOf("RESET") >= 0) {
        cmdResetReceived = true;
        Serial.println("CMD: RESET SET to TRUE");
      }
    }
  }

  // Telemetria
  if (millis() - debugPrintTimer > 1000) {
    debugPrintTimer = millis();
    if (statoCorrente == TAKE_OFF || statoCorrente == LANDING) {
       Serial.print("Dist: "); Serial.println(getDistance());
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
  lcd.print("System Ready");
  Serial.println("System Ready");
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
  int readingSum = 0;
  for (int i = 0; i < 5; i++) {
    readingSum += analogRead(temperatureSensor);
  }
  float v = (readingSum / 5.0) * (5.0 / 1023.0); 
  return (v - 0.5) * 100.0; 
}

bool checkPir() {
  return digitalRead(pirSensor) == HIGH;
}

void openDoor() { myservo.write(90); }
void closeDoor() { myservo.write(0); }