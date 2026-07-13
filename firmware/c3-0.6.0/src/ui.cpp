#include "ui.h"
#include "config.h"
#include "duender_logo.h"
#include "moonraker.h"
#include "network_manager.h"
#include <WiFi.h>

DuenderUI UI;

static const char* MAIN_ITEMS[] = {"Status", "Print", "Motion", "Temperatures", "Macros", "Files", "System", "Controls", "Settings"};
static const char* PRINT_ITEMS[] = {"Print Status", "Pause", "Resume", "Cancel", "Reprint Current", "Back"};
static const char* MOTION_ITEMS[] = {"Motion Status", "Home All", "Home X", "Home Y", "Home Z", "Disable Motors", "Park Head", "Back"};
static const char* TEMP_ITEMS[] = {"Temp Status", "Preheat PLA", "Preheat PETG", "Cooldown", "PID_HOTEND", "PID_BED", "Back"};
static const char* MACRO_ITEMS[] = {"BELT_TEST", "BED_MESH", "INPUT_SHAPER", "PID_HOTEND", "PID_BED", "CLEAN_NOZZLE", "LIGHTS_ON", "LIGHTS_OFF", "Back"};
static const char* FILE_ITEMS[] = {"Reprint Current", "Calibration", "Parts", "Dragonburner", "Back"};
static const char* SYSTEM_ITEMS[] = {"System Status", "Restart Klipper", "Restart Firmware", "Restart Moonraker", "Emergency Stop", "Back"};
static const char* CONTROL_ITEMS[] = {"Home All", "Pause", "Resume", "Cancel", "Cooldown", "Preheat PLA", "Back"};
static const char* SETTING_ITEMS[] = {"Network", "Device Name", "Buzzer On/Off", "Beeper Test", "Moonraker IP", "WiFi Portal", "Reset WiFi", "Status Page", "About", "Back"};
static const char* NETWORK_ITEMS[] = {"Quick Scan", "Full Scan", "Moonraker IP", "Known Printers", "WiFi Portal", "Reset WiFi", "Back"};

void DuenderUI::begin(U8G2_ST7920_128X64_F_SW_SPI* display, PrinterState* state) {
  u = display;
  ps = state;
  page = PAGE_STATUS;
}

void DuenderUI::showBoot(const char* msg, int percent) {
  u->clearBuffer();
  u->drawXBMP(28, 0, DUENDER_LOGO_W, DUENDER_LOGO_H, duenderLogo);
  u->setFont(u8g2_font_6x12_tr);
  String bootName = Network.getDeviceName();
  int w = u->getStrWidth(bootName.c_str());
  u->drawStr((128 - w) / 2, 45, bootName.c_str());
  w = u->getStrWidth(msg);
  u->drawStr((128 - w) / 2, 56, msg);
  drawProgress(18, 59, 92, 5, percent);
  u->sendBuffer();
}

void DuenderUI::update() {
  if (millis() - lastDraw < UI_DRAW_INTERVAL_MS) return;
  lastDraw = millis();

  if (!ps->moonrakerOk && page != PAGE_MENU) {
    // stay usable, but show connection warning on status page
  }

  switch (page) {
    case PAGE_MENU: drawMenu(); break;
    case PAGE_STATUS: drawStatus(); break;
    case PAGE_PRINT_STATUS: drawPrintStatus(); break;
    case PAGE_MOTION: drawMotion(); break;
    case PAGE_TEMPS: drawTemps(); break;
    case PAGE_SYSTEM: drawSystem(); break;
    case PAGE_ABOUT: drawAbout(); break;
    case PAGE_PRINTER_SCAN: drawPrinterScan(); break;
    case PAGE_IP_EDIT: drawIpEditor(); break;
    case PAGE_PRINTER_SELECT: drawPrinterSelect(); break;
    case PAGE_TEXT_EDIT: drawTextEditor(); break;
  }
}

