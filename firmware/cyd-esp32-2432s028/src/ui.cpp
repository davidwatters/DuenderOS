#include "ui.h"
#include "config.h"
#include "network_manager.h"
#include "moonraker.h"
#include "theme.h"
#include "du_icons.h"
#include <WiFi.h>

DuenderUI UI;

// ----------------------
// Local drawing helpers
// ----------------------

static void drawPanel(TFT_eSPI* tft, int x, int y, int w, int h) {
  tft->fillRoundRect(x, y, w, h, 8, DU_PANEL);
  tft->drawRoundRect(x, y, w, h, 8, DU_DIM);
}

static void drawRedLine(TFT_eSPI* tft, int x, int y, int w) {
  tft->drawFastHLine(x, y, w, DU_BORDER);
}

// ----------------------
// DuenderUI
// ----------------------

void DuenderUI::begin(TFT_eSPI* display, PrinterState* state) {
  tft = display;
  ps = state;
  page = PAGE_STATUS;
  needsFullRedraw = true;
  lastTouch = 0;
  resetCache();
}

void DuenderUI::showBoot(const char* msg, int percent) {
  tft->fillScreen(DU_BG);

  tft->fillRoundRect(18, 28, 284, 48, 8, DU_HEADER);
  tft->setTextDatum(MC_DATUM);
  tft->setTextColor(DU_TEXT, DU_HEADER);
  tft->setTextSize(2);
  tft->drawString("DUENDEROS", 160, 52);

  tft->setTextDatum(TL_DATUM);
  tft->setTextSize(1);
  tft->setTextColor(DU_TEXT, DU_BG);
  tft->drawString("CYD ESP32-2432S028", 82, 92, 2);
  tft->drawString(msg, 120, 120, 2);

  tft->drawRoundRect(35, 150, 250, 22, 7, DU_BORDER);
  int fill = map(constrain(percent, 0, 100), 0, 100, 0, 246);
  if (fill > 0) tft->fillRoundRect(37, 152, fill, 18, 5, DU_PROGRESS);
}

void DuenderUI::update() {
  if (needsFullRedraw) {
    drawLayout();
    needsFullRedraw = false;
    refreshData();
  }
}

void DuenderUI::forceRedraw() {
  needsFullRedraw = true;
  resetCache();
}

void DuenderUI::refreshData() {
  if (!tft || !ps) return;

  updateHeaderValues();

  switch (page) {
    case PAGE_STATUS: updateStatusValues(); break;
    case PAGE_TEMPS: updateTempsValues(); break;
    case PAGE_SYSTEM: updateSystemValues(); break;
    case PAGE_CONTROLS: break;
  }
}

bool DuenderUI::isBusy() {
  return false;
}

bool DuenderUI::inBox(const TouchPoint& tp, int x, int y, int w, int h) {
  return tp.x >= x && tp.x <= x + w && tp.y >= y && tp.y <= y + h;
}

void DuenderUI::resetCache() {
  lastHotendTemp = -9999;
  lastHotendTarget = -9999;
  lastBedTemp = -9999;
  lastBedTarget = -9999;
  lastFanPercent = -9999;
  lastProgress = -9999;
  lastWifiOk = !ps->wifiOk;
  lastMoonOk = !ps->moonrakerOk;
  lastState = "";
  lastMsg = "";
  lastIp = "";
  lastMoonHost = "";
}

void DuenderUI::clearTextArea(int x, int y, int w, int h) {
  tft->fillRect(x, y, w, h, DU_BG);
}

String DuenderUI::clipped(String s, int maxChars) {
  if (s.length() <= maxChars) return s;
  return s.substring(0, maxChars - 3) + "...";
}

