// ThingSpeakLogger.cpp
#include "ThingSpeakLogger.h"

// ============================================
// CONSTRUCTOR
// ============================================

ThingSpeakLogger::ThingSpeakLogger() {
  apiKey = THINGSPEAK_API_KEY;
  serverUrl = "http://";
  serverUrl += THINGSPEAK_SERVER;
  serverUrl += "/update";
  lastUploadTime = 0;
  uploadCount = 0;
  
  Serial.println("[ThingSpeak] Logger created");
}

// ============================================
// BEGIN
// ============================================

void ThingSpeakLogger::begin(String key) {
  if (key.length() > 0) {
    apiKey = key;
  }
  
  Serial.println("[ThingSpeak] Initialized");
  Serial.print("   API Key: ");
  Serial.println(apiKey.substring(0, 8) + "...");  // Show first 8 chars only
  Serial.print("   Server: ");
  Serial.println(THINGSPEAK_SERVER);
}

// ============================================
// UPLOAD SENSOR DATA
// ============================================

bool ThingSpeakLogger::uploadSensorData(const SensorData& data) {
  // Check rate limit (ThingSpeak: minimum 15 seconds between updates)
  unsigned long now = millis();
  if (now - lastUploadTime < 15000) {
    Serial.println("[ThingSpeak] ⏱️ Rate limit - skipping upload");
    return false;
  }
  
  // Check WiFi
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[ThingSpeak] ❌ WiFi not connected");
    return false;
  }
  
  // Check API key
  if (apiKey == "YOUR_THINGSPEAK_WRITE_KEY" || apiKey.length() == 0) {
    Serial.println("[ThingSpeak] ❌ API Key not set");
    return false;
  }
  
  HTTPClient http;
  
  // Build URL with query parameters
  String url = serverUrl;
  url += "?api_key=" + apiKey;
  
  // Field 1: Temperature (DHT22)
  if (data.temperatureDHT > -900) {
    url += "&field1=" + String(data.temperatureDHT, 2);
  }
  
  // Field 2: Humidity
  if (data.humidity > -900) {
    url += "&field2=" + String(data.humidity, 2);
  }
  
  // Field 3: Smoke Level
  url += "&field3=" + String(data.smokeLevel);
  
  // Field 4: Distance Outside
  url += "&field4=" + String(data.distanceOutside, 2);
  
  // Field 5: PIR Motion (0 or 1)
  url += "&field5=" + String(data.pirMotion ? 1 : 0);
  
  // Field 6: Temperature (DS18B20)
  url += "&field6=" + String(data.temperatureDS, 2);
  
  // Field 7: Distance Inside
  url += "&field7=" + String(data.distanceInside, 2);
  
  Serial.print("[ThingSpeak] 📤 Uploading data... ");
  
  http.begin(url);
  http.setTimeout(10000);  // 10 second timeout
  int httpCode = http.GET();
  
  if (httpCode > 0) {
    String response = http.getString();
    
    // ThingSpeak returns entry ID on success (number > 0)
    int entryId = response.toInt();
    
    if (entryId > 0) {
      Serial.print("✅ Success! Entry ID: ");
      Serial.println(entryId);
      lastUploadTime = now;
      uploadCount++;
      http.end();
      return true;
    } else {
      Serial.println("❌ Failed");
      Serial.print("   Response: ");
      Serial.println(response);
      http.end();
      return false;
    }
  } else {
    Serial.print("❌ HTTP error: ");
    Serial.println(httpCode);
    http.end();
    return false;
  }
}

// ============================================
// UPLOAD EVENT
// ============================================

bool ThingSpeakLogger::uploadEvent(String eventType, String eventData) {
  // Check rate limit
  unsigned long now = millis();
  if (now - lastUploadTime < 15000) {
    return false;  // Skip silently for events
  }
  
  if (WiFi.status() != WL_CONNECTED) {
    return false;
  }
  
  if (apiKey == "YOUR_THINGSPEAK_WRITE_KEY" || apiKey.length() == 0) {
    return false;
  }
  
  HTTPClient http;
  
  String url = serverUrl;
  url += "?api_key=" + apiKey;
  url += "&status=" + eventType + ":" + eventData;
  
  Serial.print("[ThingSpeak] 📝 Logging event: ");
  Serial.print(eventType);
  Serial.print(" - ");
  Serial.println(eventData);
  
  http.begin(url);
  http.setTimeout(10000);
  int httpCode = http.GET();
  
  bool success = false;
  if (httpCode > 0) {
    String response = http.getString();
    int entryId = response.toInt();
    
    if (entryId > 0) {
      Serial.println("   ✅ Event logged");
      lastUploadTime = now;
      uploadCount++;
      success = true;
    }
  }
  
  http.end();
  return success;
}

// ============================================
// UTILITIES
// ============================================

int ThingSpeakLogger::getUploadCount() {
  return uploadCount;
}

void ThingSpeakLogger::resetCounter() {
  uploadCount = 0;
  Serial.println("[ThingSpeak] Counter reset");
}