#include "config.h"
#include "null_logger.h"
#include "pedal_config.h"
#include <gtest/gtest.h>
#include <string>

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

// Save / restore hardwareConfig so tests don't bleed into each other.
class HardwareMismatchTest : public ::testing::Test
{
protected:
    HardwareConfig savedConfig;
    NullLogger logger;

    void SetUp() override { savedConfig = hardwareConfig; }
    void TearDown() override { hardwareConfig = savedConfig; }

    static std::string makeJson(const char* hardware, int numButtons = 4, int ledBluetooth = 26)
    {
        std::string json = R"({"hardware": ")" + std::string(hardware) +
                           R"(", "numProfiles": 3, "numSelectLeds": 2, "numButtons": )" +
                           std::to_string(numButtons) + R"(, "ledBluetooth": )" +
                           std::to_string(ledBluetooth) +
                           R"(, "ledPower": 25, "ledSelect": [5, 18], )"
                           R"("buttonSelect": 21, "buttonPins": [13, 12, 27, 14]})";
        return json;
    }
};

// ---------------------------------------------------------------------------
// Tests
// ---------------------------------------------------------------------------

TEST_F(HardwareMismatchTest, MatchingHardwareFieldAppliesOverrides)
{
    // hardwareConfig.hardware == "esp32" (from hardware_config_fake.cpp)
    std::string json = makeJson("esp32", /*numButtons=*/3, /*ledBluetooth=*/10);
    EXPECT_TRUE(loadHardwareConfigFromJson(json, &logger));
    EXPECT_EQ(hardwareConfig.numButtons, 3);
    EXPECT_EQ(hardwareConfig.ledBluetooth, 10);
}

TEST_F(HardwareMismatchTest, MismatchedHardwareFieldReturnsFalse)
{
    std::string json = makeJson("nrf52840");
    EXPECT_FALSE(loadHardwareConfigFromJson(json, &logger));
}

TEST_F(HardwareMismatchTest, MismatchedHardwareFieldDoesNotApplyOverrides)
{
    uint8_t originalLed = hardwareConfig.ledBluetooth;
    std::string json = makeJson("nrf52840", 4, originalLed + 5);
    loadHardwareConfigFromJson(json, &logger);
    EXPECT_EQ(hardwareConfig.ledBluetooth, originalLed); // unchanged
}

TEST_F(HardwareMismatchTest, MissingHardwareFieldReturnsFalse)
{
    std::string json = R"({"numProfiles": 3, "numSelectLeds": 2, "numButtons": 4,)"
                       R"( "ledBluetooth": 26, "ledPower": 25, "ledSelect": [5, 18],)"
                       R"( "buttonSelect": 21, "buttonPins": [13, 12, 27, 14]})";
    EXPECT_FALSE(loadHardwareConfigFromJson(json, &logger));
}

TEST_F(HardwareMismatchTest, MissingHardwareFieldDoesNotApplyOverrides)
{
    uint8_t originalLed = hardwareConfig.ledBluetooth;
    std::string json = R"({"numProfiles": 1, "numSelectLeds": 1, "numButtons": 1,)"
                       R"( "ledBluetooth": 99, "ledPower": 25, "ledSelect": [5],)"
                       R"( "buttonSelect": 21, "buttonPins": [13]})";
    loadHardwareConfigFromJson(json, &logger);
    EXPECT_EQ(hardwareConfig.ledBluetooth, originalLed); // unchanged
}

TEST_F(HardwareMismatchTest, InvalidJsonReturnsTrueAndDoesNotApplyOverrides)
{
    // Malformed JSON is treated as "file absent" — fall back to defaults
    uint8_t originalLed = hardwareConfig.ledBluetooth;
    EXPECT_TRUE(loadHardwareConfigFromJson("not valid json {{{", &logger));
    EXPECT_EQ(hardwareConfig.ledBluetooth, originalLed);
}

// ---------------------------------------------------------------------------
// pairing_pin tests
// ---------------------------------------------------------------------------

TEST_F(HardwareMismatchTest, PairingPinPresentEnablesPairing)
{
    std::string json = makeJson("esp32");
    json.insert(json.size() - 1, R"(, "pairing_pin": 12345)");
    EXPECT_TRUE(loadHardwareConfigFromJson(json, &logger));
    EXPECT_TRUE(hardwareConfig.pairingEnabled);
    EXPECT_EQ(hardwareConfig.pairingPin, 12345u);
}

