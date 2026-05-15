// Библиотеки
#include <Arduino.h>       // стандартная для ESP32 (пины, задержки, Serial)
#include <WiFi.h>          // для подключения к Wi-Fi
#include <PubSubClient.h>  // для MQTT
#include <DHT.h>           // для датчика температуры/влажности
#include "config.h"        // файл с настройками (ssid, password, mqtt_server)

// ===== НАСТРОЙКИ ПИНОВ И ТИПА ДАТЧИКА =====
#define DHTPIN 13          // DATA пин DHT11 подключён к GPIO13
#define DHTTYPE DHT11      // тип датчика – DHT11 (можно DHT22, если у вас другой)

// Создаём объект dht, передаём ему пин и тип
DHT dht(DHTPIN, DHTTYPE);

// ===== MQTT =====
WiFiClient espClient;          // клиент для TCP (нужен MQTT)
PubSubClient client(espClient); // MQTT клиент, использует espClient

// ===== ТАЙМЕР ДЛЯ ОТПРАВКИ ДАННЫХ (НЕ БЛОКИРУЕТ LOOP) =====
unsigned long lastMsg = 0;     // время последней отправки
const long interval = 300000;    // интервал между отправками (300000 мс = 5 минут)

void setup() {
  // --- 1. Отладка через Serial ---
  Serial.begin(115200);
  Serial.println("DHT11 + MQTT начали работу");

  // --- 2. Инициализация датчика ---
  dht.begin();

  // --- 3. Подключение к Wi-Fi ---
  Serial.print("Подключаюсь к Wi-Fi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWi-Fi подключён");
  Serial.print("IP адрес ESP32: ");
  Serial.println(WiFi.localIP());

  // --- 4. Подключение к MQTT брокеру (ваш компьютер) ---
  client.setServer(mqtt_server, 1883);
  if (client.connect("ESP32_DHT11")) {
    Serial.println("MQTT подключён");
  } else {
    Serial.println("MQTT НЕ подключён!");
  }
}

void loop() {
  // --- Обязательно! Поддерживаем MQTT связь (пинги, приём сообщений) ---
  client.loop();

  // --- Таймер: отправляем данные каждые interval миллисекунд ---
  if (millis() - lastMsg > interval) {
    lastMsg = millis();

    // --- Читаем данные с датчика ---
    float humidity = dht.readHumidity();        // влажность (%)
    float temperature = dht.readTemperature();  // температура (°C)

    // --- Проверяем, удалось ли прочитать (если нет – ошибка) ---
    if (isnan(humidity) || isnan(temperature)) {
      Serial.println("Ошибка чтения DHT11!");
      return;   // выходим, не отправляем MQTT
    }

    // --- Подготавливаем JSON строку для отправки ---
    char payload[50];   // буфер (массив символов) для сообщения
    snprintf(payload, sizeof(payload),
             "{\"temperature\":%.1f,\"humidity\":%.1f}",
             temperature, humidity);
    // snprintf форматирует строку: %.1f – одно число после запятой

    // --- Отправляем в MQTT топик "dht11/data" ---
    client.publish("dht11/data", payload);

    // --- Выводим то же самое в Serial Monitor для отладки ---
    Serial.print("MQTT отправлено: ");
    Serial.println(payload);
  }
}