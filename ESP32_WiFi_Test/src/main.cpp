#include <Arduino.h>
#include <WiFi.h>
#include "config.h"   // здесь уже есть ssid и password

void setup() {
  Serial.begin(115200);
  Serial.println("\nПытаемся подключиться к Wi-Fi...");

  WiFi.begin(ssid, password);   // берутся из config.h

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWi-Fi подключён!");
  Serial.print("IP адрес ESP32: ");
  Serial.println(WiFi.localIP());
}

void loop() {}