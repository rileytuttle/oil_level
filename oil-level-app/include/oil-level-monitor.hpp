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

    struct Params
    {
        size_t filter_size {DEFAULT_FILTER_SIZE};
        std::pair<int16_t, int16_t> tank_range {DEFAULT_TANK_RANGE};
        std::function<void(Status, float)> tank_low_cb {};
        std::function<void(Status, float)> tank_filled_cb {};
        uint32_t measurement_interval {DEFAULT_MEASUREMENT_INTERVAL};
    };

    OilLevelMonitor(const Params &params) :
        m_params(params),
        vl53(XSHUT_PIN, IRQ_PIN) {}
    OilLevelMonitor() : vl53(XSHUT_PIN, IRQ_PIN) {}
    ~OilLevelMonitor() {}
    void init();
    Status update();
    float get_level() const { return m_current_level; }
    Status get_status() const { return m_prev_status; }
    static String status_to_string(Status status);
private:
    static constexpr uint32_t DEFAULT_MEASUREMENT_INTERVAL = 1000;
    static constexpr float LOW_THRESHOLD = 0.2f; // 20 or less percent of tank is low
    static constexpr float FULL_THRESHOLD = 0.9f; // 90 percent or more is considered full
    static constexpr unsigned int CONSEC_ERROR_THRESH = 5;
    static constexpr size_t DEFAULT_FILTER_SIZE = 20;
    static constexpr std::pair<int16_t, int16_t> DEFAULT_TANK_RANGE = {120, 1000};

    Adafruit_VL53L1X vl53;
    float m_current_level {0.0f};
    bool m_gone_through_once {false};
    unsigned long prev_update_ts {0};
    bool m_distance_valid {false};
    Status m_prev_status {Status::NONE};
    unsigned int m_consec_error {0};
    Params m_params;
    std::vector<int16_t> m_measurements;

    int16_t get_distance();
    float mm_to_percent(const int16_t dist_mm) const { return 1.0f - (dist_mm / m_params.tank_range.second - m_params.tank_range.first); }
    bool recently_filled(const Status& old_status, const Status& new_status) const { return old_status != Status::FULL && new_status == Status::FULL; }
    bool tank_low_event(const Status& old_status, const Status& new_status) const { return old_status != new_status && new_status == Status::TANK_LOW; }
    bool check_for_consec_error();
    void update_level();
};
