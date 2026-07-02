#pragma once

// Change these only
#define WIFI_SSID "Watters"
#define WIFI_PASS "Dwatters0707!"

#define MOONRAKER_HOST "10.0.0.154"
#define MOONRAKER_PORT 7125

// LCD pins - Creality 12864 ST7920 serial mode
#define LCD_SCLK 5
#define LCD_MOSI 6
#define LCD_CS   4

// Creality 12864 EXP encoder pins to ESP32-C3
#define ENC_A_PIN   7
#define ENC_B_PIN   8
#define ENC_BTN_PIN 10

// Optional beeper on Creality LCD EXP1 BEEP pin
#define BEEPER_PIN 2

// Most EC11 encoders give 4 state transitions per physical click.
// If it moves 2 per click, change this to 8. If too slow, change to 2.
#define ENCODER_STEPS_PER_DETENT 4
#define ENCODER_REVERSE 0
