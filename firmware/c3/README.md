# DuenderOS PlatformIO v0.4

Major cleanup build.

## Pins

LCD ST7920 serial mode:

- LCD CS -> GPIO4
- LCD EN / SCLK -> GPIO5
- LCD D7 / SID / MOSI -> GPIO6

Creality 12864 encoder:

- ENCA -> GPIO7
- ENCB -> GPIO8
- BTN -> GPIO10
- BEEP -> GPIO2 optional
- GND -> GND

## Controls

- Turn = move
- Short press = select / open menu
- Long press = back
- Double press = home/status

## Fixes in v0.4.0

- Interrupt-driven quadrature encoder decoder
- Internal pullups enabled
- Debounced button events
- Real menu stack/back navigation
- Menus render only 3 rows so they do not overlap
- Footer/status line cleanup
- Wider boxless Duender logo
- Static DuenderOS menu tree
- Macro and control execution through Moonraker

## Encoder tuning

Edit `include/config.h`:

```cpp
#define ENCODER_STEPS_PER_DETENT 4
#define ENCODER_REVERSE 0
```

If it still barely moves, try `2`.
If it moves two items per click, try `8`.
If direction is backwards, change `ENCODER_REVERSE` to `1`.
