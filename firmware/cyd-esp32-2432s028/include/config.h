#pragma once

// DuenderOS v0.5.4 quick scan and input polish build
// First boot creates captive portal SETUP_AP_NAME.

#define SETUP_AP_NAME "DuenderOS-Setup"
#define SETUP_AP_PASSWORD "duenderos"

#define DEFAULT_MOONRAKER_HOST "10.0.0.154"
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

#define DUENDER_VERSION "0.5.4"

// Responsiveness tuning
#define MOONRAKER_POLL_INTERVAL_MS 5000
#define MOONRAKER_IDLE_AFTER_INPUT_MS 4000
#define MOONRAKER_HTTP_TIMEOUT_MS 5000
#define MOONRAKER_GCODE_TIMEOUT_MS 5000
#define QUICK_SCAN_HTTP_TIMEOUT_MS 45
#define FULL_SCAN_HTTP_TIMEOUT_MS 35
#define UI_DRAW_INTERVAL_MS 70
