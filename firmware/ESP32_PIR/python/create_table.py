import psycopg2

# Подключение к PostgreSQL
conn = psycopg2.connect(
    host="localhost",
    port=5432,
    database="iot_db",
    user="iot_user",
    password="iot_password"
)

print("Connected to PostgreSQL")

cursor = conn.cursor()

# SQL запрос на создание таблицы
cursor.execute("""
CREATE TABLE IF NOT EXISTS motion_events (
    id SERIAL PRIMARY KEY,
    device_name TEXT,
    event_type TEXT,
    event_time BIGINT
);
""")

# Сохраняем изменения
conn.commit()

print("Table motion_events created")

cursor.close()
conn.close()
