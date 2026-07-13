#pragma once

#include <Arduino.h>
#include <TFT_eSPI.h>
#include "printer_state.h"
#include "touch.h"

enum CydPage {
  PAGE_STATUS,
  PAGE_TEMPS,
  PAGE_CONTROLS,
  PAGE_SYSTEM
};

class DuenderUI {
public:
  void begin(TFT_eSPI* display, PrinterState* state);
  void showBoot(const char* msg, int percent);
  void update();
  void forceRedraw();
  void refreshData();
  bool isBusy();
  void handleTouch(const TouchPoint& tp);

private:
  TFT_eSPI* tft = nullptr;
  PrinterState* ps = nullptr;

  CydPage page = PAGE_STATUS;
  unsigned long lastTouch = 0;
  bool needsFullRedraw = true;

  float lastHotendTemp = -9999;
  float lastHotendTarget = -9999;
  float lastBedTemp = -9999;
  float lastBedTarget = -9999;
  float lastFanPercent = -9999;
  int lastProgress = -9999;
  bool lastWifiOk = false;
  bool lastMoonOk = false;
  String lastState = "";
  String lastMsg = "";
  String lastIp = "";
  String lastMoonHost = "";

  bool inBox(const TouchPoint& tp, int x, int y, int w, int h);
  void resetCache();

  void drawLayout();
  void drawHeader(const char* title);
  void drawFooterNav();
  void drawButton(int x, int y, int w, int h, const char* label, uint16_t color);

  void drawStatusStatic();
  void drawTempsStatic();
  void drawControlsStatic();
  void drawSystemStatic();

  void updateHeaderValues();
  void updateStatusValues();
  void updateTempsValues();
  void updateSystemValues();

  void clearTextArea(int x, int y, int w, int h);
  void drawProgressBar(int x, int y, int w, int h, int pct);
  String clipped(String s, int maxChars);
};

extern DuenderUI UI;
