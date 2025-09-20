// Satellite.ino
// ESP32 + HC-12: executes commands, reads sensors, replies with unified JSON
// Sensors: TMP102 (I2C 0x48), INA219 (Adafruit), (GPS optional placeholder)

#include <Arduino.h>
#include <Wire.h>
#include <ArduinoJson.h>
#include <Adafruit_INA219.h>

// ---------- HC-12 serial ----------
HardwareSerial& HC12 = Serial2; // RX/TX on pins below
const int HC12_RX = 16; // ESP32 RX <- HC-12 TX
const int HC12_TX = 17; // ESP32 TX -> HC-12 RX

// ---------- I2C ----------
const int I2C_SDA = 21;
const int I2C_SCL = 22;

// ---------- TMP102 ----------
static const uint8_t TMP102_ADDR = 0x48;
static const uint8_t TMP102_REG_TEMP = 0x00;

// ---------- INA219 ----------
Adafruit_INA219 ina219;

// ---------- JSON buffer ----------
StaticJsonDocument<512> resp;

// ---------- Helpers ----------
bool readTMP102C(float& tC) {
  Wire.beginTransmission(TMP102_ADDR);
  Wire.write(TMP102_REG_TEMP);
  if (Wire.endTransmission(false) != 0) return false;

  if (Wire.requestFrom((int)TMP102_ADDR, 2) != 2) return false;
  uint8_t msb = Wire.read();
  uint8_t lsb = Wire.read();
  int16_t raw = ((msb << 8) | lsb) >> 4;
  if (raw & 0x800) raw -= 4096;
  tC = raw * 0.0625f;
  return true;
}

bool readPowerPack(float& v, float& i_A, float& p_W) {
  v   = ina219.getBusVoltage_V();
  float i_mA = ina219.getCurrent_mA();
  i_A = i_mA / 1000.0f;
  p_W = v * i_A;
  return true;
}

// ---------- Example: GPS placeholder ----------
bool readGPS(float& lat, float& lon) {
  // TODO: replace with TinyGPS++ reading if GPS is connected
  lat = 31.950000f; // demo
  lon = 35.910000f; // demo
  return true;
}

// ---------- Fields Fillers (Handlers) ----------
bool hTEMP(JsonObject f) {
  float t;
  if (!readTMP102C(t)) return false;
  f["temp"] = t;
  return true;
}

bool hPOWER(JsonObject f) {
  float v,i,p;
  if (!readPowerPack(v,i,p)) return false;
  f["busV"]   = v;
  f["current"]= i;
  f["power"]  = p;
  return true;
}

bool hGPS(JsonObject f) {
  float lat, lon;
  if (!readGPS(lat, lon)) return false;
  f["lat"] = lat;
  f["lon"] = lon;
  return true;
}

// ---------- Command table ----------
typedef bool (*HandlerFn)(JsonObject);
struct Cmd { const char* name; HandlerFn fn; };

Cmd CMDS[] = {
  {"TEMP",  hTEMP},
  {"POWER", hPOWER},
  {"GPS",   hGPS},
  {"ALL",   nullptr}, // special
};

void sendResponse(const char* device, std::function<void(JsonObject)> fill) {
  resp.clear();
  resp["device"] = device;
  resp["ts"]     = (uint32_t)(millis()/1000); // bridge will fix if too small
  JsonObject f   = resp.createNestedObject("fields");
  fill(f);

  String out; 
  serializeJson(resp, out);
  HC12.println(out);
  Serial.print("TX: "); Serial.println(out);
}

void handleCommand(const String& cmdRaw) {
  String cmd = cmdRaw; 
  cmd.trim(); cmd.toUpperCase();

  if (cmd == "ALL") {
    sendResponse("sat1", [&](JsonObject f){
      hTEMP(f); hPOWER(f); /* hGPS(f); */ // enable if GPS available
    });
    return;
  }

  for (auto &c : CMDS) {
    if (c.fn && cmd == c.name) {
      sendResponse("sat1", [&](JsonObject f){ c.fn(f); });
      return;
    }
  }

  // JSON command form: {"cmd":"READ","fields":["TEMP","POWER","GPS"]}
  StaticJsonDocument<256> q;
  DeserializationError e = deserializeJson(q, cmdRaw);
  if (!e) {
    const char* ctype = q["cmd"] | "";
    if (String(ctype).equalsIgnoreCase("READ") && q["fields"].is<JsonArray>()) {
      sendResponse("sat1", [&](JsonObject f){
        for (JsonVariant v : q["fields"].as<JsonArray>()) {
          String key = v.as<const char*>();
          key.trim(); key.toUpperCase();
          if (key == "TEMP")  hTEMP(f);
          else if (key == "POWER") hPOWER(f);
          else if (key == "GPS")   hGPS(f);
        }
      });
      return;
    }
  }

  sendResponse("sat1", [&](JsonObject f){ f["err"] = "unknown_cmd"; });
}

void setup() {
  Serial.begin(115200);
  Wire.begin(I2C_SDA, I2C_SCL);
  HC12.begin(9600, SERIAL_8N1, HC12_RX, HC12_TX);

  if (!ina219.begin()) {
    Serial.println("INA219 not found!");
  } else {
    ina219.setCalibration_32V_2A();
  }
  Serial.println("Satellite ready.");
}

void loop() {
  static String buf;
  while (HC12.available()) {
    char c = HC12.read();
    if (c == '\n') {
      if (buf.length()) {
        Serial.print("RX CMD: "); Serial.println(buf);
        handleCommand(buf);
        buf = "";
      }
    } else {
      buf += c;
    }
  }
}
