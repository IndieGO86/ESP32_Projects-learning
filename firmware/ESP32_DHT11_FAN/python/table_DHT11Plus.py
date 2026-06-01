import psycopg2

# Подключаемся к базе
conn = psycopg2.connect(
    host="localhost",
    port=5432,
    database="iot_db",
    user="iot_user",
    password="iot_password"
)
cursor = conn.cursor()

# SQL-запрос на создание таблицы (если её ещё нет)
cursor.execute("""
CREATE TABLE IF NOT EXISTS dht11_data (
    id SERIAL PRIMARY KEY,
    timestamp TIMESTAMPTZ DEFAULT NOW(),
    temperature DECIMAL(5,2),
    humidity DECIMAL(5,2),
    relay_state INTEGER
);
""")

conn.commit()
print("Таблица создана (или уже существует).")

cursor.close()
conn.close()
