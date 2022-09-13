#include "Adafruit_VL53L1X.h"

#include <Arduino.h>
#include <vector>

#define NOTE_C4 262
#define NOTE_A3 220
#define NOTE_G3 196
#define NOTE_B3 247

class MelodyPlayer
{
public:
    MelodyPlayer(const std::vector<std::pair<int, float>>& melody, const uint8_t pin) :
        m_melody(melody),
        m_pin(pin)
    {
        pinMode(pin, OUTPUT);
    }
    ~MelodyPlayer() {}
    void update();
    void begin_playing();
private:
    static constexpr int TIME_BETWEEN_NOTES_MS = 150;
    enum class State
    {
        PLAYING,
        PAUSING,
        FINISHED,
    };
    uint8_t m_pin {};
    std::vector<std::pair<int, float>> m_melody {};
    State m_state {State::FINISHED};
    uint8_t m_cur_note_ind {0};
    uint32_t m_begin_ts {0};
    void begin_pausing();
    void start_next_note();
};

void MelodyPlayer::begin_playing()
{
    if (m_state == State::PLAYING)
    {
        return;
    }
    Serial.println("start playing");
    m_cur_note_ind = 0;
    start_next_note();
}

void MelodyPlayer::start_next_note()
{
    const std::pair<int, float> cur_note = m_melody[m_cur_note_ind];
    tone(m_pin, cur_note.first, cur_note.second * 1000);
    m_begin_ts = millis();
    m_state = State::PLAYING;
}

void MelodyPlayer::begin_pausing()
{
    if (m_cur_note_ind +1 >= static_cast<int>(m_melody.size()))
    {
        m_state = State::FINISHED;
    }
    else
    {
        m_state = State::PAUSING;
        m_begin_ts = millis();
        m_cur_note_ind += 1;
    }
}

void MelodyPlayer::update()
{
    const State prev_state = m_state;
    const uint32_t cur_time = millis();
    switch (m_state)
    {
        case State::PLAYING:
        {
            if (cur_time - m_begin_ts > m_melody[m_cur_note_ind].second * 1000)
            {
                begin_pausing();
            }
            break;
        }
        case State::PAUSING:
        {
            if (millis() - m_begin_ts > TIME_BETWEEN_NOTES_MS)
            {
                start_next_note();
            }
            break;
        }
        case State::FINISHED:
            [[fallthrough]];
        default:
        {
            // do nothing
        }
    }
    if (prev_state != m_state)
    {
        Serial.print("melody state ");
        Serial.print(static_cast<int>(prev_state));
        Serial.print("->");
        Serial.println(static_cast<int>(m_state));
    }
}

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
        LOW,      // when the tank is low (time to refill)
    };

    OilLevelMonitor() : vl53(XSHUT_PIN, IRQ_PIN) {}
    ~OilLevelMonitor() {}
    void init();
    Status update();
    float get_level() const { return m_current_level; }
    Status get_status() const { return m_prev_status; }
private:
    static constexpr unsigned long MEASUREMENT_INTERVAL = 1000;
    static constexpr float MM_TO_PERCENT_CONVERSION_FACTOR = 0.001f;
    static constexpr float LOW_THRESHOLD = 0.2f; // 20 or less percent of tank is low
    static constexpr float FULL_THRESHOLD = 0.9f; // 90 percent or more is considered full

    Adafruit_VL53L1X vl53;
    float m_current_level {0.0f};
    unsigned long prev_update_ts {0};
    bool m_distance_valid {false};
    Status m_prev_status {Status::NONE};

    int16_t get_distance();
    float mm_to_percent(const int16_t dist_mm) { return 1.0f - (dist_mm * MM_TO_PERCENT_CONVERSION_FACTOR); }
    bool recently_filled(const Status& old_status, const Status& new_status) { return old_status != Status::FULL && new_status == Status::FULL; }
};

