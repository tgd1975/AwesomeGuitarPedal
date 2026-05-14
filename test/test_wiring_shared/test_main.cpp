// Wiring-test tool — entry point.
//
// This firmware is *not* the production pedal firmware. It boots the
// embedded hardware-config JSON (generated at build time by
// scripts/generate_wiring_config_header.py from the Make target's
// CONFIG=<path>) and drives an interactive serial UI for verifying
// the electrical correctness of a freshly soldered board.
//
// Bindings (final, see also docs/builders/WIRING_TEST_TOOL.md):
//
//   Group mode (default at boot)
//     o   all LEDs ON                 c   chase-on
//     f   all LEDs OFF                C   chase-off
//     b   blink at ~2 Hz              a   cycle-all
//     m   switch to individual
//
//   Individual mode
//     m         switch to group
//     n / p     next / prev LED in iteration order
//     g<digit>  goto LED by index
//     o / f     selected LED on / off
//     O / F     every non-selected LED on / off
//     t         all-toggle (selected = NOT others; alternates on each press)
//
//   Always available
//     s   reprint status block
//     ?   reprint keystroke legend (mode-aware)
//     q   reset the device

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
    bool g_awaitingGotoDigit = false;

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

    bool inIndividual() { return g_ledRuntime.topMode == wiring_test::TopMode::Individual; }

    bool dispatchGroupKey(char c)
    {
        using wiring_test::GroupMode;
        GroupMode mode;
        switch (c)
        {
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
            // 'o' and 'f' overload — see dispatchKey below.
            default:
                return false;
        }
        wiring_test::setGroupMode(g_ledRuntime, mode, millis());
        announceMode();
        return true;
    }

    bool dispatchIndividualKey(char c)
    {
        switch (c)
        {
            case 'n':
                wiring_test::selectNextLed(g_config, g_ledRuntime);
                announceMode();
                return true;
            case 'p':
                wiring_test::selectPrevLed(g_config, g_ledRuntime);
                announceMode();
                return true;
            case 't':
                wiring_test::allToggleSelected(g_config, g_ledRuntime);
                announceMode();
                return true;
            case 'O':
                wiring_test::setAllOthersLit(g_config, g_ledRuntime, /*lit=*/true);
                announceMode();
                return true;
            case 'F':
                wiring_test::setAllOthersLit(g_config, g_ledRuntime, /*lit=*/false);
                announceMode();
                return true;
            case 'g':
                if (g_config.numLeds == 0)
                {
                    Serial.println("(no LEDs configured to goto)");
                    return true;
                }
                g_awaitingGotoDigit = true;
                Serial.printf("goto: type a digit [0..%u]\n",
                              static_cast<unsigned>(g_config.numLeds - 1u));
                return true;
        }
        return false;
    }

    void handleKey(char c)
    {
        if (c == '\r' || c == '\n')
        {
            return;
        }

        // Two-keystroke 'g<digit>' goto — only meaningful in individual mode.
        if (g_awaitingGotoDigit)
        {
            g_awaitingGotoDigit = false;
            if (c >= '0' && c <= '9')
            {
                uint8_t idx = static_cast<uint8_t>(c - '0');
                if (wiring_test::selectLedByIndex(g_config, g_ledRuntime, idx))
                {
                    Serial.printf("selected [%u]\n", static_cast<unsigned>(idx));
                    announceMode();
                }
                else
                {
                    Serial.printf("(no LED [%u] — only %u configured)\n",
                                  static_cast<unsigned>(idx),
                                  static_cast<unsigned>(g_config.numLeds));
                }
            }
            else
            {
                Serial.println("(goto cancelled — non-digit)");
            }
            return;
        }

        // 'm' — toggle between group and individual mode.
        if (c == 'm' || c == 'M')
        {
            if (inIndividual())
            {
                wiring_test::enterGroupMode(g_ledRuntime, millis());
            }
            else
            {
                wiring_test::enterIndividualMode(g_config, g_ledRuntime, millis());
            }
            announceMode();
            return;
        }

        // 'o' / 'f' overload: in group mode → set GroupMode::On / Off.
        // In individual mode → set selected LED on / off.
        if (c == 'o' || c == 'f')
        {
            if (inIndividual())
            {
                wiring_test::setSelectedLit(g_ledRuntime, c == 'o');
                announceMode();
            }
            else
            {
                using wiring_test::GroupMode;
                wiring_test::setGroupMode(
                    g_ledRuntime, c == 'o' ? GroupMode::On : GroupMode::Off, millis());
                announceMode();
            }
            return;
        }

        if (! inIndividual() && dispatchGroupKey(c))
        {
            return;
        }
        if (inIndividual() && dispatchIndividualKey(c))
        {
            return;
        }

        switch (c)
        {
            case 's':
            case 'S':
                announceMode();
                break;
            case '?':
                wiring_test::printHelpLegend(&g_ledRuntime);
                break;
            case 'q':
            case 'Q':
                Serial.println("[wiring-test] resetting on 'q' — see you in a moment.");
                Serial.flush();
                delay(50);
                ESP.restart();
                break;
            default:
                if (c >= 0x20 && c < 0x7f)
                {
                    Serial.printf("(unbound key '%c' — press '?' for the legend)\n", c);
                }
                break;
        }
    }

} // namespace

void setup()
{
    Serial.begin(115200);
    delay(1500);

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
