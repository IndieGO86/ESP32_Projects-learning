#include <Arduino.h>
#include <WiFi.h>          // библиотека для Wi-Fi
#include <PubSubClient.h>  // библиотека для MQTT
#include "config.h"        // твои настройки (ssid, password, mqtt_server)

/*
 * Ультразвуковой дальномер HC-SR04 + MQTT + звук (плавин тон) + LED
 * Управление звуком по MQTT: mute / sound
 */

// ---------- ПИНЫ ----------
const int trigPin = 13;    // выход TRIG на HC-SR04
const int echoPin = 12;    // вход ECHO на HC-SR04
const int PIN_LED = 26;    // обычный светодиод (ШИМ)
const int PIN_BUZZER = 27; // пассивный динамик (tone)
bool ledEnabled = true  ; // флаг для включения/выключения LED 


// ---------- MQTT ----------
WiFiClient espClient;
PubSubClient client(espClient);

// ---------- Глобальные переменные ----------
bool isMuted = false;          // флаг выключения звука (true = звук выключен)
unsigned long lastMsg = 0;     // для таймера отправки MQTT
const long interval = 3000;    // отправка раз в 3 секунды

// ---------- CALLBACK (обработка входящих MQTT сообщений) ----------
void callback(char* topic, byte* payload, unsigned int length) {
  // Преобразуем payload в строку
  String message = "";
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  Serial.print("MQTT received [");
  Serial.print(topic);
  Serial.print("]: ");
  Serial.println(message);

  // Управление звуком
  if (message == "mute") {
    isMuted = true;
    ledEnabled = false;
    Serial.println(">>> Sound OFF (mute)");
  }
  else if (message == "sound") {
    isMuted = false;
    ledEnabled = true;
    Serial.println(">>> Sound ON");
  }
}

// ---------- SETUP ----------
void setup() {
  Serial.begin(115200);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(PIN_LED, OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);
  Serial.println("Ultrasonic sensor ready");

  // ---- Wi-Fi ----
  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

  // ---- MQTT ----
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);   // устанавливаем функцию обратного вызова

  if (client.connect("ESP32_Ultrasonic")) {
    Serial.println("MQTT connected");
    client.subscribe("ultrasonic/control");   // подписываемся на топик команд
  } else {
    Serial.println("MQTT connection failed");
  }
}

// ---------- LOOP ----------
void loop() {
  // Поддерживаем MQTT-соединение
  client.loop();

  // Автоматическое переподключение MQTT, если связь потеряна
  if (!client.connected()) {
    Serial.println("MQTT disconnected, reconnecting...");
    if (client.connect("ESP32_Ultrasonic")) {
      client.subscribe("ultrasonic/control");
      Serial.println("MQTT reconnected");
    } else {
      Serial.println("MQTT reconnect failed");
      delay(5000);   // пауза перед следующей попыткой
      return;        // выходим, не измеряем, пока нет MQTT
    }
  }

  // ---------- ИЗМЕРЕНИЕ РАССТОЯНИЯ ----------
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  unsigned long microsecond = pulseIn(echoPin, HIGH, 30000);  // таймаут 30 мс
  float distance;
  if (microsecond == 0) {
    distance = 400;   // сигнал не пришёл – считаем далеко
  } else {
    distance = microsecond / 29.00 / 2;
    if (distance > 400) distance = 400;
  }

  // ---------- ФИЛЬТРАЦИЯ СКАЧКОВ ----------
  static float lastDistance = 0;
  if (lastDistance != 0 && lastDistance < 200 && distance > 300) {
    distance = lastDistance;   // игнорируем ложное дальнее эхо
  } else {
    lastDistance = distance;
  }

  // ---------- ЯРКОСТЬ СВЕТОДИОДА ----------
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

  if (ledEnabled) {
    analogWrite(PIN_LED, brightness);
  } else {
    analogWrite(PIN_LED, 0);
  }

  // ---------- ПЛАВНЫЙ ТОН ДИНАМИКА (с учётом isMuted) ----------
  static int lastFreq = 500;
  if (brightness > 0 && !isMuted) {
    int targetFreq = map(brightness, 0, 255, 500, 3000);
    // Фильтр для плавного изменения частоты
    int smoothFreq = (lastFreq * 0.8) + (targetFreq * 0.2);
    lastFreq = smoothFreq;
    tone(PIN_BUZZER, smoothFreq);
  } else {
    noTone(PIN_BUZZER);
  } 

  // ---------- ВЫВОД В SERIAL MONITOR ----------
  Serial.print("Distance: ");
  Serial.print(distance);
  Serial.print(" cm, Brightness: ");
  Serial.print(brightness);
  Serial.print(", Muted: ");
  Serial.println(isMuted ? "YES" : "NO");

  // ---------- ОТПРАВКА В MQTT (раз в 3 секунды) ----------
  if (millis() - lastMsg > interval) {
    lastMsg = millis();
    char msg[50];
    snprintf(msg, sizeof(msg), "{\"distance\":%.2f,\"brightness\":%d}", distance, brightness);
    client.publish("ultrasonic/data", msg);
    Serial.print("MQTT published: ");
    Serial.println(msg);
  }

  delay(300);
}