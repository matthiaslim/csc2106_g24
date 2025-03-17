from flask import Flask, request, jsonify, render_template

import time
from datetime import datetime, timezone
import db
import json

app = Flask(__name__)


@app.route('/')
def main_dashboard():
    # bins = [
    #    {"id": 1, "location": "Central Park", "temperature": 30, "capacity": 80, "status": "active", "anomaly": False},
    #    {"id": 2, "location": "Downtown", "temperature": 35, "capacity": 95, "status": "active", "anomaly": True},
    #    {"id": 3, "location": "Suburbs", "temperature": 28, "capacity": 60, "status": "inactive", "anomaly": False}
    # ]

    bins = db.get_bins()

    total_bins = len(bins)
    active_bins = sum(1 for bin in bins if bin["status"] == "active")
    full_bins = sum(1 for bin in bins if bin["capacity"] >= 90)
    anomaly_bins = sum(1 for bin in bins if bin["anomaly"])

    return render_template(
        'index.html',
        bins=bins,
        total_bins=total_bins,
        active_bins=active_bins,
        full_bins=full_bins,
        anomaly_bins=anomaly_bins
    )


@app.route('/get-latest-data')
def get_latest_bins():
    bins = db.get_latest_data()
    return jsonify(bins)


@app.route('/get-all-data')
def get_all_data():
    bins = db.get_all_data()
    return jsonify(bins)


@app.route('/ttn-webhook', methods=['POST'])
def ttn_webhook():
    data = request.json
    print(data)

    device_id = data['end_device_ids']['device_id']
    timestamp_str = data['uplink_message']['received_at']
    timestamp_str = timestamp_str[:26]  # Keep up to microseconds

    timestamp_str = timestamp_str.replace("Z", "")

    received_at = datetime.fromisoformat(
        timestamp_str).replace(tzinfo=timezone.utc).isoformat()
    payload = data['uplink_message']['decoded_payload']
    if payload:
        temperature = payload['temperature']
        fill_level = payload['fill_level']
        humidity = payload['humidity']
        smoke_concentration = payload['smoke_conc']
        lat = payload["lat"]
        lon = payload["lon"]

        db.insert_ttn_data(device_id, received_at, temperature, fill_level,
                           humidity, smoke_concentration, lat, lon)

        return jsonify({"status": "ok", "data": data})

    return jsonify({"status": "ok", "message": "Other uplink"})


if __name__ == '__main__':
    app.static_folder = 'static'
    app.run(host='127.0.0.1', port=5000, debug=True)
