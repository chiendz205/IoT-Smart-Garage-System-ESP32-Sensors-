// ThingSpeakLogger.h
#ifndef THINGSPEAK_LOGGER_H
#define THINGSPEAK_LOGGER_H

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include "config.h"
#include "SensorModule.h"

class ThingSpeakLogger {
private:
  String apiKey;
  String serverUrl;
  unsigned long lastUploadTime;
  int uploadCount;
  
public:
  ThingSpeakLogger();
  void begin(String key);
  bool uploadSensorData(const SensorData& data);
  bool uploadEvent(String eventType, String eventData);
  int getUploadCount();
  void resetCounter();
};

#endif