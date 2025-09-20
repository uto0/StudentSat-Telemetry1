# mqtt_to_influx_generic.py
# Generic bridge: subscribes to MQTT "sat/telemetry", writes ALL numeric fields into InfluxDB.

import json, time, math, re, signal, sys
from typing import Any, Dict
import paho.mqtt.client as mqtt
from influxdb_client import InfluxDBClient, Point, WritePrecision

# ---------- MQTT ----------
MQTT_HOST       = "192.168.0.102"
MQTT_PORT       = 1883
MQTT_TOPIC_IN   = "sat/telemetry"
MQTT_CLIENT_ID  = "bridge-generic"

# ---------- InfluxDB v2 ----------
INFLUXDB_URL    = "http://localhost:8086"
INFLUXDB_TOKEN  = "CF9lJknEu-htUQXguG4zQWE8Zort9rRWkrEuInGrWLkbut_kmWgBNQlyGjNMG3nhe9sl67pQLX60dHNpBYEcbQ=="
INFLUXDB_ORG    = "Space Org"
INFLUXDB_BUCKET = "Space Bucket"
MEASUREMENT     = "telemetry"
DEFAULT_DEVICE  = "sat1"

RESERVED_KEYS   = {"ts","time","ok","err","device","fields","tags"}

iclient   = InfluxDBClient(url=INFLUXDB_URL, token=INFLUXDB_TOKEN, org=INFLUXDB_ORG)
write_api = iclient.write_api()

def to_unix_seconds(v: Any) -> int:
    try:
        ts = int(float(v))
    except Exception:
        return int(time.time())
    if ts < 1_000_000_000:
        return int(time.time())
    while ts > 10_000_000_000:
        ts //= 10
    return ts

def is_number(x: Any) -> bool:
    try:
        f = float(x)
        return math.isfinite(f)
    except Exception:
        return False

def flatten_fields(d: Dict[str, Any]) -> Dict[str, float]:
    out: Dict[str, float] = {}
    if isinstance(d.get("fields"), dict):
        for k, v in d["fields"].items():
            if is_number(v):
                out[str(k)] = float(v)
    for k, v in d.items():
        if k in RESERVED_KEYS:
            continue
        if is_number(v):
            out[str(k)] = float(v)
    return out

def on_connect(client, userdata, flags, rc, properties=None):
    print("MQTT connect rc =", rc)
    if rc == 0:
        client.subscribe(MQTT_TOPIC_IN)
        print("Subscribed ->", MQTT_TOPIC_IN)

def on_message(client, userdata, msg):
    raw = msg.payload.decode("utf-8", errors="ignore").strip()
    if not raw:
        return
    clean = re.sub(r'(?<=[:\s])nan(?=[,\}\s])', 'null', raw, flags=re.IGNORECASE)
    try:
        data = json.loads(clean)
    except json.JSONDecodeError as e:
        print("JSON ERR:", e, "| raw:", raw)
        return

    device = str(data.get("device", DEFAULT_DEVICE))
    ts     = to_unix_seconds(data.get("ts", int(time.time())))
    fields = flatten_fields(data)

    if not fields:
        print("Skip (no numeric fields):", clean)
        return

    pt = Point(MEASUREMENT).tag("device", device).time(ts, WritePrecision.S)
    for k, v in fields.items():
        pt = pt.field(k, v)

    try:
        write_api.write(bucket=INFLUXDB_BUCKET, org=INFLUXDB_ORG, record=pt)
        print(f"Influx OK @{ts} ->", fields)
    except Exception as e:
        print("Influx ERR:", e)

def main():
    print("Generic bridge ready. Topic:", MQTT_TOPIC_IN)
    client = mqtt.Client(client_id=MQTT_CLIENT_ID, clean_session=True)
    client.on_connect = on_connect
    client.on_message = on_message
    client.connect_async(MQTT_HOST, MQTT_PORT, keepalive=30)
    client.loop_start()

    def stop(sig, frame):
        print("Stopping...")
        client.loop_stop(); client.disconnect(); sys.exit(0)
    signal.signal(signal.SIGINT, stop)
    signal.signal(signal.SIGTERM, stop)

    while True:
        time.sleep(1)

if __name__ == "__main__":
    main()
