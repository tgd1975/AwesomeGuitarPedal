#include "button_tracker.h"

#include <Arduino.h>
#include <stdio.h>

namespace wiring_test
{

    namespace
    {

        bool levelIsPressed(const WiringButton& b, uint8_t level)
        {
            return b.activeLow ? (level == LOW) : (level == HIGH);
        }

    } // namespace

    void initButtonTracker(const WiringConfig& cfg, ButtonState states[])
    {
        for (uint8_t i = 0; i < cfg.numButtons; i++)
        {
            pinMode(cfg.buttons[i].pin, INPUT_PULLUP);
            uint8_t level = static_cast<uint8_t>(digitalRead(cfg.buttons[i].pin));
            states[i].lastRaw = level;
            states[i].debouncedLevel = level;
            states[i].lastChangeMs = 0;
            states[i].pressCount = 0;
        }
    }

    PressEvent pollButtonTracker(const WiringConfig& cfg,
                                 ButtonState states[],
                                 uint32_t debounceMs,
                                 uint32_t nowMs)
    {
        PressEvent event{};
        for (uint8_t i = 0; i < cfg.numButtons; i++)
        {
            const auto& button = cfg.buttons[i];
            auto& s = states[i];

            uint8_t raw = static_cast<uint8_t>(digitalRead(button.pin));

            if (raw != s.lastRaw)
            {
                s.lastRaw = raw;
                s.lastChangeMs = nowMs;
                continue;
            }

            if (raw == s.debouncedLevel)
            {
                continue;
            }

            // Raw has been stable at a new level for at least debounceMs.
            if ((nowMs - s.lastChangeMs) < debounceMs)
            {
                continue;
            }

            uint8_t prevDebounced = s.debouncedLevel;
            s.debouncedLevel = raw;

            // Emit a press event only on the *into-pressed* edge.
            if (! levelIsPressed(button, prevDebounced) && levelIsPressed(button, raw))
            {
                s.pressCount++;
                if (! event.fired)
                {
                    event.fired = true;
                    event.buttonIndex = i;
                    event.debouncedLevel = raw;
                }
            }
        }
        return event;
    }

    size_t
    formatPressLine(char* buf, size_t bufLen, const WiringButton& button, uint8_t debouncedLevel)
    {
        if (buf == nullptr || bufLen == 0)
        {
            return 0;
        }
        bool pressed = levelIsPressed(button, debouncedLevel);
        int n = snprintf(buf,
                         bufLen,
                         "press: %-20s GPIO %2u  active-%s%s  state=%s\n",
                         button.name,
                         static_cast<unsigned>(button.pin),
                         button.activeLow ? "low" : "high",
                         button.isSelect ? "  (select)" : "",
                         pressed ? "PRESSED" : "RELEASED");
        return (n < 0) ? 0 : static_cast<size_t>(n);
    }

} // namespace wiring_test
