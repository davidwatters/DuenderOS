# DuenderOS v0.5.1

ESP32-C3 + Creality 12864 ST7920 Moonraker/Klipper interface.

## New in v0.5.1

- First-boot captive portal Wi-Fi setup
- Optional Moonraker IP entry from captive portal
- Saved Moonraker IP/port using ESP32 Preferences flash storage
- Moonraker auto-scan on local subnet
- Find all Moonraker/Klipper printers on the subnet
- Select/save a discovered printer from the encoder menu
- Encoder-editable Moonraker IP page
- Network menu
- Serial commands for scan/portal/reset

## First boot

1. Flash firmware.
2. If Wi-Fi is not configured, connect your phone/laptop to:
   - SSID: `DuenderOS-Setup`
   - Password: `duenderos`
3. Enter Wi-Fi info.
4. Optional: enter Moonraker IP.
5. Device reboots/connects and tries Moonraker.
6. If saved/default Moonraker fails, it auto-scans port 7125 on your subnet.

## Pins

```txt
LCD-CS  -> GPIO4
LCD-EN  -> GPIO5
LCD-D7  -> GPIO6
ENCA    -> GPIO7
ENCB    -> GPIO8
BTN     -> GPIO10
BEEP    -> GPIO2 optional
```

## Menu

Settings > Network contains:

- Moonraker IP
- Find Printers
- Auto Scan
- WiFi Portal
- Reset WiFi

## Encoder IP editor

```txt
Turn       = change octet
Press      = next octet
Last press = save
Long press = save/back
```

## Serial commands

```txt
HELP
SCAN
LIST
PORTAL
RESETWIFI
HOME
PAUSE
RESUME
CANCEL
COOL
PLA
BELT
MESH
REPRINT
GCODE YOUR_COMMAND
```
