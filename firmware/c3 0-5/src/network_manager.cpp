#include "network_manager.h"
#include "config.h"
#include <WiFi.h>
#include <WiFiClient.h>
#include <HTTPClient.h>
#include <Preferences.h>
#include <WiFiManager.h>
#include <ArduinoJson.h>

NetworkManagerDuender Network;
static Preferences prefs;

String NetworkManagerDuender::ipToString(const IPAddress& ip) {
  return String(ip[0]) + "." + String(ip[1]) + "." + String(ip[2]) + "." + String(ip[3]);
}

void NetworkManagerDuender::loadSettings() {
  prefs.begin("duender", true);
  moonHost = prefs.getString("moonHost", DEFAULT_MOONRAKER_HOST);
  moonPort = prefs.getUShort("moonPort", DEFAULT_MOONRAKER_PORT);
  prefs.end();
  if (moonHost.length() < 7) moonHost = DEFAULT_MOONRAKER_HOST;
  if (moonPort == 0) moonPort = DEFAULT_MOONRAKER_PORT;
}

void NetworkManagerDuender::saveSettings() {
  prefs.begin("duender", false);
  prefs.putString("moonHost", moonHost);
  prefs.putUShort("moonPort", moonPort);
  prefs.end();
}

void NetworkManagerDuender::begin() {
  loadSettings();
  WiFi.mode(WIFI_STA);

  WiFiManager wm;
  wm.setConfigPortalTimeout(180);
  wm.setConnectTimeout(15);
  wm.setConnectRetries(2);

  char moonBuf[32];
  moonHost.toCharArray(moonBuf, sizeof(moonBuf));
  WiFiManagerParameter moonParam("moon", "Moonraker IP", moonBuf, 31);
  wm.addParameter(&moonParam);

  String apName = String(SETUP_AP_NAME);
  bool ok = wm.autoConnect(apName.c_str(), SETUP_AP_PASSWORD);

  String entered = String(moonParam.getValue());
  entered.trim();
  if (entered.length() >= 7 && entered != moonHost) {
    moonHost = entered;
    saveSettings();
  }

  if (!ok) {
    WiFi.disconnect(false);
    delay(200);
  }
}

bool NetworkManagerDuender::isWifiConnected() {
  return WiFi.status() == WL_CONNECTED;
}

String NetworkManagerDuender::localIP() {
  if (!isWifiConnected()) return "0.0.0.0";
  return WiFi.localIP().toString();
}

String NetworkManagerDuender::getMoonrakerHost() {
  return moonHost;
}

uint16_t NetworkManagerDuender::getMoonrakerPort() {
  return moonPort;
}

void NetworkManagerDuender::setMoonrakerHost(const String& host) {
  moonHost = host;
  moonHost.trim();
  saveSettings();
}

void NetworkManagerDuender::setMoonrakerPort(uint16_t port) {
  moonPort = port;
  saveSettings();
}

bool NetworkManagerDuender::testMoonraker(const String& host, uint16_t port, uint16_t timeoutMs) {
  if (!isWifiConnected()) return false;

  WiFiClient client;
  client.setTimeout(timeoutMs);
  if (!client.connect(host.c_str(), port, timeoutMs)) {
    client.stop();
    return false;
  }
  client.print(String("GET /printer/info HTTP/1.1\r\nHost: ") + host + "\r\nConnection: close\r\n\r\n");

  unsigned long start = millis();
  String head = "";
  while (millis() - start < timeoutMs) {
    while (client.available()) {
      char c = client.read();
      head += c;
      if (head.indexOf("HTTP/1.1 200") >= 0 || head.indexOf("HTTP/1.0 200") >= 0) {
        client.stop();
        return true;
      }
      if (head.length() > 96) {
        client.stop();
        return false;
      }
    }
    delay(1);
  }
  client.stop();
  return false;
}


bool NetworkManagerDuender::getMoonrakerInfo(const String& host, FoundPrinter& out, uint16_t port, uint16_t timeoutMs) {
  if (!isWifiConnected()) return false;

  HTTPClient http;
  String url = String("http://") + host + ":" + String(port) + "/printer/info";
  http.setTimeout(timeoutMs);
  http.begin(url);
  int code = http.GET();
  if (code != 200) {
    http.end();
    return false;
  }

  String payload = http.getString();
  http.end();

  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, payload);
  if (err) return false;

  // Moonraker /printer/info generally returns result.state, result.state_message, hostname, software_version.
  JsonVariant result = doc["result"];
  if (result.isNull()) return false;

  String name = String((const char*)(result["hostname"] | ""));
  if (name.length() == 0) name = String((const char*)(result["klipper_path"] | ""));
  if (name.length() == 0) name = host;
  name.replace("/home/", "");
  name.replace("/klipper", "");
  if (name.length() > 18) name = name.substring(0, 18);

  out.name = name;
  out.host = host;
  out.port = port;
  return true;
}

