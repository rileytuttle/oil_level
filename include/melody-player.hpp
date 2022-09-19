#include <Arduino.h>
#include <cstdint>
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
