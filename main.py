from flask import Flask, request, jsonify, render_template

import time
from datetime import datetime
import db
import json

app = Flask(__name__)

@app.route('/')
def main_dashboard():
    #bins = [
    #    {"id": 1, "location": "Central Park", "temperature": 30, "capacity": 80, "status": "active", "anomaly": False},
    #    {"id": 2, "location": "Downtown", "temperature": 35, "capacity": 95, "status": "active", "anomaly": True},
    #    {"id": 3, "location": "Suburbs", "temperature": 28, "capacity": 60, "status": "inactive", "anomaly": False}
    #]

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



if __name__ == '__main__':
   app.static_folder = 'static'
   app.run(host='127.0.0.1', port=5000, debug=True)