// PushsaferNotifier.cpp
#include "pushsafer.h"
#include <WiFi.h>
#include <HTTPClient.h>

// Global instance
PushsaferNotifier psNotifier;

// ============================================
// CONSTRUCTOR
// ============================================

PushsaferNotifier::PushsaferNotifier() {
    apiKey = PUSHSAFER_API_KEY;
    apiUrl = PUSHSAFER_API_URL;
    initialized = false;
    lastSendTime = 0;
    sendCount = 0;
}

PushsaferNotifier::PushsaferNotifier(String key) {
    apiKey = key;
    apiUrl = PUSHSAFER_API_URL;
    initialized = false;
    lastSendTime = 0;
    sendCount = 0;
}

// ============================================
// KHỞI TẠO
// ============================================

void PushsaferNotifier::begin() {
    Serial.println("[Pushsafer] Initializing...");
    
    if (apiKey == "YOUR_PUSHSAFER_KEY" || apiKey.length() == 0) {
        Serial.println("[Pushsafer] ⚠️ Warning: API Key not set!");
        Serial.println("[Pushsafer] Get your key from: https://www.pushsafer.com/");
        initialized = false;
        return;
    }
    
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[Pushsafer] ⚠️ Warning: WiFi not connected!");
        initialized = false;
        return;
    }
    
    initialized = true;
    Serial.println("[Pushsafer] ✓ Ready!");
}

void PushsaferNotifier::begin(String key) {
    apiKey = key;
    begin();
}

bool PushsaferNotifier::isReady() {
    return initialized && (WiFi.status() == WL_CONNECTED);
}

// ============================================
// HELPERS
// ============================================

String PushsaferNotifier::urlEncode(String str) {
    String encoded = "";
    char c;
    char code0;
    char code1;
    
    for (unsigned int i = 0; i < str.length(); i++) {
        c = str.charAt(i);
        if (c == ' ') {
            encoded += '+';
        } else if (isalnum(c)) {
            encoded += c;
        } else {
            code1 = (c & 0xf) + '0';
            if ((c & 0xf) > 9) {
                code1 = (c & 0xf) - 10 + 'A';
            }
            c = (c >> 4) & 0xf;
            code0 = c + '0';
            if (c > 9) {
                code0 = c - 10 + 'A';
            }
            encoded += '%';
            encoded += code0;
            encoded += code1;
        }
    }
    return encoded;
}

String PushsaferNotifier::buildPostData(PushNotification notification) {
    String postData = "";
    
    // API Key (required)
    postData += "k=" + apiKey;
    
    // Title (required)
    if (notification.title.length() > 0) {
        postData += "&t=" + urlEncode(notification.title);
    }
    
    // Message (required)
    if (notification.message.length() > 0) {
        postData += "&m=" + urlEncode(notification.message);
    }
    
    // Priority
    postData += "&pr=" + String(notification.priority);
    
    // Sound
    if (notification.sound >= 0) {
        postData += "&s=" + String(notification.sound);
    }
    
    // Icon
    if (notification.icon > 0) {
        postData += "&i=" + String(notification.icon);
    }
    
    // Icon Color
    if (notification.iconColor.length() > 0) {
        postData += "&c=" + urlEncode(notification.iconColor);
    }
    
    // Vibration
    if (notification.vibration > 0) {
        postData += "&v=" + String(notification.vibration);
    }
    
    // Device
    if (notification.device.length() > 0) {
        postData += "&d=" + notification.device;
    } else {
        postData += "&d=a";  // Default: all devices
    }
    
    // Time to Live
    if (notification.timeToLive > 0) {
        postData += "&l=" + String(notification.timeToLive);
    }
    
    // Retry (for priority 2)
    if (notification.retry > 0) {
        postData += "&re=" + String(notification.retry);
    }
    
    // Expire (for priority 2)
    if (notification.expire > 0) {
        postData += "&ex=" + String(notification.expire);
    }
    
    return postData;
}

bool PushsaferNotifier::sendHTTPRequest(String postData) {
    if (!isReady()) {
        Serial.println("[Pushsafer] Not ready to send!");
        return false;
    }
    
    HTTPClient http;
    http.begin(apiUrl);
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    
    Serial.println("[Pushsafer] Sending notification...");
    
    int httpCode = http.POST(postData);
    
    if (httpCode > 0) {
        String response = http.getString();
        Serial.print("[Pushsafer] HTTP Code: ");
        Serial.println(httpCode);
        Serial.print("[Pushsafer] Response: ");
        Serial.println(response);
        
        http.end();
        
        // Check if successful
        if (response.indexOf("\"status\":1") > 0 || httpCode == 200) {
            Serial.println("[Pushsafer] ✓ Notification sent successfully!");
            lastSendTime = millis();
            sendCount++;
            return true;
        } else {
            Serial.println("[Pushsafer] ✗ API returned error");
            return false;
        }
    } else {
        Serial.print("[Pushsafer] ✗ HTTP request failed: ");
        Serial.println(httpCode);
        http.end();
        return false;
    }
}

bool PushsaferNotifier::sendNotification(PushNotification notification) {
    String postData = buildPostData(notification);
    return sendHTTPRequest(postData);
}

// ============================================
// CRITICAL NOTIFICATIONS - PRIORITY 2
// ============================================

