#pragma once
#include <Arduino.h>

struct TouchPoint {
  bool pressed;
  int x;
  int y;
};

class DuenderTouch {
public:
  void begin();
  TouchPoint read();
};

extern DuenderTouch Touch;