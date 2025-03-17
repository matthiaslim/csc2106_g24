#!/usr/bin/python

import sqlite3

# Connect to database (disable thread check for Flask compatibility)
conn = sqlite3.connect('bins.db', check_same_thread=False)
cursor = conn.cursor()

# 🟢 Insert new bin data


def insert_bin(location, temperature, capacity, status, anomaly):
    cursor.execute("INSERT INTO BINS (LOCATION, TEMPERATURE, CAPACITY, STATUS, ANOMALY) VALUES (?, ?, ?, ?, ?)",
                   (location, temperature, capacity, status, anomaly))
    conn.commit()
    print(
        f"Bin added: {location}, Temp: {temperature}, Capacity: {capacity}%, Status: {status}, Anomaly: {anomaly}")

# 🔵 Get all bin data


def get_bins():
    cursor.execute("SELECT * FROM BINS")
    rows = cursor.fetchall()
    bins = []
    for row in rows:
        bins.append({
            "id": row[0],
            "location": row[1],
            "temperature": row[2],
            "capacity": row[3],
            "status": row[4],
            "anomaly": bool(row[5])
        })
    print("Bins data:", bins)
    return bins

# 🟡 Update bin status


def update_bin_status(bin_id, status):
    cursor.execute("UPDATE BINS SET STATUS = ? WHERE ID = ?", (status, bin_id))
    conn.commit()
    print(f"Bin {bin_id} status updated to {status}")

# 🔴 Toggle anomaly flag


def toggle_anomaly(bin_id, anomaly):
    cursor.execute("UPDATE BINS SET ANOMALY = ? WHERE ID = ?",
                   (anomaly, bin_id))
    conn.commit()
    print(f"Bin {bin_id} anomaly updated to {anomaly}")

# 🟣 Get system-wide status (for future use)


def get_status_data():
    cursor.execute("SELECT * FROM STATUS")
    rows = cursor.fetchall()
    status_dict = {row[0]: row[1] for row in rows}
    print("System status:", status_dict)
    return status_dict

# Insert TTN data


def insert_ttn_data(device_id, received_at, temperature, fill_level, humidity, smoke_concentration, lat, lon):
    cursor.execute("INSERT INTO TTN_DATA (DEVICE_NAME, TIME, TEMPERATURE, FILL_LEVEL, HUMIDITY, SMOKE_CONCENTRATION, LAT, LON) VALUES (?, ?, ?, ?, ?, ?, ?, ?)",
                   (device_id, received_at, temperature, fill_level, humidity, smoke_concentration, lat, lon))
    conn.commit()
    print(f"TTN data added: {device_id}, {received_at}, Temp: {temperature}, Fill level: {fill_level}, Humidity: {humidity}, Smoke concentration: {smoke_concentration}")


def get_latest_data():
    cursor.execute('''SELECT * FROM TTN_DATA
                    WHERE id IN (
                    SELECT MAX(id) FROM TTN_DATA
                    GROUP BY device_id)
                   ''')
    rows = cursor.fetchall()
    data = []
    for row in rows:
        data.append({
            "id": row[0],
            "device_name": row[1],
            "received_at": row[2],
            "temperature": row[3],
            "fill_level": row[4],
            "humidity": row[5],
            "smoke_conc": row[6],
            "lat": row[7],
            "lon": row[8]
        })
    return data


def get_all_data():
    cursor.execute('SELECT * FROM TTN_DATA')
    rows = cursor.fetchall()
    data = []
    for row in rows:
        data.append({
            "id": row[0],
            "device_name": row[1],
            "received_at": row[2],
            "temperature": row[3],
            "fill_level": row[4],
            "humidity": row[5],
            "smoke_conc": row[6],
            "lat": row[7],
            "lon": row[8]
        })
    return data


def get_devices():
    cursor.execute('''SELECT device_id FROM TTN_DATA'
                    WHERE id IN (
                    SELECT MAX(id) FROM TTN_DATA
                    GROUP BY device_id)
                    ''')

# 🟠 Close DB connection (optional)


def close_connection():
    conn.close()