bool PushsaferNotifier::sendIntrusionAlert(bool pirDetected, bool ultrasonicDetected) {
    Serial.println("[Pushsafer] Sending INTRUSION alert!");
    
    String details = "PIR: ";
    details += pirDetected ? "YES" : "NO";
    details += ", Ultrasonic: ";
    details += ultrasonicDetected ? "YES" : "NO";
    
    PushNotification notif;
    notif.title = "🚨 ĐỘT NHẬP!";
    notif.message = "Phát hiện người trong garage đã đóng! " + details;
    notif.priority = PRIORITY_EMERGENCY;
    notif.sound = SOUND_SIREN;
    notif.icon = ICON_SECURITY;
    notif.iconColor = "#FF0000";
    notif.vibration = VIBRATION_HIGH;
    notif.timeToLive = 60;    // 60 phút
    notif.retry = 60;         // Retry mỗi 60s
    notif.expire = 3600;      // Hết hạn sau 1 giờ
    notif.device = "a";
    
    return sendNotification(notif);
}

bool PushsaferNotifier::sendFireAlert(float temperature, int smokeLevel, float humidity) {
    Serial.println("[Pushsafer] Sending FIRE alert!");
    
    String details = "Nhiệt độ: " + String(temperature, 1) + "°C, ";
    details += "Khói: " + String(smokeLevel) + ", ";
    details += "Độ ẩm: " + String(humidity, 1) + "%";
    
    PushNotification notif;
    notif.title = "🔥 HỎA HOẠN!";
    notif.message = "Phát hiện cháy trong garage! " + details + " Gọi 114 ngay!";
    notif.priority = PRIORITY_EMERGENCY;
    notif.sound = SOUND_ALARM;
    notif.icon = ICON_FIRE;
    notif.iconColor = "#FF6600";
    notif.vibration = VIBRATION_HIGH;
    notif.timeToLive = 30;
    notif.retry = 60;
    notif.expire = 1800;      // 30 phút
    notif.device = "a";
    
    return sendNotification(notif);
}

// ============================================
// HIGH PRIORITY NOTIFICATIONS - PRIORITY 1
// ============================================

bool PushsaferNotifier::sendVehicleDetected(float distance) {
    Serial.println("[Pushsafer] Sending vehicle detection!");
    
    PushNotification notif;
    notif.title = "🚗 Xe đang chờ";
    notif.message = "Phát hiện xe trước cửa garage (" + String(distance, 1) + "cm)";
    notif.priority = PRIORITY_HIGH;
    notif.sound = SOUND_ALARM;
    notif.icon = ICON_CAR;
    notif.iconColor = "#0066FF";
    notif.vibration = VIBRATION_MEDIUM;
    notif.timeToLive = 5;     // 5 phút
    notif.retry = 0;
    notif.expire = 0;
    notif.device = "a";
    
    return sendNotification(notif);
}

bool PushsaferNotifier::sendHighTemperature(float temperature) {
    Serial.println("[Pushsafer] Sending high temperature warning!");
    
    PushNotification notif;
    notif.title = "🌡️ Cảnh báo nhiệt độ";
    notif.message = "Nhiệt độ cao bất thường: " + String(temperature, 1) + "°C. Kiểm tra garage ngay!";
    notif.priority = PRIORITY_HIGH;
    notif.sound = SOUND_ALARM;
    notif.icon = ICON_WARNING;
    notif.iconColor = "#FFA500";
    notif.vibration = VIBRATION_MEDIUM;
    notif.timeToLive = 0;
    notif.retry = 0;
    notif.expire = 0;
    notif.device = "a";
    
    return sendNotification(notif);
}

bool PushsaferNotifier::sendHighSmoke(int smokeLevel) {
    Serial.println("[Pushsafer] Sending high smoke warning!");
    
    PushNotification notif;
    notif.title = "💨 Cảnh báo khói";
    notif.message = "Mức khói cao: " + String(smokeLevel) + ". Kiểm tra garage ngay!";
    notif.priority = PRIORITY_HIGH;
    notif.sound = SOUND_ALARM;
    notif.icon = ICON_WARNING;
    notif.iconColor = "#808080";
    notif.vibration = VIBRATION_MEDIUM;
    notif.timeToLive = 0;
    notif.retry = 0;
    notif.expire = 0;
    notif.device = "a";
    
    return sendNotification(notif);
}

bool PushsaferNotifier::sendAlarmActivated(const char* reason) {
    Serial.println("[Pushsafer] Sending alarm activated!");
    
    PushNotification notif;
    notif.title = "⚠️ Báo động bật";
    notif.message = "Báo động garage đã BẬT: " + String(reason);
    notif.priority = PRIORITY_HIGH;
    notif.sound = SOUND_ALARM;
    notif.icon = ICON_ERROR;
    notif.iconColor = "#FF0000";
    notif.vibration = VIBRATION_HIGH;
    notif.timeToLive = 0;
    notif.retry = 0;
    notif.expire = 0;
    notif.device = "a";
    
    return sendNotification(notif);
}

// ============================================
// UTILITIES
// ============================================

bool PushsaferNotifier::sendTest() {
    PushNotification notif;
    notif.title = "Test Notification";
    notif.message = "Hệ thống thông báo garage hoạt động bình thường";
    notif.priority = PRIORITY_HIGH;
    notif.sound = SOUND_ALARM;
    notif.icon = ICON_CAR;
    notif.iconColor = "#0066FF";
    notif.vibration = VIBRATION_MEDIUM;
    notif.timeToLive = 0;
    notif.retry = 0;
    notif.expire = 0;
    notif.device = "a";
    
    return sendNotification(notif);
}

int PushsaferNotifier::getSendCount() {
    return sendCount;
}

void PushsaferNotifier::resetCounter() {
    sendCount = 0;
}