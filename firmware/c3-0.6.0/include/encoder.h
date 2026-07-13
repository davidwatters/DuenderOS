#pragma once
#include <Arduino.h>

enum ButtonEvent : uint8_t { BTN_NONE, BTN_SHORT, BTN_LONG, BTN_DOUBLE };

class EncoderInput {
public:
  void begin();
  void update();
  int getDelta();
  ButtonEvent getButtonEvent();
  unsigned long lastActivityMs() const { return lastActivity; }

private:
  int deltaReady = 0;
  ButtonEvent eventReady = BTN_NONE;
  bool btnDown = false;
  bool longSent = false;
  unsigned long downAt = 0;
  unsigned long lastShortAt = 0;
  unsigned long lastActivity = 0;
  bool lastBtnRaw = false;
  bool stableBtn = false;
  unsigned long lastBtnChange = 0;
};

extern EncoderInput Encoder;
