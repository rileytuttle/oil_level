#include <oil-level-monitor.hpp>

void OilLevelMonitor::init()
{
    prev_update_ts = 0;
    m_current_level = 0.0f;
    m_distance_valid = false;
    m_consec_error = 0;

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
        // Serial.print(m_current_level * 100.0f, 2);
        // Serial.println(" %");
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
            ret_status = Status::TANK_LOW;
        }
        else
        {
            ret_status = Status::NOT_FULL;
        }

        if (recently_filled(m_prev_status, ret_status))
        {
            // if level is higher than it was before trigger fill notification
            // probably pass in a callback to call
            Serial.println("recently filled");
        }
        // Serial.print("prev status: ");
        // Serial.print(static_cast<int>(m_prev_status));
        // Serial.print(" new status: ");
        // Serial.println(static_cast<int>(ret_status));
        if (m_prev_status == Status::ERROR)
        {
            if (ret_status == Status::ERROR)
            {
                m_consec_error+=1;
            }
            else
            {
                m_consec_error=0;
            }
        }

        if (m_consec_error >= CONSEC_ERROR_THRESH)
        {
            m_current_level = 0;
            ret_status = Status::ERROR;
        }
        // Serial.print("consec error: ");
        // Serial.println(m_consec_error);

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
