#pragma once
#include <Arduino.h>
#include <U8g2lib.h>
#include "printer_state.h"
#include "encoder.h"
#include "network_manager.h"

class DuenderUI {
public:
  void begin(U8G2_ST7920_128X64_F_SW_SPI* display, PrinterState* state);
  void showBoot(const char* msg, int percent);
  void update();
  void handleEncoder(int delta);
  void handleButton(ButtonEvent ev);

private:
  U8G2_ST7920_128X64_F_SW_SPI* u = nullptr;
  PrinterState* ps = nullptr;

  enum MenuId : uint8_t { MENU_MAIN, MENU_PRINT, MENU_MOTION, MENU_TEMPS, MENU_MACROS, MENU_FILES, MENU_SYSTEM, MENU_CONTROLS, MENU_SETTINGS, MENU_NETWORK };
  enum PageId : uint8_t { PAGE_MENU, PAGE_STATUS, PAGE_PRINT_STATUS, PAGE_MOTION, PAGE_TEMPS, PAGE_SYSTEM, PAGE_IP_EDIT, PAGE_PRINTER_SELECT };

  PageId page = PAGE_STATUS;
  MenuId menu = MENU_MAIN;
  MenuId stack[8];
  uint8_t stackDepth = 0;
  int selected = 0;
  int topIndex = 0;
  unsigned long lastDraw = 0;
  String notice = "";
  unsigned long noticeUntil = 0;

  uint8_t ipEdit[4] = {10, 0, 0, 154};
  uint8_t ipCursor = 0;

  FoundPrinter foundPrinters[DUENDER_MAX_PRINTERS];
  int foundPrinterCount = 0;
  int foundPrinterSelected = 0;
  int foundPrinterTop = 0;

  void drawHeader(const char* title);
  void drawFooter(const char* hint = nullptr);
  void drawMenu();
  void drawStatus();
  void drawPrintStatus();
  void drawMotion();
  void drawTemps();
  void drawSystem();
  void drawIpEditor();
  void drawPrinterSelect();
  void scanForPrintersPage();
  void selectFoundPrinter();
  void drawProgress(int x, int y, int w, int h, int pct);
  void drawValueRow(int y, const char* left, const String& right);

  const char* const* menuItems(MenuId id, int& count);
  const char* menuTitle(MenuId id);
  void enterMenu(MenuId id);
  void goBack();
  void selectCurrent();
  void executeAction(const String& item);
  void setNotice(const String& s);
  String clip(String s, uint8_t maxChars);
  void loadIpEditor();
  void saveIpEditor();
};

extern DuenderUI UI;
