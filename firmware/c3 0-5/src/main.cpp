#include <Arduino.h>
#include <WiFi.h>
#include <U8g2lib.h>
#include "config.h"
#include "printer_state.h"
#include "encoder.h"
#include "moonraker.h"
#include "network_manager.h"
#include "ui.h"

U8G2_ST7920_128X64_F_SW_SPI u8g2(
  U8G2_R0,
  LCD_SCLK,
  LCD_MOSI,
  LCD_CS,
  U8X8_PIN_NONE
);

PrinterState Printer;
unsigned long lastPoll = 0;
unsigned long lastWifiTry = 0;

void handleSerialCommand(String cmd) {
  cmd.trim();
  if (!cmd.length()) return;

  String upper = cmd;
  upper.toUpperCase();

  if (upper == "HELP") {
    Serial.println("Commands: HELP NEXT SCAN PORTAL RESETWIFI HOME PAUSE RESUME CANCEL COOL PLA BELT MESH REPRINT GCODE <cmd>");
  } else if (upper == "NEXT") UI.handleEncoder(1);
  else if (upper == "SCAN") {
    Serial.println("Scanning for Moonraker...");
    bool ok = Network.autoScanMoonraker();
    Serial.println(ok ? (String("Found: ") + Network.getMoonrakerHost()) : "Not found");
  } else if (upper == "PORTAL") Network.startConfigPortal();
  else if (upper == "RESETWIFI") Network.resetWiFiAndSettings();
  else if (upper == "HOME") Moonraker.sendGcode("G28");
  else if (upper == "PAUSE") Moonraker.sendGcode("PAUSE");
  else if (upper == "RESUME") Moonraker.sendGcode("RESUME");
  else if (upper == "CANCEL") Moonraker.sendGcode("CANCEL_PRINT");
  else if (upper == "COOL") Moonraker.sendGcode("M104 S0\nM140 S0\nM107");
  else if (upper == "PLA") Moonraker.sendGcode("M104 S215\nM140 S60");
  else if (upper == "BELT") Moonraker.sendGcode("BELT_TEST");
  else if (upper == "MESH") Moonraker.sendGcode("BED_MESH");
  else if (upper == "REPRINT") {
    if (Printer.file.length()) Moonraker.sendGcode(String("SDCARD_PRINT_FILE FILENAME=\"") + Printer.file + "\"");
  } else if (upper.startsWith("GCODE ")) {
    Moonraker.sendGcode(cmd.substring(6));
  }
}

void setup() {
  Serial.begin(115200);
  delay(250);
  pinMode(BEEPER_PIN, OUTPUT);

  u8g2.begin();
  UI.begin(&u8g2, &Printer);

  UI.showBoot("Power On", 8); delay(300);
  UI.showBoot("Encoder", 20); Encoder.begin(); delay(200);

  UI.showBoot("WiFi Setup", 35);
  Network.begin();
  Printer.wifiOk = Network.isWifiConnected();

  if (!Printer.wifiOk) {
    UI.showBoot("WiFi Failed", 45);
    delay(800);
  } else {
    UI.showBoot("WiFi OK", 55);
    delay(250);
  }

  Moonraker.begin(&Printer);

  if (Printer.wifiOk) {
    UI.showBoot("Moonraker", 70);
    if (!Moonraker.poll()) {
      UI.showBoot("Auto Scan", 78);
      bool found = Network.autoScanMoonraker();
      UI.showBoot(found ? "Found" : "Use Settings", found ? 92 : 82);
      if (found) Moonraker.poll();
      delay(550);
    }
  }

  UI.showBoot("Ready", 100); delay(250);
  tone(BEEPER_PIN, 2000, 60);
}

void loop() {
  Encoder.update();
  int d = Encoder.getDelta();
  if (d) UI.handleEncoder(d);
  ButtonEvent ev = Encoder.getButtonEvent();
  if (ev != BTN_NONE) UI.handleButton(ev);

  if (Serial.available()) handleSerialCommand(Serial.readStringUntil('\n'));

  if (WiFi.status() != WL_CONNECTED && millis() - lastWifiTry > 8000) {
    lastWifiTry = millis();
    WiFi.reconnect();
  }

  if (millis() - lastPoll >= 1000) {
    lastPoll = millis();
    Moonraker.poll();
  }

  UI.update();
}
