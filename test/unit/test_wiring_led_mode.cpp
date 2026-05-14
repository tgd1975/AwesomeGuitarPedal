// Host tests for the wiring tool's LED group-mode state machine
// (TASK-385). The state machine is pure: every test drives it
// directly with a synthetic config and a synthetic time, no GPIO
// fakes needed.

#include "led_mode.h"
#include "wiring_config.h"

#include <Arduino.h>
#include <gtest/gtest.h>
#include <string.h>

using wiring_test::computeLedLevels;
using wiring_test::effectiveMode;
using wiring_test::GroupMode;
using wiring_test::initLedRuntime;
using wiring_test::kBlinkPeriodMs;
using wiring_test::kChaseStepMs;
using wiring_test::kCycleSubModeCount;
using wiring_test::kCycleSubModeMs;
using wiring_test::kCycleSubModes;
using wiring_test::LedRuntime;
using wiring_test::setGroupMode;
using wiring_test::tickLedRuntime;
using wiring_test::WiringConfig;
using wiring_test::WiringLed;

namespace
{

    WiringConfig makeConfig(uint8_t numLeds)
    {
        WiringConfig cfg{};
        for (uint8_t i = 0; i < numLeds; i++)
        {
            cfg.leds[i].pin = static_cast<uint8_t>(2 + i);
            cfg.leds[i].activeHigh = true;
            snprintf(cfg.leds[i].name, sizeof(cfg.leds[i].name), "led%u", static_cast<unsigned>(i));
        }
        cfg.numLeds = numLeds;
        return cfg;
    }

    void expectAll(const uint8_t* levels, uint8_t numLeds, uint8_t expected)
    {
        for (uint8_t i = 0; i < numLeds; i++)
        {
            EXPECT_EQ(levels[i], expected) << "led " << static_cast<int>(i);
        }
    }

    uint8_t countAtLevel(const uint8_t* levels, uint8_t numLeds, uint8_t level)
    {
        uint8_t n = 0;
        for (uint8_t i = 0; i < numLeds; i++)
        {
            if (levels[i] == level)
            {
                n++;
            }
        }
        return n;
    }

} // namespace

TEST(LedMode, OnLightsAllLeds)
{
    auto cfg = makeConfig(5);
    LedRuntime r{};
    initLedRuntime(r, 0);
    setGroupMode(r, GroupMode::On, 0);

    uint8_t levels[wiring_test::kMaxLeds]{};
    computeLedLevels(cfg, r, 100, levels);
    expectAll(levels, cfg.numLeds, HIGH);
}

TEST(LedMode, OffDarkensAllLeds)
{
    auto cfg = makeConfig(5);
    LedRuntime r{};
    initLedRuntime(r, 0);
    setGroupMode(r, GroupMode::Off, 0);

    uint8_t levels[wiring_test::kMaxLeds]{};
    computeLedLevels(cfg, r, 100, levels);
    expectAll(levels, cfg.numLeds, LOW);
}

TEST(LedMode, ActiveLowLedsInvertedForOnAndOff)
{
    auto cfg = makeConfig(2);
    cfg.leds[0].activeHigh = false;
    cfg.leds[1].activeHigh = false;

    LedRuntime r{};
    initLedRuntime(r, 0);

    uint8_t levels[wiring_test::kMaxLeds]{};
    setGroupMode(r, GroupMode::On, 0);
    computeLedLevels(cfg, r, 100, levels);
    EXPECT_EQ(levels[0], LOW);
    EXPECT_EQ(levels[1], LOW);

    setGroupMode(r, GroupMode::Off, 0);
    computeLedLevels(cfg, r, 100, levels);
    EXPECT_EQ(levels[0], HIGH);
    EXPECT_EQ(levels[1], HIGH);
}

TEST(LedMode, BlinkingTogglesOnPeriodBoundary)
{
    auto cfg = makeConfig(3);
    LedRuntime r{};
    initLedRuntime(r, 0);
    setGroupMode(r, GroupMode::Blinking, 0);

    uint8_t levels[wiring_test::kMaxLeds]{};

    // First half-period — all LEDs lit.
    computeLedLevels(cfg, r, 0, levels);
    expectAll(levels, cfg.numLeds, HIGH);
    computeLedLevels(cfg, r, kBlinkPeriodMs - 1, levels);
    expectAll(levels, cfg.numLeds, HIGH);

    // Second half-period — all LEDs dark.
    computeLedLevels(cfg, r, kBlinkPeriodMs, levels);
    expectAll(levels, cfg.numLeds, LOW);
    computeLedLevels(cfg, r, 2 * kBlinkPeriodMs - 1, levels);
    expectAll(levels, cfg.numLeds, LOW);

    // Third half-period — back to lit.
    computeLedLevels(cfg, r, 2 * kBlinkPeriodMs, levels);
    expectAll(levels, cfg.numLeds, HIGH);
}

