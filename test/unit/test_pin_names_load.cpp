#include "config.h"
#include "null_logger.h"
#include "pedal_config.h"
#include "pin_name_table.h"
#include <gtest/gtest.h>
#include <string>

// EPIC-029 / TASK-380. Tests for loadHardwareConfigFromJson's
// pinNames handling — populating g_pinNameTable, skipping
// out-of-range pin keys, and leaving the table empty when the field
// is absent.

class PinNamesLoadTest : public ::testing::Test
{
protected:
    HardwareConfig savedConfig;
    NullLogger logger;

    void SetUp() override
    {
        savedConfig = hardwareConfig;
        g_pinNameTable.clear();
    }
    void TearDown() override
    {
        hardwareConfig = savedConfig;
        g_pinNameTable.clear();
    }

    // Minimal valid config JSON, optionally with an extra pinNames block.
    static std::string makeConfig(const std::string& pinNamesBlock = "")
    {
        std::string base = R"({"hardware": "esp32", "numProfiles": 3, "numSelectLeds": 2, )"
                           R"("numButtons": 4, "ledBluetooth": 26, "ledPower": 25, )"
                           R"("ledSelect": [5, 18], "buttonSelect": 21, )"
                           R"("buttonPins": [13, 12, 27, 14])";
        if (! pinNamesBlock.empty())
            base += ", " + pinNamesBlock;
        base += "}";
        return base;
    }
};

TEST_F(PinNamesLoadTest, AbsentPinNamesLeavesTableEmpty)
{
    std::string json = makeConfig();
    EXPECT_TRUE(loadHardwareConfigFromJson(json, &logger));
    EXPECT_EQ(g_pinNameTable.size(), 0);
}

TEST_F(PinNamesLoadTest, PinNamesObjectPopulatesTable)
{
    std::string json =
        makeConfig(R"("pinNames": {"13": "button_a", "12": "button_b", "26": "led_bluetooth"})");
    EXPECT_TRUE(loadHardwareConfigFromJson(json, &logger));
    EXPECT_EQ(g_pinNameTable.size(), 3);
    EXPECT_EQ(g_pinNameTable.lookup("button_a"), 13);
    EXPECT_EQ(g_pinNameTable.lookup("button_b"), 12);
    EXPECT_EQ(g_pinNameTable.lookup("led_bluetooth"), 26);
}

TEST_F(PinNamesLoadTest, OutOfRangePinKeyIsSkipped)
{
    // GPIO 99 is outside the schema-permitted 0..39 range. The loader
    // logs and skips it rather than populating a nonsense entry.
    std::string json = makeConfig(R"("pinNames": {"99": "button_a", "13": "button_b"})");
    EXPECT_TRUE(loadHardwareConfigFromJson(json, &logger));
    EXPECT_EQ(g_pinNameTable.size(), 1);
    EXPECT_EQ(g_pinNameTable.lookup("button_b"), 13);
    EXPECT_EQ(g_pinNameTable.lookup("button_a"), static_cast<uint8_t>(PinNameTable::kUnresolved));
}

TEST_F(PinNamesLoadTest, NonNumericPinKeyIsSkipped)
{
    std::string json = makeConfig(R"("pinNames": {"D13": "button_a"})");
    EXPECT_TRUE(loadHardwareConfigFromJson(json, &logger));
    EXPECT_EQ(g_pinNameTable.size(), 0);
}

TEST_F(PinNamesLoadTest, ReloadClearsAndRebuildsTable)
{
    // First load.
    std::string firstJson = makeConfig(R"("pinNames": {"13": "button_a"})");
    EXPECT_TRUE(loadHardwareConfigFromJson(firstJson, &logger));
    EXPECT_EQ(g_pinNameTable.lookup("button_a"), 13);

    // Reload with a completely different mapping. The table must
    // reflect only the new mapping — no stale entries.
    std::string secondJson = makeConfig(R"("pinNames": {"27": "led_power"})");
    EXPECT_TRUE(loadHardwareConfigFromJson(secondJson, &logger));
    EXPECT_EQ(g_pinNameTable.lookup("button_a"), static_cast<uint8_t>(PinNameTable::kUnresolved));
    EXPECT_EQ(g_pinNameTable.lookup("led_power"), 27);
    EXPECT_EQ(g_pinNameTable.size(), 1);
}
