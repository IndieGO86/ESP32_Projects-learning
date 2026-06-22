import psycopg2
from datetime import datetime


def get_last_measurement():

    # 1. Подключаемся к базе данных
    conn = psycopg2.connect(
        host="localhost", database="iot_db", user="iot_user", password="iot_password"
    )

    # 2. Создаём курсор для выполнения запросов
    cur = conn.cursor()

    # 3. Запрашиваем последнюю запись (сортируем по id, берём первую)
    cur.execute("""
    SELECT temperature, humidity, fan_state, timestamp
    FROM Fan_dht11_data
    ORDER BY id DESC
    LIMIT 1
""")

    # 4. Получаем результат (одна строка или None)
    row = cur.fetchone()

    # 5. Если запись есть – выводим красиво
    if row:
        temp = row[0]
        hum = row[1]
        fan = row[2]
        ts = row[3]  # это объект datetime

        # Преобразуем состояние вентилятора в "ON" или "OFF"
        fan_status = "ON" if fan == 1 else "OFF"

        # Форматируем время в читаемый вид
        time_str = ts.strftime("%Y-%m-%d %H:%M:%S")

    # # Выводим результат
    # print("=== Последнее измерение ===")
    # print(f"Температура: {temp:.1f}°C")
    # print(f"Влажность:   {hum:.1f}%")
    # print(f"Вентилятор:  {fan_status}")
    # print(f"Время:       {time_str}")
    else:
        print("В таблице нет данных.")

    # 6. Закрываем соединение
    cur.close()
    conn.close()

    return {
        "temperature": float(temp),
        "humidity": float(hum),
        "fan": fan_status,
        "timestamp": time_str,
    }


data = get_last_measurement()
print(data)
