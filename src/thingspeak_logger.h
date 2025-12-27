// ThingSpeakLogger.h
#ifndef THINGSPEAK_LOGGER_H
#define THINGSPEAK_LOGGER_H

#include <Arduino.h>

// ============================================
// CẤU HÌNH THINGSPEAK
// ============================================

// Thay đổi các giá trị này theo Channel của bạn
#define THINGSPEAK_CHANNEL_ID 123456
#define THINGSPEAK_WRITE_API_KEY "YOUR_WRITE_API_KEY"
#define THINGSPEAK_READ_API_KEY "YOUR_READ_API_KEY"

// Update interval (ThingSpeak free: tối thiểu 15 giây)
#define THINGSPEAK_UPDATE_INTERVAL 30000  // 30 giây

// ============================================
// EVENT CODES
// ============================================

#define EVENT_NONE 0
#define EVENT_DOOR_OPEN 1
#define EVENT_DOOR_CLOSE 2
#define EVENT_INTRUSION 3
#define EVENT_FIRE_ALERT 4
#define EVENT_SMOKE_ALERT 5
#define EVENT_PERSON_DETECTED 6
#define EVENT_VEHICLE_DETECTED 7
#define EVENT_ALARM_ON 8
#define EVENT_ALARM_OFF 9
#define EVENT_SYSTEM_START 10

// ============================================
// STRUCT DỮ LIỆU
// ============================================

struct GarageData {
    float temperature;      // Field 1: Nhiệt độ (°C)
    float humidity;         // Field 2: Độ ẩm (%)
    int smokeLevel;         // Field 3: Mức khói (0-1023)
    bool doorOpen;          // Field 4: Trạng thái cửa (0/1)
    bool pirInside;         // Field 5: PIR bên trong (0/1)
    bool alarmOn;           // Field 6: Báo động (0/1)
    float distanceOutside;  // Field 7: Khoảng cách bên ngoài (cm)
    int eventCode;          // Field 8: Mã sự kiện
    String statusText;      // Status: Mô tả text (tùy chọn)
};

// ============================================
// CLASS THINGSPEAK LOGGER
// ============================================

class ThingSpeakLogger {
private:
    unsigned long lastUpdateTime;
    bool initialized;
    GarageData currentData;
    
public:
    // Constructor
    ThingSpeakLogger();
    
    // Khởi tạo ThingSpeak
    void begin();
    
    // Kiểm tra đã sẵn sàng chưa
    bool isReady();
    
    // ============================================
    // CÁC HÀM CẬP NHẬT DỮ LIỆU
    // ============================================
    
    // Cập nhật tất cả fields lên ThingSpeak
    bool updateAll(GarageData data);
    
    // Cập nhật một field đơn lẻ
    bool updateSingleField(int fieldNumber, float value);
    
    // Cập nhật định kỳ (tự động kiểm tra rate limit)
    bool updatePeriodic(GarageData data);
    
    // ============================================
    // CÁC HÀM LOG SỰ KIỆN
    // ============================================
    
    // Log cửa mở
    bool logDoorOpen(const char* reason, GarageData sensorData);
    
    // Log cửa đóng
    bool logDoorClose(const char* reason, GarageData sensorData);
    
    // Log đột nhập
    bool logIntrusion(GarageData sensorData);
    
    // Log hỏa hoạn
    bool logFireAlert(GarageData sensorData);
    
    // Log khói cao
    bool logSmokeAlert(GarageData sensorData);
    
    // Log phát hiện người
    bool logPersonDetected(const char* location, GarageData sensorData);
    
    // Log phát hiện xe
    bool logVehicleDetected(float distance, GarageData sensorData);
    
    // Log báo động bật
    bool logAlarmOn(const char* reason, GarageData sensorData);
    
    // Log báo động tắt
    bool logAlarmOff(const char* source, GarageData sensorData);
    
    // Log hệ thống khởi động
    bool logSystemStart();
    
    // ============================================
    // CÁC HÀM TIỆN ÍCH
    // ============================================
    
    // Kiểm tra có thể update không (rate limit)
    bool canUpdate();
    
    // Lấy thời gian còn lại đến lần update tiếp theo (giây)
    int getSecondsUntilNextUpdate();
    
    // Đọc một field từ ThingSpeak
    float readField(int fieldNumber);
    
    // Đọc status text từ ThingSpeak
    String readStatus();
    
    // Lấy dữ liệu hiện tại
    GarageData getCurrentData();
};

// Global instance (có thể dùng hoặc không)
extern ThingSpeakLogger tsLogger;

#endif