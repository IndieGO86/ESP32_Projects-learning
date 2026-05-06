#include <Arduino.h>

#include <Arduino.h>

// ===== ПИНЫ =====
const int button = 14;
const int PIN_ANALOG_IN = 36;
const int PIN_LED = 26;

// ===== СОСТОЯНИЕ =====
bool ledMode = false;          // режим (false / true)
int buttonState = 0;
int lastButtonState = HIGH;    // чтобы ловить момент нажатия

int lastAdcVal = 0;            // чтобы не спамить значениями

void setup() {
  Serial.begin(115200);

  pinMode(PIN_ANALOG_IN, INPUT);
  pinMode(PIN_LED, OUTPUT);
  pinMode(button, INPUT_PULLUP);
}

void loop() {

  // ===== 1. ЧИТАЕМ КНОПКУ =====
  buttonState = digitalRead(button);

  // ===== 2. ЛОВИМ НАЖАТИЕ (а не удержание) =====
  if (buttonState == LOW && lastButtonState == HIGH) {
    ledMode = !ledMode;  // переключаем режим

    Serial.print("TOGGLE → режим: ");
    Serial.println(ledMode ? "LOW brightness" : "HIGH brightness");
  }

  // ===== 3. ЧИТАЕМ ПОТЕНЦИОМЕТР =====
  int adcVal = analogRead(PIN_ANALOG_IN);

  // преобразуем в яркость
  int brightnessHigh = map(adcVal, 0, 4095, 0, 255);
  int brightnessLow  = map(adcVal, 0, 4095, 100, 200);

  double voltage = adcVal / 4095.0 * 3.3;

  // ===== 4. УПРАВЛЕНИЕ LED ЧЕРЕЗ СОСТОЯНИЕ =====
  if (ledMode) {
    analogWrite(PIN_LED, brightnessLow);
  } else {
    analogWrite(PIN_LED, brightnessHigh);
  }

  // ===== 5. УМНЫЙ ЛОГ (без спама) =====
  // печатаем только если значение реально изменилось
  if (abs(adcVal - lastAdcVal) > 100) {

    Serial.print("ADC: ");
    Serial.print(adcVal);

    Serial.print(" | Brightness: ");
    if (ledMode) {
      Serial.print(brightnessLow);
    } else {
      Serial.print(brightnessHigh);
    }

    Serial.print(" | Voltage: ");
    Serial.print(voltage, 2);

    Serial.print(" | Mode: ");
    Serial.println(ledMode);

    lastAdcVal = adcVal;  // запоминаем последнее значение
  }

  // ===== 6. ОБНОВЛЯЕМ СОСТОЯНИЕ КНОПКИ =====
  lastButtonState = buttonState;

  delay(50);  // маленькая пауза (пока допустимо)
}

// const int button = 14;         // пин кнопки
// const int PIN_ANALOG_IN = 36;  // потенциометр
// const int PIN_LED = 26;        // светодиод

// int lastButtonState = HIGH;    // предыдущее состояние кнопки

// void setup() {
//   Serial.begin(115200);
//   pinMode(PIN_ANALOG_IN, INPUT);
//   pinMode(PIN_LED, OUTPUT);
//   pinMode(button, INPUT_PULLUP);
// }

// void loop() {
//   int adcVal = analogRead(PIN_ANALOG_IN);
//   int brightnessHigh = map(adcVal, 0, 4095, 0, 255);
//   int brightnessLow  = map(adcVal, 0, 4095, 100, 200);
//   double voltage = adcVal / 4095.0 * 3.3;

//   int currentState = digitalRead(button);

//   //   Если состояние кнопки ИЗМЕНИЛОСЬ — выводим сообщение
//   if (currentState != lastButtonState) {
//     lastButtonState = currentState;
//     if (currentState == LOW) {
//       Serial.println("Button PRESSED");
//     } else {
//       Serial.println("Button RELEASED");
//     }
//   }

//   //   Управление яркостью в зависимости от КНОПКИ (без лишних выводов)
//   if (currentState == LOW) {
//     analogWrite(PIN_LED, brightnessLow);
//   } else {
//     analogWrite(PIN_LED, brightnessHigh);
//   }

//   //   Вывод данных потенциометра (можно убрать, если не нужно)
//   Serial.print("ADC: ");
//   Serial.print(adcVal);
//   Serial.print(" | Brightness: ");
//   Serial.print(currentState == LOW ? brightnessLow : brightnessHigh);
//   Serial.print(" | Voltage: ");
//   Serial.println(voltage, 2);

//   delay(1000);
// }