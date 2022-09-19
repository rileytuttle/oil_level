#include <melody-player.hpp>

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
