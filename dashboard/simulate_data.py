import sqlite3
from datetime import datetime, timedelta, timezone
import random
import time
from geopy.distance import geodesic

conn = sqlite3.connect('bins.db')

cursor = conn.cursor()

try:
    while True:
        random_no = random.randint(0, 1)

        device_id = "my-bin-2" if random_no == 0 else "my-lorawan"
        received_at = datetime.now(timezone.utc).isoformat(
            timespec="microseconds")
        temperature = round(random.uniform(20, 40), 2)
        fill_level = random.randint(0, 100)
        humidity = round(random.uniform(0, 100), 2)
        smoke_concentration = round(random.uniform(0, 100), 2)
        dt = datetime.fromisoformat(received_at.replace(
            "Z", "+00:00"))  # Ensure it parses correctly
        adjusted_dt = dt + timedelta(hours=8)  # Adjust by 5 hours
        formatted_time = adjusted_dt.strftime("%Y-%m-%d %H:%M:%S")

        anomalies = []

        # Look for anomalies: Smoke, Temperature, Location
        if smoke_concentration > 50:
            anomalies.append("Smoke")
        if temperature > 35:
            anomalies.append("Temperature")

        latitude = 1.370653 if random_no == 0 else 1.371038
        longitude = 103.8268 if random_no == 0 else 103.825448

        cursor.execute(
            "SELECT * FROM DEVICES WHERE DEVICE_NAME = ?", (device_id,))
        row = cursor.fetchone()

        if row:
            current_location = (row[2], row[3])
            new_location = (latitude, longitude)

            distance_moved = geodesic(current_location, new_location).meters

            if distance_moved > 500:
                anomalies.append("Location")

            anomaly_str = ", ".join(anomalies) if anomalies else "No"

            cursor.execute("UPDATE DEVICES SET TIME = ?, TEMPERATURE = ?, FILL_LEVEL = ?, HUMIDITY = ?, SMOKE_CONCENTRATION = ?, LAT = ?, LON = ?, ANOMALY = ? WHERE DEVICE_NAME = ?",
                           (formatted_time, temperature, fill_level, humidity,
                            smoke_concentration, latitude, longitude, anomaly_str, device_id)
                           )
        else:
            anomaly_str = ", ".join(anomalies) if anomalies else "No"

            cursor.execute("INSERT INTO DEVICES (DEVICE_NAME, TIME, TEMPERATURE, FILL_LEVEL, HUMIDITY, SMOKE_CONCENTRATION, FIXED_LAT, FIXED_LON, LAT, LON, ANOMALY) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)",
                           (device_id, formatted_time, temperature, fill_level,
                            humidity, smoke_concentration, latitude, longitude, latitude, longitude, anomaly_str)
                           )

        cursor.execute("INSERT INTO TTN_DATA (DEVICE_NAME, TIME, TEMPERATURE, FILL_LEVEL, HUMIDITY, SMOKE_CONCENTRATION, LAT, LON) VALUES (?, ?, ?, ?, ?, ?, ?, ?)",
                       (device_id, formatted_time, temperature, fill_level,
                        humidity, smoke_concentration, latitude, longitude)
                       )
        conn.commit()

        print(f"Inserted new data at {received_at}")

        time.sleep(1)

except KeyboardInterrupt:
    print("\nStopping data simulation.")

finally:
    conn.close()
