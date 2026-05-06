// ==========================================================
// ПРОЕКТ: ДАТЧИК ДВИЖЕНИЯ + ВЕНТИЛЯТОР + MQTT (JSON)
// ЛОГИКА: вентилятор включается при движении, выключается
//         через 10 секунд после последнего обнаружения движения.
// RGB: жёлтый (ожидание), зелёный (вентилятор работает).
// MQTT: отправляет JSON с событием, временем (Unix) и строкой даты.
// ==========================================================

#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <time.h>
#include "config.h"

// ---------- ПИНЫ ----------
const int pirPin = 14;          // датчик движения PIR
const int motorA = 12;          // IN1 драйвера HR1124S
const int motorB = 13;          // IN2
const int redPin   = 27;        // RGB красный
const int greenPin = 25;        // RGB зелёный
const int bluePin  = 26;        // RGB синий

// ---------- MQTT ----------
WiFiClient espClient;
PubSubClient client(espClient);


// ---------- ПЕРЕМЕННЫЕ ДЛЯ ТАЙМЕРА ----------
unsigned long lastMotionTime = 0;          // время последнего обнаружения движения (millis)
unsigned long motorStartTime = 0;          // время включения мотора (millis)
bool motorActive = false;
const unsigned long delayAfterMotion = 15000;  // задержка выключения (10 секунд)

unsigned long lastPirTrigger = 0;          // время последнего срабатывания PIR
const unsigned long pirDebounce = 5000;    // игнорировать новые срабатывания в течение 2 секунд

unsigned long motorOffTime = 0;            // время выключения мотора
const unsigned long motorCooldown = 5000;  // после выключения 3 секунды не реагировать на PIR

unsigned long motionHighStart = 0;
const unsigned long motionHoldTime = 2000; // 2 секунды

// ---------- NTP НАСТРОЙКИ ----------
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 10800;   // UTC+3 (Москва)
const int   daylightOffset_sec = 0;

// ---------- RGB (ШИМ через ledc для надёжности) ----------
const int redChannel = 0, greenChannel = 1, blueChannel = 2;
const int freq = 5000;
const int resolution = 8;

void setupRGB() {
  ledcSetup(redChannel, freq, resolution);
  ledcAttachPin(redPin, redChannel);
  ledcSetup(greenChannel, freq, resolution);
  ledcAttachPin(greenPin, greenChannel);
  ledcSetup(blueChannel, freq, resolution);
  ledcAttachPin(bluePin, blueChannel);
}

void setRGB(int r, int g, int b) {
  ledcWrite(redChannel, r);
  ledcWrite(greenChannel, g);
  ledcWrite(blueChannel, b);
}

// ---------- УПРАВЛЕНИЕ МОТОРОМ ----------
void motorOn() {
  digitalWrite(motorA, HIGH);
  digitalWrite(motorB, LOW);   // вращение
}

void motorOff() {
  digitalWrite(motorA, LOW);
  digitalWrite(motorB, LOW);
}

// ---------- ПОЛУЧЕНИЕ ТЕКУЩЕГО ВРЕМЕНИ (UNIX timestamp) ----------
unsigned long getCurrentTimestamp() {
  time_t now = time(nullptr);
  if (now < 100000) {   // если время ещё не синхронизировалось
    return 0;
  }
  return (unsigned long)now;
}

// !!! ДОБАВЛЕНО: преобразование Unix timestamp в строку "YYYY-MM-DD HH:MM:SS"
String formatTimestamp(time_t t) {
  struct tm* timeinfo = localtime(&t);
  char buffer[20];
  strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", timeinfo);
  return String(buffer);
}

// ---------- MQTT ОТПРАВКА JSON ----------
void publishFanEvent(const char* event, unsigned long duration = 0) {
  if (!client.connected()) return;

  unsigned long timestamp = getCurrentTimestamp();
  String timeStr = formatTimestamp(timestamp);

  String payload = "{";
  payload += "\"event\":\"" + String(event) + "\",";
  payload += "\"timestamp\":" + String(timestamp) + ",";
  payload += "\"time_str\":\"" + timeStr + "\"";
  if (duration > 0) {
    payload += ",\"duration\":" + String(duration);
  }
  payload += "}";

  client.publish("fan/status", payload.c_str());
  Serial.print("MQTT sent: ");
  Serial.println(payload);
}

// ---------- SETUP ----------
void setup() {
  Serial.begin(115200);

  // --- Настройка пинов ---
  pinMode(pirPin, INPUT);
  pinMode(motorA, OUTPUT);
  pinMode(motorB, OUTPUT);
  motorOff();

  // --- RGB ---
  setupRGB();
  setRGB(255, 255, 0);        // жёлтый – режим ожидания

  // --- Сигнал запуска: красное мигание ---
  for (int i = 0; i < 2; i++) {
    setRGB(255, 0, 0);
    delay(300);
    setRGB(0, 0, 0);
    delay(300);
  }
  setRGB(255, 255, 0);

  // --- Подключение к Wi-Fi ---
  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // --- NTP (для реального времени) ---
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  Serial.print("Waiting for NTP time");
  while (getCurrentTimestamp() == 0) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nTime synchronized");

  // --- MQTT ---
  client.setServer(mqtt_server, 1883);
  if (client.connect("ESP32_PIR_MOTOR")) {
    Serial.println("MQTT connected");
  } else {
    Serial.println("MQTT connection failed");
  }
}

// ---------- LOOP ----------
void loop() {
  client.loop();

  bool rawMotion = (digitalRead(pirPin) == HIGH);
  unsigned long now = millis();
  
  // ---- стабилизация сигнала: считаем движение только после 2 секунд HIGH ----
  static unsigned long motionHighStart = 0;
  bool motion = false;
  if (rawMotion) {
    if (motionHighStart == 0) motionHighStart = now;
    if (now - motionHighStart >= motionHoldTime) motion = true;
  } else {
    motionHighStart = 0;
  }

  // ---- игнор на время кулдауна после выключения мотора ----
  if (motion && (now - motorOffTime > motorCooldown) && (now - lastPirTrigger > pirDebounce)) {
    lastPirTrigger = now;
    lastMotionTime = now;

    if (!motorActive) {
      motorActive = true;
      motorStartTime = now;
      motorOn();
      setRGB(0, 0, 255);
      Serial.println("Motion detected, fan ON");
      publishFanEvent("ON");
    }
  }

  // ---- выключение по таймеру (если нет устойчивого движения) ----
  if (motorActive && !motion && (now - lastMotionTime >= delayAfterMotion)) {
    // motorOff();
    // motorActive = false;
    // motorOffTime = now;
    setRGB(255, 255, 0);
    // unsigned long durationSec = (now - motorStartTime) / 1000;
  //   Serial.println("No motion, fan OFF");
  //   publishFanEvent("OFF", durationSec);
  }

  delay(50);
}