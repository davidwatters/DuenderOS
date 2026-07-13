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

namespace {
static const uint16_t SCAN_PORTS[] = {7125, 7126, 7127, 7128, 7129, 7130};
static const int SCAN_PORT_COUNT = sizeof(SCAN_PORTS) / sizeof(SCAN_PORTS[0]);
}

String NetworkManagerDuender::ipToString(const IPAddress& ip) {
  return String(ip[0]) + "." + String(ip[1]) + "." + String(ip[2]) + "." + String(ip[3]);
}

void NetworkManagerDuender::loadSettings() {
  prefs.begin("duender", true);
  moonHost = prefs.getString("moonHost", DEFAULT_MOONRAKER_HOST);
  moonPort = prefs.getUShort("moonPort", DEFAULT_MOONRAKER_PORT);
  beeperEnabled = prefs.getBool("beeper", true);
  deviceName = prefs.getString("deviceName", DEVICE_DISPLAY_NAME);
  prefs.end();

  if (moonHost.length() < 7) moonHost = DEFAULT_MOONRAKER_HOST;
  if (moonPort == 0) moonPort = DEFAULT_MOONRAKER_PORT;
  deviceName.trim();
  if (deviceName.length() == 0) deviceName = DEVICE_DISPLAY_NAME;
  if (deviceName.length() > 16) deviceName = deviceName.substring(0, 16);
}

void NetworkManagerDuender::saveSettings() {
  prefs.begin("duender", false);
  prefs.putString("moonHost", moonHost);
  prefs.putUShort("moonPort", moonPort);
  prefs.putString("deviceName", deviceName);
  prefs.end();
}

void NetworkManagerDuender::saveBeeper() {
  prefs.begin("duender", false);
  prefs.putBool("beeper", beeperEnabled);
  prefs.end();
}

void NetworkManagerDuender::begin() {
  loadSettings();
  dataMutex = xSemaphoreCreateMutex();

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

  xTaskCreate(scanTaskEntry, "printer-scan", 7168, this, 1, &scanTaskHandle);
}

bool NetworkManagerDuender::isWifiConnected() {
  return WiFi.status() == WL_CONNECTED;
}

String NetworkManagerDuender::localIP() {
  return isWifiConnected() ? WiFi.localIP().toString() : "0.0.0.0";
}

