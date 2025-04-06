#!/usr/bin/python

from collections import defaultdict
from datetime import datetime, timedelta
import sqlite3
import time
from geopy.distance import geodesic

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
            "lat": row[2],
            "lon": row[3],
            "temperature": row[4],
            "capacity": row[5],
            "status": row[6],
            "anomaly": bool(row[7])
        })
    # print("Bins data:", bins)
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
    # Check if device exists first. If exists, update, else, insert
    cursor.execute("SELECT * FROM DEVICES WHERE DEVICE_NAME = ?", (device_id,))
    row = cursor.fetchone()

    anomalies = []

    # Look for anomalies: Smoke, Temperature, Location
    if smoke_concentration > 50:
        anomalies.append("Smoke")
    if temperature > 35:
        anomalies.append("Temperature")

    if row:
        current_location = (row[2], row[3])
        print(current_location)
        new_location = (lat, lon)

        distance_moved = geodesic(current_location, new_location).meters

        if distance_moved > 500:
            anomalies.append("Location")

        anomaly_str = ", ".join(anomalies) if anomalies else "No"

        cursor.execute("UPDATE DEVICES SET TIME = ?, TEMPERATURE = ?, FILL_LEVEL = ?, HUMIDITY = ?, SMOKE_CONCENTRATION = ?, LAT = ?, LON = ?, ANOMALY = ? WHERE DEVICE_NAME = ?",
                       (formatted_dt, temperature, fill_level, humidity,
                        smoke_concentration, lat, lon, anomaly_str, device_id)
                       )
    else:
        cursor.execute("INSERT INTO DEVICES (DEVICE_NAME, TIME, TEMPERATURE, FILL_LEVEL, HUMIDITY, SMOKE_CONCENTRATION, FIXED_LAT, FIXED_LON, LAT, LON, ANOMALY) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)",
                       (device_id, formatted_dt, temperature, fill_level,
                        humidity, smoke_concentration, lat, lon, lat, lon, anomaly_str)
                       )

    cursor.execute("INSERT INTO TTN_DATA (DEVICE_NAME, TIME, TEMPERATURE, FILL_LEVEL, HUMIDITY, SMOKE_CONCENTRATION, LAT, LON) VALUES (?, ?, ?, ?, ?, ?, ?, ?)",
                   (device_id, formatted_dt, temperature, fill_level,
                    humidity, smoke_concentration, lat, lon)
                   )

    conn.commit()
    print(f"TTN data added: {device_id}, {received_at}, Temp: {temperature}, Fill level: {fill_level}, Humidity: {humidity}, Smoke concentration: {smoke_concentration}")


def get_latest_data():
    cursor.execute('''SELECT * FROM DEVICES''')

    rows = cursor.fetchall()
    data = []
    for row in rows:
        current_time = datetime.now().strftime("%Y-%m-%d %H:%M:%S")

        fmt = "%Y-%m-%d %H:%M:%S"
        current_dt = datetime.strptime(current_time, fmt)
        received_dt = datetime.strptime(row[4], fmt)

        time_diff = current_dt - received_dt
        time_diff_minutes = time_diff.total_seconds() / 60

        active = "Yes" if time_diff_minutes < 5 else "No"

        data.append({
            "id": row[0],
            "device_name": row[1],
            "fixed_lat": row[2],
            "fixed_lon": row[3],
            "received_at": row[4],
            "temperature": row[5],
            "fill_level": row[6],
            "humidity": row[7],
            "smoke_concentration": row[8],
            "lat": row[9],
            "lon": row[10],
            "active": active,
            "anomaly": row[11]
        })
    return data


def get_general_metrics():
    cursor.execute('SELECT * FROM DEVICES')
    rows = cursor.fetchall()

    current_time = datetime.now().strftime("%Y-%m-%d %H:%M:%S")

    fmt = "%Y-%m-%d %H:%M:%S"
    current_dt = datetime.strptime(current_time, fmt)
    received_dt = [datetime.strptime(row[4], fmt) for row in rows]

    time_diff_minutes = [
        (current_dt - received_dt).total_seconds() / 60 for received_dt in received_dt]

    active_bins = []

    for row in rows:
        received_time = datetime.strptime(row[4], fmt)
        time_diff = (current_dt - received_time).total_seconds() / 60
        if time_diff < 5:
            active_bins.append(row)

    total_devices = len(rows)
    active_devices = len(active_bins)
    num_anomaly = sum([1 for row in active_bins if row[11] != "No"])
    num_full = sum([1 for row in active_bins if row[6] > 80])

    return {
        "total_devices": total_devices,
        "active_devices": active_devices,
        "num_anomaly": num_anomaly,
        "num_full": num_full
    }


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
            "smoke_concentration": row[6],
            "lat": row[7],
            "lon": row[8]
        })
    return data

# Get hourly full bin data
# Get last 6 hours of data based on current time of unique bins, condition should be fill_level > 80, get count of bins that are full, per hour


def get_full_bins():
    cursor.execute('''
        SELECT 
            strftime('%H', TIME) AS hour_group,
            DEVICE_NAME  -- Select only the necessary fields for the count
        FROM TTN_DATA
        WHERE TIME >= datetime('now', '-6 hours', '+8 hours')
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


def insert_metrics(metric):
    cursor.execute(
        "INSERT INTO BENCHMARK_METRICS (METRIC) VALUES (?)", (metric,))
    conn.commit()
    print(f"Metric added: {metric}")

# 🟠 Close DB connection (optional)


def close_connection():
    conn.close()