void DuenderUI::drawLayout() {
  tft->fillScreen(DU_BG);

  switch (page) {
    case PAGE_STATUS:
      drawHeader("STATUS");
      drawStatusStatic();
      break;
    case PAGE_TEMPS:
      drawHeader("TEMPS");
      drawTempsStatic();
      break;
    case PAGE_CONTROLS:
      drawHeader("CTRL");
      drawControlsStatic();
      break;
    case PAGE_SYSTEM:
      drawHeader("SYSTEM");
      drawSystemStatic();
      break;
  }

  drawFooterNav();
}

void DuenderUI::drawHeader(const char* title) {
  tft->fillRect(0, 0, 320, 38, DU_HEADER);

  tft->setTextDatum(TL_DATUM);
  tft->setTextColor(DU_TEXT, DU_HEADER);
  tft->setTextSize(2);
  tft->drawString("DUENDEROS", 8, 8);

  tft->setTextSize(1);
  tft->drawString(title, 142, 10, 2);

  // Icon-only header:
  // Ethernet plug = network/WiFi state
  // Moon = Moonraker state
  DUIcons::ethernet(tft, 246, 7, DU_TEXT);
  tft->drawFastVLine(284, 7, 22, DU_TEXT);
  DUIcons::moon(tft, 304, 18, DU_TEXT, DU_HEADER);

  tft->drawFastHLine(0, 38, 320, DU_BORDER);
}

void DuenderUI::updateHeaderValues() {
  if (lastWifiOk != ps->wifiOk) {
    tft->fillRect(236, 0, 48, 37, DU_HEADER);
    DUIcons::ethernet(tft, 246, 7, ps->wifiOk ? DU_TEXT : DU_DIM);
    tft->drawFastVLine(284, 7, 22, DU_TEXT);
    lastWifiOk = ps->wifiOk;
  }

  if (lastMoonOk != ps->moonrakerOk) {
    tft->fillRect(286, 0, 34, 37, DU_HEADER);
    DUIcons::moon(tft, 304, 18, ps->moonrakerOk ? DU_TEXT : DU_DIM, DU_HEADER);
    lastMoonOk = ps->moonrakerOk;
  }
}

void DuenderUI::drawButton(int x, int y, int w, int h, const char* label, uint16_t color) {
  bool active = (color == DU_ACTIVE || color == DU_RED_DARK);

  tft->fillRoundRect(x, y, w, h, 7, active ? color : DU_BG);
  tft->drawRoundRect(x, y, w, h, 7, DU_BORDER);

  tft->setTextDatum(MC_DATUM);
  tft->setTextSize(1);
  tft->setTextColor(DU_TEXT, active ? color : DU_BG);
  tft->drawString(label, x + w / 2, y + h / 2 - 3, 2);
  tft->setTextDatum(TL_DATUM);
}

void DuenderUI::drawFooterNav() {
  // Bottom menu with centered icons and text below them
  tft->drawFastHLine(0, 198, 320, DU_BORDER);

  const int y = 199;
  const int h = 41;

  uint16_t bgStatus = page == PAGE_STATUS ? DU_ACTIVE : DU_BG;
  uint16_t bgTemps  = page == PAGE_TEMPS ? DU_ACTIVE : DU_BG;
  uint16_t bgCtrl   = page == PAGE_CONTROLS ? DU_ACTIVE : DU_BG;
  uint16_t bgSystem = page == PAGE_SYSTEM ? DU_ACTIVE : DU_BG;

  tft->fillRect(0,   y, 80, h, bgStatus);
  tft->fillRect(80,  y, 80, h, bgTemps);
  tft->fillRect(160, y, 80, h, bgCtrl);
  tft->fillRect(240, y, 80, h, bgSystem);

  tft->drawRect(0,   y, 80, h, DU_BORDER);
  tft->drawRect(80,  y, 80, h, DU_BORDER);
  tft->drawRect(160, y, 80, h, DU_BORDER);
  tft->drawRect(240, y, 80, h, DU_BORDER);

  uint16_t c1 = page == PAGE_STATUS ? DU_TEXT : DU_DIM;
  uint16_t c2 = page == PAGE_TEMPS ? DU_TEXT : DU_DIM;
  uint16_t c3 = page == PAGE_CONTROLS ? DU_TEXT : DU_DIM;
  uint16_t c4 = page == PAGE_SYSTEM ? DU_TEXT : DU_DIM;

  DUIcons::grid(tft, 30, 203, c1);
  DUIcons::thermometer(tft, 110, 202, c2);
  DUIcons::sliders(tft, 188, 203, c3);
  DUIcons::gear(tft, 268, 203, c4);

  tft->setTextDatum(MC_DATUM);
  tft->setTextSize(1);

  tft->setTextColor(c1, bgStatus);
  tft->drawString("STATUS", 40, 229, 2);

  tft->setTextColor(c2, bgTemps);
  tft->drawString("TEMPS", 120, 229, 2);

  tft->setTextColor(c3, bgCtrl);
  tft->drawString("CTRL", 200, 229, 2);

  tft->setTextColor(c4, bgSystem);
  tft->drawString("SYSTEM", 280, 229, 2);

  tft->setTextDatum(TL_DATUM);
}

