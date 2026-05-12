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

# Создаём cursor
cursor = conn.cursor()

# Проверочный SQL запрос
cursor.execute("SELECT version();")

# Получаем результат
result = cursor.fetchone()

print(result)

# Закрываем соединение
cursor.close()
conn.close()