void DuenderUI::handleEncoder(int delta) {
  if (delta == 0) return;
  lastInput = millis();
  if (page == PAGE_TEXT_EDIT) {
    static const char CHARSET[] = " ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_.";
    const int charsetLen = sizeof(CHARSET) - 1;
    int idx = textCharIndex(textEdit[textCursor]);
    idx = (idx + delta) % charsetLen;
    if (idx < 0) idx += charsetLen;
    textEdit[textCursor] = CHARSET[idx];
    return;
  }
  if (page == PAGE_IP_EDIT) {
    int v = (int)ipEdit[ipCursor] + delta;
    if (v < 0) v = 255;
    if (v > 255) v = 0;
    ipEdit[ipCursor] = (uint8_t)v;
    return;
  }
  if (page == PAGE_PRINTER_SCAN) {
    return;
  }

  if (page == PAGE_PRINTER_SELECT) {
    if (foundPrinterCount <= 0) return;
    foundPrinterSelected += delta;
    if (foundPrinterSelected < 0) foundPrinterSelected = foundPrinterCount - 1;
    if (foundPrinterSelected >= foundPrinterCount) foundPrinterSelected = 0;
    if (foundPrinterSelected < foundPrinterTop) foundPrinterTop = foundPrinterSelected;
    if (foundPrinterSelected > foundPrinterTop + 2) foundPrinterTop = foundPrinterSelected - 2;
    return;
  }
  if (page != PAGE_MENU) {
    page = PAGE_MENU;
    menu = MENU_MAIN;
    selected = 0;
    topIndex = 0;
    return;
  }
  int count = 0;
  menuItems(menu, count);
  selected += delta;
  if (selected < 0) selected = count - 1;
  if (selected >= count) selected = 0;
  if (selected < topIndex) topIndex = selected;
  if (selected > topIndex + 2) topIndex = selected - 2;
}

void DuenderUI::handleButton(ButtonEvent ev) {
  if (ev == BTN_NONE) return;
  lastInput = millis();

  if (ev == BTN_DOUBLE) {
    page = PAGE_STATUS;
    menu = MENU_MAIN;
    selected = 0;
    topIndex = 0;
    setNotice("Home");
    return;
  }

  if (page == PAGE_TEXT_EDIT) {
    if (ev == BTN_SHORT) {
      if (textCursor < TEXT_EDIT_MAX - 1) textCursor++;
      else textCursor = 0;
    } else if (ev == BTN_LONG) {
      saveTextEditor();
    }
    return;
  }

  if (page == PAGE_IP_EDIT) {
    if (ev == BTN_SHORT) {
      if (ipCursor < 3) ipCursor++;
      else saveIpEditor();
    } else if (ev == BTN_LONG) {
      saveIpEditor();
    }
    return;
  }

  if (page == PAGE_PRINTER_SCAN) {
    if (ev == BTN_LONG) { Network.cancelPrinterScan(); page = PAGE_MENU; setNotice("Scan stopped"); }
    return;
  }

  if (page == PAGE_PRINTER_SELECT) {
    if (ev == BTN_SHORT) selectFoundPrinter();
    else if (ev == BTN_LONG) { page = PAGE_MENU; setNotice("Back"); }
    return;
  }

  if (ev == BTN_LONG) {
    if (page == PAGE_MENU) goBack();
    else {
      page = PAGE_MENU;
      menu = MENU_MAIN;
      selected = 0;
      topIndex = 0;
    }
    return;
  }

  if (ev == BTN_SHORT) {
    if (page != PAGE_MENU) {
      page = PAGE_MENU;
      menu = MENU_MAIN;
      selected = 0;
      topIndex = 0;
    } else selectCurrent();
  }
}

