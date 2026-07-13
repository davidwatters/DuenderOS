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

void keepLastGoodOrFail(PrinterState* ps, const String& err) {
  ps->error = err;

  if (ps->lastUpdate > 0 && millis() - ps->lastUpdate < 30000) {
    ps->moonrakerOk = true;
  } else {
    ps->moonrakerOk = false;
  }
}

bool MoonrakerClient::poll() {
  if (!ps) return false;

  ps->wifiOk = WiFi.status() == WL_CONNECTED;

  if (!ps->wifiOk) {
    keepLastGoodOrFail(ps, "WiFi lost");
    return false;
  }

  HTTPClient http;
  String url = baseUrl() + "/printer/objects/query?extruder&heater_bed&print_stats&display_status&virtual_sdcard&gcode_move&toolhead&fan";

  http.setTimeout(MOONRAKER_HTTP_TIMEOUT_MS);
  http.begin(url);
  http.setReuse(false);

  int code = http.GET();

  if (code != 200) {
    keepLastGoodOrFail(ps, "Poll HTTP " + String(code));
    http.end();
    return false;
  }

  String payload = http.getString();
  http.end();

  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, payload);

  if (err) {
    keepLastGoodOrFail(ps, String("JSON ") + err.c_str());
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

  Serial.printf(
    "Temps H %.1f/%.1f B %.1f/%.1f State=%s Progress=%d%%\n",
    ps->hotendTemp,
    ps->hotendTarget,
    ps->bedTemp,
    ps->bedTarget,
    ps->state.c_str(),
    ps->progress
  );

  return true;
}

bool MoonrakerClient::sendGcode(const String& script) {
  if (WiFi.status() != WL_CONNECTED) return false;

  HTTPClient http;
  String url = baseUrl() + "/printer/gcode/script";

  http.setTimeout(MOONRAKER_GCODE_TIMEOUT_MS);
  http.begin(url);
  http.setReuse(false);
  http.addHeader("Content-Type", "application/json");

  JsonDocument doc;
  doc["script"] = script;

  String body;
  serializeJson(doc, body);

  int code = http.POST(body);
  http.end();

  Serial.printf("GCODE send HTTP %d: %s\n", code, script.c_str());

  return code >= 200 && code < 300;
}

bool MoonrakerClient::emergencyStop() {
  if (WiFi.status() != WL_CONNECTED) return false;

  HTTPClient http;
  http.setTimeout(MOONRAKER_GCODE_TIMEOUT_MS);
  http.begin(baseUrl() + "/printer/emergency_stop");
  http.setReuse(false);

  int code = http.POST("");
  http.end();

  return code >= 200 && code < 300;
}