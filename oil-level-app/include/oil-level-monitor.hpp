#include <Adafruit_VL53L1X.h>
#include <functional>

#define IRQ_PIN 2
#define XSHUT_PIN 3

class OilLevelMonitor
{
public:
    enum class Status
    {
        NONE,
        ERROR,    // some kind of error has occurred
        FULL,     // when the tank is considered full
        NOT_FULL, // when the tank is somewhere between full and low
        TANK_LOW,      // when the tank is low (time to refill)
    };

    OilLevelMonitor(
        std::function<void(float)> tank_low_cb,
        std::function<void(float)> tank_filled_cb) :
        m_tank_low_cb(tank_low_cb),
        m_tank_filled_cb(tank_filled_cb),
        vl53(XSHUT_PIN, IRQ_PIN) {}
    OilLevelMonitor() : vl53(XSHUT_PIN, IRQ_PIN) {}
    ~OilLevelMonitor() {}
    void init();
    Status update();
    float get_level() const { return m_current_level; }
    Status get_status() const { return m_prev_status; }
    static String status_to_string(Status status);
private:
    static constexpr unsigned long MEASUREMENT_INTERVAL = 1000;
    static constexpr float MM_TO_PERCENT_CONVERSION_FACTOR = 0.001f;
    static constexpr float LOW_THRESHOLD = 0.2f; // 20 or less percent of tank is low
    static constexpr float FULL_THRESHOLD = 0.9f; // 90 percent or more is considered full
    static constexpr unsigned int CONSEC_ERROR_THRESH = 5;

    Adafruit_VL53L1X vl53;
    float m_current_level {0.0f};
    bool m_gone_through_once {false};
    unsigned long prev_update_ts {0};
    bool m_distance_valid {false};
    Status m_prev_status {Status::NONE};
    unsigned int m_consec_error {0};
    std::function<void(float)> m_tank_low_cb;
    std::function<void(float)> m_tank_filled_cb;

    int16_t get_distance();
    float mm_to_percent(const int16_t dist_mm) { return 1.0f - (dist_mm * MM_TO_PERCENT_CONVERSION_FACTOR); }
    bool recently_filled(const Status& old_status, const Status& new_status) const { return old_status != Status::FULL && new_status == Status::FULL; }
    bool tank_low_event(const Status& old_status, const Status& new_status) const { return old_status != new_status && new_status == Status::TANK_LOW; }
    bool check_for_consec_error();
};
