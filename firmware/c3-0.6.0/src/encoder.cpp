#include "encoder.h"
#include "config.h"

EncoderInput Encoder;

static volatile int16_t encAccum = 0;
static volatile uint8_t lastState = 0;

static const int8_t TRANSITION_TABLE[16] = {
   0, -1,  1,  0,
   1,  0,  0, -1,
  -1,  0,  0,  1,
   0,  1, -1,  0
};

static inline bool readButtonPressedRaw() {
  bool pressed = digitalRead(ENC_BTN_PIN) == LOW;
#if USE_ENCODER_ALT_BUTTON
  pressed = pressed || (digitalRead(ENC_BTN_ALT_PIN) == LOW);
#endif
  return pressed;
}

void IRAM_ATTR encoderISR() {
  uint8_t a = digitalRead(ENC_A_PIN);
  uint8_t b = digitalRead(ENC_B_PIN);
  uint8_t state = (a << 1) | b;
  uint8_t idx = (lastState << 2) | state;
  int8_t movement = TRANSITION_TABLE[idx];
#if ENCODER_REVERSE
  movement = -movement;
#endif
  if (movement != 0) {
    encAccum += movement;
    if (encAccum > 64) encAccum = 64;
    if (encAccum < -64) encAccum = -64;
  }
  lastState = state;
}

void EncoderInput::begin() {
  pinMode(ENC_A_PIN, INPUT_PULLUP);
  pinMode(ENC_B_PIN, INPUT_PULLUP);
  pinMode(ENC_BTN_PIN, INPUT_PULLUP);
#if USE_ENCODER_ALT_BUTTON
  pinMode(ENC_BTN_ALT_PIN, INPUT_PULLUP);
#endif

  lastState = (digitalRead(ENC_A_PIN) << 1) | digitalRead(ENC_B_PIN);
  encAccum = 0;

  attachInterrupt(digitalPinToInterrupt(ENC_A_PIN), encoderISR, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENC_B_PIN), encoderISR, CHANGE);

  bool pressed = readButtonPressedRaw();
  lastBtnRaw = pressed;
  stableBtn = pressed;
  btnDown = pressed;
  downAt = millis();
  longSent = false;
  lastBtnChange = millis();
  lastActivity = millis();
}

void EncoderInput::update() {
  noInterrupts();
  int16_t raw = encAccum;
  interrupts();

  int step = ENCODER_STEPS_PER_DETENT;
  if (step < 1) step = 1;

  int clicks = raw / step;
  if (clicks != 0) {
    noInterrupts();
    encAccum -= clicks * step;
    interrupts();

    // No acceleration for now. Reliability first.
    if (clicks > 1) clicks = 1;
    if (clicks < -1) clicks = -1;
    deltaReady += clicks;
    lastActivity = millis();
  }

  unsigned long now = millis();
  bool rawPressed = readButtonPressedRaw();

  // Debounced, polled button. This is intentionally not interrupt-driven so
  // bouncing GPIO9/10 edges cannot queue fake/missed clicks.
  if (rawPressed != lastBtnRaw) {
    lastBtnRaw = rawPressed;
    lastBtnChange = now;
  }

  if ((now - lastBtnChange) >= 18 && rawPressed != stableBtn) {
    stableBtn = rawPressed;
    lastActivity = now;

    if (stableBtn) {
      btnDown = true;
      longSent = false;
      downAt = now;
    } else if (btnDown) {
      btnDown = false;
      if (!longSent) eventReady = BTN_SHORT;
      longSent = false;
    }
  }

  if (btnDown && !longSent && (now - downAt) >= 700) {
    longSent = true;
    eventReady = BTN_LONG;
    lastActivity = now;
  }
}

int EncoderInput::getDelta() {
  int d = deltaReady;
  if (d > 1) d = 1;
  if (d < -1) d = -1;
  deltaReady -= d;
  return d;
}

ButtonEvent EncoderInput::getButtonEvent() {
  ButtonEvent e = eventReady;
  eventReady = BTN_NONE;
  return e;
}
