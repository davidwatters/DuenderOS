#pragma once
#include <Arduino.h>
#include <IPAddress.h>
#include "config.h"

#define DUENDER_MAX_PRINTERS 16

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

  String getDeviceName();
  void setDeviceName(const String& name);

  bool isBeeperEnabled();
  void setBeeperEnabled(bool enabled);
  bool toggleBeeper();

  bool testMoonraker(const String& host, uint16_t port = 7125, uint16_t timeoutMs = 180);
  bool getMoonrakerInfo(const String& host, FoundPrinter& out, uint16_t port = 7125, uint16_t timeoutMs = 220);

  // Background scan API. No HTTP work runs in the UI loop.
  void startPrinterScan();
  void startQuickScan();
  void startFullScan();
  bool updatePrinterScan(); // compatibility; now only reports status
  bool isPrinterScanActive();
  void cancelPrinterScan();
  int getScanProgress();
  int getScanCurrentHost();
  uint16_t getScanCurrentPort();
  int getFoundPrinterCount();
  FoundPrinter getFoundPrinter(int index);

  int scanPrinters(FoundPrinter* out, int maxCount);
  bool autoScanMoonraker();

  void startConfigPortal();
  void resetWiFiAndSettings();

private:
  String moonHost = DEFAULT_MOONRAKER_HOST;
  uint16_t moonPort = DEFAULT_MOONRAKER_PORT;
  bool beeperEnabled = true;
  String deviceName = DEVICE_DISPLAY_NAME;

  FoundPrinter found[DUENDER_MAX_PRINTERS];
  int foundCount = 0;
  volatile bool scanActive = false;
  volatile bool scanQuickOnly = false;
  volatile bool scanCancel = false;
  volatile int scanProgress = 0;
  volatile int scanCurrentHost = 0;
  volatile uint16_t scanCurrentPort = 0;

  SemaphoreHandle_t dataMutex = nullptr;
  TaskHandle_t scanTaskHandle = nullptr;

  void loadSettings();
  void saveSettings();
  void saveBeeper();
  String ipToString(const IPAddress& ip);
  bool alreadyFound(const String& host, uint16_t port);
  void addPrinterIfValid(const String& host, uint16_t port, uint16_t timeoutMs);

  static void scanTaskEntry(void* arg);
  void scanTaskLoop();
  void runScan(bool quickOnly);
};

extern NetworkManagerDuender Network;
