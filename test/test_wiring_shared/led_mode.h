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

    enum class TopMode : uint8_t
    {
        Group = 0,  // group-mode state machine (TASK-385)
        Individual, // per-LED selection + drive (TASK-386)
    };

    struct LedRuntime
    {
        TopMode topMode;
        // Group-mode bookkeeping (TASK-385). Survives a round-trip
        // through individual mode untouched, so 'm' restores cleanly.
        GroupMode groupMode;
        uint32_t modeStartMs; // wall-clock anchor for animations
        uint8_t cycleSubModeIndex;
        uint32_t cycleSubModeStartMs;
        // Individual-mode bookkeeping (TASK-386). selectedLed indexes
        // cfg.leds[]. ``individualLit[i]`` carries the user-set state
        // for LED i — populated from the live group-mode levels on
        // first transition into individual mode so the switch is not
        // jarring. ``allToggleParity`` flips on every 't' so successive
        // presses alternate (others-on/selected-off ↔ others-off/
        // selected-on).
        uint8_t selectedLed;
        bool individualLit[kMaxLeds];
        bool allToggleParity;
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

    // Resolve the *effective* group mode at a given instant — for
    // plain group modes this is the configured mode; for CycleAll
    // it is the current sub-mode. Pure. Only meaningful in
    // ``TopMode::Group``; individual mode short-circuits the level
    // computation.
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
    // CycleAll modes (no-op). Also no-op in individual mode.
    void tickLedRuntime(LedRuntime& runtime, uint32_t nowMs);

    // Individual-mode operations (TASK-386). All are pure transforms
    // on ``LedRuntime``; the firmware drives writes via
    // computeLedLevels() on the next loop iteration.

    // Enter individual mode. Snapshots the current group-mode levels
    // into ``individualLit[]`` so the LEDs do not flicker on switch.
    void enterIndividualMode(const WiringConfig& cfg, LedRuntime& runtime, uint32_t nowMs);
    // Resume the previously configured group mode. The group state
    // (mode, animation start, cycle index) was preserved.
    void enterGroupMode(LedRuntime& runtime, uint32_t nowMs);

    void selectNextLed(const WiringConfig& cfg, LedRuntime& runtime);
    void selectPrevLed(const WiringConfig& cfg, LedRuntime& runtime);
    // Returns false if the index is out of range; selection unchanged.
    bool selectLedByIndex(const WiringConfig& cfg, LedRuntime& runtime, uint8_t index);

    void setSelectedLit(LedRuntime& runtime, bool lit);
    void setAllOthersLit(const WiringConfig& cfg, LedRuntime& runtime, bool lit);
    // Sets every non-selected LED to one state and the selected LED
    // to its opposite. Successive calls flip both — see
    // ``allToggleParity`` on LedRuntime.
    void allToggleSelected(const WiringConfig& cfg, LedRuntime& runtime);

} // namespace wiring_test
