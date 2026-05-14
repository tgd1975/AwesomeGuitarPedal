// Wiring-test tool — polled, debounced button state tracker.
//
// IDEA-062 explicitly avoids ISRs here: the wiring tool is a debugging
// firmware run for minutes at a time, and a polled tracker keeps
// host-test parity (the Arduino fake_gpio shim drives ``digitalRead``
// + ``millis`` directly, no ISR machinery to mock).
//
// Each call to ``pollButtonTracker`` reads every configured button,
// applies a debounce window, and emits at most one press event per
// call. Caller polls in a tight loop; latency on a human button press
// is bounded only by loop iteration time (sub-millisecond on ESP32),
// well below the configured debounce window.

#pragma once

#include "wiring_config.h"
#include <stddef.h>
#include <stdint.h>

namespace wiring_test
{

    struct ButtonState
    {
        uint8_t lastRaw;        // last raw digitalRead() result
        uint8_t debouncedLevel; // last accepted debounced level
        uint32_t lastChangeMs;  // millis() at last raw-level change
        uint32_t pressCount;    // accepted (debounced) presses since boot
    };

    struct PressEvent
    {
        bool fired;
        uint8_t buttonIndex;    // index into cfg.buttons[]
        uint8_t debouncedLevel; // level after the transition (HIGH or LOW)
    };

    // Configure each pin as INPUT_PULLUP (project convention is
    // active-low buttons) and prime ButtonState with the current pin
    // level so the first poll does not synthesise a phantom press.
    void initButtonTracker(const WiringConfig& cfg, ButtonState states[]);

    // Poll all configured buttons. Returns at most one new press
    // event per call. Caller should loop while ``fired`` is true.
    PressEvent pollButtonTracker(const WiringConfig& cfg,
                                 ButtonState states[],
                                 uint32_t debounceMs,
                                 uint32_t nowMs);

    // Pure formatter — extracted so host tests can pin the wire format
    // without booting an Arduino fake. Returns the number of bytes
    // written (excluding the null terminator). The output is one line
    // (terminated by '\n') ready to feed to Serial.
    size_t
    formatPressLine(char* buf, size_t bufLen, const WiringButton& button, uint8_t debouncedLevel);

} // namespace wiring_test
