# Подписывается на топик dht11/data и записывает каждое сообщение в CSV-файл

import paho.mqtt.client as mqtt
import json
from datetime import datetime

# ----- НАСТРОЙКИ -----
MQTT_BROKER = "192.168.0.103"  # IP вашего MQTT брокера (компьютер)
MQTT_TOPIC = "dht11/data"  # топик, куда ESP32 публикует данные
LOG_FILE = "room1_dht11.csv"  # имя выходного файла
ROOM_NAME = "room1"  # название комнаты (будет добавлено в каждую строку)


# ----- ФУНКЦИЯ ПОДКЛЮЧЕНИЯ -----
def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print("Connected to MQTT broker")
        client.subscribe(MQTT_TOPIC)
    else:
        print(f"Connection failed with code {rc}")


# ----- ФУНКЦИЯ ОБРАБОТКИ СООБЩЕНИЙ -----
def on_message(client, userdata, msg):
    try:
        # Декодируем payload из байтов в строку
        payload_str = msg.payload.decode()
        # Преобразуем JSON в словарь Python
        data = json.loads(payload_str)
        temp = data.get("temperature")
        hum = data.get("humidity")
        if temp is None or hum is None:
            return  # если данных нет, игнорируем

        # Текущее время в читаемом формате
        now = datetime.now().strftime("%Y-%m-%d %H:%M:%S")

        # Открываем файл для добавления (a)
        with open(LOG_FILE, "a") as f:
            # Если файл пустой, записываем заголовок
            if f.tell() == 0:
                f.write("timestamp,temperature,humidity,room\n")
            # Записываем строку с данными
            f.write(f"{now},{temp:.1f},{hum:.1f},{ROOM_NAME}\n")

        print(f"[LOG] {now} - Temp: {temp:.1f}°C, Hum: {hum:.1f}%")
    except Exception as e:
        print(f"Error processing message: {e}")


# ----- СОЗДАЁМ КЛИЕНТА И ЗАПУСКАЕМ -----
client = mqtt.Client()
client.on_connect = on_connect
client.on_message = on_message
client.connect(MQTT_BROKER, 1883, 60)
client.loop_forever()  # бесконечный цикл приёма сообщений
