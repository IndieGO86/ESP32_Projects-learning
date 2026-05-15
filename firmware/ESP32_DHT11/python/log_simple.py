# log_simple.py
# Сохраняет каждое MQTT-сообщение из топика dht11/data в текстовый файл

import paho.mqtt.client as mqtt
import json
from datetime import datetime

# Конфигурация
MQTT_BROKER = "192.168.0.103"  # IP твоего компьютера (где Mosquitto)
MQTT_TOPIC = "dht11/data"
LOG_FILE = "dht11_log.txt"


# Эта функция вызывается при подключении к брокеру
def on_connect(client, userdata, flags, rc):
    print("Connected to MQTT broker")
    client.subscribe(MQTT_TOPIC)


# Эта функция вызывается при получении сообщения
def on_message(client, userdata, msg):
    try:
        # Декодируем payload (байты -> строка)
        payload_str = msg.payload.decode()
        # Преобразуем JSON строку в словарь Python
        data = json.loads(payload_str)
        temp = data.get("temperature")
        hum = data.get("humidity")

        # Получаем текущее время
        now = datetime.now().strftime("%Y-%m-%d %H:%M:%S")

        # Формируем строку для записи в файл
        log_line = f"{now}, Temp: {temp:.1f}°C, Hum: {hum:.1f}%\n"

        # Открываем файл в режиме добавления (a) и записываем строку
        with open(LOG_FILE, "a") as f:
            f.write(log_line)

        # Для наглядности выводим в консоль
        print(log_line.strip())

    except Exception as e:
        print("Error:", e)


# Создаём клиента
client = mqtt.Client()
client.on_connect = on_connect
client.on_message = on_message

# Подключаемся к брокеру
client.connect(MQTT_BROKER, 1883, 60)

# Запускаем бесконечный цикл приёма сообщений
client.loop_forever()