const char* const* DuenderUI::menuItems(MenuId id, int& count) {
  switch (id) {
    case MENU_PRINT: count = sizeof(PRINT_ITEMS)/sizeof(PRINT_ITEMS[0]); return PRINT_ITEMS;
    case MENU_MOTION: count = sizeof(MOTION_ITEMS)/sizeof(MOTION_ITEMS[0]); return MOTION_ITEMS;
    case MENU_TEMPS: count = sizeof(TEMP_ITEMS)/sizeof(TEMP_ITEMS[0]); return TEMP_ITEMS;
    case MENU_MACROS: count = sizeof(MACRO_ITEMS)/sizeof(MACRO_ITEMS[0]); return MACRO_ITEMS;
    case MENU_FILES: count = sizeof(FILE_ITEMS)/sizeof(FILE_ITEMS[0]); return FILE_ITEMS;
    case MENU_SYSTEM: count = sizeof(SYSTEM_ITEMS)/sizeof(SYSTEM_ITEMS[0]); return SYSTEM_ITEMS;
    case MENU_CONTROLS: count = sizeof(CONTROL_ITEMS)/sizeof(CONTROL_ITEMS[0]); return CONTROL_ITEMS;
    case MENU_SETTINGS: count = sizeof(SETTING_ITEMS)/sizeof(SETTING_ITEMS[0]); return SETTING_ITEMS;
    case MENU_NETWORK: count = sizeof(NETWORK_ITEMS)/sizeof(NETWORK_ITEMS[0]); return NETWORK_ITEMS;
    case MENU_MAIN:
    default: count = sizeof(MAIN_ITEMS)/sizeof(MAIN_ITEMS[0]); return MAIN_ITEMS;
  }
}

const char* DuenderUI::menuTitle(MenuId id) {
  switch (id) {
    case MENU_PRINT: return "PRINT";
    case MENU_MOTION: return "MOTION";
    case MENU_TEMPS: return "TEMPS";
    case MENU_MACROS: return "MACROS";
    case MENU_FILES: return "FILES";
    case MENU_SYSTEM: return "SYSTEM";
    case MENU_CONTROLS: return "CONTROL";
    case MENU_SETTINGS: return "SETTINGS";
    case MENU_NETWORK: return "NETWORK";
    case MENU_MAIN:
    default: return "MENU";
  }
}

void DuenderUI::enterMenu(MenuId id) {
  if (stackDepth < 8) stack[stackDepth++] = menu;
  menu = id;
  selected = 0;
  topIndex = 0;
  page = PAGE_MENU;
}

void DuenderUI::goBack() {
  if (stackDepth > 0) {
    menu = stack[--stackDepth];
    selected = 0;
    topIndex = 0;
    page = PAGE_MENU;
  } else {
    page = PAGE_STATUS;
  }
}

void DuenderUI::selectCurrent() {
  int count = 0;
  const char* const* items = menuItems(menu, count);
  String item = items[selected];

  if (item == "Back") { goBack(); return; }

  if (menu == MENU_MAIN) {
    if (item == "Status") page = PAGE_STATUS;
    else if (item == "Print") enterMenu(MENU_PRINT);
    else if (item == "Motion") enterMenu(MENU_MOTION);
    else if (item == "Temperatures") enterMenu(MENU_TEMPS);
    else if (item == "Macros") enterMenu(MENU_MACROS);
    else if (item == "Files") enterMenu(MENU_FILES);
    else if (item == "System") enterMenu(MENU_SYSTEM);
    else if (item == "Controls") enterMenu(MENU_CONTROLS);
    else if (item == "Settings") enterMenu(MENU_SETTINGS);
    return;
  }

  if (item == "Print Status") { page = PAGE_PRINT_STATUS; return; }
  if (item == "Motion Status") { page = PAGE_MOTION; return; }
  if (item == "Temp Status") { page = PAGE_TEMPS; return; }
  if (item == "System Status") { page = PAGE_SYSTEM; return; }
  if (item == "About") { page = PAGE_ABOUT; return; }
  if (item == "Network") { enterMenu(MENU_NETWORK); return; }
  if (item == "Quick Scan") { startPrinterScanPage(true); return; }
  if (item == "Full Scan") { startPrinterScanPage(false); return; }
  if (item == "Known Printers") { startPrinterScanPage(true); return; }
  if (item == "Moonraker IP") { loadIpEditor(); page = PAGE_IP_EDIT; return; }
  if (item == "Device Name") { loadTextEditor(); page = PAGE_TEXT_EDIT; return; }

  executeAction(item);
}

