#pragma once
#include <Arduino.h>

enum ButtonEvent : uint8_t { BTN_NONE, BTN_SHORT, BTN_LONG, BTN_DOUBLE };

class EncoderInput {
public:
  void begin();
  void update();
  int getDelta();
  ButtonEvent getButtonEvent();

private:
  int deltaReady = 0;
  ButtonEvent eventReady = BTN_NONE;
  bool lastBtn = true;
  bool btnDown = false;
  bool longSent = false;
  unsigned long downAt = 0;
  unsigned long lastBtnChange = 0;
  unsigned long lastShortAt = 0;
};

extern EncoderInput Encoder;
