#include <Arduino.h>
#include <WiFi.h>          // библиотека для Wi-Fi
#include <PubSubClient.h>  // библиотека для MQTT
#include "config.h"        // твои настройки (ssid, password, mqtt_server)

/*
 * Ультразвуковой дальномер HC-SR04
 * Принцип: посылаем щелчок (TRIG), замеряем время возврата эха (ECHO)
 * Формула: расстояние = время_звука * скорость_звука / 2
 */

// Подключаем пины: TRIG — управление, ECHO — приём
const int trigPin = 13;   // пин, куда подаём импульс (выход)
const int echoPin = 12;   // пин, с которого читаем длительность эха (вход)
const int PIN_LED = 26;   // пин, куда подаём импульс светодиода (выход)
const int PIN_BUZZER = 27;  // пин, куда подаём импульс динамика (выход)
const int pwmChannel = 0;
const int pwmFreq = 5000;
const int pwmResolution = 8;

// ---- ДОБАВЛЕНО ДЛЯ MQTT ----
WiFiClient espClient;             // объект для работы с Wi-Fi (нужен MQTT)
PubSubClient client(espClient);   // MQTT клиент

// ===== ТАЙМЕР ОТПРАВКИ =====
unsigned long lastMsg = 0;        // для отправки по таймеру
const long interval = 2000;       // отправка раз в 2 секунды

void setup() {
  Serial.begin(115200);               // запускаем общение с компьютером
  pinMode(trigPin, OUTPUT);         // TRIG — выход
  pinMode(echoPin, INPUT);          // ECHO — вход
  ledcSetup(pwmChannel, pwmFreq, pwmResolution);
  ledcAttachPin(PIN_LED, pwmChannel);
  pinMode(PIN_BUZZER, OUTPUT);      // PIN_BUZZER — выход
  Serial.println("Ultrasonic sensor ready");

  // ===== ДОБАВЛЕНО: ПОДКЛЮЧЕНИЕ К WI-FI =====
  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, password);               // берём из config.h
  while (WiFi.status() != WL_CONNECTED) {   // ждём подключения
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());           // покажет IP ESP32 в сети

  // ===== ДОБАВЛЕНО: ПОДКЛЮЧЕНИЕ К MQTT =====
  client.setServer(mqtt_server, 1883);      // берём из config.h
  if (client.connect("ESP32_Ultrasonic")) { // уникальное имя клиента
    Serial.println("MQTT connected");
  } else {
    Serial.println("MQTT connection failed");
  }
}

void loop() {
  // ===== ДОБАВЛЕНО: ПОДДЕРЖКА MQTT СОЕДИНЕНИЯ =====
  client.loop();   // обязательно! без этого MQTT отвалится

  // 1. Готовим датчик к измерению
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);             // пауза, чтобы сигнал точно упал

  // 2. Отдаём команду «измеряй»
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);            // импульс ровно 10 микросекунд (требование датчика)
  digitalWrite(trigPin, LOW);

  // pulseIn с таймаутом 30 000 мкс (если сигнал не пришёл, вернёт 0)
  unsigned long microsecond = pulseIn(echoPin, HIGH, 30000);

  float distance;
  if (microsecond == 0) {
  // сигнал не пришёл (препятствие дальше 5 метров или датчик завис)
    distance = 400;   // максимальное значение (можно поставить 0)
  } else {
    distance = microsecond / 29.00 / 2;
  if (distance > 400) distance = 400;   // ограничение сверху
  }

   // === ФИЛЬТРАЦИЯ СКАЧКОВ ===
  static float lastDistance = 0;        // предыдущее значение (static — сохраняется между вызовами)

  // Если предыдущее расстояние было меньше 200 см, а текущее > 300 см — оставляем предыдущее
  if (lastDistance != 0 && lastDistance < 200 && distance > 300) {
    distance = lastDistance;
  } else {
    lastDistance = distance;            // иначе обновляем сохранённое значение
  }

  // Преобразуем расстояние в яркость
  const float minDist = 10.0;
  const float maxDist = 100.0;
  int brightness;
  if (distance <= minDist) {
    brightness = 255;
  } else if (distance >= maxDist) {
    brightness = 0;
  } else {
    brightness = map(distance, minDist, maxDist, 255, 0);
  }
  ledcWrite(pwmChannel, brightness);

    // --- управление динамиком (звук) ---
    //   if (brightness > 0) {
    //   tone(PIN_BUZZER, map(brightness, 0, 255, 500, 3000));
    // } else {
    // noTone(PIN_BUZZER);
    // }
    // ============================================================
  // ПЛАВНОЕ ИЗМЕНЕНИЕ ТОНА (без резких скачков)
  // ============================================================
  // Переменная lastFreq объявлена как static, чтобы сохранять
  // значение между вызовами loop(). static внутри функции означает,
  // что переменная создаётся один раз и не обнуляется при каждом входе.
  static int lastFreq = 500;      // предыдущая частота (стартуем с 500 Гц)

  // Яркость brightness уже вычислена: 0 (далеко) … 255 (близко)
  if (brightness > 0) {
      // Целевая частота: чем ближе объект (выше brightness), тем выше тон
      int targetFreq = map(brightness, 0, 255, 500, 3000);
      
      // Фильтр низких частот (сглаживание):
      // новое значение = 20% от целевой частоты + 80% от предыдущего значения
      // Коэффициенты можно менять: 0.9 = очень плавно, 0.5 = быстро, но возможны скачки
      int smoothFreq = (lastFreq * 0.8) + (targetFreq * 0.2);
      
      lastFreq = smoothFreq;      // запоминаем для следующего цикла
      tone(PIN_BUZZER, smoothFreq);
  } else {
      noTone(PIN_BUZZER);
  }

  // 5. Показываем результат в мониторе порта
  Serial.print("Distance: ");
  Serial.print(distance);
  Serial.println(" cm");

  // ===== ДОБАВЛЕНО: ОТПРАВКА ДАННЫХ В MQTT (раз в 2 секунды) =====
  if (millis() - lastMsg > interval) {
  lastMsg = millis();

  // --- готовим строку для отправки: "distance brightness" ---
  char msg[50];                                 // достаточно большой буфер
  snprintf(msg, sizeof(msg),
    "{\"distance\":%.2f,\"brightness\":%d}",
    distance,
    brightness
  );  // форматируем

  // --- отправляем в MQTT ---
  client.publish("ultrasonic/data", msg);

  // // --- отладочный вывод (опционально) ---
  // Serial.print("MQTT sent: ");
  // Serial.println(msg);
}

  // 6. Пауза между измерениями (чтобы не забивать порт)
  delay(300);
}