void DuenderUI::executeAction(const String& item) {
  String g = "";
  if (item == "Pause") g = "PAUSE";
  else if (item == "Resume") g = "RESUME";
  else if (item == "Cancel") g = "CANCEL_PRINT";
  else if (item == "Home All") g = "G28";
  else if (item == "Home X") g = "G28 X";
  else if (item == "Home Y") g = "G28 Y";
  else if (item == "Home Z") g = "G28 Z";
  else if (item == "Disable Motors") g = "M84";
  else if (item == "Park Head") g = "PARK";
  else if (item == "Preheat PLA") g = "M104 S215\nM140 S60";
  else if (item == "Preheat PETG") g = "M104 S240\nM140 S75";
  else if (item == "Cooldown") g = "M104 S0\nM140 S0\nM107";
  else if (item == "Reprint Current") {
    if (ps->file.length() == 0) { setNotice("No file"); return; }
    g = String("SDCARD_PRINT_FILE FILENAME=\"") + ps->file + "\"";
  }
  else if (item == "Restart Klipper") g = "RESTART";
  else if (item == "Restart Firmware") g = "FIRMWARE_RESTART";
  else if (item == "Restart Moonraker") g = "M117 Restart Moonraker from DuenderOS";
  else if (item == "Emergency Stop") {
    bool ok = Moonraker.emergencyStop();
    setNotice(ok ? "ESTOP sent" : "ESTOP fail");
    return;
  }
  else if (item == "Use First Found") {
    startPrinterScanPage(true);
    return;
  }
  else if (item == "WiFi Portal") {
    setNotice("Portal start");
    Network.startConfigPortal();
    setNotice("Portal done");
    return;
  }
  else if (item == "Reset WiFi") {
    setNotice("Reset/reboot");
    delay(500);
    Network.resetWiFiAndSettings();
    return;
  }
  else if (item == "Buzzer On/Off") {
    bool on = Network.toggleBeeper();
    setNotice(on ? "Buzzer On" : "Buzzer Off");
    return;
  }
  else if (item == "Beeper Test") {
    beep(2200, 80);
    setNotice("Beep");
    return;
  }
  else if (item == "Invert Encoder") {
    setNotice("Edit config.h");
    return;
  }
  else if (item == "Calibration" || item == "Parts" || item == "Dragonburner") {
    setNotice("Files soon");
    return;
  }
  else {
    // Treat remaining entries as Klipper macro names.
    g = item;
  }

  bool ok = Moonraker.sendGcode(g);
  setNotice(ok ? "Sent: " + item : "Send failed");
}

void DuenderUI::drawHeader(const char* title) {
  u->setDrawColor(1);
  u->drawBox(0, 0, 128, 11);
  u->setDrawColor(0);
  u->setFont(u8g2_font_6x12_tr);

  String right = title ? String(title) : String("");
  int rightWidth = u->getStrWidth(right.c_str());

  // The left label is configurable in config.h. Trim it only when needed so
  // long printer names cannot overlap the live state/page title on the right.
  String left = Network.getDeviceName();
  const int maxLeftWidth = max(0, 120 - rightWidth);
  while (left.length() > 0 && u->getStrWidth(left.c_str()) > maxLeftWidth) {
    left.remove(left.length() - 1);
  }

  u->drawStr(2, 9, left.c_str());
  u->drawStr(126 - rightWidth, 9, right.c_str());
  u->setDrawColor(1);
}

void DuenderUI::drawFooter(const char* hint) {
  u->drawHLine(0, 53, 128);
  u->setFont(u8g2_font_5x7_tr);
  if (notice.length() && millis() < noticeUntil) {
    u->drawStr(1, 63, clip(notice, 25).c_str());
  } else if (hint) {
    u->drawStr(1, 63, hint);
  } else {
    u->drawStr(1, 63, "Press=OK  Hold=Back");
  }
}

void DuenderUI::drawMenu() {
  int count = 0;
  const char* const* items = menuItems(menu, count);
  u->clearBuffer();
  drawHeader(menuTitle(menu));
  u->setFont(u8g2_font_6x12_tr);

  for (int row = 0; row < 3; row++) {
    int idx = topIndex + row;
    if (idx >= count) break;
    int y = 24 + row * 14;
    if (idx == selected) {
      u->drawBox(0, y - 11, 128, 13);
      u->setDrawColor(0);
      u->drawStr(2, y, "> ");
      u->drawStr(14, y, clip(items[idx], 18).c_str());
      u->setDrawColor(1);
    } else {
      u->drawStr(14, y, clip(items[idx], 18).c_str());
    }
  }

  char buf[28];
  snprintf(buf, sizeof(buf), "%d/%d", selected + 1, count);
  drawFooter(buf);
  u->sendBuffer();
}

