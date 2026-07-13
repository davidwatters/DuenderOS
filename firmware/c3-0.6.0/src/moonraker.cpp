#include "moonraker.h"
#include "config.h"
#include "network_manager.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

MoonrakerClient Moonraker;

namespace {
struct NetworkCommand {
  uint8_t type; // 1=gcode, 2=emergency stop
  char payload[384];
};

static uint32_t nextBackoff(uint32_t current) {
  if (current < 1000) return 1000;
  if (current < 2000) return 2000;
  if (current < 5000) return 5000;
  if (current < 10000) return 10000;
  return 30000;
}
}

void MoonrakerClient::begin(PrinterState* state) {
  ps = state;
  stateMutex = xSemaphoreCreateMutex();
  commandQueue = xQueueCreate(8, sizeof(NetworkCommand));
  immediatePoll = true;

  xTaskCreate(
    taskEntry,
    "moonraker",
    8192,
    this,
    1,
    &taskHandle
  );
}

void MoonrakerClient::taskEntry(void* arg) {
  static_cast<MoonrakerClient*>(arg)->taskLoop();
}

void MoonrakerClient::publish(const PrinterState& state) {
  if (!stateMutex) return;
  if (xSemaphoreTake(stateMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
    pendingState = state;
    pendingReady = true;
    xSemaphoreGive(stateMutex);
  }
}

void MoonrakerClient::update() {
  if (!ps || !stateMutex) return;

  if (xSemaphoreTake(stateMutex, 0) == pdTRUE) {
    if (pendingReady) {
      *ps = pendingState;
      pendingReady = false;
    }
    xSemaphoreGive(stateMutex);
  }
}

bool MoonrakerClient::poll() {
  immediatePoll = true;
  if (taskHandle) xTaskNotifyGive(taskHandle);
  return true;
}

bool MoonrakerClient::sendGcode(const String& script) {
  if (!commandQueue || script.length() == 0) return false;

  NetworkCommand cmd{};
  cmd.type = 1;
  script.substring(0, sizeof(cmd.payload) - 1).toCharArray(cmd.payload, sizeof(cmd.payload));
  return xQueueSend(commandQueue, &cmd, 0) == pdTRUE;
}

bool MoonrakerClient::emergencyStop() {
  if (!commandQueue) return false;
  NetworkCommand cmd{};
  cmd.type = 2;
  return xQueueSendToFront(commandQueue, &cmd, 0) == pdTRUE;
}

bool MoonrakerClient::fetchSnapshot(PrinterState& out, const String& host, uint16_t port) {
  out.wifiOk = WiFi.status() == WL_CONNECTED;
  if (!out.wifiOk) {
    out.moonrakerOk = false;
    out.error = "WiFi lost";
    out.state = "offline";
    return false;
  }

  HTTPClient http;
  String url = String("http://") + host + ":" + String(port) +
    "/printer/objects/query?extruder&heater_bed&print_stats&display_status&virtual_sdcard&gcode_move&toolhead&fan&idle_timeout";

  // This runs in a background FreeRTOS task, so a failed connection cannot stall the UI.
  http.setConnectTimeout(MOONRAKER_CONNECT_TIMEOUT_MS);
  http.setTimeout(MOONRAKER_HTTP_TIMEOUT_MS);
  http.begin(url);
  int code = http.GET();

  if (code != 200) {
    out.moonrakerOk = false;
    out.error = "Moonraker " + host + ":" + String(port) + " HTTP " + String(code);
    out.state = "offline";
    http.end();
    return false;
  }

  String payload = http.getString();
  http.end();

  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, payload);
  if (err) {
    out.moonrakerOk = false;
    out.error = String("JSON ") + err.c_str();
    out.state = "error";
    return false;
  }

  JsonObject st = doc["result"]["status"];
  out.hotendTemp = st["extruder"]["temperature"] | 0.0;
  out.hotendTarget = st["extruder"]["target"] | 0.0;
  out.bedTemp = st["heater_bed"]["temperature"] | 0.0;
  out.bedTarget = st["heater_bed"]["target"] | 0.0;
  out.progress = constrain((int)((st["display_status"]["progress"] | 0.0) * 100.0), 0, 100);
  out.message = String((const char*)(st["display_status"]["message"] | ""));
  out.activityState = String((const char*)(st["idle_timeout"]["state"] | "Idle"));
  out.state = String((const char*)(st["print_stats"]["state"] | "unknown"));
  out.file = String((const char*)(st["print_stats"]["filename"] | ""));
  out.x = st["gcode_move"]["gcode_position"][0] | 0.0;
  out.y = st["gcode_move"]["gcode_position"][1] | 0.0;
  out.z = st["gcode_move"]["gcode_position"][2] | 0.0;
  out.speedFactor = (st["gcode_move"]["speed_factor"] | 1.0) * 100.0;
  out.fanPercent = (st["fan"]["speed"] | 0.0) * 100.0;
  out.moonrakerOk = true;
  out.error = "";
  out.lastUpdate = millis();
  return true;
}

