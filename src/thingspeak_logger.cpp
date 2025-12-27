// ThingSpeakLogger.cpp
#include "thingspeak_logger.h"
#include <WiFi.h>
#include <ThingSpeak.h>

// WiFi client cho ThingSpeak
WiFiClient tsClient;

// Global instance
ThingSpeakLogger tsLogger;

// ============================================
// CONSTRUCTOR
// ============================================

ThingSpeakLogger::ThingSpeakLogger() {
    lastUpdateTime = 0;
    initialized = false;
    
    // Khởi tạo currentData với giá trị mặc định
    currentData.temperature = 0;
    currentData.humidity = 0;
    currentData.smokeLevel = 0;
    currentData.doorOpen = false;
    currentData.pirInside = false;
    currentData.alarmOn = false;
    currentData.distanceOutside = 0;
    currentData.eventCode = EVENT_NONE;
    currentData.statusText = "";
}

// ============================================
// KHỞI TẠO
// ============================================

void ThingSpeakLogger::begin() {
    Serial.println("[ThingSpeak] Initializing...");
    ThingSpeak.begin(tsClient);
    initialized = true;
    Serial.println("[ThingSpeak] Ready!");
}

bool ThingSpeakLogger::isReady() {
    return initialized;
}

// ============================================
// CẬP NHẬT DỮ LIỆU
// ============================================

bool ThingSpeakLogger::updateAll(GarageData data) {
    if (!initialized) {
        Serial.println("[ThingSpeak] Not initialized!");
        return false;
    }
    
    Serial.println("[ThingSpeak] Updating all fields...");
    
    // Set các fields
    ThingSpeak.setField(1, data.temperature);
    ThingSpeak.setField(2, data.humidity);
    ThingSpeak.setField(3, data.smokeLevel);
    ThingSpeak.setField(4, data.doorOpen ? 1 : 0);
    ThingSpeak.setField(5, data.pirInside ? 1 : 0);
    ThingSpeak.setField(6, data.alarmOn ? 1 : 0);
    ThingSpeak.setField(7, data.distanceOutside);
    ThingSpeak.setField(8, data.eventCode);
    
    // Set status text nếu có
    if (data.statusText.length() > 0) {
        ThingSpeak.setStatus(data.statusText);
    }
    
    // Write to ThingSpeak
    int httpCode = ThingSpeak.writeFields(THINGSPEAK_CHANNEL_ID, THINGSPEAK_WRITE_API_KEY);
    
    if (httpCode == 200) {
        Serial.println("[ThingSpeak] ✓ Update successful!");
        lastUpdateTime = millis();
        currentData = data;
        return true;
    } else {
        Serial.print("[ThingSpeak] ✗ Update failed. HTTP code: ");
        Serial.println(httpCode);
        return false;
    }
}

bool ThingSpeakLogger::updateSingleField(int fieldNumber, float value) {
    if (!initialized) {
        return false;
    }
    
    // Kiểm tra rate limit
    if (!canUpdate()) {
        Serial.println("[ThingSpeak] Rate limit: Too soon to update");
        return false;
    }
    
    Serial.print("[ThingSpeak] Updating field ");
    Serial.print(fieldNumber);
    Serial.print(" = ");
    Serial.println(value);
    
    int httpCode = ThingSpeak.writeField(THINGSPEAK_CHANNEL_ID, fieldNumber, value, THINGSPEAK_WRITE_API_KEY);
    
    if (httpCode == 200) {
        Serial.println("[ThingSpeak] ✓ Field updated!");
        lastUpdateTime = millis();
        return true;
    } else {
        Serial.print("[ThingSpeak] ✗ Failed. HTTP code: ");
        Serial.println(httpCode);
        return false;
    }
}

bool ThingSpeakLogger::updatePeriodic(GarageData data) {
    if (!canUpdate()) {
        return false;
    }
    
    return updateAll(data);
}

// ============================================
// LOG CÁC SỰ KIỆN
// ============================================

bool ThingSpeakLogger::logDoorOpen(const char* reason, GarageData sensorData) {
    if (!canUpdate()) {
        Serial.println("[ThingSpeak] Skipping door open log (rate limit)");
        return false;
    }
    
    Serial.println("[ThingSpeak] Logging: Door OPEN");
    
    GarageData data = sensorData;
    data.doorOpen = true;
    data.eventCode = EVENT_DOOR_OPEN;
    data.statusText = "Door opened: " + String(reason);
    
    return updateAll(data);
}

bool ThingSpeakLogger::logDoorClose(const char* reason, GarageData sensorData) {
    if (!canUpdate()) {
        Serial.println("[ThingSpeak] Skipping door close log (rate limit)");
        return false;
    }
    
    Serial.println("[ThingSpeak] Logging: Door CLOSE");
    
    GarageData data = sensorData;
    data.doorOpen = false;
    data.eventCode = EVENT_DOOR_CLOSE;
    data.statusText = "Door closed: " + String(reason);
    
    return updateAll(data);
}

bool ThingSpeakLogger::logIntrusion(GarageData sensorData) {
    // Intrusion là sự kiện quan trọng, bỏ qua rate limit nếu cần
    Serial.println("[ThingSpeak] Logging: INTRUSION ALERT!");
    
    GarageData data = sensorData;
    data.eventCode = EVENT_INTRUSION;
    data.alarmOn = true;
    data.statusText = "INTRUSION DETECTED!";
    
    // Force update bằng cách reset timer nếu cần
    bool oldCanUpdate = canUpdate();
    if (!oldCanUpdate) {
        Serial.println("[ThingSpeak] Emergency update - bypassing rate limit");
        lastUpdateTime = millis() - THINGSPEAK_UPDATE_INTERVAL;
    }
    
    return updateAll(data);
}

