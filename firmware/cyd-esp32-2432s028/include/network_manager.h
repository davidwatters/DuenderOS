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

  bool isBeeperEnabled();
  void setBeeperEnabled(bool enabled);
  bool toggleBeeper();

  bool testMoonraker(const String& host, uint16_t port = 7125, uint16_t timeoutMs = 180);
  bool getMoonrakerInfo(const String& host, FoundPrinter& out, uint16_t port = 7125, uint16_t timeoutMs = 220);

  // Non-blocking scan. Call startPrinterScan(), then call updatePrinterScan()
  // repeatedly from loop/UI until isPrinterScanActive() returns false.
  void startPrinterScan();   // compatibility: starts Full Scan
  void startQuickScan();
  void startFullScan();
  bool updatePrinterScan();
  bool isPrinterScanActive();
  void cancelPrinterScan();
  int getScanProgress();
  int getScanCurrentHost();
  int getFoundPrinterCount();
  FoundPrinter getFoundPrinter(int index);

  // Blocking helper kept for serial testing only.
  int scanPrinters(FoundPrinter* out, int maxCount);
  bool autoScanMoonraker();

  void startConfigPortal();
  void resetWiFiAndSettings();

private:
  String moonHost = "10.0.0.154";
  uint16_t moonPort = 7125;
  bool beeperEnabled = true;

  FoundPrinter found[DUENDER_MAX_PRINTERS];
  int foundCount = 0;
  bool scanActive = false;
  int scanStep = 0;
  int scanLastOctet = 1;
  bool scanQuickOnly = false;
  unsigned long lastScanTick = 0;

  void loadSettings();
  void saveSettings();
  void saveBeeper();
  String ipToString(const IPAddress& ip);
  bool alreadyFound(const String& host);
  void addPrinterIfValid(const String& host, uint16_t timeoutMs);
};

extern NetworkManagerDuender Network;