bool MoonrakerClient::postGcode(const String& script, const String& host, uint16_t port) {
  if (WiFi.status() != WL_CONNECTED) return false;

  HTTPClient http;
  String url = String("http://") + host + ":" + String(port) + "/printer/gcode/script";
  http.setConnectTimeout(MOONRAKER_CONNECT_TIMEOUT_MS);
  http.setTimeout(MOONRAKER_GCODE_TIMEOUT_MS);
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

bool MoonrakerClient::postEmergencyStop(const String& host, uint16_t port) {
  if (WiFi.status() != WL_CONNECTED) return false;

  HTTPClient http;
  String url = String("http://") + host + ":" + String(port) + "/printer/emergency_stop";
  http.setConnectTimeout(MOONRAKER_CONNECT_TIMEOUT_MS);
  http.setTimeout(MOONRAKER_GCODE_TIMEOUT_MS);
  http.begin(url);
  int code = http.POST("");
  http.end();
  return code >= 200 && code < 300;
}

void MoonrakerClient::taskLoop() {
  uint32_t retryDelayMs = 1000;
  uint32_t nextPollAt = 0;
  String lastHost;
  uint16_t lastPort = 0;

  // Preserve the last valid snapshot through short Moonraker/network hiccups.
  // A macro can temporarily make an HTTP poll slow, but that does not mean
  // Moonraker is actually offline.
  PrinterState lastGoodState;
  bool haveGoodState = false;
  uint8_t consecutiveFailures = 0;
  uint32_t lastGoodAt = 0;

  for (;;) {
    NetworkCommand cmd{};
    while (commandQueue && xQueueReceive(commandQueue, &cmd, 0) == pdTRUE) {
      String host = Network.getMoonrakerHost();
      uint16_t port = Network.getMoonrakerPort();

      if (cmd.type == 1) {
        postGcode(String(cmd.payload), host, port);
      } else if (cmd.type == 2) {
        postEmergencyStop(host, port);
      }

      // Give Klipper/Moonraker a moment to begin processing the command before
      // requesting a new snapshot. This avoids a needless immediate timeout.
      immediatePoll = false;
      nextPollAt = millis() + 300;
    }

    String host = Network.getMoonrakerHost();
    uint16_t port = Network.getMoonrakerPort();

    if (host != lastHost || port != lastPort) {
      lastHost = host;
      lastPort = port;
      retryDelayMs = 1000;
      nextPollAt = 0;
      immediatePoll = true;
      consecutiveFailures = 0;
      haveGoodState = false;
      lastGoodAt = 0;
    }

    uint32_t now = millis();
    if (immediatePoll || (int32_t)(now - nextPollAt) >= 0) {
      immediatePoll = false;

      PrinterState next;
      bool ok = fetchSnapshot(next, host, port);

      if (ok) {
        consecutiveFailures = 0;
        retryDelayMs = 1000;
        lastGoodAt = millis();
        lastGoodState = next;
        haveGoodState = true;
        publish(next);
        nextPollAt = millis() + MOONRAKER_POLL_INTERVAL_MS;
      } else {
        if (consecutiveFailures < 255) consecutiveFailures++;

        const bool graceExpired = !haveGoodState ||
          (millis() - lastGoodAt >= MOONRAKER_STALE_GRACE_MS);
        const bool failureLimitReached =
          consecutiveFailures >= MOONRAKER_OFFLINE_FAILURES;

        if (graceExpired && failureLimitReached) {
          // Only now replace the screen with OFFLINE.
          publish(next);
        } else if (haveGoodState) {
          // Keep displaying the last valid printer/macro state. Update Wi-Fi
          // status, but do not flash OFFLINE because of a single failed poll.
          PrinterState stale = lastGoodState;
          stale.wifiOk = WiFi.status() == WL_CONNECTED;
          stale.moonrakerOk = true;
          stale.error = "Retrying " + host + ":" + String(port);
          publish(stale);
        }

        nextPollAt = millis() + retryDelayMs;
        retryDelayMs = nextBackoff(retryDelayMs);
      }
    }

    ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(10));
  }
}

