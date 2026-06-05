// Библиотеки
#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>
#include "config.h"
#include "lcd128_32_io.h"

// ===== ПИНЫ ДАТЧИКА =====
#define DHTPIN 13
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// ===== LCD =====
lcd lcd(21, 22); // SDA=GPIO21, SCL=GPIO22

// ===== MQTT =====
WiFiClient espClient;
PubSubClient client(espClient);

// ===== ТАЙМЕРЫ =====
unsigned long lastMsg = 0;        // для MQTT отправки
unsigned long lastSensorRead = 0; // для опроса датчика
const long mqttInterval = 100000; // 100 секунд между MQTT отправками
const long sensorInterval = 5000; // 5 секунд между чтениями DHT11
// ===== ДЛЯ LCD таймеры =====
unsigned long lastDisplayUpdate = 0;
const long displayInterval = 1000; // обновление дисплея раз в секунду

// ===== RGB СВЕТОДИОД =====
#define RED_PIN 27
#define GREEN_PIN 25
#define BLUE_PIN 26

const float TEMP_WARN = 24.71;   // порог предупреждения
const float TEMP_DANGER = 25.00; // порог аварии

bool alarmBlinkState = false;
unsigned long lastBlink = 0;
const long blinkInterval = 300;

// ===== ПЕРЕМЕННЫЕ ДЛЯ ХРАНЕНИЯ ПОСЛЕДНИХ ДАННЫХ =====
String systemState = "NORMAL"; // cостояние системы: NORMAL, WARNING, DANGER

float currentTemperature = 0.0;
float currentHumidity = 0.0;

// ===== НАСТРОЙКИ ШИМ (PWM) =====
#define PWM_FREQ 5000 // частота ШИМ 5 кГц
#define PWM_RES 8     // разрешение 8 бит (0-255)

int redChannel = 0;
int greenChannel = 1;
int blueChannel = 2;

int fanPWMChannel = 3; // для управления вентилятором через ШИМ

// ===== Вентилятор =====
#define FAN_PIN_Min 12
#define FAN_PIN_Plus 14
bool fanState = false;
bool lastFanState = false;
int fanSpeed = 0;   // 0-255 для ШИМ
void publishData(); // прототип  для функции публикации данных в MQTT, которая будет использоваться в нескольких местах