TEST(LedMode, ChaseOnWalksWalkerInDeclaredOrder)
{
    auto cfg = makeConfig(4);
    LedRuntime r{};
    initLedRuntime(r, 100);
    setGroupMode(r, GroupMode::ChaseOn, 100);

    uint8_t levels[wiring_test::kMaxLeds]{};

    // Step 0 — walker on led 0, others off.
    computeLedLevels(cfg, r, 100, levels);
    EXPECT_EQ(levels[0], HIGH);
    EXPECT_EQ(levels[1], LOW);
    EXPECT_EQ(levels[2], LOW);
    EXPECT_EQ(levels[3], LOW);
    EXPECT_EQ(countAtLevel(levels, cfg.numLeds, HIGH), 1);

    // Step 1.
    computeLedLevels(cfg, r, 100 + kChaseStepMs, levels);
    EXPECT_EQ(levels[1], HIGH);
    EXPECT_EQ(countAtLevel(levels, cfg.numLeds, HIGH), 1);

    // Step 3 — walker on last LED.
    computeLedLevels(cfg, r, 100 + 3 * kChaseStepMs, levels);
    EXPECT_EQ(levels[3], HIGH);
    EXPECT_EQ(countAtLevel(levels, cfg.numLeds, HIGH), 1);

    // Step 4 — wraps back to led 0.
    computeLedLevels(cfg, r, 100 + 4 * kChaseStepMs, levels);
    EXPECT_EQ(levels[0], HIGH);
    EXPECT_EQ(countAtLevel(levels, cfg.numLeds, HIGH), 1);
}

TEST(LedMode, ChaseOffWalksWalkerDarkAmongLitNeighbours)
{
    auto cfg = makeConfig(4);
    LedRuntime r{};
    initLedRuntime(r, 0);
    setGroupMode(r, GroupMode::ChaseOff, 0);

    uint8_t levels[wiring_test::kMaxLeds]{};

    computeLedLevels(cfg, r, 0, levels);
    EXPECT_EQ(levels[0], LOW);
    EXPECT_EQ(levels[1], HIGH);
    EXPECT_EQ(levels[2], HIGH);
    EXPECT_EQ(levels[3], HIGH);
    EXPECT_EQ(countAtLevel(levels, cfg.numLeds, LOW), 1);

    computeLedLevels(cfg, r, 2 * kChaseStepMs, levels);
    EXPECT_EQ(levels[2], LOW);
    EXPECT_EQ(countAtLevel(levels, cfg.numLeds, LOW), 1);
}

TEST(LedMode, SwitchingFromChaseOffToOnClearsStaleDarkness)
{
    auto cfg = makeConfig(4);
    LedRuntime r{};
    initLedRuntime(r, 0);
    setGroupMode(r, GroupMode::ChaseOff, 0);

    uint8_t levels[wiring_test::kMaxLeds]{};
    computeLedLevels(cfg, r, kChaseStepMs, levels);
    // led 1 is the walker — currently dark.
    EXPECT_EQ(levels[1], LOW);

    // Switch to On — the walker LED must light up immediately.
    setGroupMode(r, GroupMode::On, kChaseStepMs);
    computeLedLevels(cfg, r, kChaseStepMs, levels);
    expectAll(levels, cfg.numLeds, HIGH);
}

TEST(LedMode, CycleAllAdvancesSubModeOnSchedule)
{
    LedRuntime r{};
    initLedRuntime(r, 0);
    setGroupMode(r, GroupMode::CycleAll, 0);

    EXPECT_EQ(effectiveMode(r, 0), kCycleSubModes[0]);
    tickLedRuntime(r, kCycleSubModeMs - 1);
    EXPECT_EQ(effectiveMode(r, kCycleSubModeMs - 1), kCycleSubModes[0]);

    tickLedRuntime(r, kCycleSubModeMs);
    EXPECT_EQ(effectiveMode(r, kCycleSubModeMs), kCycleSubModes[1]);

    tickLedRuntime(r, 2 * kCycleSubModeMs);
    EXPECT_EQ(effectiveMode(r, 2 * kCycleSubModeMs), kCycleSubModes[2]);

    // Wrap.
    for (uint8_t i = 0; i < kCycleSubModeCount; i++)
    {
        tickLedRuntime(r, (3u + i) * kCycleSubModeMs);
    }
    EXPECT_EQ(effectiveMode(r, kCycleSubModeCount * kCycleSubModeMs + kCycleSubModeMs / 2),
              kCycleSubModes[(3u + kCycleSubModeCount - 1u) % kCycleSubModeCount]);
}

TEST(LedMode, TickIsNoopForNonCycleAllModes)
{
    LedRuntime r{};
    initLedRuntime(r, 0);
    setGroupMode(r, GroupMode::Blinking, 0);
    uint8_t before = r.cycleSubModeIndex;
    tickLedRuntime(r, 10 * kCycleSubModeMs);
    EXPECT_EQ(r.cycleSubModeIndex, before);
    EXPECT_EQ(r.groupMode, GroupMode::Blinking);
}

TEST(LedMode, ZeroLedsConfigIsHandledCleanly)
{
    auto cfg = makeConfig(0);
    LedRuntime r{};
    initLedRuntime(r, 0);
    setGroupMode(r, GroupMode::ChaseOn, 0);

    uint8_t levels[wiring_test::kMaxLeds]{};
    // Just must not crash; no levels to assert.
    computeLedLevels(cfg, r, 0, levels);
    SUCCEED();
}

TEST(LedMode, ModeNamesMatchExpectedLabels)
{
    using wiring_test::groupModeName;
    EXPECT_STREQ(groupModeName(GroupMode::Off), "off");
    EXPECT_STREQ(groupModeName(GroupMode::On), "on");
    EXPECT_STREQ(groupModeName(GroupMode::Blinking), "blinking");
    EXPECT_STREQ(groupModeName(GroupMode::ChaseOn), "chase-on");
    EXPECT_STREQ(groupModeName(GroupMode::ChaseOff), "chase-off");
    EXPECT_STREQ(groupModeName(GroupMode::CycleAll), "cycle-all");
}