void DuenderUI::drawProgress(int x, int y, int w, int h, int pct) {
  pct = constrain(pct, 0, 100);
  u->drawFrame(x, y, w, h);
  int fill = map(pct, 0, 100, 0, w - 2);
  if (fill > 0) u->drawBox(x + 1, y + 1, fill, h - 2);
}

void DuenderUI::drawStatus() {
  u->clearBuffer();

  const bool wifiOk = ps->wifiOk;
  const bool moonOk = ps->moonrakerOk;
  const bool printing = ps->state == "printing";
  const bool paused = ps->state == "paused";
  const bool macroBusy = ps->activityState.equalsIgnoreCase("Printing") && !printing && !paused;
  const bool activeMessage = macroBusy && ps->message.length() > 0;

  String title;
  if (!wifiOk) title = "NO WIFI";
  else if (!moonOk) title = "OFFLINE";
  else if (printing) title = "PRINTING";
  else if (paused) title = "PAUSED";
  else if (macroBusy) title = "ACTIVE";
  else title = "STANDBY";

  drawHeader(title.c_str());

  // Compact temperature cards with small monochrome icons.
  u->setFont(u8g2_font_5x7_tr);

  // Hotend icon.
  u->drawFrame(1, 15, 5, 8);
  u->drawPixel(2, 23);
  u->drawPixel(4, 23);
  u->drawHLine(0, 24, 7);

  // Bed icon.
  u->drawHLine(66, 22, 10);
  u->drawLine(68, 19, 70, 17);
  u->drawLine(72, 19, 74, 17);

  char tempLine[24];
  snprintf(tempLine, sizeof(tempLine), "H %3.0f/%3.0f", ps->hotendTemp, ps->hotendTarget);
  u->drawStr(9, 22, tempLine);
  snprintf(tempLine, sizeof(tempLine), "B %3.0f/%3.0f", ps->bedTemp, ps->bedTarget);
  u->drawStr(78, 22, tempLine);

  if (!wifiOk || !moonOk) {
    u->setFont(u8g2_font_6x12_tr);
    String warning = !wifiOk ? "WIFI DISCONNECTED" : "MOONRAKER OFFLINE";
    int warningW = u->getStrWidth(warning.c_str());
    u->drawStr(max(0, (128 - warningW) / 2), 38, warning.c_str());

    u->setFont(u8g2_font_5x7_tr);
    String endpoint = Network.getMoonrakerHost() + ":" + String(Network.getMoonrakerPort());
    endpoint = clip(endpoint, 25);
    int endpointW = u->getStrWidth(endpoint.c_str());
    u->drawStr(max(0, (128 - endpointW) / 2), 49, endpoint.c_str());
  } else if (printing || paused) {
    String file = ps->file.length() ? ps->file : "Print job";
    u->setFont(u8g2_font_5x7_tr);
    u->drawStr(1, 33, clip(file, 25).c_str());

    drawProgress(1, 38, 105, 10, ps->progress);
    char pct[8];
    snprintf(pct, sizeof(pct), "%d%%", ps->progress);
    u->setFont(u8g2_font_6x12_tr);
    u->drawStr(108, 48, pct);
  } else if (macroBusy) {
    // Klipper macros can populate this area through M117/display_status.message.
    u->setFont(u8g2_font_6x12_tr);
    String msg = clip(ps->message.length() ? ps->message : "RUNNING", 20);
    int msgW = u->getStrWidth(msg.c_str());
    u->drawStr(max(0, (128 - msgW) / 2), 39, msg.c_str());

    // Small animated activity indicator.
    uint8_t frame = (millis() / 250) % 4;
    for (uint8_t i = 0; i < 3; i++) {
      if (i < frame) u->drawDisc(55 + i * 9, 47, 2);
      else u->drawCircle(55 + i * 9, 47, 2);
    }
  } else {
    // Idle home card. No empty progress bar when nothing is printing.
    u->setFont(u8g2_font_7x14B_tr);
    const char* ready = "READY";
    int readyW = u->getStrWidth(ready);
    u->drawStr((128 - readyW) / 2, 40, ready);

    u->setFont(u8g2_font_5x7_tr);
    const char* sub = "PRESS KNOB FOR MENU";
    int subW = u->getStrWidth(sub);
    u->drawStr((128 - subW) / 2, 49, sub);
  }

  // Footer: connection health instead of permanently showing the IP address.
  u->drawHLine(0, 53, 128);
  u->setFont(u8g2_font_5x7_tr);

  if (notice.length() && millis() < noticeUntil) {
    u->drawStr(1, 63, clip(notice, 25).c_str());
  } else {
    // Wi-Fi strength bars.
    int rssi = WiFi.RSSI();
    int bars = 0;
    if (wifiOk) {
      if (rssi > -55) bars = 4;
      else if (rssi > -65) bars = 3;
      else if (rssi > -75) bars = 2;
      else bars = 1;
    }
    u->drawStr(1, 63, "WIFI");
    for (int i = 0; i < 4; i++) {
      int h = 2 + i * 2;
      if (i < bars) u->drawBox(25 + i * 4, 63 - h, 3, h);
      else u->drawFrame(25 + i * 4, 63 - h, 3, h);
    }

    u->drawStr(48, 63, "MR");
    if (moonOk) u->drawDisc(64, 60, 3);
    else u->drawCircle(64, 60, 3);

    String stateText = printing ? "PRINT" : paused ? "PAUSE" : activeMessage ? "BUSY" : "READY";
    int stateW = u->getStrWidth(stateText.c_str());
    u->drawStr(127 - stateW, 63, stateText.c_str());
  }

  u->sendBuffer();
}

