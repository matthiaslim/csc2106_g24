import base64
import os
from flask import Flask, request, jsonify, render_template

from dotenv import load_dotenv
from datetime import datetime, timedelta, timezone

import db

load_dotenv()

app = Flask(__name__)

TTN_USERNAME = os.getenv("TTN_WEBHOOK_USERNAME", "myuser")
TTN_PASSWORD = os.getenv("TTN_WEBHOOK_PASSWORD", "mypassword")
TTN_API_KEY = os.getenv("TTN_API_KEY", "")
api_key = os.getenv("MAP_API_KEY", "")


def verify_basic_auth(auth_header):
    """Verify Basic Auth Credentials"""
    if not auth_header or not auth_header.startswith("Basic "):
        return False

    # Decode Base64 encoded credentials
    encoded_credentials = auth_header.split(" ")[1]  # Extract Base64 part
    decoded_credentials = base64.b64decode(
        encoded_credentials).decode()  # Decode to string

    # Split into username:password
    username, password = decoded_credentials.split(":", 1)

    return username == TTN_USERNAME and password == TTN_PASSWORD


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


    general_metrics = db.get_general_metrics()

    full_bin_history = db.get_full_bins()

    latest_data = db.get_latest_data()

    total_bins = general_metrics["total_devices"]
    active_bins = general_metrics["active_devices"]
    full_bins = general_metrics["num_full"]
    active_bins_graph = active_bins - full_bins
    full_bins_perctg = int((full_bins / active_bins) *
                           100) if active_bins > 0 else 0
    anomaly_bins = general_metrics["num_anomaly"]
    inactive_bins = total_bins - active_bins

    data = {
        "bins": latest_data,
        "total_bins": total_bins,
        "active_bins": active_bins,
        "full_bins": full_bins,
        "full_bins_perctg": full_bins_perctg,
        "full_bin_history": full_bin_history,
        "anomaly_bins": anomaly_bins,
        "inactive_bins": inactive_bins,
        "active_bins_graph": active_bins_graph
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


@app.route('/bin_table')
def bin_table():
    bins = db.get_latest_data()
    api_key = get_api_key()

    return render_template(
        'table.html',
        data=bins,
        api_key=api_key

    )


@app.route("/get_bins")
def get_bins(): # for the polling refresh of dashboard

    data = get_data()

    return jsonify(data)


@app.route('/ttn-webhook', methods=['POST'])
def ttn_webhook():
    headers = request.headers.get("Authorization")

    if not verify_basic_auth(headers):
        return jsonify({"status": "error", "message": "Unauthorized"}), 401

    data = request.json

    device_id = data['end_device_ids']['device_id']
    timestamp_str = data['uplink_message']['received_at']
    timestamp_str = timestamp_str[:26] + "Z"

    device_timestamp = datetime.strptime(
        timestamp_str, "%Y-%m-%dT%H:%M:%S.%fZ").replace(tzinfo=timezone.utc)

    current_time = (datetime.now() - timedelta(hours=8)
                    ).replace(tzinfo=timezone.utc)

    time_difference = (current_time - device_timestamp).total_seconds()
    print(f"{time_difference} {current_time} {device_timestamp}")

    if time_difference > 120:
        return jsonify({"error": "Replay Attack Detected!"}), 403

    timestamp_str = timestamp_str.replace("Z", "")

    received_at = datetime.fromisoformat(
        timestamp_str).replace(tzinfo=timezone.utc).isoformat()
    if 'decoded_payload' in data['uplink_message']:
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
    else:
        return jsonify({"status": "error", "message": "No decoded payload"})

    return jsonify({"status": "ok", "message": "Other uplink"})


@app.route('/benchmark-uplink', methods=['POST'])
def benchmark_uplink():
    data = request.json

    device_id = data['end_device_ids']['device_id']
    print(device_id)
    timestamp_str = data['uplink_message']['received_at']
    timestamp_str = timestamp_str[:26] + "Z"

    device_timestamp = datetime.strptime(
        timestamp_str, "%Y-%m-%dT%H:%M:%S.%fZ").replace(tzinfo=timezone.utc)

    current_time = (datetime.now() - timedelta(hours=8)
                    ).replace(tzinfo=timezone.utc)

    time_difference = (current_time - device_timestamp).total_seconds()
    print(f"{time_difference} {current_time} {device_timestamp}")

    if time_difference > 120:
        return jsonify({"error": "Replay Attack Detected!"}), 403

    timestamp_str = timestamp_str.replace("Z", "")

    received_at = datetime.fromisoformat(
        timestamp_str).replace(tzinfo=timezone.utc).isoformat()

    # Check if there's a decoded payload in the data
    if 'uplink_message' in data and 'decoded_payload' in data['uplink_message']:
        # Access the decoded payload data field (updated for TTN V3 format)
        payload = data['uplink_message']['decoded_payload']

        if payload:

            # Example LoRa timestamp in seconds (from epoch)
            lora_epoch = payload["startTime"]

            # Convert LoRa epoch seconds to a datetime object
            lora_time = datetime.utcfromtimestamp(lora_epoch)

            # Get the current Flask server time in UTC (with microseconds)
            flask_time = datetime.utcnow()

            # Calculate the time difference
            time_difference = flask_time - lora_time

            # Convert the difference to milliseconds
            milliseconds_difference = time_difference.total_seconds() * 1000

            db.insert_metrics(milliseconds_difference)

            # Print the time difference
            print(f"Time difference: {milliseconds_difference:.2f} ms")
    else:
        return jsonify({"status": "error", "message": "No decoded payload"})

    return jsonify({"status": "ok", "message": "Other uplink"})


if __name__ == '__main__':
    app.static_folder = 'static'
    app.run(host='127.0.0.1', port=5000, debug=True)
