# read_dht11_data.py
# Выводит все записи из таблицы dht11_data в консоль

import psycopg2

conn = psycopg2.connect(
    host="localhost",
    port=5432,
    database="iot_db",
    user="iot_user",
    password="iot_password",
)

cursor = conn.cursor()

# Читаем все строки из таблицы
cursor.execute("SELECT * FROM dht11_data;")

rows = cursor.fetchall()

if not rows:
    print("No data in dht11_data table")
else:
    # Выводим заголовки для красоты
    print("id | timestamp | temperature | humidity | room_name")
    print("-" * 60)
    for row in rows:
        print(f"{row[0]} | {row[1]} | {row[2]} | {row[3]} | {row[4]}")

cursor.close()
conn.close()
