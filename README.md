# Smart Waste Bin Management

![Smart Waste Bin Management System](https://github.com/user-attachments/assets/e73b523b-c91c-491e-9ac6-e1af3a18a490)

## 📋 Overview

The Smart Waste Bin Management system is an IoT-based solution for optimizing waste collection and management. This project combines hardware sensors with cloud connectivity to monitor bin status in real-time, enabling data-driven waste management decisions.

## ✨ Features

- **Real-time Fill Level Monitoring**: Ultrasonic sensors detect bin fill levels
- **Environmental Monitoring**: Temperature and humidity tracking with DHT22 sensors
- **Smoke Detection**: MQ2 sensors for fire hazard prevention
- **LoRaWAN Connectivity**: Long-range, low-power wireless communication
- **Cloud Dashboard**: Web interface for monitoring all bins in the network
- **Alert System**: Notifications for bins requiring immediate attention

## 🛠️ Technologies

- **Hardware**:
  - Arduino Uno microcontrollers
  - HC-SR04 ultrasonic sensors
  - DHT22 temperature & humidity sensors
  - MQ2 smoke sensors
  - LoRaWAN communication modules

- **Software & Services**:
  - C++ (Arduino firmware)
  - Python (Data processing & backend)
  - HTML/CSS/JavaScript (Web dashboard)
  - The Things Network (LoRaWAN infrastructure)

## 📊 System Architecture

```
┌─────────────┐     ┌──────────────┐     ┌────────────────┐
│  IoT Nodes  │     │              │     │                │
│ (Smart Bins)│────▶│ LoRaWAN      │────▶│ Web Dashboard  |
│             │     │ Gateway      │     │                │     
└─────────────┘     └──────────────┘     └────────────────┘     
```

## 🚀 Getting Started

### Hardware Requirements

- Arduino Uno
- LoRaWAN shield (RFM95)
- DHT22 temperature & humidity sensor
- HC-SR04 ultrasonic sensor
- MQ2 smoke sensor
- Jumper wires
- Power supply (5V power bank for deployment)

### Software Setup

1. **Arduino Configuration**
   ```bash
   # Clone this repository
   git clone https://github.com/matthiaslim/csc2106_g24.git
   cd csc2106_g24/LoRa/sensors
   
   # Open the Arduino sketch in Arduino IDE
   # Install required libraries:
   # - LMIC
   # - Adafruit DHT
   # - SPI
   ```

2. **Dashboard Setup**
   ```bash
   cd dashboard
   pip install -r requirements.txt
   python server.py
   ```

## 📁 Project Structure

```
csc2106_g24/
├── LoRa/               # Arduino code for waste bin nodes
├── dashboard/          # Web dashboard UI
└── README.md           # Project information
```

## 📝 Usage

1. **Device Setup**
   - Assemble hardware components according to schematics
   - Upload firmware using Arduino IDE
   - Register devices on The Things Network

2. **Monitoring**
   - Access the web dashboard at [project-url]
   - View individual bin status, fill levels, and environmental data
   - Configure alerts based on custom thresholds

## 🙏 Acknowledgments

- Faculty advisors at Singapore Institute of Technology
- [The Things Network](https://www.thethingsnetwork.org/) for LoRaWAN infrastructure
- Code contributions and inspiration:
  - [DHT22 Library](https://github.com/adafruit/DHT-sensor-library) by Adafruit - Used for temperature/humidity sensing
  - [Ultrasonic Sensor Code](https://github.com/gamegine/HCSR04-ultrasonic-sensor-lib) - Adapted for our fill level detection
  - [MQ2 Calibration Code](https://sandboxelectronics.com/?p=165) - Modified for smoke detection

---

*Last Updated: April 2025*
