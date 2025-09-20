# StudentSat-Telemetry

# StudentSat-Telemetry 

An educational satellite-ground station telemetry project using **ESP32**, **HC-12 wireless modules**, **sensors**, **MQTT**, **InfluxDB**, and **Grafana**.

##Overview
- **Satellite ESP32** collects data from sensors (e.g., TMP102 temperature, INA219 power, FRAM, etc.).
- Sends telemetry to **Ground Station ESP32** via **HC-12**.
- **Ground Station ESP32** forwards telemetry to an **MQTT broker**.
- A **Python bridge script** subscribes to MQTT and writes data to **InfluxDB**.
- **Grafana** displays real-time dashboards for students.
- Students interact by sending commands (`TEMP`, `POWER`, `ALL`, etc.) via MQTT or a simple web interface.

---

##Hardware Used
- ESP32 (x2)
- HC-12 (x2)
- TMP102 (Temperature sensor)
- INA219 (Current & Voltage sensor)
- FRAM I2C (Data logging)
- (Optional) SD Card
- (Optional) Any other sensor (e.g., GPS, Camera)

---

## 🗂 Project Structure
StudentSat-Telemetry/
│
├── satellite/
│ └── satellite.ino # ESP32 code for satellite
│
├── ground_station/
│ └── ground_station.ino # ESP32 code for ground station
│
├── bridge/
│ └── mqtt_to_influx.py # Python script to forward MQTT → InfluxDB
│
├── dashboard/
│ └── grafana_setup.json # Example Grafana dashboard export
│
└── README.md # Documentation (this file)


---

## 🛠 Setup Instructions

### 1. Flash ESP32 Codes
- Open `satellite/satellite.ino` → Upload to **satellite ESP32**.
- Open `ground_station/ground_station.ino` → Upload to **ground station ESP32**.

### 2. Install MQTT Broker (Mosquitto)
```bash
# Windows (Run as Admin in CMD/PowerShell)
net start mosquitto
