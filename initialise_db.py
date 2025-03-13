# !/usr/bin/python

import sqlite3

# Connect to SQLite database (creates file if not exists)
conn = sqlite3.connect('bins.db')
cursor = conn.cursor()

# Drop existing tables (for re-initialization, optional)
cursor.execute("DROP TABLE IF EXISTS BINS")
cursor.execute("DROP TABLE IF EXISTS STATUS")

# Create BINS table
cursor.execute('''CREATE TABLE BINS (
    ID INTEGER PRIMARY KEY AUTOINCREMENT,
    LOCATION TEXT NOT NULL,
    TEMPERATURE FLOAT NOT NULL,
    CAPACITY INTEGER NOT NULL,
    STATUS TEXT NOT NULL CHECK(STATUS IN ('active', 'inactive')),
    ANOMALY BOOL NOT NULL
)''')

# Create STATUS table (for system-wide settings, optional)
cursor.execute('''CREATE TABLE STATUS (
    ITEM TEXT PRIMARY KEY,
    VALUE BOOL NOT NULL
)''')

# Insert initial bin data (you can modify this for our project)
bins_data = [
    ("Central Park", 30.5, 80, "active", 0),
    ("Downtown", 35.2, 95, "active", 1),
    ("Suburbs", 28.0, 60, "inactive", 0)
]

cursor.executemany("INSERT INTO BINS (LOCATION, TEMPERATURE, CAPACITY, STATUS, ANOMALY) VALUES (?, ?, ?, ?, ?)", bins_data)

# Insert initial system status
status_data = [
    ("LIGHT", 0),
    ("PUMP", 0)
]
cursor.executemany("INSERT INTO STATUS (ITEM, VALUE) VALUES (?, ?)", status_data)

# Commit and close connection
conn.commit()
print("Database initialized successfully")
conn.close()
