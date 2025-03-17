from flask import Flask, request, jsonify, render_template

import time
from datetime import datetime, timezone
import db
import json

app = Flask(__name__)


def get_api_key():
    try:
        with open("api_key.txt", "r") as file:
            return file.read().strip()
    except FileNotFoundError:
        return ""
    except Exception as e:
        print(f"Error reading API key: {e}")
        return ""


def get_data():

    bins = db.get_bins()
    total_bins = len(bins)
    active_bins = sum(1 for bin in bins if bin["status"] == "active")
    full_bins = sum(1 for bin in bins if bin["capacity"] >= 90)
    anomaly_bins = sum(1 for bin in bins if bin["anomaly"])
    full_bins_perctg = int((full_bins/active_bins)*100)
    normal_bins = active_bins - full_bins - anomaly_bins

    data = {
        "bins": bins,
        "total_bins": total_bins,
        "active_bins": active_bins,
        "full_bins": full_bins,
        "full_bins_perctg": full_bins_perctg,
        "anomaly_bins": anomaly_bins,
        "normal_bins": normal_bins
    }

    return data


@app.route('/')
def main_dashboard():

    api_key = get_api_key()

    data = get_data()

    bins = data["bins"]
    total_bins = data["total_bins"]
    active_bins = data["active_bins"]
    full_bins = data["full_bins"]
    full_bins_perctg = data["full_bins_perctg"]
    anomaly_bins = data["anomaly_bins"]

    return render_template(
        'index.html',
        bins=bins,
        total_bins=total_bins,
        active_bins=active_bins,
        full_bins=full_bins,
        anomaly_bins=anomaly_bins,
        full_bins_perctg=full_bins_perctg,
        api_key=api_key
    )


@app.route('/table')
def table():

    api_key = get_api_key()

    data = get_data()

    bins = data["bins"]
    total_bins = data["total_bins"]
    active_bins = data["active_bins"]
    full_bins = data["full_bins"]
    full_bins_perctg = data["full_bins_perctg"]
    anomaly_bins = data["anomaly_bins"]

    return render_template(
        'table.html',
        bins=bins,
        total_bins=total_bins,
        active_bins=active_bins,
        full_bins=full_bins,
        anomaly_bins=anomaly_bins,
        full_bins_perctg=full_bins_perctg,
        api_key=api_key
    )


@app.route('/bin_table')
def bin_table():
    data = get_data()

    bins = data["bins"]

    return render_template('tables.html', bins=bins)


@app.route("/get_bins")
def get_bins():

    data = get_data()

    return jsonify(data)


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
