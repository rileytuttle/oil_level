#include <oil-level-monitor.hpp>
#include <melody-player.hpp>

#include <Arduino.h>
#include <vector>

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
    if (prev_status != status && status == OilLevelMonitor::Status::TANK_LOW)
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
