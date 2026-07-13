#pragma once
#include <TFT_eSPI.h>
#include "theme.h"

// DuenderOS tiny vector icon library.
// All icons are drawn with TFT_eSPI primitives so they scale cleanly
// and do not require bitmap assets.

namespace DUIcons {

inline void wifi(TFT_eSPI* tft, int x, int y, uint16_t color, uint16_t bg) {
  // Upright WiFi icon, centered around x/y.
  tft->drawArc(x, y + 12, 18, 15, 225, 315, color, bg);
  tft->drawArc(x, y + 12, 12, 10, 230, 310, color, bg);
  tft->drawArc(x, y + 12, 6, 5, 235, 305, color, bg);
  tft->fillCircle(x, y + 22, 2, color);
}

inline void moon(TFT_eSPI* tft, int x, int y, uint16_t color, uint16_t bg) {
  tft->fillCircle(x, y, 9, color);
  tft->fillCircle(x + 5, y - 3, 9, bg);
}

inline void ethernet(TFT_eSPI* tft, int x, int y, uint16_t color) {
  // RJ45/network plug
  tft->drawRoundRect(x, y + 5, 24, 18, 3, color);
  tft->drawRect(x + 5, y, 14, 8, color);
  tft->drawFastVLine(x + 6, y + 13, 5, color);
  tft->drawFastVLine(x + 11, y + 13, 5, color);
  tft->drawFastVLine(x + 16, y + 13, 5, color);
  tft->drawFastVLine(x + 21, y + 13, 5, color);
}

inline void nozzle(TFT_eSPI* tft, int x, int y, uint16_t color) {
  tft->drawRect(x + 7, y, 14, 15, color);
  tft->drawRect(x + 4, y + 10, 20, 7, color);
  tft->drawLine(x + 8, y + 17, x + 14, y + 25, color);
  tft->drawLine(x + 20, y + 17, x + 14, y + 25, color);
  tft->drawLine(x + 9, y + 25, x + 19, y + 25, color);
}

inline void bed(TFT_eSPI* tft, int x, int y, uint16_t color) {
  tft->drawRect(x, y + 15, 30, 7, color);
  tft->drawLine(x + 4, y + 22, x + 1, y + 27, color);
  tft->drawLine(x + 26, y + 22, x + 29, y + 27, color);
  tft->drawFastVLine(x + 6, y + 3, 8, color);
  tft->drawFastVLine(x + 15, y + 3, 8, color);
  tft->drawFastVLine(x + 24, y + 3, 8, color);
}

inline void fan(TFT_eSPI* tft, int x, int y, uint16_t color) {
  tft->fillCircle(x, y, 4, color);
  tft->fillCircle(x, y - 10, 6, color);
  tft->fillCircle(x + 9, y + 5, 6, color);
  tft->fillCircle(x - 9, y + 5, 6, color);
}

inline void grid(TFT_eSPI* tft, int x, int y, uint16_t color) {
  for (int r = 0; r < 2; r++) {
    for (int c = 0; c < 2; c++) {
      tft->drawRect(x + c * 10, y + r * 10, 7, 7, color);
    }
  }
}

inline void thermometer(TFT_eSPI* tft, int x, int y, uint16_t color) {
  tft->drawRoundRect(x + 7, y, 7, 18, 4, color);
  tft->fillCircle(x + 10, y + 21, 6, color);
  tft->drawFastVLine(x + 10, y + 5, 14, color);
}

inline void sliders(TFT_eSPI* tft, int x, int y, uint16_t color) {
  tft->drawFastHLine(x, y + 3, 24, color);
  tft->drawFastHLine(x, y + 12, 24, color);
  tft->drawFastHLine(x, y + 21, 24, color);
  tft->fillCircle(x + 7, y + 3, 2, color);
  tft->fillCircle(x + 17, y + 12, 2, color);
  tft->fillCircle(x + 11, y + 21, 2, color);
}

inline void gear(TFT_eSPI* tft, int x, int y, uint16_t color) {
  tft->drawCircle(x + 11, y + 11, 9, color);
  tft->drawCircle(x + 11, y + 11, 3, color);
  tft->drawFastHLine(x, y + 11, 22, color);
  tft->drawFastVLine(x + 11, y, 22, color);
}

} // namespace DUIcons
