#pragma once
#include <Arduino.h>
#include "printer_state.h"

class MoonrakerClient {
public:
  void begin(PrinterState* state);
  bool poll();
  bool sendGcode(const String& script);
  bool emergencyStop();
private:
  PrinterState* ps = nullptr;
  String baseUrl();
};

extern MoonrakerClient Moonraker;
