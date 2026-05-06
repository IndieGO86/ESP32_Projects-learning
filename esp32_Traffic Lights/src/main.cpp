#include <Arduino.h>

/*
 * Filename: Traffic_Lights
 * Function: traffic lights
 * Compiling IDE：ARDUINO 2.3.3
 * Author: https://www.keyestudio.com/ 
/*
 * Filename: Traffic_Lights
 * Function: Жёлтый мигает 10 раз с ускорением
*/
const int PIN_LED_RED = 13;
const int PIN_LED_YELLOW = 14;
const int PIN_LED_GREEN = 16;

void setup() {
  pinMode(PIN_LED_RED, OUTPUT);
  pinMode(PIN_LED_YELLOW, OUTPUT);
  pinMode(PIN_LED_GREEN, OUTPUT);
}

void loop() {
  // Зелёный
  digitalWrite(PIN_LED_GREEN, HIGH);
  delay(5000);
  digitalWrite(PIN_LED_GREEN, LOW);
  delay(500);

  // Жёлтый — мигает 10 раз с ускорением
//   for (int i = 1; i <= 10; i++) {
//     int blinkSpeed = 500 / i;
//     digitalWrite(PIN_LED_YELLOW, HIGH);
//     delay(blinkSpeed);
//     digitalWrite(PIN_LED_YELLOW, LOW);
//     delay(blinkSpeed);
//   }

   // Жёлтый — 3 мигания с задержкой 500 мс
   for (int i = 0; i < 3; i++) {
     digitalWrite(PIN_LED_YELLOW, HIGH);
     delay(500);
     digitalWrite(PIN_LED_YELLOW, LOW);
     delay(500);
   }

   // Жёлтый — 3 мигания с задержкой 300 мс
   for (int i = 0; i < 3; i++) {
     digitalWrite(PIN_LED_YELLOW, HIGH);
     delay(300);
     digitalWrite(PIN_LED_YELLOW, LOW);
     delay(300);
   }

   // Жёлтый — 3 мигания с задержкой 100 мс
   for (int i = 0; i < 3; i++) {
     digitalWrite(PIN_LED_YELLOW, HIGH);
     delay(100);
     digitalWrite(PIN_LED_YELLOW, LOW);
     delay(100);
   }

  // Красный
  digitalWrite(PIN_LED_RED, HIGH);
  delay(5000);
  digitalWrite(PIN_LED_RED, LOW);
  delay(500);
}