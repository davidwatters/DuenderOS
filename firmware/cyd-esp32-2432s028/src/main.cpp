#include <Arduino.h>
#include <WiFi.h>
#include <TFT_eSPI.h>

#include "config.h"
#include "printer_state.h"
#include "moonraker.h"
#include "network_manager.h"
#include "ui.h"
#include "touch.h"

TFT_eSPI tft = TFT_eSPI();

PrinterState Printer;

unsigned long lastPoll = 0;
unsigned long lastWifiTry = 0;

void setup() {
    Serial.begin(115200);
    delay(250);

    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, HIGH);

    tft.init();
    tft.setRotation(1);

    UI.begin(&tft, &Printer);
    Touch.begin();

    UI.showBoot("Power On", 10);
    delay(300);

    UI.showBoot("WiFi Setup", 35);
    Network.begin();

    Printer.wifiOk = Network.isWifiConnected();

    UI.showBoot(
        Printer.wifiOk ? "WiFi OK" : "WiFi Failed",
        Printer.wifiOk ? 60 : 45
    );
    delay(300);

    Moonraker.begin(&Printer);

    if (Printer.wifiOk) {
        UI.showBoot("Connecting Moonraker", 80);

        Moonraker.poll();

        delay(300);
    }

    UI.showBoot("Ready", 100);
    delay(500);

    UI.forceRedraw();
    UI.update();
}

void loop() {
    if (Serial.available()) {
        String cmd = Serial.readStringUntil('\n');
        cmd.trim();

        if (cmd == "HOME") Moonraker.sendGcode("G28");
        else if (cmd == "PAUSE") Moonraker.sendGcode("PAUSE");
        else if (cmd == "RESUME") Moonraker.sendGcode("RESUME");
        else if (cmd == "CANCEL") Moonraker.sendGcode("CANCEL_PRINT");
    }

    if (WiFi.status() != WL_CONNECTED && millis() - lastWifiTry > 8000) {
        lastWifiTry = millis();
        WiFi.reconnect();
    }

    Printer.wifiOk = WiFi.status() == WL_CONNECTED;

    unsigned long now = millis();

    if (now - lastPoll >= MOONRAKER_POLL_INTERVAL_MS) {
        lastPoll = now;

        if (Moonraker.poll()) {
            UI.refreshData();
        }
    }

    TouchPoint tp = Touch.read();

    if (tp.pressed) {
        Serial.printf("Touch: %d,%d\n", tp.x, tp.y);
        UI.handleTouch(tp);
    }

    UI.update();
}