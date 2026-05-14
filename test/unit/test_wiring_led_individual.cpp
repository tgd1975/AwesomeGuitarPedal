// Host tests for the wiring tool's individual LED mode (TASK-386).
// The tested ops are pure transforms on LedRuntime; tests drive
// the runtime directly and assert via computeLedLevels().

#include "led_mode.h"
#include "wiring_config.h"

#include <Arduino.h>
#include <gtest/gtest.h>
#include <string.h>

using wiring_test::allToggleSelected;
using wiring_test::computeLedLevels;
using wiring_test::enterGroupMode;
using wiring_test::enterIndividualMode;
using wiring_test::GroupMode;
using wiring_test::initLedRuntime;
using wiring_test::LedRuntime;
using wiring_test::selectLedByIndex;
using wiring_test::selectNextLed;
using wiring_test::selectPrevLed;
using wiring_test::setAllOthersLit;
using wiring_test::setGroupMode;
using wiring_test::setSelectedLit;
using wiring_test::TopMode;
using wiring_test::WiringConfig;

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

    uint8_t levelsCountAt(const WiringConfig& cfg, const LedRuntime& r, uint8_t target)
    {
        uint8_t levels[wiring_test::kMaxLeds]{};
        computeLedLevels(cfg, r, 0, levels);
        uint8_t n = 0;
        for (uint8_t i = 0; i < cfg.numLeds; i++)
        {
            if (levels[i] == target)
            {
                n++;
            }
        }
        return n;
    }

} // namespace

TEST(LedIndividual, EnterIndividualSnapshotsCurrentLevels)
{
    auto cfg = makeConfig(4);
    LedRuntime r{};
    initLedRuntime(r, 0);
    setGroupMode(r, GroupMode::On, 0);

    enterIndividualMode(cfg, r, 0);
    EXPECT_EQ(r.topMode, TopMode::Individual);
    // All LEDs were lit in group On — individual snapshot must agree.
    for (uint8_t i = 0; i < cfg.numLeds; i++)
    {
        EXPECT_TRUE(r.individualLit[i]);
    }
    EXPECT_EQ(levelsCountAt(cfg, r, HIGH), cfg.numLeds);
}

TEST(LedIndividual, NextWrapsAtEnd)
{
    auto cfg = makeConfig(3);
    LedRuntime r{};
    initLedRuntime(r, 0);
    enterIndividualMode(cfg, r, 0);
    EXPECT_EQ(r.selectedLed, 0);
    selectNextLed(cfg, r);
    EXPECT_EQ(r.selectedLed, 1);
    selectNextLed(cfg, r);
    EXPECT_EQ(r.selectedLed, 2);
    selectNextLed(cfg, r);
    EXPECT_EQ(r.selectedLed, 0);
}

TEST(LedIndividual, PrevWrapsAtZero)
{
    auto cfg = makeConfig(3);
    LedRuntime r{};
    initLedRuntime(r, 0);
    enterIndividualMode(cfg, r, 0);
    EXPECT_EQ(r.selectedLed, 0);
    selectPrevLed(cfg, r);
    EXPECT_EQ(r.selectedLed, 2);
    selectPrevLed(cfg, r);
    EXPECT_EQ(r.selectedLed, 1);
}

TEST(LedIndividual, SelectByIndexInRangeWorks)
{
    auto cfg = makeConfig(5);
    LedRuntime r{};
    initLedRuntime(r, 0);
    enterIndividualMode(cfg, r, 0);
    EXPECT_TRUE(selectLedByIndex(cfg, r, 3));
    EXPECT_EQ(r.selectedLed, 3);
}

TEST(LedIndividual, SelectByIndexOutOfRangeRejected)
{
    auto cfg = makeConfig(3);
    LedRuntime r{};
    initLedRuntime(r, 0);
    enterIndividualMode(cfg, r, 0);
    selectLedByIndex(cfg, r, 1);
    EXPECT_FALSE(selectLedByIndex(cfg, r, 99));
    EXPECT_EQ(r.selectedLed, 1); // unchanged
}

TEST(LedIndividual, SetSelectedLitOnlyAffectsSelected)
{
    auto cfg = makeConfig(3);
    LedRuntime r{};
    initLedRuntime(r, 0);
    enterIndividualMode(cfg, r, 0); // start all-off (group default was Off)

    selectLedByIndex(cfg, r, 1);
    setSelectedLit(r, true);

    EXPECT_FALSE(r.individualLit[0]);
    EXPECT_TRUE(r.individualLit[1]);
    EXPECT_FALSE(r.individualLit[2]);
}