String NetworkManagerDuender::getMoonrakerHost() {
  String value;
  if (dataMutex && xSemaphoreTake(dataMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
    value = moonHost;
    xSemaphoreGive(dataMutex);
  } else {
    value = moonHost;
  }
  return value;
}

uint16_t NetworkManagerDuender::getMoonrakerPort() {
  uint16_t value = moonPort;
  if (dataMutex && xSemaphoreTake(dataMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
    value = moonPort;
    xSemaphoreGive(dataMutex);
  }
  return value;
}

void NetworkManagerDuender::setMoonrakerHost(const String& host) {
  if (dataMutex && xSemaphoreTake(dataMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
    moonHost = host;
    moonHost.trim();
    xSemaphoreGive(dataMutex);
  } else {
    moonHost = host;
    moonHost.trim();
  }
  saveSettings();
}

void NetworkManagerDuender::setMoonrakerPort(uint16_t port) {
  if (port == 0) return;
  if (dataMutex && xSemaphoreTake(dataMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
    moonPort = port;
    xSemaphoreGive(dataMutex);
  } else {
    moonPort = port;
  }
  saveSettings();
}

String NetworkManagerDuender::getDeviceName() {
  String value = deviceName;
  if (dataMutex && xSemaphoreTake(dataMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
    value = deviceName;
    xSemaphoreGive(dataMutex);
  }
  if (value.length() == 0) value = DEVICE_DISPLAY_NAME;
  return value;
}

void NetworkManagerDuender::setDeviceName(const String& name) {
  String clean = name;
  clean.trim();
  if (clean.length() == 0) clean = DEVICE_DISPLAY_NAME;
  if (clean.length() > 16) clean = clean.substring(0, 16);

  if (dataMutex && xSemaphoreTake(dataMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
    deviceName = clean;
    xSemaphoreGive(dataMutex);
  } else {
    deviceName = clean;
  }
  saveSettings();
}

bool NetworkManagerDuender::isBeeperEnabled() { return beeperEnabled; }
void NetworkManagerDuender::setBeeperEnabled(bool enabled) { beeperEnabled = enabled; saveBeeper(); }
bool NetworkManagerDuender::toggleBeeper() { beeperEnabled = !beeperEnabled; saveBeeper(); return beeperEnabled; }

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
      if (head.length() > 160) {
        client.stop();
        return false;
      }
    }
    vTaskDelay(1);
  }

  client.stop();
  return false;
}

bool NetworkManagerDuender::getMoonrakerInfo(const String& host, FoundPrinter& out, uint16_t port, uint16_t timeoutMs) {
  if (!isWifiConnected()) return false;

  HTTPClient http;
  String url = String("http://") + host + ":" + String(port) + "/printer/info";
  http.setConnectTimeout(timeoutMs);
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
  if (name.length() == 0) name = host;
  name.replace("Klipper", "");
  name.trim();
  if (name.length() == 0) name = host;
  if (name.length() > 16) name = name.substring(0, 16);

  out.name = name;
  out.host = host;
  out.port = port;
  return true;
}

bool NetworkManagerDuender::alreadyFound(const String& host, uint16_t port) {
  bool result = false;
  if (dataMutex && xSemaphoreTake(dataMutex, pdMS_TO_TICKS(20)) == pdTRUE) {
    for (int i = 0; i < foundCount; i++) {
      if (found[i].host == host && found[i].port == port) {
        result = true;
        break;
      }
    }
    xSemaphoreGive(dataMutex);
  }
  return result;
}

void NetworkManagerDuender::addPrinterIfValid(const String& host, uint16_t port, uint16_t timeoutMs) {
  if (alreadyFound(host, port)) return;

  FoundPrinter fp;
  if (!getMoonrakerInfo(host, fp, port, timeoutMs)) return;

  if (dataMutex && xSemaphoreTake(dataMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
    if (foundCount < DUENDER_MAX_PRINTERS) found[foundCount++] = fp;
    xSemaphoreGive(dataMutex);
  }
}

void NetworkManagerDuender::startPrinterScan() { startFullScan(); }

void NetworkManagerDuender::startQuickScan() {
  if (!isWifiConnected()) return;
  cancelPrinterScan();
  if (dataMutex && xSemaphoreTake(dataMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
    foundCount = 0;
    xSemaphoreGive(dataMutex);
  }
  scanQuickOnly = true;
  scanCancel = false;
  scanProgress = 0;
  scanActive = true;
  if (scanTaskHandle) xTaskNotifyGive(scanTaskHandle);
}

void NetworkManagerDuender::startFullScan() {
  if (!isWifiConnected()) return;
  cancelPrinterScan();
  if (dataMutex && xSemaphoreTake(dataMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
    foundCount = 0;
    xSemaphoreGive(dataMutex);
  }
  scanQuickOnly = false;
  scanCancel = false;
  scanProgress = 0;
  scanActive = true;
  if (scanTaskHandle) xTaskNotifyGive(scanTaskHandle);
}

void NetworkManagerDuender::scanTaskEntry(void* arg) {
  static_cast<NetworkManagerDuender*>(arg)->scanTaskLoop();
}

void NetworkManagerDuender::scanTaskLoop() {
  for (;;) {
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    if (scanActive) runScan(scanQuickOnly);
  }
}

void NetworkManagerDuender::runScan(bool quickOnly) {
  IPAddress ip = WiFi.localIP();
  IPAddress gw = WiFi.gatewayIP();
  String prefix = String(ip[0]) + "." + String(ip[1]) + "." + String(ip[2]) + ".";

  static const uint8_t common[] = {
    2,3,4,5,10,20,30,40,50,60,70,80,81,82,83,84,85,90,91,92,
    100,101,102,110,120,130,140,150,151,152,153,154,155,156,
    160,170,180,190,200,201,202,210,220,230,240,250
  };

  String candidates[80];
  int candidateCount = 0;
  auto addCandidate = [&](const String& host) {
    if (host.length() < 7) return;
    for (int i = 0; i < candidateCount; i++) if (candidates[i] == host) return;
    if (candidateCount < 80) candidates[candidateCount++] = host;
  };

  addCandidate(getMoonrakerHost());
  addCandidate(DEFAULT_MOONRAKER_HOST);
  if (gw[3] > 0 && gw[3] < 255) addCandidate(prefix + String(gw[3]));

  const int nearRadius = 12;
  for (int n = 0; n < nearRadius * 2 + 1; n++) {
    int offset = (n == 0) ? 0 : ((n + 1) / 2) * ((n % 2) ? 1 : -1);
    int last = (int)ip[3] + offset;
    if (last > 0 && last < 255) addCandidate(prefix + String(last));
  }

  for (uint8_t value : common) addCandidate(prefix + String(value));

  int quickTargets = candidateCount * SCAN_PORT_COUNT;
  int fullTargets = 254 * SCAN_PORT_COUNT;
  int totalTargets = quickOnly ? quickTargets : quickTargets + fullTargets;
  int done = 0;

  for (int i = 0; i < candidateCount && !scanCancel; i++) {
    int octet = candidates[i].substring(candidates[i].lastIndexOf('.') + 1).toInt();
    scanCurrentHost = octet;
    for (int p = 0; p < SCAN_PORT_COUNT && !scanCancel; p++) {
      scanCurrentPort = SCAN_PORTS[p];
      addPrinterIfValid(candidates[i], SCAN_PORTS[p], QUICK_SCAN_HTTP_TIMEOUT_MS);
      done++;
      scanProgress = constrain((done * 100) / max(totalTargets, 1), 0, 100);
      vTaskDelay(1);
    }
  }

  if (!quickOnly && !scanCancel) {
    for (int last = 1; last <= 254 && !scanCancel; last++) {
      if (last == ip[3]) {
        done += SCAN_PORT_COUNT;
        continue;
      }
      String host = prefix + String(last);
      scanCurrentHost = last;
      for (int p = 0; p < SCAN_PORT_COUNT && !scanCancel; p++) {
        scanCurrentPort = SCAN_PORTS[p];
        addPrinterIfValid(host, SCAN_PORTS[p], FULL_SCAN_HTTP_TIMEOUT_MS);
        done++;
        scanProgress = constrain((done * 100) / max(totalTargets, 1), 0, 100);
        vTaskDelay(1);
      }
    }
  }

  scanProgress = 100;
  scanActive = false;
}

bool NetworkManagerDuender::updatePrinterScan() { return scanActive; }
bool NetworkManagerDuender::isPrinterScanActive() { return scanActive; }
void NetworkManagerDuender::cancelPrinterScan() { scanCancel = true; scanActive = false; }
int NetworkManagerDuender::getScanProgress() { return scanProgress; }
int NetworkManagerDuender::getScanCurrentHost() { return scanCurrentHost; }
uint16_t NetworkManagerDuender::getScanCurrentPort() { return scanCurrentPort; }

int NetworkManagerDuender::getFoundPrinterCount() {
  int count = 0;
  if (dataMutex && xSemaphoreTake(dataMutex, pdMS_TO_TICKS(20)) == pdTRUE) {
    count = foundCount;
    xSemaphoreGive(dataMutex);
  }
  return count;
}

FoundPrinter NetworkManagerDuender::getFoundPrinter(int index) {
  FoundPrinter result;
  result.name = "";
  result.host = "";
  result.port = DEFAULT_MOONRAKER_PORT;

  if (dataMutex && xSemaphoreTake(dataMutex, pdMS_TO_TICKS(20)) == pdTRUE) {
    if (index >= 0 && index < foundCount) result = found[index];
    xSemaphoreGive(dataMutex);
  }
  return result;
}

int NetworkManagerDuender::scanPrinters(FoundPrinter* out, int maxCount) {
  startQuickScan();
  unsigned long started = millis();
  while (isPrinterScanActive() && millis() - started < 15000) vTaskDelay(pdMS_TO_TICKS(10));

  int n = min(getFoundPrinterCount(), maxCount);
  for (int i = 0; i < n; i++) out[i] = getFoundPrinter(i);
  return n;
}

bool NetworkManagerDuender::autoScanMoonraker() {
  if (!isWifiConnected()) return false;
  if (testMoonraker(getMoonrakerHost(), getMoonrakerPort(), 180)) return true;

  startQuickScan();
  unsigned long started = millis();
  while (isPrinterScanActive() && getFoundPrinterCount() == 0 && millis() - started < 8000) {
    vTaskDelay(pdMS_TO_TICKS(10));
  }

  if (getFoundPrinterCount() > 0) {
    FoundPrinter first = getFoundPrinter(0);
    setMoonrakerHost(first.host);
    setMoonrakerPort(first.port);
    cancelPrinterScan();
    return true;
  }
  return false;
}

void NetworkManagerDuender::startConfigPortal() {
  WiFiManager wm;
  wm.setConfigPortalTimeout(240);

  char moonBuf[32];
  getMoonrakerHost().toCharArray(moonBuf, sizeof(moonBuf));
  WiFiManagerParameter moonParam("moon", "Moonraker IP", moonBuf, 31);
  wm.addParameter(&moonParam);
  wm.startConfigPortal(SETUP_AP_NAME, SETUP_AP_PASSWORD);

  String entered = String(moonParam.getValue());
  entered.trim();
  if (entered.length() >= 7) setMoonrakerHost(entered);
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
