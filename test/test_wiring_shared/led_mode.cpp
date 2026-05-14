#include "led_mode.h"

#include <Arduino.h>

namespace wiring_test
{

    const GroupMode kCycleSubModes[kCycleSubModeCount] = {
        GroupMode::On,
        GroupMode::Off,
        GroupMode::Blinking,
        GroupMode::ChaseOn,
        GroupMode::ChaseOff,
    };

    const char* groupModeName(GroupMode m)
    {
        switch (m)
        {
            case GroupMode::Off:
                return "off";
            case GroupMode::On:
                return "on";
            case GroupMode::Blinking:
                return "blinking";
            case GroupMode::ChaseOn:
                return "chase-on";
            case GroupMode::ChaseOff:
                return "chase-off";
            case GroupMode::CycleAll:
                return "cycle-all";
        }
        return "?";
    }

    void initLedRuntime(LedRuntime& runtime, uint32_t nowMs)
    {
        runtime.groupMode = GroupMode::Off;
        runtime.modeStartMs = nowMs;
        runtime.cycleSubModeIndex = 0;
        runtime.cycleSubModeStartMs = nowMs;
    }

    void setGroupMode(LedRuntime& runtime, GroupMode mode, uint32_t nowMs)
    {
        runtime.groupMode = mode;
        runtime.modeStartMs = nowMs;
        runtime.cycleSubModeIndex = 0;
        runtime.cycleSubModeStartMs = nowMs;
    }

    GroupMode effectiveMode(const LedRuntime& runtime, uint32_t nowMs)
    {
        if (runtime.groupMode != GroupMode::CycleAll)
        {
            return runtime.groupMode;
        }
        // CycleAll: the index is whatever ``tickLedRuntime`` last
        // advanced to. Stay defensive against an out-of-range index
        // (would only happen on a future struct mismatch).
        uint8_t idx = runtime.cycleSubModeIndex;
        if (idx >= kCycleSubModeCount)
        {
            idx = 0;
        }
        // Suppress an unused-parameter warning on host builds where
        // nowMs is not consulted directly here; the tick advances
        // off the runtime itself.
        (void) nowMs;
        return kCycleSubModes[idx];
    }

    void tickLedRuntime(LedRuntime& runtime, uint32_t nowMs)
    {
        if (runtime.groupMode != GroupMode::CycleAll)
        {
            return;
        }
        if ((nowMs - runtime.cycleSubModeStartMs) >= kCycleSubModeMs)
        {
            runtime.cycleSubModeIndex =
                static_cast<uint8_t>((runtime.cycleSubModeIndex + 1u) % kCycleSubModeCount);
            runtime.cycleSubModeStartMs = nowMs;
        }
    }

    namespace
    {

        uint8_t levelFor(const WiringLed& led, bool lit)
        {
            if (led.activeHigh)
            {
                return lit ? HIGH : LOW;
            }
            return lit ? LOW : HIGH;
        }

        void fillStatic(const WiringConfig& cfg, uint8_t levels[], bool lit)
        {
            for (uint8_t i = 0; i < cfg.numLeds; i++)
            {
                levels[i] = levelFor(cfg.leds[i], lit);
            }
        }

        void fillBlinking(const WiringConfig& cfg,
                          const LedRuntime& runtime,
                          uint32_t nowMs,
                          uint8_t levels[])
        {
            uint32_t elapsed = nowMs - runtime.modeStartMs;
            bool lit = ((elapsed / kBlinkPeriodMs) % 2u) == 0u;
            fillStatic(cfg, levels, lit);
        }

        void fillChase(const WiringConfig& cfg,
                       const LedRuntime& runtime,
                       uint32_t nowMs,
                       uint8_t levels[],
                       bool baseLit,
                       bool walkerLit)
        {
            if (cfg.numLeds == 0)
            {
                return;
            }
            uint32_t elapsed = nowMs - runtime.modeStartMs;
            uint8_t walker = static_cast<uint8_t>((elapsed / kChaseStepMs) % cfg.numLeds);
            for (uint8_t i = 0; i < cfg.numLeds; i++)
            {
                bool lit = (i == walker) ? walkerLit : baseLit;
                levels[i] = levelFor(cfg.leds[i], lit);
            }
        }

    } // namespace

    void computeLedLevels(const WiringConfig& cfg,
                          const LedRuntime& runtime,
                          uint32_t nowMs,
                          uint8_t levels[])
    {
        GroupMode mode = effectiveMode(runtime, nowMs);
        switch (mode)
        {
            case GroupMode::Off:
                fillStatic(cfg, levels, /*lit=*/false);
                break;
            case GroupMode::On:
                fillStatic(cfg, levels, /*lit=*/true);
                break;
            case GroupMode::Blinking:
                fillBlinking(cfg, runtime, nowMs, levels);
                break;
            case GroupMode::ChaseOn:
                fillChase(cfg, runtime, nowMs, levels, /*baseLit=*/false, /*walkerLit=*/true);
                break;
            case GroupMode::ChaseOff:
                fillChase(cfg, runtime, nowMs, levels, /*baseLit=*/true, /*walkerLit=*/false);
                break;
            case GroupMode::CycleAll:
                // effectiveMode() short-circuits this — should never
                // reach here unless cycleSubModeIndex points at
                // CycleAll itself, which the table above forbids.
                fillStatic(cfg, levels, /*lit=*/false);
                break;
        }
    }

} // namespace wiring_test