// ----------------------
// STATUS PAGE
// ----------------------

void DuenderUI::drawStatusStatic() {
  drawPanel(tft, 8, 47, 304, 112);

  tft->drawFastHLine(8, 75, 304, DU_PANEL2);
  tft->drawFastHLine(8, 103, 304, DU_PANEL2);
  tft->drawFastHLine(8, 131, 304, DU_PANEL2);

  DUIcons::nozzle(tft, 16, 51, DU_RED);
  DUIcons::bed(tft, 15, 80, DU_RED);

  tft->setTextDatum(TL_DATUM);
  tft->setTextSize(1);
  tft->setTextColor(DU_RED, DU_PANEL);

  tft->drawString("HOTEND", 48, 55, 2);
  tft->drawString("BED", 48, 83, 2);
  tft->drawString("STATE", 48, 111, 2);
  tft->drawString("FILE", 48, 139, 2);

  drawPanel(tft, 8, 164, 304, 31);
  tft->setTextColor(DU_RED, DU_PANEL);
  tft->drawString("PROGRESS", 18, 169, 2);
  tft->drawRoundRect(18, 184, 245, 8, 4, DU_DIM);
}

void DuenderUI::updateStatusValues() {
  char line[64];

  if (lastHotendTemp != ps->hotendTemp || lastHotendTarget != ps->hotendTarget) {
    tft->fillRect(150, 51, 150, 20, DU_PANEL);
    tft->setTextColor(DU_TEXT, DU_PANEL);
    tft->setTextSize(2);
    snprintf(line, sizeof(line), "%.0f / %.0f", ps->hotendTemp, ps->hotendTarget);
    tft->drawString(line, 150, 51);
    lastHotendTemp = ps->hotendTemp;
    lastHotendTarget = ps->hotendTarget;
  }

  if (lastBedTemp != ps->bedTemp || lastBedTarget != ps->bedTarget) {
    tft->fillRect(150, 79, 150, 20, DU_PANEL);
    tft->setTextColor(DU_TEXT, DU_PANEL);
    tft->setTextSize(2);
    snprintf(line, sizeof(line), "%.0f / %.0f", ps->bedTemp, ps->bedTarget);
    tft->drawString(line, 150, 79);
    lastBedTemp = ps->bedTemp;
    lastBedTarget = ps->bedTarget;
  }

  String state = ps->state.length() ? ps->state : "unknown";
  if (state != lastState) {
    tft->fillRect(150, 109, 150, 18, DU_PANEL);
    tft->setTextSize(1);
    tft->setTextColor(DU_WARNING, DU_PANEL);
    tft->drawString(clipped(state, 18), 150, 111, 2);
    lastState = state;
  }

  String msg = ps->message.length() ? ps->message : ps->file;
  if (!ps->moonrakerOk && ps->error.length()) msg = ps->error;
  if (msg.length() == 0) msg = ps->moonrakerOk ? "No file selected" : "Connecting...";
  msg = clipped(msg, 22);

  if (msg != lastMsg) {
    tft->fillRect(150, 137, 155, 18, DU_PANEL);
    tft->setTextSize(1);
    tft->setTextColor(DU_TEXT, DU_PANEL);
    tft->drawString(msg, 150, 139, 2);
    lastMsg = msg;
  }

  if (lastProgress != ps->progress) {
    drawProgressBar(18, 184, 245, 8, ps->progress);
    tft->fillRect(267, 169, 42, 20, DU_PANEL);
    tft->setTextColor(DU_TEXT, DU_PANEL);
    tft->setTextSize(1);
    snprintf(line, sizeof(line), "%d%%", ps->progress);
    tft->drawString(line, 270, 170, 2);
    lastProgress = ps->progress;
  }
}

