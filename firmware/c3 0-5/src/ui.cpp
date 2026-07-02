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
static const char* SETTING_ITEMS[] = {"Network", "Moonraker IP", "Auto Scan", "WiFi Portal", "Reset WiFi", "Status Page", "Beeper Test", "About", "Back"};
static const char* NETWORK_ITEMS[] = {"Find Printers", "Moonraker IP", "Auto Scan", "WiFi Portal", "Reset WiFi", "Back"};

void DuenderUI::begin(U8G2_ST7920_128X64_F_SW_SPI* display, PrinterState* state) {
  u = display;
  ps = state;
  page = PAGE_STATUS;
}

void DuenderUI::showBoot(const char* msg, int percent) {
  u->clearBuffer();
  u->drawXBMP(28, 0, DUENDER_LOGO_W, DUENDER_LOGO_H, duenderLogo);
  u->setFont(u8g2_font_6x12_tr);
  int w = u->getStrWidth("DUENDER OS");
  u->drawStr((128 - w) / 2, 45, "DUENDER OS");
  w = u->getStrWidth(msg);
  u->drawStr((128 - w) / 2, 56, msg);
  drawProgress(18, 59, 92, 5, percent);
  u->sendBuffer();
}

void DuenderUI::update() {
  if (millis() - lastDraw < 90) return;
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
    case PAGE_IP_EDIT: drawIpEditor(); break;
    case PAGE_PRINTER_SELECT: drawPrinterSelect(); break;
  }
}

void DuenderUI::handleEncoder(int delta) {
  if (delta == 0) return;
  if (page == PAGE_IP_EDIT) {
    int v = (int)ipEdit[ipCursor] + delta;
    if (v < 0) v = 255;
    if (v > 255) v = 0;
    ipEdit[ipCursor] = (uint8_t)v;
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

  if (ev == BTN_DOUBLE) {
    page = PAGE_STATUS;
    menu = MENU_MAIN;
    selected = 0;
    topIndex = 0;
    setNotice("Home");
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
  if (item == "System Status" || item == "About") { page = PAGE_SYSTEM; return; }
  if (item == "Network") { enterMenu(MENU_NETWORK); return; }
  if (item == "Find Printers") { scanForPrintersPage(); return; }
  if (item == "Moonraker IP") { loadIpEditor(); page = PAGE_IP_EDIT; return; }

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
  else if (item == "Auto Scan") {
    setNotice("Scanning...");
    bool ok = Network.autoScanMoonraker();
    setNotice(ok ? String("Found ") + Network.getMoonrakerHost() : "Not found");
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
  else if (item == "Beeper Test") {
    tone(BEEPER_PIN, 2200, 80);
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
  u->drawStr(2, 9, "DUENDER");
  int w = u->getStrWidth(title);
  u->drawStr(126 - w, 9, title);
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
  String title = ps->state; title.toUpperCase();
  drawHeader(title.c_str());
  u->setFont(u8g2_font_6x12_tr);

  char line[32];
  snprintf(line, sizeof(line), "H %3.0f/%3.0f  B %3.0f/%3.0f", ps->hotendTemp, ps->hotendTarget, ps->bedTemp, ps->bedTarget);
  u->drawStr(0, 23, line);

  String msg = ps->message.length() ? ps->message : ps->file;
  if (!ps->moonrakerOk) msg = ps->error;
  if (msg.length() == 0) msg = ps->moonrakerOk ? "Ready / Idle" : "Connecting...";
  u->drawStr(0, 36, clip(msg, 21).c_str());

  drawProgress(0, 42, 100, 9, ps->progress);
  snprintf(line, sizeof(line), "%d%%", ps->progress);
  u->drawStr(104, 51, line);

  drawFooter(Network.localIP().c_str());
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


void DuenderUI::scanForPrintersPage() {
  u->clearBuffer();
  drawHeader("SCAN");
  u->setFont(u8g2_font_6x12_tr);
  u->drawStr(0, 26, "Scanning network...");
  u->drawStr(0, 40, "This can take a bit");
  drawProgress(0, 48, 128, 5, 25);
  u->sendBuffer();

  foundPrinterCount = Network.scanPrinters(foundPrinters, DUENDER_MAX_PRINTERS);
  foundPrinterSelected = 0;
  foundPrinterTop = 0;
  page = PAGE_PRINTER_SELECT;
  setNotice(foundPrinterCount ? String("Found ") + String(foundPrinterCount) : "None found");
}

void DuenderUI::selectFoundPrinter() {
  if (foundPrinterCount <= 0) {
    page = PAGE_MENU;
    return;
  }
  FoundPrinter& fp = foundPrinters[foundPrinterSelected];
  Network.setMoonrakerHost(fp.host);
  Network.setMoonrakerPort(fp.port);
  setNotice(String("Saved ") + fp.host);
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
    if (label == foundPrinters[idx].host) label = foundPrinters[idx].host;
    else label = label + " " + foundPrinters[idx].host.substring(foundPrinters[idx].host.lastIndexOf('.') + 1);

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

void DuenderUI::setNotice(const String& s) {
  notice = s;
  noticeUntil = millis() + 1600;
  tone(BEEPER_PIN, 1800, 35);
}

String DuenderUI::clip(String s, uint8_t maxChars) {
  if (s.length() <= maxChars) return s;
  return s.substring(0, maxChars - 1) + "~";
}
