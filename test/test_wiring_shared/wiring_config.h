// Wiring-test tool — parsed hardware-config types.
//
// The wiring tool ingests a hardware-config JSON at *build time* (one
// recompile per board) and produces a self-describing list of buttons
// and LEDs the firmware then drives. No LittleFS, no runtime config
// loader — that lives in the production firmware. See EPIC-030.

#pragma once

#include <stdint.h>

namespace wiring_test
{

    constexpr uint8_t kMaxButtons = 27; // 26 action buttons + 1 select
    constexpr uint8_t kMaxLeds = 10;    // power + bluetooth + up to 6 select + slack
    constexpr uint8_t kMaxNameLen = 24;

    struct WiringButton
    {
        char name[kMaxNameLen];
        uint8_t pin;
        bool activeLow; // project convention: INPUT_PULLUP, press = LOW
        bool isSelect;
    };

    struct WiringLed
    {
        char name[kMaxNameLen];
        uint8_t pin;
        bool activeHigh; // project convention: HIGH = lit
    };

    struct WiringConfig
    {
        char hardware[16];
        WiringButton buttons[kMaxButtons];
        uint8_t numButtons;
        WiringLed leds[kMaxLeds];
        uint8_t numLeds;
    };

    enum class ParseStatus
    {
        Ok,
        InvalidJson,
        MissingField,
        Overflow,
    };

    struct ParseResult
    {
        ParseStatus status;
        char message[96];
    };

    // Parses the embedded JSON into ``out``. On any failure, ``status``
    // is non-Ok and ``message`` carries a one-line diagnostic suitable for
    // printing on the serial console.
    ParseResult parseWiringConfig(const char* json, WiringConfig& out);

} // namespace wiring_test
