// ============================================================
// ESP32_WiFi_MQTT_Template
// Шаблон для проектов с Wi-Fi и MQTT
// ============================================================

// Подключаем необходимые библиотеки
#include <Arduino.h>      // стандартная библиотека ESP32
#include <WiFi.h>         // для работы с Wi-Fi
#include <PubSubClient.h> // для MQTT
#include "config.h"       // наши настройки (ssid, password, mqtt_server)

// --- Глобальные объекты ---
WiFiClient espClient;           // объект для работы с TCP (нужен MQTT)
PubSubClient client(espClient); // MQTT клиент, использует espClient

// Переменная для хранения последнего времени переподключения
unsigned long lastReconnectAttempt = 0;

// --- Прототипы функций (объявляем, чтобы компилятор знал о них) ---
void setup_wifi();               // подключение к Wi-Fi
boolean reconnect_mqtt();        // подключение к MQTT брокеру

// ============================================================
// setup() - выполняется один раз при старте
// ============================================================
void setup() {
  // Включаем Serial для вывода отладочных сообщений
  Serial.begin(115200);
  Serial.println("\n\nESP32 starting...");

  // Подключаемся к Wi-Fi
  setup_wifi();

  // Настраиваем MQTT сервер
  client.setServer(mqtt_server, mqtt_port);
  // (можно добавить callback для приёма сообщений, но пока не нужно)
}

// ============================================================
// loop() - выполняется бесконечно
// ============================================================
void loop() {
  // Если MQTT ещё не подключён или связь потеряна
  if (!client.connected()) {
    // Пытаемся переподключиться
    if (reconnect_mqtt()) {
      Serial.println("MQTT reconnected");
    } else {
      // Если не получилось, ждём 5 секунд и повторим
      delay(5000);
      return;
    }
  }

  // Поддерживаем MQTT соединение (обработка входящих сообщений, пинги)
  client.loop();

  // --- ЗДЕСЬ БУДЕТ КОД ТВОИХ ДАТЧИКОВ ---
  // Например, отправка тестового сообщения каждые 10 секунд
  static unsigned long lastMsg = 0;
  if (millis() - lastMsg > 10000) {
    lastMsg = millis();
    
    // Отправляем приветственное сообщение
    const char* msg = "Hello from ESP32!";
    client.publish("esp32/test", msg);
    Serial.print("Published: ");
    Serial.println(msg);
  }
}

// ============================================================
// Функция подключения к Wi-Fi
// ============================================================
void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  // Пытаемся подключиться
  WiFi.begin(ssid, password);

  // Ждём, пока не подключимся
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  // Подключились, выводим информацию
  Serial.println();
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());   // покажет IP ESP32 в локальной сети
}

// ============================================================
// Функция подключения к MQTT брокеру
// Возвращает true, если подключились успешно
// ============================================================
boolean reconnect_mqtt() {
  // Генерируем уникальное имя клиента (по MAC-адресу)
  String clientId = "ESP32_";
  clientId += String(WiFi.macAddress());

  // Пытаемся подключиться
  if (client.connect(clientId.c_str())) {
    Serial.print("MQTT connected as ");
    Serial.println(clientId);
    // Можно подписаться на топики, но пока не нужно
    // client.subscribe("esp32/command");
    return true;
  } else {
    Serial.print("MQTT connection failed, rc=");
    Serial.print(client.state());
    Serial.println(" try again in 5 seconds");
    return false;
  }
}