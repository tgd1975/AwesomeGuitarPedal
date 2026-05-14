// Host tests for the wiring tool's JSON config parser (TASK-384).
//
// Pins the wire format the embed script feeds the firmware: required
// fields, defaults, pinNames overrides, and the EPIC-028 debounce
// clamp.

#include "wiring_config.h"

#include <gtest/gtest.h>
#include <string.h>

using wiring_test::ParseStatus;
using wiring_test::parseWiringConfig;
using wiring_test::WiringConfig;

namespace
{

    const WiringConfig& mustParse(const char* json, WiringConfig& out)
    {
        auto r = parseWiringConfig(json, out);
        EXPECT_EQ(r.status, ParseStatus::Ok) << r.message;
        return out;
    }

} // namespace

TEST(WiringConfigParse, RealWorldConfigPopulatesEverything)
{
    const char* json = R"({
        "hardware": "esp32",
        "numButtons": 4,
        "numSelectLeds": 3,
        "buttonPins": [13, 12, 27, 14],
        "buttonSelect": 21,
        "ledPower": 2,
        "ledBluetooth": 26,
        "ledSelect": [5, 18, 19],
        "debounceMs": 100,
        "pinNames": {
          "13": "button_a",
          "21": "button_select",
          "26": "led_bluetooth"
        }
    })";

    WiringConfig cfg{};
    mustParse(json, cfg);

    EXPECT_STREQ(cfg.hardware, "esp32");
    EXPECT_EQ(cfg.debounceMs, 100u);

    // 4 action buttons + 1 select.
    ASSERT_EQ(cfg.numButtons, 5);
    EXPECT_STREQ(cfg.buttons[0].name, "button_a"); // pinNames override
    EXPECT_EQ(cfg.buttons[0].pin, 13);
    EXPECT_FALSE(cfg.buttons[0].isSelect);

    EXPECT_STREQ(cfg.buttons[1].name, "B"); // fallback
    EXPECT_EQ(cfg.buttons[1].pin, 12);

    EXPECT_STREQ(cfg.buttons[4].name, "button_select"); // select via pinNames
    EXPECT_EQ(cfg.buttons[4].pin, 21);
    EXPECT_TRUE(cfg.buttons[4].isSelect);

    // 1 power + 1 BT + 3 select.
    ASSERT_EQ(cfg.numLeds, 5);
    EXPECT_STREQ(cfg.leds[0].name, "ledPower");
    EXPECT_EQ(cfg.leds[0].pin, 2);

    EXPECT_STREQ(cfg.leds[1].name, "led_bluetooth"); // pinNames override
    EXPECT_EQ(cfg.leds[1].pin, 26);

    EXPECT_STREQ(cfg.leds[2].name, "ledSelect[0]");
    EXPECT_EQ(cfg.leds[2].pin, 5);
}

TEST(WiringConfigParse, MissingButtonPinsRejected)
{
    const char* json = R"({"hardware":"esp32","ledPower":2})";
    WiringConfig cfg{};
    auto r = parseWiringConfig(json, cfg);
    EXPECT_EQ(r.status, ParseStatus::MissingField);
    EXPECT_NE(strstr(r.message, "buttonPins"), nullptr);
}

TEST(WiringConfigParse, MalformedJsonRejected)
{
    const char* json = "{ not valid";
    WiringConfig cfg{};
    auto r = parseWiringConfig(json, cfg);
    EXPECT_EQ(r.status, ParseStatus::InvalidJson);
}

TEST(WiringConfigParse, DebounceDefaultsTo100WhenMissing)
{
    const char* json = R"({"hardware":"esp32","buttonPins":[13]})";
    WiringConfig cfg{};
    mustParse(json, cfg);
    EXPECT_EQ(cfg.debounceMs, 100u);
}

TEST(WiringConfigParse, DebounceClampsBelowOne)
{
    const char* json = R"({"hardware":"esp32","buttonPins":[13],"debounceMs":0})";
    WiringConfig cfg{};
    mustParse(json, cfg);
    EXPECT_EQ(cfg.debounceMs, 1u);
}

TEST(WiringConfigParse, DebounceClampsAboveOneThousand)
{
    const char* json = R"({"hardware":"esp32","buttonPins":[13],"debounceMs":99999})";
    WiringConfig cfg{};
    mustParse(json, cfg);
    EXPECT_EQ(cfg.debounceMs, 1000u);
}

TEST(WiringConfigParse, NumButtonsCapsButtonPins)
{
    const char* json = R"({"hardware":"esp32","buttonPins":[13,12,27,14],"numButtons":2})";
    WiringConfig cfg{};
    mustParse(json, cfg);
    EXPECT_EQ(cfg.numButtons, 2);
    EXPECT_EQ(cfg.buttons[0].pin, 13);
    EXPECT_EQ(cfg.buttons[1].pin, 12);
}

TEST(WiringConfigParse, EmptyPinNamesFallsBackToDefaults)
{
    const char* json = R"({"hardware":"esp32","buttonPins":[13]})";
    WiringConfig cfg{};
    mustParse(json, cfg);
    EXPECT_STREQ(cfg.buttons[0].name, "A");
}
