import psycopg2


conn = psycopg2.connect(
    host="localhost", database="iot_db", user="iot_user", password="iot_password"
)

cur = conn.cursor()

# cur.execute("SELECT COUNT(*) FROM fan_dht11_data")
# count = cur.fetchone()[0]

# cur.execute("SELECT AVG(temperature) FROM fan_dht11_data")
# avg_temp = cur.fetchone()[0]

# cur.execute("SELECT MAX(temperature) FROM fan_dht11_data")
# max_temp = cur.fetchone()[0]

# cur.execute("SELECT MIN(temperature) FROM fan_dht11_data")
# min_temp = cur.fetchone()[0]

# cur.execute("""
#     SELECT
#         COUNT(*),
#         AVG(temperature),
#         MAX(temperature),
#         MIN(temperature)
#     FROM fan_dht11_data
# """)

# count, avg_temp, max_temp, min_temp = cur.fetchone()

# print(f"Всего записей: {count}")
# print(f"Средняя температура: {avg_temp:.2f}")
# print(f"Максимальная температура: {max_temp:.2f}")
# print(f"Минимальная температура: {min_temp:.2f}")


# 2. Последние 10 записей
cur.execute(
    "SELECT id, temperature, humidity, fan_state FROM Fan_dht11_data ORDER BY id DESC LIMIT 10;"
)
rows = cur.fetchall()
print("\nПоследние 10 записей:")
for row in rows:
    print(f"ID:{row[0]} Temp:{row[1]}°C Hum:{row[2]}% Fan:{row[3]}")

# Колличество раз включения вентелятора
cur.execute("SELECT COUNT(*) FROM Fan_dht11_data WHERE fan_state = 1")
count_fanON = cur.fetchone()[0]
print(f"\nВключение вентелятора - {count_fanON} раз")

# вывести все записи, где fan_state = 1
cur.execute(
    "SELECT id, temperature, humidity, fan_state FROM Fan_dht11_data WHERE fan_state = 1;"
)
rows = cur.fetchall()
print("\nВсе записи, где fan_state = 1:")
for row in rows:
    print(f"ID:{row[0]} Temp:{row[1]}°C Hum:{row[2]}% Fan:{row[3]}")
    
    
#Вывести статистику только за последние 10 дней.
cur.execute("""
    SELECT AVG(temperature) FROM Fan_dht11_data
    WHERE timestamp >= NOW() - INTERVAL '10 days'
""")
avg_temp = cur.fetchone()[0]
if avg_temp:
    print(f"Средняя температура за последние 10 дней: {avg_temp:.2f}°C")
else:
    print("Нет данных за последние 10 дней")
    

cur.close()
conn.close()
