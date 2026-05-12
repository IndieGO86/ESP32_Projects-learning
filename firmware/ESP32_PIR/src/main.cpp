#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include "config.h"
#include <time.h>

// ===== ПИНЫ =====
const int PIR_PIN = 14;     // датчик движения
const int LED_R = 27;
const int LED_G = 25;
const int LED_B = 26;

const char* device_id = "esp32_room_1";

// ===== MQTT =====
WiFiClient espClient;
PubSubClient client(espClient);

// ===== СОСТОЯНИЕ =====
bool lastMotionState = false;   // чтобы не спамить

void setColor(int r, int g, int b) {
  analogWrite(LED_R, r);
  analogWrite(LED_G, g);
  analogWrite(LED_B, b);
}

// ===== SETUP =====
void setup() {
  Serial.begin(115200);

  pinMode(PIR_PIN, INPUT);
  pinMode(LED_R, OUTPUT);
  pinMode(LED_G, OUTPUT);
  pinMode(LED_B, OUTPUT);

  // --- WiFi ---
  Serial.print("Connecting WiFi");
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi OK");


  configTime(0, 0, "pool.ntp.org");  // получаем время из интернета

  Serial.print("Syncing time");
  time_t now = time(nullptr);

  while (now < 100000) {   // ждём пока время подтянется
    delay(500);
    Serial.print(".");
    now = time(nullptr);
  }

  Serial.println("\nTime synced");

  // --- MQTT ---
  client.setServer(mqtt_server, 1883);

  if (client.connect("ESP32_PIR")) {
    Serial.println("MQTT connected");
  } else {
    Serial.println("MQTT failed");
  }
}

// ===== LOOP =====
void loop() {
  client.loop();

  // если отвалился MQTT — пробуем переподключиться
  if (!client.connected()) {
    if (client.connect("ESP32_PIR")) {
      Serial.println("MQTT reconnected");
    }
  }

  // ===== ЧИТАЕМ ДАТЧИК =====
  int motion = digitalRead(PIR_PIN);

  // ===== СОБЫТИЯ =====
  if (motion == HIGH && lastMotionState == false) {
    // движение НАЧАЛОСЬ

    setColor(255, 0, 0);  //  красный цвет

    char msg[150];
    time_t now = time(nullptr);

    snprintf(msg, sizeof(msg),
      "{\"event\":\"motion_start\",\"device\":\"%s\",\"time\":%ld}",
      device_id,
      now
    );

    client.publish("pir/event", msg);

    lastMotionState = true;
  }

  if (motion == LOW && lastMotionState == true) {
    // движение ЗАКОНЧИЛОСЬ

    setColor(0, 255, 0);  // зелёный цвет

    char msg[150];
    time_t now = time(nullptr);
    
    snprintf(msg, sizeof(msg),
    "{\"event\":\"motion_end\",\"device\":\"%s\",\"time\":%ld}",
    device_id,
    now
    );

client.publish("pir/event", msg);


    lastMotionState = false;
  }
  delay(100); // небольшая пауза
}