int NetworkManagerDuender::scanPrinters(FoundPrinter* out, int maxCount) {
  if (!isWifiConnected() || !out || maxCount <= 0) return 0;

  int found = 0;
  auto tryHost = [&](const String& host, uint16_t timeoutMs) {
    if (found >= maxCount) return;
    for (int i = 0; i < found; i++) {
      if (out[i].host == host) return;
    }
    FoundPrinter fp;
    if (getMoonrakerInfo(host, fp, DEFAULT_MOONRAKER_PORT, timeoutMs)) {
      out[found++] = fp;
    }
  };

  // Saved/default first so known printers appear at the top.
  tryHost(moonHost, 650);
  if (String(DEFAULT_MOONRAKER_HOST) != moonHost) tryHost(DEFAULT_MOONRAKER_HOST, 450);

  IPAddress ip = WiFi.localIP();
  uint8_t a = ip[0], b = ip[1], c = ip[2];

  const uint8_t quick[] = {2, 3, 4, 5, 10, 20, 50, 80, 81, 82, 83, 90, 91, 92, 100, 101, 150, 154, 200, 201, 220, 250};
  for (uint8_t i = 0; i < sizeof(quick) && found < maxCount; i++) {
    String host = String(a) + "." + String(b) + "." + String(c) + "." + String(quick[i]);
    tryHost(host, 190);
  }

  // Full /24 scan. Keep each timeout low so the UI doesn't hang forever.
  for (int last = 1; last <= 254 && found < maxCount; last++) {
    if (last == ip[3]) continue;
    String host = String(a) + "." + String(b) + "." + String(c) + "." + String(last);
    tryHost(host, 80);
  }

  return found;
}

bool NetworkManagerDuender::autoScanMoonraker() {
  if (!isWifiConnected()) return false;

  IPAddress ip = WiFi.localIP();
  uint8_t a = ip[0], b = ip[1], c = ip[2];

  // Try the saved address first.
  if (testMoonraker(moonHost, moonPort, 500)) return true;

  // Common Klipper host addresses first.
  const uint8_t quick[] = {2, 3, 4, 5, 10, 20, 50, 100, 101, 150, 154, 200, 201, 220, 250};
  for (uint8_t i = 0; i < sizeof(quick); i++) {
    String host = String(a) + "." + String(b) + "." + String(c) + "." + String(quick[i]);
    if (host == moonHost) continue;
    FoundPrinter fp;
    if (getMoonrakerInfo(host, fp, DEFAULT_MOONRAKER_PORT, 220)) {
      moonHost = host;
      moonPort = DEFAULT_MOONRAKER_PORT;
      saveSettings();
      return true;
    }
  }

  // Full /24 scan. This is blocking but simple and reliable.
  for (int last = 1; last <= 254; last++) {
    if (last == ip[3]) continue;
    String host = String(a) + "." + String(b) + "." + String(c) + "." + String(last);
    FoundPrinter fp;
    if (getMoonrakerInfo(host, fp, DEFAULT_MOONRAKER_PORT, 110)) {
      moonHost = host;
      moonPort = DEFAULT_MOONRAKER_PORT;
      saveSettings();
      return true;
    }
  }
  return false;
}

void NetworkManagerDuender::startConfigPortal() {
  WiFiManager wm;
  wm.setConfigPortalTimeout(240);

  char moonBuf[32];
  moonHost.toCharArray(moonBuf, sizeof(moonBuf));
  WiFiManagerParameter moonParam("moon", "Moonraker IP", moonBuf, 31);
  wm.addParameter(&moonParam);

  wm.startConfigPortal(SETUP_AP_NAME, SETUP_AP_PASSWORD);

  String entered = String(moonParam.getValue());
  entered.trim();
  if (entered.length() >= 7) {
    moonHost = entered;
    saveSettings();
  }
}

void NetworkManagerDuender::resetWiFiAndSettings() {
  WiFiManager wm;
  wm.resetSettings();
  prefs.begin("duender", false);
  prefs.clear();
  prefs.end();
  delay(300);
  ESP.restart();
}