void OilLevelMonitor::init()
{
    prev_update_ts = 0;
    m_current_level = 0.0f;
    m_distance_valid = false;

    Wire.begin();
    if (! vl53.begin(0x29, &Wire)) {
        Serial.print(F("Error on init of VL sensor: "));
        Serial.println(vl53.vl_status);
        while (1)       delay(10);
    }
    Serial.println(F("VL53L1X sensor OK!"));

    Serial.print(F("Sensor ID: 0x"));
    Serial.println(vl53.sensorID(), HEX);

    if (! vl53.startRanging()) {
        Serial.print(F("Couldn't start ranging: "));
        Serial.println(vl53.vl_status);
        while (1)       delay(10);
    }
    Serial.println(F("Ranging started"));

    // Valid timing budgets: 15, 20, 33, 50, 100, 200 and 500ms!
    vl53.setTimingBudget(50);
    Serial.print(F("Timing budget (ms): "));
    Serial.println(vl53.getTimingBudget());
    /*
    vl.VL53L1X_SetDistanceThreshold(100, 300, 3, 1);
    vl.VL53L1X_SetInterruptPolarity(0);
    */
}

int16_t OilLevelMonitor::get_distance()
{
    m_distance_valid = false;
    int16_t distance = 0;
    if (vl53.dataReady()) {
        distance = vl53.distance();
        if (distance == -1) {
            // something went wrong!
            Serial.print(F("Couldn't get distance: "));
            Serial.println(vl53.vl_status);
        }
        else
        {
            m_distance_valid = true;
        }
        // Serial.print(F("Distance: "));
        // Serial.print(distance);
        // Serial.println(" mm");
        // data is read out, time for another reading!
        vl53.clearInterrupt();
    }
    return distance;
}

OilLevelMonitor::Status OilLevelMonitor::update()
{
    const unsigned long current_millis = millis();
    Status ret_status = m_prev_status;
    if (current_millis - prev_update_ts > MEASUREMENT_INTERVAL)
    {
        m_current_level = mm_to_percent(get_distance());
        Serial.print(m_current_level);
        Serial.println(" %");
        if (!m_distance_valid)
        {
            ret_status = Status::ERROR;
        }
        else if (m_current_level > FULL_THRESHOLD)
        {
            ret_status = Status::FULL;
        }
        else if (m_current_level < LOW_THRESHOLD)
        {
            ret_status = Status::LOW;
        }
        else
        {
            ret_status = Status::NOT_FULL;
        }

        if (recently_filled(m_prev_status, ret_status))
        {
            // if level is higher than it was before trigger fill notification
            // probably pass in a callback to call
        }

        prev_update_ts = current_millis;
    }
    if (m_prev_status != ret_status)
    {
        Serial.print("status ");
        Serial.print(static_cast<int>(m_prev_status));
        Serial.print("->");
        Serial.println(static_cast<int>(ret_status));
    }
    m_prev_status = ret_status;
    return ret_status;
}

OilLevelMonitor oil_level_monitor;

const unsigned long PUBLISH_INTERVAL = 1000;
unsigned long prev_publish_ts = 0;



// notes in the melody:
// note durations: 4 = quarter note, 8 = eighth note, etc.:
const std::vector<std::pair<int, float>> MELODY =
{
    {NOTE_C4, 0.25f},
    {NOTE_G3, 0.125f},
    {NOTE_G3, 0.125f},
    {NOTE_A3, 0.25f},
    {NOTE_G3, 0.25f},
    {0,       0.25f},
    {NOTE_B3, 0.25f},
    {NOTE_C4, 0.25f},
};

#define MELODY_PIN 8

MelodyPlayer melody_player(MELODY, MELODY_PIN); 

void setup() {
    Serial.begin(115200);
    while (!Serial) delay(10);

    Serial.println(F("Adafruit VL53L1X sensor demo"));

    oil_level_monitor.init();

    prev_publish_ts = 0;
}

static void publish_task_update()
{
    // do nothing for now
}

void loop() {
    const unsigned long current_millis = millis();
    const OilLevelMonitor::Status prev_status = oil_level_monitor.get_status();

    const OilLevelMonitor::Status status = oil_level_monitor.update();
    if (prev_status != status && status == OilLevelMonitor::Status::LOW)
    {
        melody_player.begin_playing();
    }

    melody_player.update();
    if (current_millis - prev_publish_ts > PUBLISH_INTERVAL)
    {
        publish_task_update();
        prev_publish_ts = current_millis;
    }
}
