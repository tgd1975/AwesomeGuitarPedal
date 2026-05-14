// Host tests for the wiring tool's polled button tracker (TASK-384).
//
// Drives the Arduino fake_gpio shim's pin states + fake_time::value
// to exercise the debouncer's edge-detect, debounce-window, and
// counter-increment behaviour without booting an Arduino.

#include "button_tracker.h"
#include "wiring_config.h"

#include <Arduino.h>
#include <gtest/gtest.h>
#include <string.h>

using wiring_test::ButtonState;
using wiring_test::formatPressLine;
using wiring_test::initButtonTracker;
using wiring_test::pollButtonTracker;
using wiring_test::WiringButton;
using wiring_test::WiringConfig;

namespace
{

    WiringConfig makeConfig(uint8_t pinA, uint8_t pinB)
    {
        WiringConfig cfg{};
        strncpy(cfg.buttons[0].name, "A", sizeof(cfg.buttons[0].name) - 1);
        cfg.buttons[0].pin = pinA;
        cfg.buttons[0].activeLow = true;
        cfg.buttons[0].isSelect = false;
        strncpy(cfg.buttons[1].name, "B", sizeof(cfg.buttons[1].name) - 1);
        cfg.buttons[1].pin = pinB;
        cfg.buttons[1].activeLow = true;
        cfg.buttons[1].isSelect = false;
        cfg.numButtons = 2;
        cfg.debounceMs = 50;
        return cfg;
    }

    void setLevel(uint8_t pin, int level, uint32_t nowMs)
    {
        fake_time::value = nowMs;
        fake_gpio::setPinState(pin, level);
    }

    class ButtonTrackerTest : public ::testing::Test
    {
    protected:
        void SetUp() override
        {
            fake_gpio::reset();
            fake_time::value = 0;
        }
    };

} // namespace

TEST_F(ButtonTrackerTest, IdleAtBootEmitsNoEvents)
{
    auto cfg = makeConfig(13, 14);
    fake_gpio::setPinState(13, HIGH);
    fake_gpio::setPinState(14, HIGH);

    ButtonState states[wiring_test::kMaxButtons]{};
    initButtonTracker(cfg, states);

    for (uint32_t t = 1; t < 500; t += 10)
    {
        fake_time::value = t;
        auto event = pollButtonTracker(cfg, states, cfg.debounceMs, t);
        EXPECT_FALSE(event.fired);
    }
    EXPECT_EQ(states[0].pressCount, 0u);
    EXPECT_EQ(states[1].pressCount, 0u);
}

TEST_F(ButtonTrackerTest, SinglePressEmitsExactlyOneEvent)
{
    auto cfg = makeConfig(13, 14);
    fake_gpio::setPinState(13, HIGH);
    fake_gpio::setPinState(14, HIGH);

    ButtonState states[wiring_test::kMaxButtons]{};
    initButtonTracker(cfg, states);

    // Press button A at t=100; hold past debounce window.
    setLevel(13, LOW, 100);
    auto e1 = pollButtonTracker(cfg, states, cfg.debounceMs, 100); // raw change recorded
    EXPECT_FALSE(e1.fired);

    // Inside debounce window — no event yet.
    fake_time::value = 130;
    auto e2 = pollButtonTracker(cfg, states, cfg.debounceMs, 130);
    EXPECT_FALSE(e2.fired);

    // Past debounce window — event fires.
    fake_time::value = 160;
    auto e3 = pollButtonTracker(cfg, states, cfg.debounceMs, 160);
    EXPECT_TRUE(e3.fired);
    EXPECT_EQ(e3.buttonIndex, 0);
    EXPECT_EQ(e3.debouncedLevel, LOW);
    EXPECT_EQ(states[0].pressCount, 1u);

    // Subsequent polls while still pressed — no new event.
    for (uint32_t t = 200; t < 400; t += 10)
    {
        fake_time::value = t;
        auto e = pollButtonTracker(cfg, states, cfg.debounceMs, t);
        EXPECT_FALSE(e.fired);
    }
    EXPECT_EQ(states[0].pressCount, 1u);
}