void DuenderUI::drawPrintStatus() {
  u->clearBuffer();
  drawHeader("PRINT");
  u->setFont(u8g2_font_6x12_tr);
  u->drawStr(0, 23, clip(ps->file.length() ? ps->file : "No active file", 21).c_str());
  u->drawStr(0, 36, clip(ps->message.length() ? ps->message : ps->state, 21).c_str());
  drawProgress(0, 42, 100, 9, ps->progress);
  char b[12]; snprintf(b, sizeof(b), "%d%%", ps->progress); u->drawStr(104, 51, b);
  drawFooter();
  u->sendBuffer();
}

void DuenderUI::drawMotion() {
  u->clearBuffer();
  drawHeader("MOTION");
  u->setFont(u8g2_font_6x12_tr);
  char b[32];
  snprintf(b, sizeof(b), "X %7.2f", ps->x); u->drawStr(0, 23, b);
  snprintf(b, sizeof(b), "Y %7.2f", ps->y); u->drawStr(0, 35, b);
  snprintf(b, sizeof(b), "Z %7.2f", ps->z); u->drawStr(0, 47, b);
  snprintf(b, sizeof(b), "Speed %.0f%%", ps->speedFactor);
  drawFooter(b);
  u->sendBuffer();
}

void DuenderUI::drawTemps() {
  u->clearBuffer();
  drawHeader("TEMPS");
  u->setFont(u8g2_font_6x12_tr);
  char b[32];
  snprintf(b, sizeof(b), "Hotend %3.0f / %3.0f", ps->hotendTemp, ps->hotendTarget); u->drawStr(0, 23, b);
  snprintf(b, sizeof(b), "Bed    %3.0f / %3.0f", ps->bedTemp, ps->bedTarget); u->drawStr(0, 35, b);
  snprintf(b, sizeof(b), "Fan    %3.0f%%", ps->fanPercent); u->drawStr(0, 47, b);
  drawFooter();
  u->sendBuffer();
}

void DuenderUI::drawSystem() {
  u->clearBuffer();
  drawHeader("SYSTEM");
  u->setFont(u8g2_font_6x12_tr);
  u->drawStr(0, 23, ps->wifiOk ? "WiFi      OK" : "WiFi      FAIL");
  u->drawStr(0, 35, ps->moonrakerOk ? "Moonraker OK" : "Moonraker FAIL");
  u->drawStr(0, 47, (String("IP ") + Network.localIP()).c_str());
  drawFooter("DuenderOS v" DUENDER_VERSION);
  u->sendBuffer();
}

