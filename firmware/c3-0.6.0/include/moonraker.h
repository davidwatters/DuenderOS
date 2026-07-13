#pragma once
#include <Arduino.h>
#include "printer_state.h"

class MoonrakerClient {
public:
  void begin(PrinterState* state);

  // Non-blocking API. These only queue work for the background task.
  bool poll();
  bool sendGcode(const String& script);
  bool emergencyStop();

  // Call every main loop. Copies completed network results into the UI state.
  void update();

private:
  PrinterState* ps = nullptr;
  SemaphoreHandle_t stateMutex = nullptr;
  QueueHandle_t commandQueue = nullptr;
  TaskHandle_t taskHandle = nullptr;

  PrinterState pendingState;
  bool pendingReady = false;
  volatile bool immediatePoll = true;

  static void taskEntry(void* arg);
  void taskLoop();
  bool fetchSnapshot(PrinterState& out, const String& host, uint16_t port);
  bool postGcode(const String& script, const String& host, uint16_t port);
  bool postEmergencyStop(const String& host, uint16_t port);
  void publish(const PrinterState& state);
};

extern MoonrakerClient Moonraker;
