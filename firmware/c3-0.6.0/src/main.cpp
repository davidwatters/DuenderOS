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
unsigned long lastWifiTry = 0;

void safeBeep(uint16_t freq, uint16_t ms) {
  if (Network.isBeeperEnabled()) tone(BEEPER_PIN, freq, ms);
}

void handleSerialCommand(String cmd) {
  cmd.trim();
  if (!cmd.length()) return;

  String upper = cmd;
  upper.toUpperCase();

  if (upper == "HELP") {
    Serial.println("Commands: HELP NEXT QUICKSCAN FULLSCAN LIST PORTAL RESETWIFI NAME <text> HOME PAUSE RESUME CANCEL COOL PLA BELT MESH REPRINT BEEPON BEEPOFF GCODE <cmd>");
  } else if (upper == "NEXT") UI.handleEncoder(1);
  else if (upper == "QUICKSCAN") Network.startQuickScan();
  else if (upper == "FULLSCAN" || upper == "SCAN") Network.startFullScan();
  else if (upper == "LIST") {
    int n = Network.getFoundPrinterCount();
    Serial.printf("Found %d printer instances\n", n);
    for (int i = 0; i < n; i++) {
      FoundPrinter fp = Network.getFoundPrinter(i);
      Serial.printf("%d: %s %s:%u\n", i + 1, fp.name.c_str(), fp.host.c_str(), fp.port);
    }
  } else if (upper == "PORTAL") Network.startConfigPortal();
  else if (upper.startsWith("NAME ")) { Network.setDeviceName(cmd.substring(5)); Serial.println("Name saved"); }
  else if (upper == "RESETWIFI") Network.resetWiFiAndSettings();
  else if (upper == "BEEPON") Network.setBeeperEnabled(true);
  else if (upper == "BEEPOFF") Network.setBeeperEnabled(false);
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

  UI.showBoot("Power On", 8); delay(150);
  UI.showBoot("Encoder", 20); Encoder.begin(); delay(100);

  UI.showBoot("WiFi Setup", 35);
  Network.begin();
  Printer.wifiOk = Network.isWifiConnected();

  UI.showBoot(Printer.wifiOk ? "WiFi OK" : "WiFi Failed", Printer.wifiOk ? 55 : 45);
  delay(150);

  Moonraker.begin(&Printer);
  UI.showBoot("Background Net", 80); delay(150);
  UI.showBoot("Ready", 100); delay(200);
  safeBeep(2000, 60);
}

void loop() {
  // Input and rendering always stay in the Arduino/UI loop.
  Encoder.update();
  int d = Encoder.getDelta();
  if (d) UI.handleEncoder(d);

  ButtonEvent ev = Encoder.getButtonEvent();
  if (ev != BTN_NONE) UI.handleButton(ev);

  // Apply completed network snapshots without waiting for HTTP.
  Moonraker.update();

  if (Serial.available()) handleSerialCommand(Serial.readStringUntil('\n'));

  if (WiFi.status() != WL_CONNECTED && millis() - lastWifiTry > 8000) {
    lastWifiTry = millis();
    WiFi.reconnect();
  }

  UI.update();
  delay(1);
}
