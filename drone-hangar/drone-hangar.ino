/* 
AUTHORS:
BOTTEGHI MATTEO     0001129907
MULARONI MATTIA     0001126065
MONTANARI NICOLAS   0001128064 
*/

#include <Servo.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

enum State {
    DRONE_INSIDE, 
    TAKE_OFF, 
    DRONE_OUT,
    LANDING,
    ALARM
  };
State lastState;

LiquidCrystal_I2C lcd(0x27, 16, 2);

void setup() {

}

void loop() {
  checkSerial();
}

void checkSerial() {

}
