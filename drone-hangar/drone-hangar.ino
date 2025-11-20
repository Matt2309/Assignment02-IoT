/* AUTHORS:
BOTTEGHI MATTEO     0001129907
MULARONI MATTIA     0001126065
MONTANARI NICOLAS   0001128064 
*/

#include <Servo.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

const int greenLedPins[] = {10, 11}; 
const int redLedPin = 12; 
const int btn = 4; 

const int trigPin = 6; 
const int echoPin = 7; 

const int pirSensor = 2;
const int temperatureSensor = A0;
const int servoPin = 5;
const int NUM_greenLed = 2; 

enum State {
    DRONE_INSIDE, 
    TAKE_OFF, 
    DRONE_OUT,
    LANDING,
    ALARM
  };
State lastState;

LiquidCrystal_I2C lcd(0x27, 16, 2);

Servo myservo; 



//funzione per inzializzazione
void initHardware(){
  Serial.begin(9600);
  
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0,0);
  lcd.print("System Init...");

  for (int i = 0; i < NUM_greenLed; i++) {
    pinMode(greenLedPins[i], OUTPUT);
  }
  pinMode(redLedPin, OUTPUT);

  //configurazione Sensori Digitali
  pinMode(btn, INPUT);
  pinMode(pirSensor, INPUT);

  //configurazione Sensore Distanza 
  pinMode(trigPin, OUTPUT); // PIN 6 invia segnale
  pinMode(echoPin, INPUT);  // PIN 7 legge ritorno

  //configurazione Servo
  myservo.attach(servoPin);
  myservo.write(0); //porta a 0 gradi

  lcd.clear();
  Serial.println("Inizializzazione completata. Avvio Test Loop...");
}

void setup() {
  initHardware();
  
}

void loop() {



  checkSerial();
}

void checkSerial() {
}