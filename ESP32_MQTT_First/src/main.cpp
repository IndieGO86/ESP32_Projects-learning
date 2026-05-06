// Файл: src/main.cpp
// ESP32 отправляет MQTT-сообщение каждые 10 секунд

#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include "config.h"

WiFiClient espClient;
PubSubClient client(espClient);

// Переменная для хранения времени последней отправки сообщения
// static означает, что она сохраняет своё значение между вызовами loop()
// unsigned long — тип для больших положительных чисел (время в миллисекундах)
static unsigned long lastMsg = 0;

void setup() {
  Serial.begin(115200);
  Serial.println();

  // ===== 1. Подключение к Wi-Fi =====
  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

  // ===== 2. Подключение к MQTT брокеру и отправка одного сообщения =====
  client.setServer(mqtt_server, 1883);
  if (client.connect("ESP32_Test")) {
    Serial.println("MQTT connected");
    client.publish("esp32/test", "Hello from ESP32!");
    Serial.println("Message sent");
  } else {
    Serial.println("MQTT connection failed");
  }
}

void loop() {
  // ===== 3. Поддержание MQTT связи (ОБЯЗАТЕЛЬНО!) =====
  client.loop();   // без этой строки MQTT отвалится через минуту

  // ===== 4. Отправка сообщения каждые 10 секунд =====
  // millis() — возвращает время в миллисекундах с момента запуска ESP32
  // lastMsg — хранит время последней отправки
  if (millis() - lastMsg > 10000) {   // если прошло больше 10 секунд
    lastMsg = millis();                // обновляем время последней отправки
    client.publish("esp32/test", "Hello from ESP32!");   // отправляем
    Serial.println("Message sent");
  }
}