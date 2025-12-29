// PushsaferNotifier.h
#ifndef PUSHSAFER_NOTIFIER_H
#define PUSHSAFER_NOTIFIER_H

#include <Arduino.h>
#include "config.h"

// ============================================
// PRIORITY LEVELS
// ============================================

#define PRIORITY_SILENT    -2  // Không notification, chỉ badge
#define PRIORITY_LOW       -1  // Không âm thanh
#define PRIORITY_NORMAL     0  // Âm thanh mặc định
#define PRIORITY_HIGH       1  // Âm thanh cao hơn
#define PRIORITY_EMERGENCY  2  // Yêu cầu acknowledge, retry

// ============================================
// SOUNDS (0-62)
// ============================================

#define SOUND_SILENT        0
#define SOUND_AHEM          1
#define SOUND_ALARM         8
#define SOUND_SIREN        24
#define SOUND_POSITIVE      4

// ============================================
// ICONS (1-181)
// ============================================

#define ICON_INFO           1
#define ICON_WARNING        2
#define ICON_ERROR          3
#define ICON_SUCCESS        4
#define ICON_HOME          33
#define ICON_FIRE          62
#define ICON_SECURITY      96
#define ICON_CAR          139

// ============================================
// VIBRATION (1-3)
// ============================================

#define VIBRATION_LOW       1
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
    // HÀM GỬI CƠ BẢN
    // ============================================
    
    // Gửi notification đơn giản
    bool send(String title, String message);
    
    // Gửi với priority
    bool send(String title, String message, int priority);
    
    // Gửi đầy đủ tham số
    bool sendNotification(PushNotification notification);
    
    // ============================================
    // GARAGE NOTIFICATIONS - CRITICAL (Priority 2)
    // ============================================
    
    // 🚨 Đột nhập
    bool sendIntrusionAlert(bool pirDetected, bool ultrasonicDetected);
    
    // 🔥 Hỏa hoạn
    bool sendFireAlert(float temperature, int smokeLevel, float humidity);
    
    // ============================================
    // GARAGE NOTIFICATIONS - HIGH PRIORITY (Priority 1)
    // ============================================
    
    // 🚗 Phát hiện xe trước cửa
    bool sendVehicleDetected(float distance);
    
    // 🌡️ Cảnh báo nhiệt độ cao
    bool sendHighTemperature(float temperature);
    
    // 💨 Cảnh báo khói cao
    bool sendHighSmoke(int smokeLevel);
    
    // ⚠️ Báo động bật
    bool sendAlarmActivated(const char* reason);
    
    // ============================================
    // GARAGE NOTIFICATIONS - NORMAL PRIORITY (Priority 0)
    // ============================================
    
    // 🚪 Cửa mở
    bool sendDoorOpened(const char* reason);
    
    // 🚪 Cửa đóng
    bool sendDoorClosed(const char* reason);
    
    // ✅ Báo động tắt
    bool sendAlarmDeactivated(const char* source);
    
    // ============================================
    // GARAGE NOTIFICATIONS - LOW PRIORITY (Priority -1)
    // ============================================
    
    // 💡 Hệ thống online
    bool sendSystemOnline();
    
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

// Global instance (optional)
extern PushsaferNotifier psNotifier;

#endif