bool ThingSpeakLogger::logFireAlert(GarageData sensorData) {
    // Fire alert là sự kiện cực kỳ quan trọng
    Serial.println("[ThingSpeak] Logging: FIRE ALERT!");
    
    GarageData data = sensorData;
    data.eventCode = EVENT_FIRE_ALERT;
    data.alarmOn = true;
    
    String statusMsg = "FIRE! Temp:" + String(data.temperature, 1) + "C Smoke:" + String(data.smokeLevel);
    data.statusText = statusMsg;
    
    // Force update
    if (!canUpdate()) {
        Serial.println("[ThingSpeak] Emergency fire update");
        lastUpdateTime = millis() - THINGSPEAK_UPDATE_INTERVAL;
    }
    
    return updateAll(data);
}

bool ThingSpeakLogger::logSmokeAlert(GarageData sensorData) {
    if (!canUpdate()) {
        Serial.println("[ThingSpeak] Skipping smoke alert (rate limit)");
        return false;
    }
    
    Serial.println("[ThingSpeak] Logging: Smoke Alert");
    
    GarageData data = sensorData;
    data.eventCode = EVENT_SMOKE_ALERT;
    data.statusText = "High smoke detected: " + String(data.smokeLevel);
    
    return updateAll(data);
}

bool ThingSpeakLogger::logPersonDetected(const char* location, GarageData sensorData) {
    if (!canUpdate()) {
        return false;
    }
    
    Serial.println("[ThingSpeak] Logging: Person Detected");
    
    GarageData data = sensorData;
    data.eventCode = EVENT_PERSON_DETECTED;
    data.pirInside = true;
    data.statusText = "Person at: " + String(location);
    
    return updateAll(data);
}

bool ThingSpeakLogger::logVehicleDetected(float distance, GarageData sensorData) {
    if (!canUpdate()) {
        return false;
    }
    
    Serial.println("[ThingSpeak] Logging: Vehicle Detected");
    
    GarageData data = sensorData;
    data.eventCode = EVENT_VEHICLE_DETECTED;
    data.distanceOutside = distance;
    data.statusText = "Vehicle at " + String(distance, 1) + "cm";
    
    return updateAll(data);
}

bool ThingSpeakLogger::logAlarmOn(const char* reason, GarageData sensorData) {
    if (!canUpdate()) {
        return false;
    }
    
    Serial.println("[ThingSpeak] Logging: Alarm ON");
    
    GarageData data = sensorData;
    data.eventCode = EVENT_ALARM_ON;
    data.alarmOn = true;
    data.statusText = "Alarm activated: " + String(reason);
    
    return updateAll(data);
}

bool ThingSpeakLogger::logAlarmOff(const char* source, GarageData sensorData) {
    if (!canUpdate()) {
        return false;
    }
    
    Serial.println("[ThingSpeak] Logging: Alarm OFF");
    
    GarageData data = sensorData;
    data.eventCode = EVENT_ALARM_OFF;
    data.alarmOn = false;
    data.statusText = "Alarm OFF by " + String(source);
    
    return updateAll(data);
}

bool ThingSpeakLogger::logSystemStart() {
    Serial.println("[ThingSpeak] Logging: System Start");
    
    GarageData data;
    data.temperature = 0;
    data.humidity = 0;
    data.smokeLevel = 0;
    data.doorOpen = false;
    data.pirInside = false;
    data.alarmOn = false;
    data.distanceOutside = 0;
    data.eventCode = EVENT_SYSTEM_START;
    data.statusText = "System started";
    
    // Cho phép log ngay lập tức khi khởi động
    lastUpdateTime = 0;
    
    return updateAll(data);
}

// ============================================
// TIỆN ÍCH
// ============================================

bool ThingSpeakLogger::canUpdate() {
    unsigned long currentMillis = millis();
    return (currentMillis - lastUpdateTime >= THINGSPEAK_UPDATE_INTERVAL);
}

int ThingSpeakLogger::getSecondsUntilNextUpdate() {
    unsigned long currentMillis = millis();
    unsigned long elapsed = currentMillis - lastUpdateTime;
    
    if (elapsed >= THINGSPEAK_UPDATE_INTERVAL) {
        return 0;
    }
    
    return (THINGSPEAK_UPDATE_INTERVAL - elapsed) / 1000;
}

float ThingSpeakLogger::readField(int fieldNumber) {
    if (!initialized) {
        return 0;
    }
    
    Serial.print("[ThingSpeak] Reading field ");
    Serial.println(fieldNumber);
    
    float value = ThingSpeak.readFloatField(THINGSPEAK_CHANNEL_ID, fieldNumber, THINGSPEAK_READ_API_KEY);
    
    int statusCode = ThingSpeak.getLastReadStatus();
    if (statusCode == 200) {
        Serial.print("[ThingSpeak] Field value: ");
        Serial.println(value);
        return value;
    } else {
        Serial.print("[ThingSpeak] Read failed. Status: ");
        Serial.println(statusCode);
        return 0;
    }
}

String ThingSpeakLogger::readStatus() {
    if (!initialized) {
        return "";
    }
    
    Serial.println("[ThingSpeak] Reading status...");
    
    String status = ThingSpeak.readStatus(THINGSPEAK_CHANNEL_ID, THINGSPEAK_READ_API_KEY);
    
    int statusCode = ThingSpeak.getLastReadStatus();
    if (statusCode == 200) {
        Serial.print("[ThingSpeak] Status: ");
        Serial.println(status);
        return status;
    } else {
        Serial.println("[ThingSpeak] Read status failed");
        return "";
    }
}

GarageData ThingSpeakLogger::getCurrentData() {
    return currentData;
}