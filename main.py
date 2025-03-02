from flask import Flask, request, jsonify, render_template


import time
from datetime import datetime
import db
import json

app = Flask(__name__)





@app.route("/")
def main_dashboard():
    # plant_data_dict = db.get_plant_data()
    # status_data_dict = db.get_status_data()
    # print("plant data dict", plant_data_dict)
    # time = []
    # moisture = []
    # light = []
    # light_status = status_data_dict['LIGHT']
    # pump_status = status_data_dict['PUMP']
    # current_moisture = plant_data_dict[-1]['moisture']
    # current_light = plant_data_dict[-1]['light']

    # for entry in plant_data_dict:
    #     time.append(entry['time'])
    #     moisture.append(entry['moisture'])
    #     light.append(entry['light'])

    data = {
        "time": ["00:00:00", "13:00:00", "14:00:00", "15:00:00", "16:00:00", "17:00:00", "18:00:00", "19:00:00", "20:00:00", "21:00:00"],
        "moisture": [1, 1, 1, 1, 1, 0, 0, 0, 1, 1],
        "light": [95, 90, 84, 71, 68, 67, 52, 50, 48, 48],
        "light_status": 0,
        "pump_status": 0,
        "current_moisture": 1,
        "current_light": 48
    }
    
    # return render_template('dashboard.html', time = time, moisture = moisture, light = light, light_status = light_status, pump_status = pump_status, current_moisture = current_moisture, current_light = current_light)
    return render_template('index.html', data = data)
    return "<p>Hello, World!</p>"


if __name__ == '__main__':
   app.static_folder = 'static'
   app.run(host='127.0.0.1', port=5000, debug=True)