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
  beeperEnabled = prefs.getBool("beeper", true);
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

void NetworkManagerDuender::saveBeeper() {
  prefs.begin("duender", false);
  prefs.putBool("beeper", beeperEnabled);
  prefs.end();
}

void NetworkManagerDuender::begin() {
  loadSettings();

  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);

  WiFiManager wm;
  wm.setConfigPortalTimeout(180);
  wm.setConnectTimeout(10);
  wm.setConnectRetries(1);

  char moonBuf[32];
  moonHost.toCharArray(moonBuf, sizeof(moonBuf));
  WiFiManagerParameter moonParam("moon", "Moonraker IP", moonBuf, 31);
  wm.addParameter(&moonParam);

  bool ok = wm.autoConnect(SETUP_AP_NAME, SETUP_AP_PASSWORD);

  String entered = String(moonParam.getValue());
  entered.trim();

  if (entered.length() >= 7 && entered != moonHost) {
    moonHost = entered;
    saveSettings();
  }

  if (!ok) {
    WiFi.disconnect(false);
    delay(100);
  }
}

bool NetworkManagerDuender::isWifiConnected() {
  return WiFi.status() == WL_CONNECTED;
}

String NetworkManagerDuender::localIP() {
  return isWifiConnected() ? WiFi.localIP().toString() : "0.0.0.0";
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

bool NetworkManagerDuender::isBeeperEnabled() {
  return beeperEnabled;
}

void NetworkManagerDuender::setBeeperEnabled(bool enabled) {
  beeperEnabled = enabled;
  saveBeeper();
}

bool NetworkManagerDuender::toggleBeeper() {
  beeperEnabled = !beeperEnabled;
  saveBeeper();
  return beeperEnabled;
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
  String head;

  while (millis() - start < timeoutMs) {
    while (client.available()) {
      char c = client.read();
      head += c;

      if (head.indexOf("HTTP/1.1 200") >= 0 || head.indexOf("HTTP/1.0 200") >= 0) {
        client.stop();
        return true;
      }

      if (head.length() > 128) {
        client.stop();
        return false;
      }
    }

    delay(0);
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

  if (deserializeJson(doc, payload)) return false;

  JsonVariant result = doc["result"];
  if (result.isNull()) return false;

  String name = String((const char*)(result["hostname"] | ""));

  if (name.length() == 0) name = String((const char*)(result["state_message"] | ""));
  if (name.length() == 0) name = host;

  name.replace("Klipper", "");
  name.trim();

  if (name.length() == 0) name = host;
  if (name.length() > 18) name = name.substring(0, 18);

  out.name = name;
  out.host = host;
  out.port = port;

  return true;
}

bool NetworkManagerDuender::alreadyFound(const String& host) {
  for (int i = 0; i < foundCount; i++) {
    if (found[i].host == host) return true;
  }

  return false;
}

void NetworkManagerDuender::addPrinterIfValid(const String& host, uint16_t timeoutMs) {
  if (foundCount >= DUENDER_MAX_PRINTERS) return;
  if (alreadyFound(host)) return;

  FoundPrinter fp;

  if (getMoonrakerInfo(host, fp, DEFAULT_MOONRAKER_PORT, timeoutMs)) {
    found[foundCount++] = fp;
  }
}

void NetworkManagerDuender::startPrinterScan() {
  startFullScan();
}

void NetworkManagerDuender::startQuickScan() {
  foundCount = 0;
  scanActive = isWifiConnected();
  scanQuickOnly = true;
  scanStep = 0;
  scanLastOctet = 1;
  lastScanTick = 0;
}

void NetworkManagerDuender::startFullScan() {
  foundCount = 0;
  scanActive = isWifiConnected();
  scanQuickOnly = false;
  scanStep = 0;
  scanLastOctet = 1;
  lastScanTick = 0;
}

bool NetworkManagerDuender::updatePrinterScan() {
  if (!scanActive) return false;

  if (!isWifiConnected()) {
    scanActive = false;
    return false;
  }

  if (millis() - lastScanTick < 4) return true;
  lastScanTick = millis();

  IPAddress ip = WiFi.localIP();
  IPAddress gw = WiFi.gatewayIP();
  String prefix = String(ip[0]) + "." + String(ip[1]) + "." + String(ip[2]) + ".";

  static const uint8_t common[] = {
    2, 3, 4, 5, 10, 20, 30, 40, 50, 60, 70,
    80, 81, 82, 83, 84, 85, 90, 91, 92,
    100, 101, 102, 110, 120, 130, 140,
    150, 151, 152, 153, 154, 155, 156,
    160, 170, 180, 190, 200, 201, 202,
    210, 220, 230, 240, 250
  };

  const int commonCount = sizeof(common) / sizeof(common[0]);

  if (scanStep == 0) {
    addPrinterIfValid(moonHost, QUICK_SCAN_HTTP_TIMEOUT_MS);
    scanStep++;
    return true;
  }

  if (scanStep == 1) {
    addPrinterIfValid(DEFAULT_MOONRAKER_HOST, QUICK_SCAN_HTTP_TIMEOUT_MS);
    scanStep++;
    return true;
  }

  if (scanStep == 2) {
    if (gw[3] > 0 && gw[3] < 255) {
      addPrinterIfValid(prefix + String(gw[3]), QUICK_SCAN_HTTP_TIMEOUT_MS);
    }

    scanStep++;
    return true;
  }

  const int nearRadius = 12;

  if (scanStep >= 3 && scanStep < 3 + (nearRadius * 2 + 1)) {
    int n = scanStep - 3;
    int offset = (n == 0) ? 0 : ((n + 1) / 2) * ((n % 2) ? 1 : -1);
    int last = (int)ip[3] + offset;

    if (last > 0 && last < 255) {
      addPrinterIfValid(prefix + String(last), QUICK_SCAN_HTTP_TIMEOUT_MS);
    }

    scanStep++;
    return true;
  }

  int commonBase = 3 + (nearRadius * 2 + 1);

  if (scanStep >= commonBase && scanStep < commonBase + commonCount) {
    uint8_t last = common[scanStep - commonBase];

    if (last != ip[3]) {
      addPrinterIfValid(prefix + String(last), QUICK_SCAN_HTTP_TIMEOUT_MS);
    }

    scanStep++;
    return true;
  }

  if (scanQuickOnly) {
    scanActive = false;
    return false;
  }

  while (scanLastOctet <= 254) {
    int last = scanLastOctet++;

    if (last == ip[3]) continue;

    addPrinterIfValid(prefix + String(last), FULL_SCAN_HTTP_TIMEOUT_MS);
    return true;
  }

  scanActive = false;
  return false;
}

bool NetworkManagerDuender::isPrinterScanActive() {
  return scanActive;
}

void NetworkManagerDuender::cancelPrinterScan() {
  scanActive = false;
}

int NetworkManagerDuender::getScanProgress() {
  if (!scanActive) return 100;

  const int nearRadius = 12;
  const int quickTotal = 3 + (nearRadius * 2 + 1) + 47;

  if (scanQuickOnly) {
    return constrain(map(scanStep, 0, quickTotal, 0, 100), 0, 100);
  }

  int done = scanStep < quickTotal ? scanStep : quickTotal + scanLastOctet;
  int total = quickTotal + 254;

  return constrain(map(done, 0, total, 0, 100), 0, 100);
}

int NetworkManagerDuender::getScanCurrentHost() {
  return scanLastOctet;
}

int NetworkManagerDuender::getFoundPrinterCount() {
  return foundCount;
}

FoundPrinter NetworkManagerDuender::getFoundPrinter(int index) {
  if (index < 0 || index >= foundCount) {
    FoundPrinter blank;
    blank.name = "";
    blank.host = "";
    blank.port = DEFAULT_MOONRAKER_PORT;
    return blank;
  }

  return found[index];
}

int NetworkManagerDuender::scanPrinters(FoundPrinter* out, int maxCount) {
  startQuickScan();

  unsigned long start = millis();

  while (isPrinterScanActive() && millis() - start < 8000) {
    updatePrinterScan();
    delay(0);
  }

  int n = min(foundCount, maxCount);

  for (int i = 0; i < n; i++) {
    out[i] = found[i];
  }

  return n;
}

bool NetworkManagerDuender::autoScanMoonraker() {
  if (!isWifiConnected()) return false;

  if (testMoonraker(moonHost, moonPort, 180)) return true;

  startQuickScan();

  unsigned long start = millis();

  while (isPrinterScanActive() && foundCount == 0 && millis() - start < 5000) {
    updatePrinterScan();
    delay(0);
  }

  if (foundCount > 0) {
    moonHost = found[0].host;
    moonPort = found[0].port;
    saveSettings();
    scanActive = false;
    return true;
  }

  scanActive = false;
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

  delay(500);
  ESP.restart();
}