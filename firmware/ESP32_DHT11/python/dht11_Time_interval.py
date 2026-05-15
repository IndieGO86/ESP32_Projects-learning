# Подписывается на топик dht11/data, сохраняет в CSV и PostgreSQL

import json
import time
import paho.mqtt.client as mqtt
import psycopg2

# ===== Подключение к PostgreSQL =====
conn = psycopg2.connect(
    host="localhost",
    port=5432,
    database="iot_db",
    user="iot_user",
    password="iot_password",
)
cursor = conn.cursor()
print("Connected to PostgreSQL")

# ===== Настройки =====
MQTT_BROKER = "192.168.0.103"  # IP вашего MQTT брокера
MQTT_TOPIC = "dht11/data"
LOG_FILE = "room1_dht11.csv"
ROOM_NAME = "room1"

# ===== CSV: если файл пустой, добавим заголовок =====
try:
    with open(LOG_FILE, "r") as f:
        pass
except FileNotFoundError:
    with open(LOG_FILE, "w") as f:
        f.write("timestamp,temperature,humidity,room\n")


# ===== MQTT callbacks =====
def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print("Connected to MQTT broker")
        client.subscribe(MQTT_TOPIC)
    else:
        print(f"Connection failed with code {rc}")


def on_message(client, userdata, msg):
    try:
        # Разбор JSON из MQTT
        data = json.loads(msg.payload.decode())
        temp = data.get("temperature")
        hum = data.get("humidity")
        if temp is None or hum is None:
            return

        # Читаемое время
        now = time.strftime("%Y-%m-%d %H:%M:%S")

        # Вывод в консоль
        print(f"[DHT11] {now} - Temp: {temp:.1f}°C, Hum: {hum:.1f}%")

        # 1. Сохраняем в CSV
        with open(LOG_FILE, "a") as f:
            f.write(f"{now},{temp:.1f},{hum:.1f},{ROOM_NAME}\n")

        # 2. Сохраняем в PostgreSQL
        cursor.execute(
            """
            INSERT INTO dht11_data (temperature, humidity, room_name, timestamp)
            VALUES (%s, %s, %s, %s)
        """,
            (temp, hum, ROOM_NAME, now),
        )
        conn.commit()

    except Exception as e:
        print(f"Error: {e}")


# ===== Запуск клиента =====
client = mqtt.Client()
client.on_connect = on_connect
client.on_message = on_message
client.connect(MQTT_BROKER, 1883, 60)

print("Listening for DHT11 data...")
try:
    while True:
        client.loop()
        time.sleep(0.1)
except KeyboardInterrupt:
    print("\nОстановка программы")
finally:
    cursor.close()
    conn.close()
