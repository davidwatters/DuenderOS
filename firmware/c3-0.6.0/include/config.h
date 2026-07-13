#pragma once

// DuenderOS v0.6.0 runtime device-name editor build
// First boot creates captive portal SETUP_AP_NAME.

#define SETUP_AP_NAME "DuenderOS-Setup"

// Name shown in the boot screen and the left side of every page header.
// Change only this line to brand the display for another printer.
#define DEVICE_DISPLAY_NAME "DuenderOS"
#define SETUP_AP_PASSWORD "duenderos"

#define DEFAULT_MOONRAKER_HOST "0.0.0.0"
#define DEFAULT_MOONRAKER_PORT 7125

// LCD pins - Creality 12864 ST7920 serial mode
#define LCD_SCLK 5
#define LCD_MOSI 6
#define LCD_CS   4

// Creality 12864 EXP encoder pins to ESP32-C3
#define ENC_A_PIN   7
#define ENC_B_PIN   8

// Primary button pin. Some earlier builds used GPIO9, some used GPIO10.
// v0.5.3 can watch both so you do not have to rewire immediately.
#define ENC_BTN_PIN 10
#define ENC_BTN_ALT_PIN 9
#define USE_ENCODER_ALT_BUTTON 0

#define BEEPER_PIN 2

// More responsive default. If it moves 2 menu items per click, change to 4.
// If it still feels too slow, try 1.
#define ENCODER_STEPS_PER_DETENT 4
#define ENCODER_REVERSE 0

#define DUENDER_VERSION "0.6.0"

// Responsiveness tuning
#define MOONRAKER_POLL_INTERVAL_MS 5000
#define MOONRAKER_IDLE_AFTER_INPUT_MS 4000
#define MOONRAKER_CONNECT_TIMEOUT_MS 600
#define MOONRAKER_HTTP_TIMEOUT_MS 1200
#define MOONRAKER_GCODE_TIMEOUT_MS 1200

// Do not declare Moonraker offline after one transient timeout.
#define MOONRAKER_OFFLINE_FAILURES 3
#define MOONRAKER_STALE_GRACE_MS 20000
#define QUICK_SCAN_HTTP_TIMEOUT_MS 80
#define FULL_SCAN_HTTP_TIMEOUT_MS 55
#define UI_DRAW_INTERVAL_MS 70
