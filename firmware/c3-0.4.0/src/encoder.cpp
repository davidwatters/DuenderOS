#include "encoder.h"
#include "config.h"

EncoderInput Encoder;

static volatile int8_t encAccum = 0;
static volatile uint8_t lastState = 0;
static volatile uint32_t lastIsrUs = 0;

static const int8_t TRANSITION_TABLE[16] = {
  0, -1,  1,  0,
  1,  0,  0, -1,
 -1,  0,  0,  1,
  0,  1, -1,  0
};

void IRAM_ATTR encoderISR() {
  uint32_t now = micros();
  if (now - lastIsrUs < 250) return; // knock down switch chatter
  lastIsrUs = now;

  uint8_t a = digitalRead(ENC_A_PIN);
  uint8_t b = digitalRead(ENC_B_PIN);
  uint8_t state = (a << 1) | b;
  uint8_t idx = (lastState << 2) | state;
  int8_t movement = TRANSITION_TABLE[idx];
#if ENCODER_REVERSE
  movement = -movement;
#endif
  encAccum += movement;
  lastState = state;
}

void EncoderInput::begin() {
  pinMode(ENC_A_PIN, INPUT_PULLUP);
  pinMode(ENC_B_PIN, INPUT_PULLUP);
  pinMode(ENC_BTN_PIN, INPUT_PULLUP);

  lastState = (digitalRead(ENC_A_PIN) << 1) | digitalRead(ENC_B_PIN);
  attachInterrupt(digitalPinToInterrupt(ENC_A_PIN), encoderISR, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENC_B_PIN), encoderISR, CHANGE);

  lastBtn = digitalRead(ENC_BTN_PIN);
}

void EncoderInput::update() {
  noInterrupts();
  int8_t raw = encAccum;
  interrupts();

  while (raw >= ENCODER_STEPS_PER_DETENT) {
    deltaReady++;
    noInterrupts(); encAccum -= ENCODER_STEPS_PER_DETENT; raw = encAccum; interrupts();
  }
  while (raw <= -ENCODER_STEPS_PER_DETENT) {
    deltaReady--;
    noInterrupts(); encAccum += ENCODER_STEPS_PER_DETENT; raw = encAccum; interrupts();
  }

  bool btn = digitalRead(ENC_BTN_PIN);
  unsigned long now = millis();

  if (btn != lastBtn && now - lastBtnChange > 25) {
    lastBtnChange = now;
    lastBtn = btn;

    if (!btn) { // active low
      btnDown = true;
      longSent = false;
      downAt = now;
    } else if (btnDown) {
      btnDown = false;
      if (!longSent) {
        if (now - lastShortAt < 350) {
          eventReady = BTN_DOUBLE;
          lastShortAt = 0;
        } else {
          eventReady = BTN_SHORT;
          lastShortAt = now;
        }
      }
    }
  }

  if (btnDown && !longSent && now - downAt > 650) {
    longSent = true;
    eventReady = BTN_LONG;
  }
}

int EncoderInput::getDelta() {
  int d = deltaReady;
  deltaReady = 0;
  return d;
}

ButtonEvent EncoderInput::getButtonEvent() {
  ButtonEvent e = eventReady;
  eventReady = BTN_NONE;
  return e;
}
