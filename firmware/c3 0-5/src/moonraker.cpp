#include "moonraker.h"
#include "config.h"
#include "network_manager.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

MoonrakerClient Moonraker;

String MoonrakerClient::baseUrl() {
  return String("http://") + Network.getMoonrakerHost() + ":" + String(Network.getMoonrakerPort());
}

void MoonrakerClient::begin(PrinterState* state) {
  ps = state;
}

bool MoonrakerClient::poll() {
  if (!ps) return false;
  ps->wifiOk = WiFi.status() == WL_CONNECTED;
  if (!ps->wifiOk) {
    ps->moonrakerOk = false;
    ps->error = "WiFi lost";
    return false;
  }

  HTTPClient http;
  String url = baseUrl() + "/printer/objects/query?extruder&heater_bed&print_stats&display_status&virtual_sdcard&gcode_move&toolhead&fan";
  http.setTimeout(1400);
  http.begin(url);
  int code = http.GET();

  if (code != 200) {
    ps->moonrakerOk = false;
    ps->error = "Moonraker HTTP " + String(code);
    http.end();
    return false;
  }

  String payload = http.getString();
  http.end();

  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, payload);
  if (err) {
    ps->moonrakerOk = false;
    ps->error = String("JSON ") + err.c_str();
    return false;
  }

  JsonObject st = doc["result"]["status"];
  ps->hotendTemp = st["extruder"]["temperature"] | 0.0;
  ps->hotendTarget = st["extruder"]["target"] | 0.0;
  ps->bedTemp = st["heater_bed"]["temperature"] | 0.0;
  ps->bedTarget = st["heater_bed"]["target"] | 0.0;
  ps->progress = constrain((int)((st["display_status"]["progress"] | 0.0) * 100.0), 0, 100);
  ps->message = String((const char*)(st["display_status"]["message"] | ""));
  ps->state = String((const char*)(st["print_stats"]["state"] | "unknown"));
  ps->file = String((const char*)(st["print_stats"]["filename"] | ""));
  ps->x = st["gcode_move"]["gcode_position"][0] | 0.0;
  ps->y = st["gcode_move"]["gcode_position"][1] | 0.0;
  ps->z = st["gcode_move"]["gcode_position"][2] | 0.0;
  ps->speedFactor = (st["gcode_move"]["speed_factor"] | 1.0) * 100.0;
  ps->fanPercent = (st["fan"]["speed"] | 0.0) * 100.0;
  ps->moonrakerOk = true;
  ps->error = "";
  ps->lastUpdate = millis();
  return true;
}

bool MoonrakerClient::sendGcode(const String& script) {
  if (WiFi.status() != WL_CONNECTED) return false;

  HTTPClient http;
  String url = baseUrl() + "/printer/gcode/script";
  http.begin(url);
  http.addHeader("Content-Type", "application/json");

  JsonDocument doc;
  doc["script"] = script;
  String body;
  serializeJson(doc, body);
  int code = http.POST(body);
  http.end();
  return code >= 200 && code < 300;
}

bool MoonrakerClient::emergencyStop() {
  if (WiFi.status() != WL_CONNECTED) return false;
  HTTPClient http;
  http.begin(baseUrl() + "/printer/emergency_stop");
  int code = http.POST("");
  http.end();
  return code >= 200 && code < 300;
}
