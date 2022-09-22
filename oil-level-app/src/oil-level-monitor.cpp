#include <oil-level-monitor.hpp>
#include <numeric>

String OilLevelMonitor::status_to_string(Status status)
{
    switch (status)
    {
        case Status::NONE:
            return String("None");
        case Status::ERROR:
            return String("Error");
        case Status::FULL:
            return String("Full");
        case Status::NOT_FULL:
            return String("Not full");
        case Status::TANK_LOW:
            return String("Low");
        default:
            return String("Unexpected Case");
    }
}

void OilLevelMonitor::init()
{
    prev_update_ts = 0;
    m_current_level = 0.0f;
    m_distance_valid = false;
    m_consec_error = 0;
    m_gone_through_once = false;
    m_measurements.clear();

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

void OilLevelMonitor::update_level()
{
    m_distance_valid = false;
    int16_t raw_distance = 0;
    if (vl53.dataReady()) {
        raw_distance = vl53.distance();
        if (raw_distance == -1) {
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
    if (m_distance_valid)
    {
        const int16_t shifted_distance = raw_distance - m_params.tank_range.first;
        if (m_measurements.size() == m_params.filter_size)
        {
            m_measurements.erase(m_measurements.begin());
        }
        m_measurements.push_back(shifted_distance);
        // should never be empty at this point
        // no need to check for it
        const float avg = std::reduce(m_measurements.begin(), m_measurements.end()) / m_measurements.size();
        m_current_level = mm_to_percent(avg);
    }
}

OilLevelMonitor::Status OilLevelMonitor::update()
{
    const unsigned long current_millis = millis();
    Status ret_status = m_prev_status;
    if (current_millis - prev_update_ts > m_params.measurement_interval)
    {
        update_level();
        m_current_level = get_level();
        // Serial.print(m_current_level * 100.0f, 2);
        // Serial.println(" %");
        if (m_current_level > FULL_THRESHOLD)
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

        if (m_gone_through_once)
        {
            if (recently_filled(m_prev_status, ret_status))
            {
                // if level is higher than it was before trigger fill notification
                // probably pass in a callback to call
                // Serial.println("recently filled");
                if (m_params.tank_filled_cb) { m_params.tank_filled_cb(ret_status, m_current_level); }
            }
            if (tank_low_event(m_prev_status, ret_status))
            {
                // Serial.println("tank is low");
                if (m_params.tank_low_cb) { m_params.tank_low_cb(ret_status, m_current_level); }
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
        }
        else
        {
            m_gone_through_once = true;
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