TEST(LedIndividual, SetAllOthersLitDoesNotTouchSelected)
{
    auto cfg = makeConfig(4);
    LedRuntime r{};
    initLedRuntime(r, 0);
    enterIndividualMode(cfg, r, 0);

    selectLedByIndex(cfg, r, 2);
    setSelectedLit(r, false); // selected explicitly off
    setAllOthersLit(cfg, r, true);

    EXPECT_TRUE(r.individualLit[0]);
    EXPECT_TRUE(r.individualLit[1]);
    EXPECT_FALSE(r.individualLit[2]); // selected unchanged
    EXPECT_TRUE(r.individualLit[3]);
}

TEST(LedIndividual, AllToggleAlternatesParity)
{
    auto cfg = makeConfig(4);
    LedRuntime r{};
    initLedRuntime(r, 0);
    enterIndividualMode(cfg, r, 0);
    selectLedByIndex(cfg, r, 1);

    // First press: others lit, selected dark.
    allToggleSelected(cfg, r);
    EXPECT_TRUE(r.individualLit[0]);
    EXPECT_FALSE(r.individualLit[1]);
    EXPECT_TRUE(r.individualLit[2]);
    EXPECT_TRUE(r.individualLit[3]);

    // Second press: flipped — others dark, selected lit.
    allToggleSelected(cfg, r);
    EXPECT_FALSE(r.individualLit[0]);
    EXPECT_TRUE(r.individualLit[1]);
    EXPECT_FALSE(r.individualLit[2]);
    EXPECT_FALSE(r.individualLit[3]);

    // Third press: back to first.
    allToggleSelected(cfg, r);
    EXPECT_TRUE(r.individualLit[0]);
    EXPECT_FALSE(r.individualLit[1]);
}

TEST(LedIndividual, GroupRoundTripPreservesGroupMode)
{
    auto cfg = makeConfig(3);
    LedRuntime r{};
    initLedRuntime(r, 0);
    setGroupMode(r, GroupMode::Blinking, 100);

    // Switch to individual and back. Group mode must stay Blinking.
    enterIndividualMode(cfg, r, 200);
    EXPECT_EQ(r.topMode, TopMode::Individual);
    EXPECT_EQ(r.groupMode, GroupMode::Blinking);

    // Twiddle the selection to confirm group state isn't affected.
    selectNextLed(cfg, r);
    setSelectedLit(r, true);

    enterGroupMode(r, 300);
    EXPECT_EQ(r.topMode, TopMode::Group);
    EXPECT_EQ(r.groupMode, GroupMode::Blinking);
}

TEST(LedIndividual, SelectionPersistsAcrossModeRoundTrip)
{
    auto cfg = makeConfig(5);
    LedRuntime r{};
    initLedRuntime(r, 0);
    enterIndividualMode(cfg, r, 0);
    selectLedByIndex(cfg, r, 3);

    enterGroupMode(r, 100);
    enterIndividualMode(cfg, r, 200);
    EXPECT_EQ(r.selectedLed, 3);
}

TEST(LedIndividual, ComputeLevelsHonoursIndividualState)
{
    auto cfg = makeConfig(3);
    LedRuntime r{};
    initLedRuntime(r, 0);
    enterIndividualMode(cfg, r, 0);
    selectLedByIndex(cfg, r, 0);
    setSelectedLit(r, true);
    selectLedByIndex(cfg, r, 2);
    setSelectedLit(r, true);

    uint8_t levels[wiring_test::kMaxLeds]{};
    computeLedLevels(cfg, r, 1000, levels);
    EXPECT_EQ(levels[0], HIGH);
    EXPECT_EQ(levels[1], LOW);
    EXPECT_EQ(levels[2], HIGH);
}

TEST(LedIndividual, ComputeLevelsRespectsActiveLowLeds)
{
    auto cfg = makeConfig(2);
    cfg.leds[0].activeHigh = false;
    cfg.leds[1].activeHigh = false;

    LedRuntime r{};
    initLedRuntime(r, 0);
    enterIndividualMode(cfg, r, 0);
    selectLedByIndex(cfg, r, 0);
    setSelectedLit(r, true);

    uint8_t levels[wiring_test::kMaxLeds]{};
    computeLedLevels(cfg, r, 0, levels);
    EXPECT_EQ(levels[0], LOW);  // active-low + lit
    EXPECT_EQ(levels[1], HIGH); // active-low + dark
}

TEST(LedIndividual, EmptyConfigSelectionAndOpsAreSafe)
{
    auto cfg = makeConfig(0);
    LedRuntime r{};
    initLedRuntime(r, 0);
    enterIndividualMode(cfg, r, 0);
    selectNextLed(cfg, r); // no-op
    selectPrevLed(cfg, r);
    EXPECT_FALSE(selectLedByIndex(cfg, r, 0));
    setAllOthersLit(cfg, r, true); // no-op
    allToggleSelected(cfg, r);
    SUCCEED();
}
