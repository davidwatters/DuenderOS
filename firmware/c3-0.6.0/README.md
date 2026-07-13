# DuenderOS v0.6.0

Release-ready Wi-Fi setup and improved activity handling.

## Changes

- No Wi-Fi SSID or password is compiled into the project.
- Default Moonraker endpoint is now `0.0.0.0:7125` instead of a personal printer IP.
- First boot with empty Wi-Fi storage opens the `DuenderOS-Setup` captive portal.
- Macro animation is tied to Klipper `idle_timeout.state`. It animates while Klipper is executing a macro and stops when Klipper returns to Ready/Idle.
- Actual prints still show filename, progress bar, and percentage.
- Background Moonraker polling, connection grace, and IP+port printer profiles remain enabled.

## Factory-first-boot testing

Wi-Fi credentials live in the ESP32 NVS, not inside the firmware ZIP or compiled BIN. Flashing a new build over an already configured board normally preserves them. To test the true first-use captive portal, erase the board before uploading:

```bash
pio run --target erase
pio run --target upload
```

Then connect to the setup access point:

- SSID: `DuenderOS-Setup`
- Password: `duenderos`


## Change the displayed name

Edit `include/config.h`:

```cpp
#define DEVICE_DISPLAY_NAME "DuenderOS"
```

That value is used on the boot screen and the left side of every page header. Long names are automatically shortened so they do not overlap the status text.


## Runtime device name editor
Open **Settings > Device Name**.

- Turn the encoder to change the selected character.
- Short press moves to the next character.
- Long press saves the name to ESP32 flash.
- Spaces at the end are trimmed automatically.
- The saved name is used on the boot screen and every page header.