TEST_F(ButtonTrackerTest, ChatterDuringDebounceWindowDoesNotProducePresses)
{
    auto cfg = makeConfig(13, 14);
    fake_gpio::setPinState(13, HIGH);
    fake_gpio::setPinState(14, HIGH);

    ButtonState states[wiring_test::kMaxButtons]{};
    initButtonTracker(cfg, states);

    // Bouncing for 40 ms (less than the 50 ms debounce window).
    for (uint32_t t = 0; t < 40; t += 5)
    {
        setLevel(13, (t / 5) % 2 == 0 ? LOW : HIGH, t);
        auto e = pollButtonTracker(cfg, states, cfg.debounceMs, t);
        EXPECT_FALSE(e.fired);
    }
    // Settle LOW and wait past debounce.
    setLevel(13, LOW, 50);
    pollButtonTracker(cfg, states, cfg.debounceMs, 50);
    fake_time::value = 110;
    auto e = pollButtonTracker(cfg, states, cfg.debounceMs, 110);
    EXPECT_TRUE(e.fired);
    EXPECT_EQ(states[0].pressCount, 1u);
}

TEST_F(ButtonTrackerTest, PressReleasePressIncrementsCounterTwice)
{
    auto cfg = makeConfig(13, 14);
    fake_gpio::setPinState(13, HIGH);
    fake_gpio::setPinState(14, HIGH);

    ButtonState states[wiring_test::kMaxButtons]{};
    initButtonTracker(cfg, states);

    auto step = [&](int level, uint32_t t)
    {
        setLevel(13, level, t);
        return pollButtonTracker(cfg, states, cfg.debounceMs, t);
    };

    step(LOW, 100); // raw change
    fake_time::value = 200;
    pollButtonTracker(cfg, states, cfg.debounceMs, 200); // press 1 lands
    EXPECT_EQ(states[0].pressCount, 1u);

    step(HIGH, 300); // raw change to release
    fake_time::value = 400;
    pollButtonTracker(cfg, states, cfg.debounceMs, 400); // release lands

    step(LOW, 500); // raw change to press 2
    fake_time::value = 600;
    pollButtonTracker(cfg, states, cfg.debounceMs, 600); // press 2 lands
    EXPECT_EQ(states[0].pressCount, 2u);
}

TEST_F(ButtonTrackerTest, UnconfiguredPinIsInvisible)
{
    // cfg has pin 13 only; pin 27 toggling should never produce events.
    WiringConfig cfg{};
    strncpy(cfg.buttons[0].name, "A", sizeof(cfg.buttons[0].name) - 1);
    cfg.buttons[0].pin = 13;
    cfg.buttons[0].activeLow = true;
    cfg.numButtons = 1;
    cfg.debounceMs = 50;

    fake_gpio::setPinState(13, HIGH);
    fake_gpio::setPinState(27, HIGH);

    ButtonState states[wiring_test::kMaxButtons]{};
    initButtonTracker(cfg, states);

    for (uint32_t t = 0; t < 500; t += 10)
    {
        // Aggressively wiggle the unconfigured pin.
        fake_gpio::setPinState(27, (t / 10) % 2 == 0 ? LOW : HIGH);
        fake_time::value = t;
        auto e = pollButtonTracker(cfg, states, cfg.debounceMs, t);
        EXPECT_FALSE(e.fired);
    }
    EXPECT_EQ(states[0].pressCount, 0u);
}

TEST_F(ButtonTrackerTest, FormatPressLineMatchesWireFormat)
{
    WiringButton b{};
    strncpy(b.name, "button_a", sizeof(b.name) - 1);
    b.pin = 13;
    b.activeLow = true;
    b.isSelect = false;

    char line[128]{};
    size_t n = formatPressLine(line, sizeof(line), b, LOW);
    EXPECT_GT(n, 0u);
    EXPECT_NE(strstr(line, "press:"), nullptr);
    EXPECT_NE(strstr(line, "button_a"), nullptr);
    EXPECT_NE(strstr(line, "GPIO 13"), nullptr);
    EXPECT_NE(strstr(line, "active-low"), nullptr);
    EXPECT_NE(strstr(line, "state=PRESSED"), nullptr);
    EXPECT_EQ(line[n - 1], '\n');
}

TEST_F(ButtonTrackerTest, FormatPressLineSelectBadge)
{
    WiringButton b{};
    strncpy(b.name, "select", sizeof(b.name) - 1);
    b.pin = 21;
    b.activeLow = true;
    b.isSelect = true;

    char line[128]{};
    formatPressLine(line, sizeof(line), b, HIGH);
    EXPECT_NE(strstr(line, "(select)"), nullptr);
    EXPECT_NE(strstr(line, "state=RELEASED"), nullptr);
}
