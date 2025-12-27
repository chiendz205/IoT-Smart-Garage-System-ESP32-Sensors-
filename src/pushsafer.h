
#ifndef PUSHSAFER_NOTIFIER_H
#define PUSHSAFER_NOTIFIER_H

#include <Arduino.h>

// ============================================
// CẤU HÌNH PUSHSAFER
// ============================================

// API Key - LẤY TỪ: https://www.pushsafer.com/
#define PUSHSAFER_API_KEY "YOUR_PUSHSAFER_KEY"

// API Endpoint
#define PUSHSAFER_API_URL "https://www.pushsafer.com/api"

// ============================================
// PRIORITY LEVELS
// ============================================

#define PRIORITY_HIGH       1  // Âm thanh cao hơn
#define PRIORITY_EMERGENCY  2  // Yêu cầu acknowledge, retry

// ============================================
// SOUNDS (0-62)
// ============================================

#define SOUND_ALARM         8
#define SOUND_SIREN        24

// ============================================
// ICONS (1-181)
// ============================================

#define ICON_WARNING        2
#define ICON_ERROR          3
#define ICON_FIRE          62
#define ICON_SECURITY      96
#define ICON_CAR          139

// ============================================
// VIBRATION (1-3)
// ============================================

#define VIBRATION_MEDIUM    2
#define VIBRATION_HIGH      3

// ============================================
// STRUCT NOTIFICATION
// ============================================

struct PushNotification {
    String title;
    String message;
    int priority;
    int sound;
    int icon;
    String iconColor;
    int vibration;
    int timeToLive;      // minutes (0 = no expire)
    int retry;           // seconds (for priority 2)
    int expire;          // seconds (for priority 2)
    String device;       // "a" = all devices
};

// ============================================
// CLASS PUSHSAFER NOTIFIER
// ============================================

class PushsaferNotifier {
private:
    String apiKey;
    String apiUrl;
    bool initialized;
    unsigned long lastSendTime;
    int sendCount;
    
    // Build POST data từ struct
    String buildPostData(PushNotification notification);
    
    // URL encode helper
    String urlEncode(String str);
    
    // Send HTTP POST request
    bool sendHTTPRequest(String postData);
    
public:
    // Constructor
    PushsaferNotifier();
    PushsaferNotifier(String key);
    
    // Khởi tạo
    void begin();
    void begin(String key);
    
    // Kiểm tra ready
    bool isReady();
    
    // ============================================
    // CRITICAL NOTIFICATIONS - PRIORITY 2
    // ============================================
    
    // Đột nhập (Priority 2 - Emergency)
    bool sendIntrusionAlert(bool pirDetected, bool ultrasonicDetected);
    
    // Hỏa hoạn (Priority 2 - Emergency)
    bool sendFireAlert(float temperature, int smokeLevel, float humidity);
    
    // ============================================
    // HIGH PRIORITY NOTIFICATIONS - PRIORITY 1
    // ============================================
    
    // Phát hiện xe trước cửa (Priority 1)
    bool sendVehicleDetected(float distance);
    
    // Cảnh báo nhiệt độ cao (Priority 1)
    bool sendHighTemperature(float temperature);
    
    // Cảnh báo khói cao (Priority 1)
    bool sendHighSmoke(int smokeLevel);
    
    // Báo động bật (Priority 1)
    bool sendAlarmActivated(const char* reason);
    
    // ============================================
    // TIỆN ÍCH
    // ============================================
    
    // Gửi test notification
    bool sendTest();
    
    // Lấy số lượng notification đã gửi
    int getSendCount();
    
    // Reset counter
    void resetCounter();
};

// Global instance
extern PushsaferNotifier psNotifier;

#endif