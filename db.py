#!/usr/bin/python

from collections import defaultdict
from datetime import datetime, timedelta
import json
import sqlite3
import time

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
            "latitude": row[2],
            "longitude": row[3],
            "temperature": row[4],
            "capacity": row[5],
            "status": row[6],
            "anomaly": bool(row[7])
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
    dt = datetime.fromisoformat(received_at)
    local_time = dt + timedelta(hours=8)
    formatted_dt = local_time.strftime("%Y-%m-%d %H:%M:%S")

    cursor.execute("INSERT INTO TTN_DATA (DEVICE_NAME, TIME, TEMPERATURE, FILL_LEVEL, HUMIDITY, SMOKE_CONCENTRATION, LAT, LON) VALUES (?, ?, ?, ?, ?, ?, ?, ?)",
                   (device_id, formatted_dt, temperature, fill_level, humidity, smoke_concentration, lat, lon))
    conn.commit()
    print(f"TTN data added: {device_id}, {received_at}, Temp: {temperature}, Fill level: {fill_level}, Humidity: {humidity}, Smoke concentration: {smoke_concentration}")


def get_latest_data():
    cursor.execute('''SELECT * FROM TTN_DATA
                    WHERE id IN (
                    SELECT MAX(id) FROM TTN_DATA
                    GROUP BY device_name)
                   ''')
    rows = cursor.fetchall()
    data = []
    for row in rows:
        smoke = "Yes" if row[6] > 300 else "No"

        data.append({
            "id": row[0],
            "device_name": row[1],
            "received_at": row[2],
            "temperature": row[3],
            "fill_level": row[4],
            "humidity": row[5],
            "smoke": smoke,
            "lat": row[7],
            "lon": row[8]
        })
    return data


def get_all_data():
    cursor.execute('SELECT * FROM TTN_DATA')
    rows = cursor.fetchall()
    data = []
    for row in rows:
        smoke = "Yes" if row[6] > 300 else "No"

        data.append({
            "id": row[0],
            "device_name": row[1],
            "received_at": row[2],
            "temperature": row[3],
            "fill_level": row[4],
            "humidity": row[5],
            "smoke": smoke,
            "lat": row[7],
            "lon": row[8]
        })
    return data


def get_devices():
    cursor.execute('''SELECT device_name FROM TTN_DATA'
                    WHERE id IN (
                    SELECT MAX(id) FROM TTN_DATA
                    GROUP BY device_name)
                    ''')

# Get hourly full bin data
# Get last 6 hours of data based on current time of unique bins, condition should be fill_level > 80, get count of bins that are full, per hour


def get_full_bins():
    cursor.execute('''
        SELECT 
            strftime('%H', TIME) AS hour_group,
            DEVICE_NAME  -- Select only the necessary fields for the count
        FROM TTN_DATA
        WHERE TIME >= datetime('now', '-6 hours')
        AND FILL_LEVEL > 80
        ORDER BY hour_group DESC, TIME DESC;
    ''')
    rows = cursor.fetchall()
    data = []

    current_hour = int(time.strftime("%H"))
    last_six_hours = [(current_hour - i) % 24 for i in range(6)]

    hourly_bin_counts = {str(hour).zfill(2): 0 for hour in last_six_hours}

    temp_counts = defaultdict(set)

    for row in rows:
        hour = row[0]  # Extract hour
        device_name = row[1]  # Extract device name
        temp_counts[hour].add(device_name)  # Add unique device to the hour

    # Update the final hourly_bin_counts from temp_counts
    for hour, devices in temp_counts.items():
        hourly_bin_counts[hour] = len(devices)

    # Convert to JSON-friendly format
    data = [{"hour": hour, "full_bins": count}
            for hour, count in sorted(hourly_bin_counts.items(), reverse=True)]
    return data

# 🟠 Close DB connection (optional)


def close_connection():
    conn.close()
