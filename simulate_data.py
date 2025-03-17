import sqlite3
from datetime import datetime, timezone
import random
import time

conn = sqlite3.connect('bins.db')

cursor = conn.cursor()

try:
    while True:
        device_id = "my-bin-2"
        received_at = datetime.now(timezone.utc).isoformat(
            timespec="microseconds")
        temperature = round(random.uniform(20, 40), 2)
        fill_level = random.randint(0, 100)
        humidity = round(random.uniform(0, 100), 2)
        smoke_concentration = round(random.uniform(0, 100), 2)

        latitude = 1.370653
        longitude = 103.8268

        cursor.execute("INSERT INTO TTN_DATA (DEVICE_NAME, TIME, TEMPERATURE, FILL_LEVEL, HUMIDITY, SMOKE_CONCENTRATION, LAT, LON) VALUES (?, ?, ?, ?, ?, ?, ?, ?)",
                       (device_id, received_at, temperature, fill_level,
                        humidity, smoke_concentration, latitude, longitude)
                       )
        conn.commit()

        print(f"Inserted new data at {received_at}")

        time.sleep(30)

except KeyboardInterrupt:
    print("\nStopping data simulation.")

finally:
    conn.close()
