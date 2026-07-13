#include "touch.h"
#include <SPI.h>
#include <XPT2046_Touchscreen.h>

#define TOUCH_CS_PIN   33
#define TOUCH_IRQ_PIN  36

#define TOUCH_MOSI     32
#define TOUCH_MISO     39
#define TOUCH_CLK      25

XPT2046_Touchscreen ts(TOUCH_CS_PIN, TOUCH_IRQ_PIN);
DuenderTouch Touch;

void DuenderTouch::begin() {
  pinMode(TOUCH_CS_PIN, OUTPUT);
  digitalWrite(TOUCH_CS_PIN, HIGH);

  SPI.begin(TOUCH_CLK, TOUCH_MISO, TOUCH_MOSI, TOUCH_CS_PIN);

  ts.begin();
  ts.setRotation(1);

  Serial.println("Touch begin on CYD SPI pins");
}

TouchPoint DuenderTouch::read() {
  TouchPoint out{false, 0, 0};

  if (!ts.touched()) return out;

  TS_Point p = ts.getPoint();

  Serial.printf("RAW TOUCH: x=%d y=%d z=%d\n", p.x, p.y, p.z);

  if (p.z < 100) return out;

  out.x = map(p.x, 200, 3800, 0, 320);
  out.y = map(p.y, 200, 3800, 0, 240);

  out.x = constrain(out.x, 0, 319);
  out.y = constrain(out.y, 0, 239);
  out.pressed = true;

  return out;
}