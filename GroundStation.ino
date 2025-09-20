// GroundStation.ino
// ESP32 + WiFi + MQTT + HC-12: relays commands and publishes telemetry

#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>

// ---------- WiFi ----------
const char* WIFI_SSID = "SpacePoint";
const char* WIFI_PASS = "Space2025";

// ---------- MQTT ----------
const char* MQTT_HOST = "192.168.0.102";
const uint16_t MQTT_PORT = 1883;
const char* TOPIC_CMD = "gs/cmd";           // subscribe
const char* TOPIC_TLM = "sat/telemetry";    // publish

WiFiClient espClient;
PubSubClient mqtt(espClient);

// ---------- HC-12 ----------
HardwareSerial& HC12 = Serial2;
const int HC12_RX = 16; // ESP32 RX <- HC-12 TX
const int HC12_TX = 17; // ESP32 TX -> HC-12 RX

void connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("WiFi...");
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  Serial.print(" OK IP="); Serial.println(WiFi.localIP());
}

void mqttReconnect() {
  while (!mqtt.connected()) {
    Serial.print("MQTT...");
    if (mqtt.connect("esp32-ground")) {
      Serial.println(" OK");
      mqtt.subscribe(TOPIC_CMD);
    } else {
      Serial.print(" rc="); Serial.println(mqtt.state());
      delay(1000);
    }
  }
}

void onMqttMsg(char* topic, byte* payload, unsigned int length) {
  String cmd; cmd.reserve(length+1);
  for (unsigned int i=0; i<length; ++i) cmd += (char)payload[i];
  cmd.trim();

  Serial.print("MQTT cmd ["); Serial.print(topic); Serial.print("]: ");
  Serial.println(cmd);

  // Forward to satellite
  HC12.print(cmd); HC12.print("\n");
}

void setup() {
  Serial.begin(115200);
  HC12.begin(9600, SERIAL_8N1, HC12_RX, HC12_TX);

  connectWiFi();
  mqtt.setServer(MQTT_HOST, MQTT_PORT);
  mqtt.setCallback(onMqttMsg);
}

void loop() {
  if (!mqtt.connected()) mqttReconnect();
  mqtt.loop();

  // Read JSON from satellite and publish
  static String line;
  while (HC12.available()) {
    char c = HC12.read();
    if (c == '\n') {
      line.trim();
      if (line.length()) {
        Serial.print("RX "); Serial.println(line);
        mqtt.publish(TOPIC_TLM, line.c_str());
      }
      line = "";
    } else {
      line += c;
    }
  }
}
