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
lcd lcd(21, 22);   // SDA=GPIO21, SCL=GPIO22


// ===== MQTT =====
WiFiClient espClient;
PubSubClient client(espClient);

// ===== ТАЙМЕРЫ =====
unsigned long lastMsg = 0;        // для MQTT отправки
unsigned long lastSensorRead = 0; // для опроса датчика
const long mqttInterval = 100000; // 100 секунд между MQTT отправками
const long sensorInterval = 2000; // 2 секунды между чтениями DHT11
// ===== ДЛЯ LCD таймеры =====
unsigned long lastDisplayUpdate = 0;
const long displayInterval = 1000;   // обновление дисплея раз в секунду

// ===== RGB СВЕТОДИОД =====
#define RED_PIN   27
#define GREEN_PIN 25
#define BLUE_PIN  26

const float TEMP_WARN = 23.80;     // порог предупреждения
const float TEMP_DANGER = 24.0;   // порог аварии

bool alarmBlinkState = false;
unsigned long lastBlink = 0;
const long blinkInterval = 300;

// ===== ПЕРЕМЕННЫЕ ДЛЯ ХРАНЕНИЯ ПОСЛЕДНИХ ДАННЫХ =====
float currentTemperature = 0.0;
float currentHumidity = 0.0;


// ===== НАСТРОЙКИ ШИМ (PWM) =====
#define PWM_FREQ 5000      // частота ШИМ 5 кГц
#define PWM_RES 8          // разрешение 8 бит (0-255)

int redChannel = 0;
int greenChannel = 1;
int blueChannel = 2;

// ===== Вентилятор =====
#define FAN_PIN 12
const float TEMP_COOL_ON  = 24.0;   // выше этой – включаем
const float TEMP_COOL_OFF = 23.80;   // ниже этой – выключаем (гистерезис)
bool fanState = false;


void setup() {
  Serial.begin(115200);
  Serial.println("DHT11 + MQTT + RGB начали работу");

  dht.begin();

  // Подключение Wi-Fi
  Serial.print("Подключаюсь к Wi-Fi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWi-Fi подключён");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

  // MQTT
  client.setServer(mqtt_server, 1883);
  if (client.connect("ESP32_DHT11")) {
    Serial.println("MQTT подключён");
  } else {
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

  // Инициализация LCD  
  lcd.Init();      // инициализация дисплея
  lcd.Clear();     // очистка
  lcd.Cursor(0, 0);
  lcd.Display("DHT11 + RGB");
  lcd.Cursor(1, 0);
  lcd.Display("Starting...");
  delay(2000);
  lcd.Clear();
  

  // Вентилятор
  pinMode(FAN_PIN, OUTPUT);
  digitalWrite(FAN_PIN, LOW);
}

// Функция установки цвета с яркостью 0-255
void setRGB(int r, int g, int b) {
  ledcWrite(redChannel, r);
  ledcWrite(greenChannel, g);
  ledcWrite(blueChannel, b);
}

void updateRGB() {
  if (currentTemperature < TEMP_WARN) {
    // 🟢 норма – зелёный на полную (или можно немного приглушить)
    setRGB(0, 200, 0);
  }
  else if (currentTemperature < TEMP_DANGER && currentTemperature >= TEMP_WARN) {
    // 🟡 предупреждение – жёлтый (красный 255, зелёный ~150 для баланса)
    setRGB(150, 30, 0);
  }
  else {
    // 🔴 авария – мигание красным (красный на полную)
    if (millis() - lastBlink >= blinkInterval) {
      lastBlink = millis();
      alarmBlinkState = !alarmBlinkState;
      if (alarmBlinkState) {
        setRGB(255, 0, 0);
      } else {
        setRGB(0, 0, 0);
      }
    }
  }

}


void publishFanStatus() {
    char buf[32];
    snprintf(buf, sizeof(buf), "{\"fan\":%d}", fanState);
    client.publish("esp32/fan/status", buf);
}


// // Функция управления вентилятором с гистерезисом 
void updateFan() {
    if (currentTemperature > TEMP_COOL_ON && !fanState) {  
        fanState = true;
        digitalWrite(FAN_PIN, HIGH);
        Serial.println("Fan ON");
        publishFanStatus();
    }
    else if (currentTemperature < TEMP_COOL_OFF && fanState) { 
        fanState = false;
        digitalWrite(FAN_PIN, LOW);
        Serial.println("Fan OFF");
        publishFanStatus();
    }   
}

void loop() {
  // Обязательно для MQTT
  client.loop();

  // --- Чтение датчика каждые 2 секунды (обновляет currentTemperature) ---
  if (millis() - lastSensorRead >= sensorInterval) {
    lastSensorRead = millis();

    float h = dht.readHumidity();
    float t = dht.readTemperature();

    if (!isnan(h) && !isnan(t)) {
      currentHumidity = h;
      currentTemperature = t;

      Serial.print("Температура: ");
      Serial.print(currentTemperature);
      Serial.println("%");
    }
  }
    if (millis() - lastDisplayUpdate >= displayInterval) {
    lastDisplayUpdate = millis();
    
    // Строка 0: температура и влажность
    lcd.Cursor(0, 0);
    char line0[17];
    snprintf(line0, sizeof(line0), "T:%.1fC H:%.1f%%", currentTemperature, currentHumidity);
    lcd.Display(line0);
    
    

    // Строка 2: статус
    lcd.Cursor(2, 0);
    if (currentTemperature < TEMP_WARN) {
        lcd.Display("NORMAL          ");
    } else if (currentTemperature < TEMP_DANGER) {
        lcd.Display("WARNING         ");
    } else {
        lcd.Display("ALARM!!!        ");
    }
    
    // Строки 2 и 3 можно оставить пустыми или использовать для другой информации
    lcd.Cursor(1, 0);
    lcd.Display("                ");
    lcd.Cursor(3, 0);
    if (fanState) {
        lcd.Display("      | FAN: ON ");
    } else {
        lcd.Display("      | FAN: OFF");
    }
        
  }

  // --- Обновление цвета RGB (постоянно, используя currentTemperature) ---
  updateRGB();

  

  
  // --- Управление вентилятором (постоянно, используя currentTemperature) ---
  updateFan();


  // --- Отправка MQTT раз в 100 секунд ---
  if (millis() - lastMsg >= mqttInterval) {
    lastMsg = millis();

    // Подготавливаем JSON с актуальными значениями
    char payload[80];
    snprintf(payload, sizeof(payload),
         "{\"temperature\":%.1f,\"humidity\":%.1f,\"fan\":%d}",
         currentTemperature, currentHumidity, fanState ? 1 : 0);
    
    client.publish("dht11/data", payload);
    Serial.print("MQTT отправлено: ");
    Serial.println(payload);
  }
}