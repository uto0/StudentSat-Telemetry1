@echo off
set "INFLUX_EXE=C:\Users\abood\OneDrive\Desktop\influxdb2-2.7.12-windows\influxd.exe"
set "GRAFANA_HOME=C:\Users\abood\OneDrive\Desktop\grafana-enterprise-12.1.0.windows-amd64\grafana-v12.1.0"
set "MOSQ_EXE=C:\Program Files\mosquitto\mosquitto.exe"
set "MOSQ_CONF=C:\Program Files\mosquitto\mosquitto.conf"
set "PY_DIR=C:\Users\abood\OneDrive\Desktop"
set "PY_SCRIPT=%PY_DIR%\mqtt_to_influx_generic.py"
set "UI_FILE=%PY_DIR%\telemetry_ui.html"

start "InfluxDB" cmd /k "%INFLUX_EXE%"
start "Grafana"  cmd /k "cd /d "%GRAFANA_HOME%" && bin\grafana-server.exe --homepath "%GRAFANA_HOME%""
start "Mosquitto" cmd /k ""%MOSQ_EXE%" -v -c "%MOSQ_CONF%""
start "MQTT->Influx" cmd /k "cd /d "%PY_DIR%" && python "%PY_SCRIPT%""
start "" "%UI_FILE%"
