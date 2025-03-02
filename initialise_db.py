#!/usr/bin/python

import sqlite3

conn = sqlite3.connect('test.db')

conn.execute('''CREATE TABLE PLANT
         (SN INT PRIMARY KEY,
         TIME           VARCHAR(30)    NOT NULL,
         MOISTURE           BOOL     NOT NULL,
      LIGHT           INTEGER(10)    NOT NULL
             );''')

conn.execute('''CREATE TABLE STATUS
             (
         ITEM          VARCHAR(20)    NOT NULL,
         VALUE           BOOL     NOT NULL
             );''')

conn.execute("INSERT INTO STATUS (ITEM,VALUE) \
      VALUES ('LIGHT', 0)")

conn.execute("INSERT INTO STATUS (ITEM,VALUE) \
      VALUES ('PUMP', 0)")

conn.execute("INSERT INTO PLANT (TIME,MOISTURE,LIGHT) \
      VALUES ('00:00:00', 1, 95)")

# conn.execute("INSERT INTO PLANT (TIME,MOISTURE,LIGHT) \
#       VALUES ('13:00:00', 1, 90)")

# conn.execute("INSERT INTO PLANT (TIME,MOISTURE,LIGHT) \
#       VALUES ('14:00:00', 1, 84)")

# conn.execute("INSERT INTO PLANT (TIME,MOISTURE,LIGHT) \
#       VALUES ('15:00:00', 1, 71)")

# conn.execute("INSERT INTO PLANT (TIME,MOISTURE,LIGHT) \
#       VALUES ('16:00:00', 1, 68)")

# conn.execute("INSERT INTO PLANT (TIME,MOISTURE,LIGHT) \
#       VALUES ('17:00:00', 0, 67)")

# conn.execute("INSERT INTO PLANT (TIME,MOISTURE,LIGHT) \
#       VALUES ('18:00:00', 0, 52)")

# conn.execute("INSERT INTO PLANT (TIME,MOISTURE,LIGHT) \
#       VALUES ('19:00:00', 0, 50)")

# conn.execute("INSERT INTO PLANT (TIME,MOISTURE,LIGHT) \
#       VALUES ('20:00:00', 1, 48)")

# conn.execute("INSERT INTO PLANT (TIME,MOISTURE,LIGHT) \
#       VALUES ('20:00:00', 1, 48)")

# conn.execute("INSERT INTO PLANT (TIME,MOISTURE,LIGHT) \
#       VALUES ('20:00:00', 1, 48)")

# conn.execute("INSERT INTO PLANT (TIME,MOISTURE,LIGHT) \
#       VALUES ('20:00:00', 1, 48)")

# conn.execute("INSERT INTO PLANT (TIME,MOISTURE,LIGHT) \
#       VALUES ('20:00:00', 1, 48)")

# conn.execute("INSERT INTO PLANT (TIME,MOISTURE,LIGHT) \
#       VALUES ('20:00:00', 1, 48)")

# conn.execute("INSERT INTO PLANT (TIME,MOISTURE,LIGHT) \
#       VALUES ('20:00:00', 1, 48)")

# conn.execute("INSERT INTO PLANT (TIME,MOISTURE,LIGHT) \
#       VALUES ('20:00:00', 1, 48)")

# conn.execute("INSERT INTO PLANT (TIME,MOISTURE,LIGHT) \
#       VALUES ('20:00:00', 1, 48)")

# conn.execute("INSERT INTO PLANT (TIME,MOISTURE,LIGHT) \
#       VALUES ('20:00:00', 1, 48)")

# conn.execute("INSERT INTO PLANT (TIME,MOISTURE,LIGHT) \
#       VALUES ('20:00:00', 1, 48)")

# conn.execute("INSERT INTO PLANT (TIME,MOISTURE,LIGHT) \
#       VALUES ('20:00:00', 1, 48)")

conn.commit()
print("Records created successfully")
conn.close()