// ----------------------
// TEMPS PAGE
// ----------------------

void DuenderUI::drawTempsStatic() {
  drawPanel(tft, 8, 48, 148, 82);
  drawPanel(tft, 164, 48, 148, 82);

  tft->setTextDatum(TL_DATUM);
  tft->setTextSize(1);

  tft->setTextColor(DU_RED, DU_PANEL);
  tft->drawString("HOTEND", 18, 56, 2);
  DUIcons::nozzle(tft, 120, 55, DU_RED);

  tft->drawString("BED", 174, 56, 2);
  DUIcons::bed(tft, 276, 55, DU_RED);

  drawRedLine(tft, 18, 100, 128);
  drawRedLine(tft, 174, 100, 128);

  tft->setTextColor(DU_LABEL, DU_PANEL);
  tft->drawString("TARGET", 18, 107, 2);
  tft->drawString("TARGET", 174, 107, 2);

  drawPanel(tft, 8, 134, 304, 24);
  DUIcons::fan(tft, 28, 146, DU_RED);
  tft->setTextColor(DU_RED, DU_PANEL);
  tft->drawString("FAN", 54, 138, 2);

  // Temp presets
  drawButton(8, 162, 96, 17, "PLA", DU_BG);
  drawButton(112, 162, 96, 17, "PETG", DU_BG);
  drawButton(216, 162, 96, 17, "COOL", DU_BG);

  // Fan controls
  drawButton(8, 182, 70, 15, "FAN OFF", DU_BG);
  drawButton(84, 182, 70, 15, "50%", DU_BG);
  drawButton(160, 182, 70, 15, "100%", DU_BG);
  drawButton(236, 182, 76, 15, "M106", DU_BG);
}

void DuenderUI::updateTempsValues() {
  char line[64];

  if (lastHotendTemp != ps->hotendTemp) {
    tft->fillRect(35, 73, 80, 25, DU_PANEL);
    tft->setTextColor(DU_TEXT, DU_PANEL);
    tft->setTextSize(3);
    snprintf(line, sizeof(line), "%.0f", ps->hotendTemp);
    tft->drawString(line, 38, 70);
    tft->setTextSize(2);
    tft->drawString("°", 102, 70);
    lastHotendTemp = ps->hotendTemp;
  }

  if (lastHotendTarget != ps->hotendTarget) {
    tft->fillRect(103, 105, 43, 20, DU_PANEL);
    tft->setTextColor(DU_TEXT, DU_PANEL);
    tft->setTextSize(2);
    snprintf(line, sizeof(line), "%.0f", ps->hotendTarget);
    tft->drawString(line, 104, 105);
    tft->setTextSize(1);
    tft->drawString("°", 140, 107, 2);
    lastHotendTarget = ps->hotendTarget;
  }

  if (lastBedTemp != ps->bedTemp) {
    tft->fillRect(191, 73, 80, 25, DU_PANEL);
    tft->setTextColor(DU_TEXT, DU_PANEL);
    tft->setTextSize(3);
    snprintf(line, sizeof(line), "%.0f", ps->bedTemp);
    tft->drawString(line, 194, 70);
    tft->setTextSize(2);
    tft->drawString("°", 258, 70);
    lastBedTemp = ps->bedTemp;
  }

  if (lastBedTarget != ps->bedTarget) {
    tft->fillRect(259, 105, 43, 20, DU_PANEL);
    tft->setTextColor(DU_TEXT, DU_PANEL);
    tft->setTextSize(2);
    snprintf(line, sizeof(line), "%.0f", ps->bedTarget);
    tft->drawString(line, 260, 105);
    tft->setTextSize(1);
    tft->drawString("°", 296, 107, 2);
    lastBedTarget = ps->bedTarget;
  }

  if (lastFanPercent != ps->fanPercent) {
    tft->fillRect(260, 138, 45, 16, DU_PANEL);
    tft->setTextColor(DU_TEXT, DU_PANEL);
    tft->setTextSize(1);
    snprintf(line, sizeof(line), "%.0f%%", ps->fanPercent);
    tft->drawString(line, 266, 138, 2);
    lastFanPercent = ps->fanPercent;
  }
}

