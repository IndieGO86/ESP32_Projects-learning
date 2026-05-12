#include <Arduino.h>

/*
 * Ультразвуковой дальномер HC-SR04
 * Принцип: посылаем щелчок (TRIG), замеряем время возврата эха (ECHO)
 * Формула: расстояние = время_звука * скорость_звука / 2
 */

// Подключаем пины: TRIG — управление, ECHO — приём
const int trigPin = 13;   // пин, куда подаём импульс (выход)
const int echoPin = 12;   // пин, с которого читаем длительность эха (вход)
const int PIN_LED = 26;   // пин, куда подаём импульс светодиода (выход)


void setup() {
  Serial.begin(9600);               // запускаем общение с компьютером
  pinMode(trigPin, OUTPUT);         // TRIG — выход
  pinMode(echoPin, INPUT);          // ECHO — вход
  pinMode(PIN_LED, OUTPUT);         // PIN_LED — выход
  Serial.println("Ultrasonic sensor ready");
}

void loop() {
  // 1. Готовим датчик к измерению
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);             // пауза, чтобы сигнал точно упал

  // 2. Отдаём команду «измеряй»
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);            // импульс ровно 10 микросекунд (требование датчика)
  digitalWrite(trigPin, LOW);

  // 3. Измеряем, сколько времени эхо было HIGH
  //    pulseIn возвращает длительность в микросекундах
  unsigned long microsecond = pulseIn(echoPin, HIGH);

  // 4. Переводим микросекунды в сантиметры
  //    Скорость звука ~343 м/с → 1 см звук проходит за 29 мкс.
  //    Делим на 2, потому что звук летит туда и обратно.
  float distance = microsecond / 29.00 / 2;

   // Преобразуем расстояние в яркость
  const float minDist = 5.0;
  const float maxDist = 50.0;

  int brightness;
  if (distance <= minDist) {
    brightness = 255;
  } else if (distance >= maxDist) {
    brightness = 0;
  } else {
    brightness = map(distance, minDist, maxDist, 255, 0);
  }

  analogWrite(PIN_LED, brightness);

  // 5. Показываем результат в мониторе порта
  Serial.print("Distance: ");
  Serial.print(distance);
  Serial.println(" cm");

  // 6. Пауза между измерениями (чтобы не забивать порт)
  delay(200);
}