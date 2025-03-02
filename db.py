#!/usr/bin/python

import sqlite3

conn = sqlite3.connect('test.db', check_same_thread=False)

def insert_data(time, moisture, light):
    conn.execute("INSERT INTO PLANT (TIME,MOISTURE,LIGHT) \
      VALUES (?, ?,?)", (time, moisture, light))
    conn.commit()
    print("Records created successfully, time = ", time, " moisture_level = ", moisture , " light_level = ", light)
    print("############################")
    
def toggle_light(value):
    conn.execute("UPDATE STATUS SET VALUE = ? WHERE ITEM = 'LIGHT'", (value,))
    conn.commit()
    print("Light status updated successfully, status = ", value)

def toggle_pump(value):
    conn.execute("UPDATE STATUS SET VALUE = ? WHERE ITEM = 'PUMP'", (value,))
    conn.commit()
    print("Pump status updated successfully, status = ", value)

def get_status_data():
    cursor = conn.execute("SELECT * from STATUS")
    rows = cursor.fetchall()
    data = {}
    for row in rows:

        data[row[0]] = row[1]

    print(data, "status data")
    return data

def get_plant_data():
    data = []
    cursor = conn.execute("SELECT * from PLANT")
    rows = cursor.fetchall()
    for row in rows:
        temp_dict = {
            "time": row[1],
            "moisture": row[2],
            "light": row[3]
        }
        data.append(temp_dict)
    print(data, "polant")
    return data

            # "sn": row[0],
            # "time": row[1],
            # "moisture": row[2],
            # "light": row[3]