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
    print(f"Bin added: {location}, Temp: {temperature}, Capacity: {capacity}%, Status: {status}, Anomaly: {anomaly}")

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
    cursor.execute("UPDATE BINS SET ANOMALY = ? WHERE ID = ?", (anomaly, bin_id))
    conn.commit()
    print(f"Bin {bin_id} anomaly updated to {anomaly}")

# 🟣 Get system-wide status (for future use)
def get_status_data():
    cursor.execute("SELECT * FROM STATUS")
    rows = cursor.fetchall()
    status_dict = {row[0]: row[1] for row in rows}
    print("System status:", status_dict)
    return status_dict

# 🟠 Close DB connection (optional)
def close_connection():
    conn.close()
