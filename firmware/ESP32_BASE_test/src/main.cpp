#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include "config.h"

// Пины
const int redPin = 27;
const int greenPin = 25;
const int bluePin = 26;
const int buttonPin = 14;

// MQTT
WiFiClient espClient;
PubSubClient client(espClient);

// Функция установки цвета
void setColor(int red, int green, int blue) {
  analogWrite(redPin, red);
  analogWrite(greenPin, green);
  analogWrite(bluePin, blue);
}

// Обработчик входящих MQTT сообщений (для управления цветом)
void callback(char* topic, byte* payload, unsigned int length) {
  String message = "";
  for (int i = 0; i < length; i++) message += (char)payload[i];
  Serial.print("MQTT received: ");
  Serial.println(message);

  // Разбираем "255,0,0" на три числа
  int firstComma = message.indexOf(',');
  int secondComma = message.indexOf(',', firstComma + 1);
  if (firstComma != -1 && secondComma != -1) {
    int r = message.substring(0, firstComma).toInt();
    int g = message.substring(firstComma + 1, secondComma).toInt();
    int b = message.substring(secondComma + 1).toInt();
    setColor(r, g, b);
    Serial.print("Set color: "); Serial.print(r); Serial.print(","); Serial.print(g); Serial.print(","); Serial.println(b);
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(redPin, OUTPUT);
  pinMode(greenPin, OUTPUT);
  pinMode(bluePin, OUTPUT);
  pinMode(buttonPin, INPUT);  // внешняя подтяжка к 3.3V
  setColor(0, 0, 0);

  // Wi-Fi
  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  Serial.println("\nWiFi connected");

  // MQTT
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);   // регистрируем обработчик
  if (client.connect("ESP32_RGB")) {
    client.subscribe("rgb/control");
    Serial.println("MQTT connected");
  } else {
    Serial.println("MQTT connection failed");
  }
}

void loop() {
  client.loop();   // обязательно

  int buttonState = digitalRead(buttonPin);
  if (buttonState == LOW) {
    client.publish("button/status", "ON");
    // мигаем красным 3 раза
    for (int i = 0; i < 3; i++) {
      setColor(255, 0, 0); delay(200);
      setColor(0, 0, 0); delay(200);
    }
    Serial.println("Button pressed, 3 blinks");
  } else {
    client.publish("button/status", "OFF");
    // не трогаем цвет, оставляем тот, который установлен через MQTT
    // (чтобы не сбивать управление)
    Serial.println("Button released");
  }
  delay(100);
}