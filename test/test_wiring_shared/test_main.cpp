// Wiring-test tool — entry point.
//
// This firmware is *not* the production pedal firmware. It boots the
// embedded hardware-config JSON (generated at build time by
// scripts/generate_wiring_config_header.py from the Make target's
// CONFIG=<path>) and drives an interactive serial UI for verifying
// the electrical correctness of a freshly soldered board.
//
//   - TASK-383 — env scaffolding, banner, single-key console
//   - TASK-384 — button press logging + coverage summary
//   - TASK-385 — LED group modes (on/off/blinking/chase/cycle-all) ← this task
//   - TASK-386 — LED individual mode
//   - TASK-387 — final keystroke bindings + ? legend + builder doc
//
// Provisional bindings used here will be revised in TASK-387:
//   's' summary  | 'o' on  | 'f' off  | 'b' blinking
//   'c' chase-on | 'C' chase-off  | 'a' cycle-all

#include "button_tracker.h"
#include "led_mode.h"
#include "status_display.h"
#include "wiring_config.h"
#include "wiring_config_embedded.h"
#include <Arduino.h>

namespace
{

    wiring_test::WiringConfig g_config{};
    wiring_test::ButtonState g_buttonStates[wiring_test::kMaxButtons]{};
    wiring_test::LedRuntime g_ledRuntime{};
    uint8_t g_lastWrittenLevels[wiring_test::kMaxLeds]{};

    void halt(const char* reason)
    {
        while (true)
        {
            Serial.printf("[wiring-test] FATAL: %s\n", reason);
            delay(2000);
        }
    }

    void initLedPins()
    {
        for (uint8_t i = 0; i < g_config.numLeds; i++)
        {
            pinMode(g_config.leds[i].pin, OUTPUT);
            // Park each LED in its physical-off level so a builder
            // sees a clean board immediately on boot.
            uint8_t off = g_config.leds[i].activeHigh ? LOW : HIGH;
            digitalWrite(g_config.leds[i].pin, off);
            g_lastWrittenLevels[i] = off;
        }
    }

    void writeLedLevels()
    {
        uint8_t levels[wiring_test::kMaxLeds]{};
        wiring_test::computeLedLevels(g_config, g_ledRuntime, millis(), levels);
        for (uint8_t i = 0; i < g_config.numLeds; i++)
        {
            if (levels[i] != g_lastWrittenLevels[i])
            {
                digitalWrite(g_config.leds[i].pin, levels[i]);
                g_lastWrittenLevels[i] = levels[i];
            }
        }
    }

    void announceMode()
    {
        wiring_test::printStatusBlock(
            g_config, wiring_test::CONFIG_FILENAME, g_buttonStates, &g_ledRuntime, millis());
    }

    bool dispatchGroupMode(char c)
    {
        using wiring_test::GroupMode;
        GroupMode mode;
        switch (c)
        {
            case 'o':
                mode = GroupMode::On;
                break;
            case 'f':
                mode = GroupMode::Off;
                break;
            case 'b':
                mode = GroupMode::Blinking;
                break;
            case 'c':
                mode = GroupMode::ChaseOn;
                break;
            case 'C':
                mode = GroupMode::ChaseOff;
                break;
            case 'a':
                mode = GroupMode::CycleAll;
                break;
            default:
                return false;
        }
        wiring_test::setGroupMode(g_ledRuntime, mode, millis());
        announceMode();
        return true;
    }

    void handleKey(char c)
    {
        if (c == '\r' || c == '\n')
        {
            return;
        }
        if (dispatchGroupMode(c))
        {
            return;
        }
        switch (c)
        {
            case 's':
            case 'S':
                announceMode();
                break;
            default:
                if (c >= 0x20 && c < 0x7f)
                {
                    Serial.printf("(unbound key '%c' — bindings land in TASK-386/387)\n", c);
                }
                break;
        }
    }

} // namespace

void setup()
{
    Serial.begin(115200);
    delay(1500); // give USB-CDC / monitor a moment to attach

    auto result = wiring_test::parseWiringConfig(wiring_test::CONFIG_JSON, g_config);
    if (result.status != wiring_test::ParseStatus::Ok)
    {
        halt(result.message);
    }

    wiring_test::initButtonTracker(g_config, g_buttonStates);
    wiring_test::initLedRuntime(g_ledRuntime, millis());
    initLedPins();
    announceMode();
}

void loop()
{
    while (Serial.available() > 0)
    {
        int ch = Serial.read();
        if (ch < 0)
        {
            break;
        }
        handleKey(static_cast<char>(ch));
    }

    while (true)
    {
        auto event =
            wiring_test::pollButtonTracker(g_config, g_buttonStates, g_config.debounceMs, millis());
        if (! event.fired)
        {
            break;
        }
        char line[96];
        wiring_test::formatPressLine(
            line, sizeof(line), g_config.buttons[event.buttonIndex], event.debouncedLevel);
        Serial.print(line);
    }

    wiring_test::tickLedRuntime(g_ledRuntime, millis());
    writeLedLevels();
}