void DuenderUI::drawAbout() {
  u->clearBuffer();
  drawHeader("ABOUT");
  u->setFont(u8g2_font_6x12_tr);
  u->drawStr(0, 22, "DuenderOS v" DUENDER_VERSION);
  u->drawStr(0, 34, "Universal Klipper");
  u->drawStr(0, 46, "By Stones");
  drawFooter(Network.getMoonrakerHost().c_str());
  u->sendBuffer();
}

void DuenderUI::loadIpEditor() {
  String host = Network.getMoonrakerHost();
  int parts[4] = {10, 0, 0, 154};
  sscanf(host.c_str(), "%d.%d.%d.%d", &parts[0], &parts[1], &parts[2], &parts[3]);
  for (int i = 0; i < 4; i++) ipEdit[i] = constrain(parts[i], 0, 255);
  ipCursor = 0;
}

void DuenderUI::saveIpEditor() {
  String host = String(ipEdit[0]) + "." + String(ipEdit[1]) + "." + String(ipEdit[2]) + "." + String(ipEdit[3]);
  Network.setMoonrakerHost(host);
  setNotice("Saved " + host);
  page = PAGE_MENU;
}

void DuenderUI::drawIpEditor() {
  u->clearBuffer();
  drawHeader("MOON IP");
  u->setFont(u8g2_font_6x12_tr);
  u->drawStr(0, 23, "Moonraker IP");

  char buf[24];
  snprintf(buf, sizeof(buf), "%03d.%03d.%03d.%03d", ipEdit[0], ipEdit[1], ipEdit[2], ipEdit[3]);
  u->setFont(u8g2_font_7x14B_tr);
  u->drawStr(0, 42, buf);

  int x = ipCursor * 28 + 7;
  u->drawHLine(x, 45, 14);
  drawFooter("Turn=Edit Press=Next");
  u->sendBuffer();
}


int DuenderUI::textCharIndex(char c) {
  static const char CHARSET[] = " ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_.";
  for (int i = 0; CHARSET[i] != '\0'; i++) {
    if (CHARSET[i] == c) return i;
  }
  return 0;
}

void DuenderUI::loadTextEditor() {
  String name = Network.getDeviceName();
  memset(textEdit, ' ', TEXT_EDIT_MAX);
  textEdit[TEXT_EDIT_MAX] = '\0';
  for (uint8_t i = 0; i < TEXT_EDIT_MAX && i < name.length(); i++) {
    textEdit[i] = name[i];
  }
  textCursor = 0;
}

void DuenderUI::saveTextEditor() {
  String name(textEdit);
  name.trim();
  if (name.length() == 0) name = DEVICE_DISPLAY_NAME;
  Network.setDeviceName(name);
  setNotice("Saved: " + name);
  page = PAGE_MENU;
}

void DuenderUI::drawTextEditor() {
  u->clearBuffer();
  drawHeader("NAME");

  u->setFont(u8g2_font_5x7_tr);
  u->drawStr(1, 20, "DEVICE DISPLAY NAME");

  // Draw the 16-character editor in two groups for readability.
  u->setFont(u8g2_font_6x12_tr);
  char first[9];
  char second[9];
  memcpy(first, textEdit, 8); first[8] = '\0';
  memcpy(second, textEdit + 8, 8); second[8] = '\0';
  u->drawStr(8, 35, first);
  u->drawStr(72, 35, second);

  int cursorX = textCursor < 8 ? 8 + textCursor * 6 : 72 + (textCursor - 8) * 6;
  u->drawHLine(cursorX, 39, 5);

  u->setFont(u8g2_font_5x7_tr);
  char pos[28];
  snprintf(pos, sizeof(pos), "CHAR %d/%d  HOLD=SAVE", textCursor + 1, TEXT_EDIT_MAX);
  u->drawStr(1, 49, pos);
  drawFooter("Turn=Char  OK=Next");
  u->sendBuffer();
}

void DuenderUI::startPrinterScanPage(bool quick) {
  foundPrinterCount = 0;
  foundPrinterSelected = 0;
  foundPrinterTop = 0;
  if (quick) Network.startQuickScan();
  else Network.startFullScan();
  page = PAGE_PRINTER_SCAN;
  setNotice(quick ? "Quick scan" : "Full scan");
}

