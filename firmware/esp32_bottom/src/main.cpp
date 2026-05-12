#include <Arduino.h>

const int button = 14;   // пин кнопки (GPIO14)
const int PIN_LED_RED = 26;
int val = 0;             // переменная для хранения состояния кнопки



void setup() {
  Serial.begin(115200);            // скорость Serial-порта
  pinMode(button, INPUT_PULLUP);;        // пин настроен как вход (без подтяжки)
  pinMode(PIN_LED_RED, OUTPUT);   // пин настроен как выход
}

void loop() {
  val = digitalRead(button);     // читаем состояние кнопки (HIGH или LOW)
  Serial.print(val);             // выводим 1 или 0

  if (val == 0) {                // если 0 (кнопка нажата) 
    Serial.print("    ");
    Serial.println("Button is PRESSED");
    digitalWrite(PIN_LED_RED, HIGH);  // выключаем светодиод
    delay(500);
  } else {                       // если 1 (кнопка отпущена)
    Serial.print("    ");
    Serial.println("Button is RELEASED");
    digitalWrite(PIN_LED_RED, LOW);   // включаем светодиод
    delay(500);
  }
}