// ----------------------
// CONTROLS PAGE
// ----------------------

void DuenderUI::drawControlsStatic() {
  drawButton(8,   50, 96, 36, "HOME", DU_BG);
  drawButton(112, 50, 96, 36, "PAUSE", DU_BG);
  drawButton(216, 50, 96, 36, "RESUME", DU_BG);

  drawButton(8,   96, 96, 36, "CANCEL", DU_RED_DARK);
  drawButton(112, 96, 96, 36, "MOTORS", DU_BG);
  drawButton(216, 96, 96, 36, "PARK", DU_BG);

  drawButton(8,   145, 148, 34, "BED MESH", DU_BG);
  drawButton(164, 145, 148, 34, "SHAPER", DU_BG);
}

// ----------------------
// SYSTEM PAGE
// ----------------------

void DuenderUI::drawSystemStatic() {
  drawPanel(tft, 8, 48, 304, 101);

  tft->setTextDatum(TL_DATUM);
  tft->setTextSize(1);
  tft->setTextColor(DU_RED, DU_PANEL);

  DUIcons::ethernet(tft, 16, 53, DU_RED);
  tft->drawString("NETWORK", 54, 58, 2);
  tft->drawString("IP", 54, 82, 2);
  tft->drawString("MOONRAKER", 54, 106, 2);
  tft->drawString("PORT", 54, 130, 2);

  tft->drawFastHLine(8, 75, 304, DU_PANEL2);
  tft->drawFastHLine(8, 99, 304, DU_PANEL2);
  tft->drawFastHLine(8, 123, 304, DU_PANEL2);

  drawButton(8, 158, 148, 36, "WIFI PORTAL", DU_BG);
  drawButton(164, 158, 148, 36, "RESET WIFI", DU_RED_DARK);
}

void DuenderUI::updateSystemValues() {
  String wifi = ps->wifiOk ? "OK" : "FAIL";
  if (wifi != lastState) {
    tft->fillRect(132, 56, 160, 18, DU_PANEL);
    tft->setTextColor(ps->wifiOk ? DU_SUCCESS : DU_ERROR, DU_PANEL);
    tft->drawString(wifi, 132, 58, 2);
    lastState = wifi;
  }

  String ip = Network.localIP();
  if (ip != lastIp) {
    tft->fillRect(132, 80, 160, 18, DU_PANEL);
    tft->setTextColor(DU_TEXT, DU_PANEL);
    tft->drawString(ip, 132, 82, 2);
    lastIp = ip;
  }

  String host = Network.getMoonrakerHost();
  if (host != lastMoonHost) {
    tft->fillRect(132, 104, 160, 18, DU_PANEL);
    tft->setTextColor(DU_TEXT, DU_PANEL);
    tft->drawString(host, 132, 106, 2);
    lastMoonHost = host;
  }

  tft->fillRect(132, 128, 160, 18, DU_PANEL);
  tft->setTextColor(DU_TEXT, DU_PANEL);
  tft->drawString(String(Network.getMoonrakerPort()), 132, 130, 2);
}