void DuenderUI::drawPrinterScan() {
  // Scanning runs in a background task; this page only reads progress/results.
  foundPrinterCount = min(Network.getFoundPrinterCount(), DUENDER_MAX_PRINTERS);
  for (int i = 0; i < foundPrinterCount; i++) foundPrinters[i] = Network.getFoundPrinter(i);

  if (!Network.isPrinterScanActive()) {
    page = PAGE_PRINTER_SELECT;
    setNotice(foundPrinterCount ? String("Found ") + String(foundPrinterCount) : "None found");
    return;
  }

  u->clearBuffer();
  drawHeader("SCAN");
  u->setFont(u8g2_font_6x12_tr);
  u->drawStr(0, 24, "Finding Klipper...");
  char b[32];
  snprintf(b, sizeof(b), "Found: %d", foundPrinterCount);
  u->drawStr(0, 37, b);
  snprintf(b, sizeof(b), ".%d:%u", Network.getScanCurrentHost(), Network.getScanCurrentPort());
  u->drawStr(66, 37, b);
  drawProgress(0, 44, 128, 8, Network.getScanProgress());
  drawFooter("Hold = stop");
  u->sendBuffer();
}

void DuenderUI::selectFoundPrinter() {
  if (foundPrinterCount <= 0) {
    page = PAGE_MENU;
    return;
  }
  FoundPrinter& fp = foundPrinters[foundPrinterSelected];
  Network.setMoonrakerHost(fp.host);
  Network.setMoonrakerPort(fp.port);
  setNotice(String("Saved ") + fp.host + ":" + String(fp.port));
  Moonraker.poll();
  page = PAGE_STATUS;
}

void DuenderUI::drawPrinterSelect() {
  u->clearBuffer();
  drawHeader("PRINTERS");
  u->setFont(u8g2_font_6x12_tr);

  if (foundPrinterCount <= 0) {
    u->drawStr(0, 28, "No printers found");
    u->drawStr(0, 42, "Hold = back");
    drawFooter("Try manual IP");
    u->sendBuffer();
    return;
  }

  for (int row = 0; row < 3; row++) {
    int idx = foundPrinterTop + row;
    if (idx >= foundPrinterCount) break;
    int y = 24 + row * 14;
    String label = foundPrinters[idx].name;
    String suffix = ":" + String(foundPrinters[idx].port);
    if (label == foundPrinters[idx].host) {
      label = foundPrinters[idx].host.substring(foundPrinters[idx].host.lastIndexOf('.') + 1) + suffix;
    } else {
      label = label + suffix;
    }

    if (idx == foundPrinterSelected) {
      u->drawBox(0, y - 11, 128, 13);
      u->setDrawColor(0);
      u->drawStr(2, y, ">");
      u->drawStr(14, y, clip(label, 18).c_str());
      u->setDrawColor(1);
    } else {
      u->drawStr(14, y, clip(label, 18).c_str());
    }
  }

  char buf[28];
  snprintf(buf, sizeof(buf), "%d/%d OK=Save", foundPrinterSelected + 1, foundPrinterCount);
  drawFooter(buf);
  u->sendBuffer();
}

void DuenderUI::beep(uint16_t freq, uint16_t ms) {
  if (Network.isBeeperEnabled()) tone(BEEPER_PIN, freq, ms);
}

void DuenderUI::setNotice(const String& s) {
  notice = s;
  noticeUntil = millis() + 1600;
  beep(1800, 35);
}

bool DuenderUI::isBusy() {
  // Busy means do not do Moonraker HTTP polling. Keep encoder/UI responsive.
  return page == PAGE_PRINTER_SCAN || page == PAGE_IP_EDIT || page == PAGE_TEXT_EDIT || page == PAGE_MENU || page == PAGE_PRINTER_SELECT;
}

unsigned long DuenderUI::lastInputMs() {
  return lastInput;
}

String DuenderUI::clip(String s, uint8_t maxChars) {
  if (s.length() <= maxChars) return s;
  return s.substring(0, maxChars - 1) + "~";
}
