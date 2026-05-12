// Подключаем библиотеку Arduino
#include <Arduino.h>
/*
 * File name: Hello,LED
 * Function: LED blinks 1s
 * Compiling IDE: ARDUINO 2.3.3
 * Author: https://www.keyestudio.com/
*/
const int ledPin = 26;  // The GPIO pin for the LED
int CHN = 0;   // define the pwm channel
int FRQ = 5000;   // define the pwm frequency
int PWM_BIT = 10;   // define the pwm resolution for ledc channel

// void setup() {
//   ledcSetup(CHN, FRQ, PWM_BIT); // setup pwm channel，frequency and resolution for ledc channel.
//   ledcAttachPin(ledPin, CHN);  // attach the led pin to pwm channel
// }

// void loop() {
//   for (int i = 0; i < 1024; i++) { //make light fade in
//     ledcWrite(CHN, i);
//     delay(5);
//   }
//   for (int i = 1024; i > -1; i--) {  //make light fade out
//     ledcWrite(CHN, i);
//     delay(5);
//   }
// }

void setup() {
  ledcSetup(0, 1000, 8);
  ledcAttachPin(26, 0);
}

void loop() {
  ledcWrite(0, 64);   // 64 / 255 ~ 25% яркости
  delay(1000);
  ledcWrite(0, 128);  // ~50% яркости
  delay(1000);
  ledcWrite(0, 192);  // ~75% яркости
  delay(1000);
  ledcWrite(0, 255);  // 100% яркости
  delay(1000);
}