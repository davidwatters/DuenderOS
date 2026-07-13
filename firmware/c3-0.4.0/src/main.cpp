#include <Arduino.h>
#include <WiFi.h>
#include <U8g2lib.h>
#include "config.h"
#include "printer_state.h"
#include "encoder.h"
#include "moonraker.h"
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
  cmd.toUpperCase();

  if (cmd == "HELP") {
    Serial.println("Commands: HELP NEXT HOME PAUSE RESUME CANCEL COOL PLA BELT MESH REPRINT GCODE <cmd>");
  } else if (cmd == "NEXT") {
    UI.handleEncoder(1);
  } else if (cmd == "HOME") Moonraker.sendGcode("G28");
  else if (cmd == "PAUSE") Moonraker.sendGcode("PAUSE");
  else if (cmd == "RESUME") Moonraker.sendGcode("RESUME");
  else if (cmd == "CANCEL") Moonraker.sendGcode("CANCEL_PRINT");
  else if (cmd == "COOL") Moonraker.sendGcode("M104 S0\nM140 S0\nM107");
  else if (cmd == "PLA") Moonraker.sendGcode("M104 S215\nM140 S60");
  else if (cmd == "BELT") Moonraker.sendGcode("BELT_TEST");
  else if (cmd == "MESH") Moonraker.sendGcode("BED_MESH");
  else if (cmd == "REPRINT") {
    if (Printer.file.length()) Moonraker.sendGcode(String("SDCARD_PRINT_FILE FILENAME=\"") + Printer.file + "\"");
  } else if (cmd.startsWith("GCODE ")) {
    Moonraker.sendGcode(cmd.substring(6));
  }
}

void connectWifi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
}

void setup() {
  Serial.begin(115200);
  delay(250);
  pinMode(BEEPER_PIN, OUTPUT);

  u8g2.begin();
  UI.begin(&u8g2, &Printer);

  UI.showBoot("Power On", 10); delay(350);
  UI.showBoot("Encoder", 25); Encoder.begin(); delay(250);
  UI.showBoot("WiFi", 45); connectWifi();

  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 9000) {
    UI.showBoot("Connecting WiFi", 45 + ((millis() - start) / 220) % 20);
    delay(120);
  }

  Printer.wifiOk = WiFi.status() == WL_CONNECTED;
  UI.showBoot(Printer.wifiOk ? "Moonraker" : "WiFi Failed", Printer.wifiOk ? 75 : 60);
  Moonraker.begin(&Printer);
  if (Printer.wifiOk) Moonraker.poll();
  delay(450);
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
    connectWifi();
  }

  if (millis() - lastPoll >= 1000) {
    lastPoll = millis();
    Moonraker.poll();
  }

  UI.update();
}