void setup()
{
  Serial.begin(115200);
  Serial.println("DHT11 + MQTT + RGB начали работу");

  dht.begin();

  // Подключение Wi-Fi
  Serial.print("Подключаюсь к Wi-Fi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWi-Fi подключён");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

  // MQTT
  client.setServer(mqtt_server, 1883);
  if (client.connect("ESP32_DHT11"))
  {
    Serial.println("MQTT подключён");
  }
  else
  {
    Serial.println("MQTT НЕ подключён!");
  }

  // Пины RGB
  pinMode(RED_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
  pinMode(BLUE_PIN, OUTPUT);

  // Настройка ШИМ-каналов
  ledcSetup(redChannel, PWM_FREQ, PWM_RES);
  ledcSetup(greenChannel, PWM_FREQ, PWM_RES);
  ledcSetup(blueChannel, PWM_FREQ, PWM_RES);

  ledcAttachPin(RED_PIN, redChannel);
  ledcAttachPin(GREEN_PIN, greenChannel);
  ledcAttachPin(BLUE_PIN, blueChannel);

  ledcSetup(fanPWMChannel, 35000, PWM_RES); // частота 35 кГц, разрешение 8 бит
  ledcAttachPin(FAN_PIN_Plus, fanPWMChannel);

  // Инициализация LCD
  lcd.Init();  // инициализация дисплея
  lcd.Clear(); // очистка
  lcd.Cursor(0, 0);
  lcd.Display("DHT11 + RGB");
  lcd.Cursor(1, 0);
  lcd.Display("Starting...");
  delay(2000);
  lcd.Clear();

  // Вентилятор
  pinMode(FAN_PIN_Min, OUTPUT);
  digitalWrite(FAN_PIN_Min, LOW);
}

void readSensors()
{
  // Читаем датчик только когда пришло время
  if (millis() - lastSensorRead >= sensorInterval)
  {
    lastSensorRead = millis();
    float h = dht.readHumidity();
    float t = dht.readTemperature();
    if (!isnan(h) && !isnan(t))
    {
      currentHumidity = h;
      currentTemperature = t;
      Serial.print("Температура: ");
      Serial.print(currentTemperature);
      Serial.println("°C");
    }
    else
    {
      Serial.println("Ошибка чтения DHT11");
    }
  }
}

void updateSystemState()
{
  if (currentTemperature < TEMP_WARN)
  {
    systemState = "NORMAL";
  }
  else if (currentTemperature < TEMP_DANGER)
  {
    systemState = "WARNING";
  }
  else
  {
    systemState = "DANGER";
  }
}

// Функция установки цвета с яркостью 0-255
void setRGB(int r, int g, int b)
{
  ledcWrite(redChannel, r);
  ledcWrite(greenChannel, g);
  ledcWrite(blueChannel, b);
}

void updateRGB()
{
  if (systemState == "NORMAL")
  {
    // 🟢 норма – зелёный на полную (или можно немного приглушить)
    setRGB(0, 200, 0);
  }
  else if (systemState == "WARNING")
  {
    // 🟡 предупреждение – жёлтый (красный 255, зелёный ~150 для баланса)
    setRGB(150, 30, 0);
  }
  else
  {
    // 🔴 авария – мигание красным (красный на полную)
    if (millis() - lastBlink >= blinkInterval)
    {
      lastBlink = millis();
      alarmBlinkState = !alarmBlinkState;
      if (alarmBlinkState)
      {
        setRGB(255, 0, 0);
      }
      else
      {
        setRGB(0, 0, 0);
      }
    }
  }
}

int computeFanSpeed()
{
  if (systemState == "NORMAL")
  {
    return 0;
  }

  else if (systemState == "WARNING")
  {
    float range = TEMP_DANGER - TEMP_WARN;
    float percent = (currentTemperature - TEMP_WARN) / range;
    float speed = 120 + percent * (200 - 120);

    return constrain((int)speed, 120, 200);
  }

  else
  {
    return 200;
  }
}
void publishFanStatus()
{
  char buf[32];
  snprintf(buf, sizeof(buf), "{\"fan\":%d}", fanState);
  client.publish("esp32/fan/status", buf);
}

// // Функция управления вентилятором с гистерезисом
void updateFan()
{
  int newSpeed = computeFanSpeed();

  if (newSpeed != fanSpeed)
  {
    fanSpeed = newSpeed;
    ledcWrite(fanPWMChannel, fanSpeed);
    fanState = (fanSpeed > 0);
    Serial.print("Fan speed: ");
    Serial.println(fanSpeed);
    publishFanStatus();

    // Отправляем полные данные ТОЛЬКО если состояние включения/выключения изменилось
    if (fanState != lastFanState)
    {
      lastFanState = fanState;
      publishData();
    }
  }
}

// Функция обновления LCD дисплея
void updateDisplay()
{

  if (millis() - lastDisplayUpdate >= displayInterval)
  {
    lastDisplayUpdate = millis();

    // Строка 0: температура и влажность
    lcd.Cursor(0, 0);
    char line0[17];
    snprintf(line0, sizeof(line0), "T:%.1fC H:%.1f%%", currentTemperature, currentHumidity);
    lcd.Display(line0);

    // Строка 2: статус
    lcd.Cursor(2, 0);
    char line2[17];
    snprintf(line2, sizeof(line2), "Status: %s", systemState.c_str());
    lcd.Display(line2);

    // Строки 2 и 3 можно оставить пустыми или использовать для другой информации
    lcd.Cursor(1, 0);
    lcd.Display("                ");

    lcd.Cursor(3, 0);
    char line3[17];
    snprintf(line3, sizeof(line3), "FAN:%d SPD:%d", fanState, fanSpeed);
    lcd.Display(line3);
  }
}

// Функция публикации данных в MQTT
void publishData()
{
  char payload[80];
  snprintf(payload, sizeof(payload),
           "{\"temperature\":%.1f,\"humidity\":%.1f,\"fan\":%d}",
           currentTemperature, currentHumidity, fanState ? 1 : 0);
  client.publish("dht11/data", payload);
  Serial.print("MQTT data sent: ");
  Serial.println(payload);
}

// --- Отправка MQTT раз в 100 секунд ---
void publishMQTT()
{
  if (millis() - lastMsg >= mqttInterval)
  {
    lastMsg = millis();
    publishData();
  }
}

void loop()
{
  // Обязательно для MQTT
  client.loop();

  // --- Чтение датчика каждые 2 секунды (обновляет currentTemperature) ---
  readSensors();
  // ===== ОБНОВЛЕНИЕ СОСТОЯНИЯ =====
  updateSystemState();

  // ===== ОБНОВЛЕНИЕ RGB =====
  updateRGB();

  // ===== УПРАВЛЕНИЕ ВЕНТИЛЯТОРОМ =====
  updateFan();

  // ===== ОБНОВЛЕНИЕ LCD =====
  updateDisplay();

  // ===== ОТПРАВКА MQTT =====
  publishMQTT();
}
