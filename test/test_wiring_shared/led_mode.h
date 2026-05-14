// Wiring-test tool — LED mode state machine (TASK-385 group modes).
//
// Pure: every public function depends only on (cfg, runtime, nowMs)
// — no GPIO writes, no timers, no static state. The firmware entry
// point in test_main.cpp drives ``computeLedLevels`` once per loop
// iteration and writes the resulting array to the GPIOs.
//
// TASK-386 will extend ``LedRuntime`` with an individual-mode
// branch; the existing fields stay backward-compatible so the group
// state survives a round-trip through individual mode.

#pragma once

#include "wiring_config.h"
#include <stdint.h>

namespace wiring_test
{

    enum class GroupMode : uint8_t
    {
        Off = 0,  // 'f' — every LED off
        On,       // 'o' — every LED on
        Blinking, // 'b' — every LED on/off in sync at ~2 Hz
        ChaseOn,  // 'c' — all off, one LED walks the set "on"
        ChaseOff, // 'C' — all on, one LED walks the set "off"
        CycleAll, // 'a' — sweeps every mode above back-to-back
    };

    struct LedRuntime
    {
        GroupMode groupMode;
        uint32_t modeStartMs; // wall-clock anchor for animations
        // Sub-mode pointer used only by GroupMode::CycleAll. Index
        // into the kCycleSubModes table; advances on a fixed cadence.
        uint8_t cycleSubModeIndex;
        uint32_t cycleSubModeStartMs;
    };

    // Period for the blinking and chase animations.
    constexpr uint32_t kBlinkPeriodMs = 500u; // 2 Hz on/off cycle
    constexpr uint32_t kChaseStepMs = 400u;
    // CycleAll sub-mode cadence — long enough to read the active-mode
    // label and watch the animation tick over, short enough that the
    // full sweep finishes in a builder-attention timeframe.
    constexpr uint32_t kCycleSubModeMs = 3000u;
    constexpr uint8_t kCycleSubModeCount = 5;

    // Stable iteration order — sub-modes the cycle-all sweep walks
    // through. Off comes after On so a builder-attentive eye sees
    // "all on" briefly before the first chase, useful as a sanity
    // check that every LED responds.
    extern const GroupMode kCycleSubModes[kCycleSubModeCount];

    const char* groupModeName(GroupMode m);

    void initLedRuntime(LedRuntime& runtime, uint32_t nowMs);
    void setGroupMode(LedRuntime& runtime, GroupMode mode, uint32_t nowMs);

    // Resolve the *effective* mode at a given instant — for plain
    // group modes this is the configured mode; for CycleAll it is
    // the current sub-mode. Pure.
    GroupMode effectiveMode(const LedRuntime& runtime, uint32_t nowMs);

    // Compute the desired physical levels (HIGH/LOW) for every LED in
    // ``cfg.leds[]`` at ``nowMs``. ``levels`` must be sized at least
    // ``cfg.numLeds``. Pure.
    void computeLedLevels(const WiringConfig& cfg,
                          const LedRuntime& runtime,
                          uint32_t nowMs,
                          uint8_t levels[]);

    // Tick CycleAll's sub-mode index forward when its cadence elapses.
    // Caller invokes once per loop iteration; safe to call for non-
    // CycleAll modes (no-op).
    void tickLedRuntime(LedRuntime& runtime, uint32_t nowMs);

} // namespace wiring_test
