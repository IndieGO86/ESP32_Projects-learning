#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include "config.h"          // твой файл с ssid, password, mqtt_server

// ----- Пины RGB (общий катод) -----
const int ledPins[] = {27, 25, 26};   // R, G, B
const byte chns[] = {0, 1, 2};        // ШИМ-каналы

WiFiClient espClient;
PubSubClient client(espClient);

// Переменные для хранения текущего цвета
int currentRed = 0, currentGreen = 0, currentBlue = 0;

// ----- Функция установки цвета (определена до её использования) -----
void setColor(int red, int green, int blue) {
  ledcWrite(chns[0], red);   // записать яркость в красный канал
  ledcWrite(chns[1], green); // в зелёный
  ledcWrite(chns[2], blue);  // в синий
}

// ----- Callback для обработки входящих MQTT-сообщений -----
void callback(char* topic, byte* payload, unsigned int length) {
  String message = "";
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  Serial.print("MQTT received: ");
  Serial.println(message);

  // Ожидаем команды: "color:R,G,B" или "off" / "on"
  if (message.startsWith("color:")) {
    String rgb = message.substring(6); // отрезаем "color:"
    int firstComma = rgb.indexOf(',');
    int secondComma = rgb.indexOf(',', firstComma + 1);
    if (firstComma > 0 && secondComma > 0) {
      int r = rgb.substring(0, firstComma).toInt();
      int g = rgb.substring(firstComma + 1, secondComma).toInt();
      int b = rgb.substring(secondComma + 1).toInt();
      currentRed = constrain(r, 0, 255);
      currentGreen = constrain(g, 0, 255);
      currentBlue = constrain(b, 0, 255);
      setColor(currentRed, currentGreen, currentBlue);
    }
  }
  else if (message == "off") {
    setColor(0, 0, 0);
  }
  else if (message == "on") {
    setColor(currentRed, currentGreen, currentBlue);
  }
}

void setup() {
  Serial.begin(115200);
  
  // Настройка ШИМ для RGB
  for (int i = 0; i < 3; i++) {
    ledcSetup(chns[i], 1000, 8);
    ledcAttachPin(ledPins[i], chns[i]);
  }
  setColor(0, 0, 0);   // начальный цвет чёрный

  // Подключение к Wi-Fi
  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // MQTT
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  if (client.connect("ESP32_RGB")) {
    client.subscribe("rgb/control");
    Serial.println("MQTT connected");
  } else {
    Serial.println("MQTT connection failed");
  }
}

void loop() {
  client.loop();   // поддержка MQTT
  
  if (!client.connected()) {
    // попытка переподключения
    if (client.connect("ESP32_RGB")) {
      client.subscribe("rgb/control");
      Serial.println("MQTT reconnected");
    } else {
      delay(5000);
    }
  }
}