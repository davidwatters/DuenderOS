# DuenderOS v0.5.4

ESP32-C3 + Creality 12864 ST7920 Moonraker/Klipper controller.

## What changed in v0.5.4

- Added **Quick Scan** for Moonraker printers.
- Added **Full Scan** as a separate fallback option.
- Quick Scan checks saved/default/gateway/nearby/common IPs first instead of waiting through a full `/24` scan.
- Encoder now has **no acceleration** and only returns one menu move at a time.
- Default encoder steps changed to `4` to reduce skipping.
- Button handling changed to debounced polling for reliability.
- Alt button GPIO9 is disabled by default. Primary button is GPIO10.

## Pins

```txt
LCD CS   -> GPIO4
LCD CLK  -> GPIO5
LCD SID  -> GPIO6
ENC A    -> GPIO7
ENC B    -> GPIO8
BTN      -> GPIO10
BEEP     -> GPIO2 optional
```

## Menu paths

```txt
Settings > Network > Quick Scan
Settings > Network > Full Scan
Settings > Network > Moonraker IP
Settings > Buzzer On/Off
Settings > About
```

## First boot captive portal

```txt
SSID: DuenderOS-Setup
Password: duenderos
```

## If encoder still skips

In `include/config.h` try:

```cpp
#define ENCODER_STEPS_PER_DETENT 2
```

If it moves backwards:

```cpp
#define ENCODER_REVERSE 1
```

If your button is wired to GPIO9 instead of GPIO10:

```cpp
#define ENC_BTN_PIN 9
```