TEST_F(HardwareMismatchTest, PairingPinNullDisablesPairing)
{
    std::string json = makeJson("esp32");
    json.insert(json.size() - 1, R"(, "pairing_pin": null)");
    EXPECT_TRUE(loadHardwareConfigFromJson(json, &logger));
    EXPECT_FALSE(hardwareConfig.pairingEnabled);
}

TEST_F(HardwareMismatchTest, PairingPinAbsentDisablesPairing)
{
    std::string json = makeJson("esp32");
    EXPECT_TRUE(loadHardwareConfigFromJson(json, &logger));
    EXPECT_FALSE(hardwareConfig.pairingEnabled);
}

TEST_F(HardwareMismatchTest, PairingPinZeroIsValid)
{
    std::string json = makeJson("esp32");
    json.insert(json.size() - 1, R"(, "pairing_pin": 0)");
    EXPECT_TRUE(loadHardwareConfigFromJson(json, &logger));
    EXPECT_TRUE(hardwareConfig.pairingEnabled);
    EXPECT_EQ(hardwareConfig.pairingPin, 0u);
}

TEST_F(HardwareMismatchTest, PairingPinMaxValueIsValid)
{
    std::string json = makeJson("esp32");
    json.insert(json.size() - 1, R"(, "pairing_pin": 999999)");
    EXPECT_TRUE(loadHardwareConfigFromJson(json, &logger));
    EXPECT_TRUE(hardwareConfig.pairingEnabled);
    EXPECT_EQ(hardwareConfig.pairingPin, 999999u);
}

TEST_F(HardwareMismatchTest, PairingPinOutOfRangeDisablesPairing)
{
    std::string json = makeJson("esp32");
    json.insert(json.size() - 1, R"(, "pairing_pin": 1000000)");
    EXPECT_TRUE(loadHardwareConfigFromJson(json, &logger));
    EXPECT_FALSE(hardwareConfig.pairingEnabled);
}

// ---------------------------------------------------------------------------
// debounceMs tests (EPIC-028 / TASK-375)
// ---------------------------------------------------------------------------

TEST_F(HardwareMismatchTest, DebounceMsPresentAppliesValue)
{
    std::string json = makeJson("esp32");
    json.insert(json.size() - 1, R"(, "debounceMs": 50)");
    EXPECT_TRUE(loadHardwareConfigFromJson(json, &logger));
    EXPECT_EQ(hardwareConfig.debounceMs, 50u);
}

TEST_F(HardwareMismatchTest, DebounceMsAbsentDefaultsTo100)
{
    std::string json = makeJson("esp32");
    EXPECT_TRUE(loadHardwareConfigFromJson(json, &logger));
    EXPECT_EQ(hardwareConfig.debounceMs, 100u);
}

TEST_F(HardwareMismatchTest, DebounceMsBelowMinDefaultsTo100)
{
    // Schema rejects values < 1; this guards against a hand-edited config that
    // bypassed schema validation. Out-of-range falls back to 100, same shape
    // as the pairing_pin contract.
    std::string json = makeJson("esp32");
    json.insert(json.size() - 1, R"(, "debounceMs": 0)");
    EXPECT_TRUE(loadHardwareConfigFromJson(json, &logger));
    EXPECT_EQ(hardwareConfig.debounceMs, 100u);
}

TEST_F(HardwareMismatchTest, DebounceMsAboveMaxDefaultsTo100)
{
    std::string json = makeJson("esp32");
    json.insert(json.size() - 1, R"(, "debounceMs": 2000)");
    EXPECT_TRUE(loadHardwareConfigFromJson(json, &logger));
    EXPECT_EQ(hardwareConfig.debounceMs, 100u);
}

TEST_F(HardwareMismatchTest, DebounceMsAtMinBoundaryApplies)
{
    std::string json = makeJson("esp32");
    json.insert(json.size() - 1, R"(, "debounceMs": 1)");
    EXPECT_TRUE(loadHardwareConfigFromJson(json, &logger));
    EXPECT_EQ(hardwareConfig.debounceMs, 1u);
}

TEST_F(HardwareMismatchTest, DebounceMsAtMaxBoundaryApplies)
{
    std::string json = makeJson("esp32");
    json.insert(json.size() - 1, R"(, "debounceMs": 1000)");
    EXPECT_TRUE(loadHardwareConfigFromJson(json, &logger));
    EXPECT_EQ(hardwareConfig.debounceMs, 1000u);
}
