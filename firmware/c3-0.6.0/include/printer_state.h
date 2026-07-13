#pragma once
#include <Arduino.h>

struct PrinterState {
  bool wifiOk = false;
  bool moonrakerOk = false;
  String error = "";

  String state = "boot";
  String file = "";
  String message = "";
  String activityState = "Idle";  // idle_timeout: Idle, Ready, or Printing

  float hotendTemp = 0;
  float hotendTarget = 0;
  float bedTemp = 0;
  float bedTarget = 0;
  float chamberTemp = 0;

  int progress = 0;
  float x = 0, y = 0, z = 0;
  float speedFactor = 100;
  float fanPercent = 0;
  unsigned long lastUpdate = 0;
};
