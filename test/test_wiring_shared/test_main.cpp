// Wiring-test tool — entry point (TASK-383 skeleton).
//
// This firmware is *not* the production pedal firmware. It boots the
// embedded hardware-config JSON (generated at build time by
// scripts/generate_wiring_config_header.py from the Make target's
// CONFIG=<path>) and prints a banner describing the configured
// buttons and LEDs. Subsequent tasks fill in the interactive surface:
//   - TASK-384 — button press logging + coverage summary
//   - TASK-385 — LED group modes (on/off/blinking/chase/cycle-all)
//   - TASK-386 — LED individual mode
//   - TASK-387 — final keystroke bindings + ? legend + builder doc
//
// For now, every keystroke is acknowledged on the serial console so a
// builder can verify their terminal is delivering single chars (no
// line buffering) before the real bindings land.

#include "status_display.h"
#include "wiring_config.h"
#include "wiring_config_embedded.h"
#include <Arduino.h>

namespace
{

    wiring_test::WiringConfig g_config{};

    void halt(const char* reason)
    {
        while (true)
        {
            Serial.printf("[wiring-test] FATAL: %s\n", reason);
            delay(2000);
        }
    }

    void echoKey(char c)
    {
        if (c == '\r' || c == '\n')
        {
            return;
        }
        if (c >= 0x20 && c < 0x7f)
        {
            Serial.printf("key: '%c' (0x%02x)\n", c, static_cast<unsigned>(c) & 0xffu);
        }
        else
        {
            Serial.printf("key: 0x%02x\n", static_cast<unsigned>(c) & 0xffu);
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

    wiring_test::printBanner(g_config, wiring_test::CONFIG_FILENAME);
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
        echoKey(static_cast<char>(ch));
    }
}
