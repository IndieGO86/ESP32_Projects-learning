import psycopg2

conn = psycopg2.connect(
    host="localhost",
    port=5432,
    database="iot_db",
    user="iot_user",
    password="iot_password"
)

cursor = conn.cursor()

cursor.execute("SELECT * FROM motion_events;")

rows = cursor.fetchall()

for row in rows:
    print(row)

cursor.close()
conn.close()
