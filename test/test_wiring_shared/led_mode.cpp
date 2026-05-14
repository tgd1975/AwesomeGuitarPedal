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
        runtime.topMode = TopMode::Group;
        runtime.groupMode = GroupMode::Off;
        runtime.modeStartMs = nowMs;
        runtime.cycleSubModeIndex = 0;
        runtime.cycleSubModeStartMs = nowMs;
        runtime.selectedLed = 0;
        for (uint8_t i = 0; i < kMaxLeds; i++)
        {
            runtime.individualLit[i] = false;
        }
        runtime.allToggleParity = false;
    }

    void setGroupMode(LedRuntime& runtime, GroupMode mode, uint32_t nowMs)
    {
        runtime.topMode = TopMode::Group;
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
        if (runtime.topMode != TopMode::Group)
        {
            return;
        }
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
        if (runtime.topMode == TopMode::Individual)
        {
            for (uint8_t i = 0; i < cfg.numLeds; i++)
            {
                levels[i] = levelFor(cfg.leds[i], runtime.individualLit[i]);
            }
            return;
        }

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

    void enterIndividualMode(const WiringConfig& cfg, LedRuntime& runtime, uint32_t nowMs)
    {
        // Snapshot the current group-mode levels so the LEDs hold
        // their last visible state across the mode switch.
        uint8_t levels[kMaxLeds]{};
        computeLedLevels(cfg, runtime, nowMs, levels);
        for (uint8_t i = 0; i < cfg.numLeds; i++)
        {
            runtime.individualLit[i] = (levels[i] == levelFor(cfg.leds[i], /*lit=*/true));
        }
        runtime.topMode = TopMode::Individual;
        if (runtime.selectedLed >= cfg.numLeds && cfg.numLeds > 0)
        {
            runtime.selectedLed = 0;
        }
    }

    void enterGroupMode(LedRuntime& runtime, uint32_t nowMs)
    {
        runtime.topMode = TopMode::Group;
        // Reset the group animation anchor so the first frame of the
        // resumed mode starts cleanly (otherwise blink/chase would
        // freeze-frame on whatever phase they were in pre-individual).
        runtime.modeStartMs = nowMs;
        runtime.cycleSubModeStartMs = nowMs;
    }

    void selectNextLed(const WiringConfig& cfg, LedRuntime& runtime)
    {
        if (cfg.numLeds == 0)
        {
            return;
        }
        runtime.selectedLed = static_cast<uint8_t>((runtime.selectedLed + 1u) % cfg.numLeds);
    }

    void selectPrevLed(const WiringConfig& cfg, LedRuntime& runtime)
    {
        if (cfg.numLeds == 0)
        {
            return;
        }
        runtime.selectedLed =
            static_cast<uint8_t>((runtime.selectedLed + cfg.numLeds - 1u) % cfg.numLeds);
    }

    bool selectLedByIndex(const WiringConfig& cfg, LedRuntime& runtime, uint8_t index)
    {
        if (index >= cfg.numLeds)
        {
            return false;
        }
        runtime.selectedLed = index;
        return true;
    }

    void setSelectedLit(LedRuntime& runtime, bool lit)
    {
        runtime.individualLit[runtime.selectedLed] = lit;
    }

    void setAllOthersLit(const WiringConfig& cfg, LedRuntime& runtime, bool lit)
    {
        for (uint8_t i = 0; i < cfg.numLeds; i++)
        {
            if (i != runtime.selectedLed)
            {
                runtime.individualLit[i] = lit;
            }
        }
    }

    void allToggleSelected(const WiringConfig& cfg, LedRuntime& runtime)
    {
        // Each press flips the polarity so successive 't' presses
        // alternate (others-on/selected-off ↔ others-off/selected-on).
        bool othersLit = ! runtime.allToggleParity;
        for (uint8_t i = 0; i < cfg.numLeds; i++)
        {
            runtime.individualLit[i] = (i == runtime.selectedLed) ? ! othersLit : othersLit;
        }
        runtime.allToggleParity = ! runtime.allToggleParity;
    }

} // namespace wiring_test
