# !/usr/bin/python

import sqlite3

# Connect to SQLite database (creates file if not exists)
conn = sqlite3.connect('bins.db')
cursor = conn.cursor()

# Drop existing tables (for re-initialization, optional)
cursor.execute("DROP TABLE IF EXISTS STATUS")
cursor.execute("DROP TABLE IF EXISTS TTN_DATA")
cursor.execute("DROP TABLE IF EXISTS DEVICES")


# Create STATUS table (for system-wide settings, optional)
cursor.execute('''CREATE TABLE STATUS (
    ITEM TEXT PRIMARY KEY,
    VALUE BOOL NOT NULL
)''')

# Create table to store TTN data
# timestamp, device_id, fill level, humidity, smoke concentration, temperature
cursor.execute('''
    CREATE TABLE TTN_DATA (
        ID INTEGER PRIMARY KEY AUTOINCREMENT,
        DEVICE_NAME TEXT NOT NULL,
        TIME TEXT NOT NULL,
        TEMPERATURE FLOAT NOT NULL,
        FILL_LEVEL INTEGER NOT NULL,
        HUMIDITY FLOAT NOT NULL,
        SMOKE_CONCENTRATION FLOAT NOT NULL,
        LAT FLOAT NOT NULL,
        LON LOAT NOT NULL
    )
''')

cursor.execute('''
    CREATE TABLE DEVICES (
        ID INTEGER PRIMARY KEY AUTOINCREMENT,
        DEVICE_NAME TEXT NOT NULL,
        FIXED_LAT FLOAT NOT NULL,
        FIXED_LON FLOAT NOT NULL,
        TIME TEXT NOT NULL,
        TEMPERATURE FLOAT NOT NULL,
        FILL_LEVEL INTEGER NOT NULL,
        HUMIDITY FLOAT NOT NULL,
        SMOKE_CONCENTRATION FLOAT NOT NULL,
        LAT FLOAT NOT NULL,
        LON LOAT NOT NULL,
        ANOMALY TEXT NOT NULL
    )
''')



# Commit and close connection
conn.commit()
print("Database initialized successfully")
conn.close()
