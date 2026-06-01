import paho.mqtt.client as mqtt
import json
import psycopg2

# Параметры подключения к PostgreSQL
DB_CONFIG = {
    "host": "localhost",
    "port": 5432,
    "database": "iot_db",
    "user": "iot_user",
    "password": "iot_password",
}


def save_to_db(temperature, humidity, relay_state):
    """Функция для вставки одной строки в таблицу dht11_data"""
    try:
        # Устанавливаем соединение с БД
        conn = psycopg2.connect(**DB_CONFIG)
        cur = conn.cursor()
        # Выполняем INSERT
        cur.execute(
            "INSERT INTO dht11_data (temperature, humidity, relay_state) VALUES (%s, %s, %s)",
            (temperature, humidity, relay_state),
        )
        conn.commit()  # фиксируем изменения
        cur.close()
        conn.close()
        print(f"Сохранено: t={temperature}, h={humidity}, relay={relay_state}")
        return True
    except Exception as e:
        print(f"Ошибка БД: {e}")
        return False


def on_connect(client, userdata, flags, rc):
    """Вызывается при подключении к MQTT брокеру"""
    if rc == 0:
        print("Подключено к MQTT брокеру")
        # Подписываемся на топик
        client.subscribe("dht11/data")
    else:
        print(f"Ошибка подключения к MQTT, код {rc}")


def on_message(client, userdata, msg):
    """Вызывается при получении сообщения в подписанный топик"""
    try:
        # Декодируем payload (байты -> строка)
        payload_str = msg.payload.decode()
        # Превращаем JSON строку в словарь Python
        data = json.loads(payload_str)
        # Извлекаем значения
        temp = data.get("temperature")
        hum = data.get("humidity")
        relay = data.get("relayState")  # поле из вашего ESP32
        # Если не хватает обязательных полей, выходим
        if temp is None or hum is None:
            print("В сообщении нет temperature или humidity")
            return
        # Сохраняем в БД
        save_to_db(temp, hum, relay)
    except json.JSONDecodeError as e:
        print(f"Ошибка разбора JSON: {e}")
    except Exception as e:
        print(f"Ошибка обработки MQTT: {e}")


# Создаём MQTT клиента
client = mqtt.Client()
# Назначаем callback-функции
client.on_connect = on_connect
client.on_message = on_message

# Подключаемся к брокеру (здесь localhost, порт 1883)
client.connect("localhost", 1883, 60)

print("Ожидание MQTT сообщений...")
# Запускаем бесконечный цикл обработки сообщений
client.loop_forever()
