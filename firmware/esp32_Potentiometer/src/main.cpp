#include <Arduino.h>

const int PIN_ANALOG_IN = 36;   // потенциометр на GPIO36
const int PIN_LED = 26;         // светодиод на GPIO26 (PWM)

void setup() {
  Serial.begin(115000); // инициализация последовательного порта для отладки
  pinMode(PIN_ANALOG_IN, INPUT);
  pinMode(PIN_LED, OUTPUT);     // пин светодиода — как выход
}

void loop() {
  int adcVal = analogRead(PIN_ANALOG_IN);
  int brightness = map(adcVal, 0, 4095, 50, 200);
  double voltage = adcVal / 4095.0 * 3.3;

  analogWrite(PIN_LED, brightness);          // управляем яркостью светодиода

  Serial.print("ADC Val: ");
  Serial.print(adcVal);
  Serial.print(" | Brightness: ");
  Serial.print(brightness);
  Serial.print(" | Voltage: ");
  Serial.println(voltage, 2);

  delay(200);
}