// ----------------------
// Shared widgets
// ----------------------

void DuenderUI::drawProgressBar(int x, int y, int w, int h, int pct) {
  pct = constrain(pct, 0, 100);

  tft->fillRoundRect(x + 1, y + 1, w - 2, h - 2, 4, DU_BG);
  int fill = map(pct, 0, 100, 0, w - 2);
  if (fill > 0) {
    tft->fillRoundRect(x + 1, y + 1, fill, h - 2, 4, DU_PROGRESS);
  }
  tft->drawRoundRect(x, y, w, h, 4, DU_DIM);
}

// ----------------------
// Touch routing
// ----------------------

void DuenderUI::handleTouch(const TouchPoint& tp) {
  if (!tp.pressed) return;
  if (millis() - lastTouch < 350) return;
  lastTouch = millis();

  // Footer nav touch zones
  if (inBox(tp, 0, 199, 80, 41)) {
    page = PAGE_STATUS;
    forceRedraw();
    return;
  }

  if (inBox(tp, 80, 199, 80, 41)) {
    page = PAGE_TEMPS;
    forceRedraw();
    return;
  }

  if (inBox(tp, 160, 199, 80, 41)) {
    page = PAGE_CONTROLS;
    forceRedraw();
    return;
  }

  if (inBox(tp, 240, 199, 80, 41)) {
    page = PAGE_SYSTEM;
    forceRedraw();
    return;
  }

  if (page == PAGE_TEMPS) {
    // Temp presets
    if (inBox(tp, 8, 162, 96, 17)) {
      Moonraker.sendGcode("M104 S215\nM140 S60");
    } else if (inBox(tp, 112, 162, 96, 17)) {
      Moonraker.sendGcode("M104 S240\nM140 S75");
    } else if (inBox(tp, 216, 162, 96, 17)) {
      Moonraker.sendGcode("M104 S0\nM140 S0\nM107");

    // Fan controls
    } else if (inBox(tp, 8, 182, 70, 15)) {
      Moonraker.sendGcode("M106 S0");
    } else if (inBox(tp, 84, 182, 70, 15)) {
      Moonraker.sendGcode("M106 S128");
    } else if (inBox(tp, 160, 182, 70, 15)) {
      Moonraker.sendGcode("M106 S255");
    } else if (inBox(tp, 236, 182, 76, 15)) {
      Moonraker.sendGcode("M106");
    }

    forceRedraw();
    return;
  }

  if (page == PAGE_CONTROLS) {
    if (inBox(tp, 8, 50, 96, 36)) {
      Moonraker.sendGcode("G28");
    } else if (inBox(tp, 112, 50, 96, 36)) {
      Moonraker.sendGcode("PAUSE");
    } else if (inBox(tp, 216, 50, 96, 36)) {
      Moonraker.sendGcode("RESUME");
    } else if (inBox(tp, 8, 96, 96, 36)) {
      Moonraker.sendGcode("CANCEL_PRINT");
    } else if (inBox(tp, 112, 96, 96, 36)) {
      Moonraker.sendGcode("M84");
    } else if (inBox(tp, 216, 96, 96, 36)) {
      Moonraker.sendGcode("PARK");
    } else if (inBox(tp, 8, 145, 148, 34)) {
      Moonraker.sendGcode("BED_MESH_CALIBRATE");
    } else if (inBox(tp, 164, 145, 148, 34)) {
      Moonraker.sendGcode("SHAPER_CALIBRATE");
    }
    forceRedraw();
    return;
  }

  if (page == PAGE_SYSTEM) {
    if (inBox(tp, 8, 158, 148, 36)) {
      Network.startConfigPortal();
    } else if (inBox(tp, 164, 158, 148, 36)) {
      Network.resetWiFiAndSettings();
    }
    forceRedraw();
    return;
  }
}
