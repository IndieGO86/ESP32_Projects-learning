import json
import time
import paho.mqtt.client as mqtt
import psycopg2

eventCount = 0
only_date = ""

# ===== PostgreSQL connection =====

conn = psycopg2.connect(
    host="localhost",
    port=5432,
    database="iot_db",
    user="iot_user",
    password="iot_password",
)

cursor = conn.cursor()

print("Connected to PostgreSQL")


# Функция, которая вызывается при успешном подключении к MQTT-брокеру
def on_connect(client, userdata, flags, rc):
    # rc == 0 означает успешное подключение
    print("Connected to MQTT")
    # Подписываемся на топик "pir/event" – ESP32 будет публиковать туда события
    client.subscribe("pir/event")


# Функция, которая вызывается при каждом получении MQTT-сообщения
def on_message(client, userdata, msg):
    global eventCount
    global only_date
    # # # Преобразуем payload (байты) в строку и парсим JSON в словарь Python
    data = json.loads(msg.payload.decode())

    # # # Извлекаем значение по ключу "timestamp" (если оно есть)
    timestamp = data.get("timestamp")

    # # # Преобразуем Unix-время в читаемую строку: год-месяц-день час:мин:сек
    readable_time = time.strftime("%Y-%m-%d %H:%M:%S", time.localtime(timestamp))
    only_date = time.strftime("%Y-%m-%d", time.localtime(timestamp))

    # # Выводим информацию о событии
    # print("TOPIC:", msg.topic)
    # print(" device:", data.get("device"))

    if data["event"] == "motion_start":
        eventCount += 1
        print("Движение")
        print(" device:", data.get("device"))
        print(" event :", data.get("event"))
        print(" time  :", readable_time)
        print("-" * 30)
    else:
        print("Тихо")
        print(" device:", data.get("device"))
        print(" event :", data.get("event"))
        print(" time  :", readable_time)
        print("-" * 30)

    # добавляем данные в файл events.log
    with open("events.log", "a") as f:
        f.write(json.dumps(data) + "\n")

    # ===== сохраняем событие в PostgreSQL =====
    cursor.execute(
        """
        INSERT INTO motion_events (device_name, event_type, event_time)
        VALUES (%s, %s, %s)
        """,
        (data.get("device"), data.get("event"), data.get("time")),
    )

    conn.commit()


# Создаём экземпляр MQTT-клиента
client = mqtt.Client()
# Привязываем наши callback-функции
client.on_connect = on_connect
client.on_message = on_message

client.connect("localhost", 1883, 60)


# Запускаем бесконечный цикл обработки сетевого трафика и вызовов callback'ов
try:
    while True:
        client.loop()
        time.sleep(0.1)

except KeyboardInterrupt:
    print("\nОстановка программы")
    print(only_date)


finally:
    print("ИТОГО motion_start:", eventCount)

    with open("events.log", "a") as f:
        f.write(f"\nTOTAL motion_start: {eventCount}\n")
