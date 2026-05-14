// Wiring-test tool — entry point.
//
// This firmware is *not* the production pedal firmware. It boots the
// embedded hardware-config JSON (generated at build time by
// scripts/generate_wiring_config_header.py from the Make target's
// CONFIG=<path>) and drives an interactive serial UI for verifying
// the electrical correctness of a freshly soldered board.
//
//   - TASK-383 — env scaffolding, banner, single-key console
//   - TASK-384 — button press logging + coverage summary  ← this task
//   - TASK-385 — LED group modes (on/off/blinking/chase/cycle-all)
//   - TASK-386 — LED individual mode
//   - TASK-387 — final keystroke bindings + ? legend + builder doc
//
// Provisional bindings used here will be revised in TASK-387:
//   's'  — print summary block (banner + per-button press counters)

#include "button_tracker.h"
#include "status_display.h"
#include "wiring_config.h"
#include "wiring_config_embedded.h"
#include <Arduino.h>

namespace
{

    wiring_test::WiringConfig g_config{};
    wiring_test::ButtonState g_buttonStates[wiring_test::kMaxButtons]{};

    void halt(const char* reason)
    {
        while (true)
        {
            Serial.printf("[wiring-test] FATAL: %s\n", reason);
            delay(2000);
        }
    }

    void handleKey(char c)
    {
        if (c == '\r' || c == '\n')
        {
            return;
        }
        switch (c)
        {
            case 's':
            case 'S':
                wiring_test::printStatusBlock(
                    g_config, wiring_test::CONFIG_FILENAME, g_buttonStates);
                break;
            default:
                if (c >= 0x20 && c < 0x7f)
                {
                    Serial.printf("(unbound key '%c' — bindings land in TASK-385/386/387)\n", c);
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
    wiring_test::printStatusBlock(g_config, wiring_test::CONFIG_FILENAME, g_buttonStates);
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

    // Drain any presses that landed since the last loop iteration.
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
}
