# create_dht11_table.py
# Запускается один раз для создания таблицы в базе данных iot_db

import psycopg2

# Подключение к вашей существующей базе данных
conn = psycopg2.connect(
    host="localhost",
    port=5432,
    database="iot_db",
    user="iot_user",
    password="iot_password",
)

print("Connected to PostgreSQL")

cursor = conn.cursor()

# SQL запрос на создание таблицы dht11_data (если её ещё нет)
cursor.execute("""
CREATE TABLE IF NOT EXISTS dht11_data (
    id SERIAL PRIMARY KEY,
    timestamp TIMESTAMPTZ DEFAULT NOW(),
    temperature DECIMAL(5,2),
    humidity DECIMAL(5,2),
    room_name VARCHAR(50) DEFAULT 'room1'
);
""")

# Сохраняем изменения
conn.commit()

print("Table 'dht11_data' created or already exists")

cursor.close()
conn.close()
