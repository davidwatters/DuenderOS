#pragma once
#include <Arduino.h>
#include <IPAddress.h>

#define DUENDER_MAX_PRINTERS 10

struct FoundPrinter {
  String name;
  String host;
  uint16_t port;
};

class NetworkManagerDuender {
public:
  void begin();
  bool isWifiConnected();
  String localIP();

  String getMoonrakerHost();
  uint16_t getMoonrakerPort();
  void setMoonrakerHost(const String& host);
  void setMoonrakerPort(uint16_t port);

  bool testMoonraker(const String& host, uint16_t port = 7125, uint16_t timeoutMs = 650);
  bool autoScanMoonraker();
  int scanPrinters(FoundPrinter* out, int maxCount);
  bool getMoonrakerInfo(const String& host, FoundPrinter& out, uint16_t port = 7125, uint16_t timeoutMs = 700);

  void startConfigPortal();
  void resetWiFiAndSettings();

private:
  String moonHost = "10.0.0.154";
  uint16_t moonPort = 7125;
  void loadSettings();
  void saveSettings();
  String ipToString(const IPAddress& ip);
};

extern NetworkManagerDuender Network;
