import paho.mqtt.client as mqtt
import psycopg2
import json


def on_connect(client, userdata, flags, rc):
    print("Connected to MQTT")
    client.subscribe("dht11/data")


def on_message(client, userdata, msg):
    try:
        data = json.loads(msg.payload.decode())
        temp = data.get("temperature")
        hum = data.get("humidity")
        fan = data.get("fan")
        if temp is not None and hum is not None:
            save_to_db(temp, hum, fan)
        else:
            print("Missing temperature or humidity in message")
    except Exception as e:
        print(f"Error processing message: {e}")


def save_to_db(temperature, humidity, fan_state):
    conn = psycopg2.connect(
        host="localhost", database="iot_db", user="iot_user", password="iot_password"
    )
    cur = conn.cursor()
    cur.execute(
        "INSERT INTO Fan_dht11_data (temperature, humidity, fan_state) VALUES (%s, %s, %s)",
        (temperature, humidity, fan_state),
    )
    conn.commit()
    cur.close()
    conn.close()
    print(f"Saved to DB: {temperature}°C, {humidity}%, fan={fan_state}")


client = mqtt.Client()
client.on_connect = on_connect
client.on_message = on_message
client.connect("localhost", 1883, 60)
client.loop_forever()
