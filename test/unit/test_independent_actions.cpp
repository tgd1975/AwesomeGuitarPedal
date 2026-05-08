#include "arduino_shim.h"
#include "button_constants.h"
#include "config_loader.h"
#include "file_system.h"
#include "i_ble_keyboard.h"
#include "mock_led_controller.h"
#include "null_logger.h"
#include "pin_action.h"
#include "profile_manager.h"
#include <gtest/gtest.h>
#include <vector>

namespace fake_gpio
{
    extern int written_pin;
    extern int written_value;
}

namespace
{

    class StubKeyboard : public IBleKeyboard
    {
    public:
        void begin() override {}
        bool isConnected() override { return true; }
        void write(uint8_t key) override { (void) key; }
        void write(const MediaKeyReport key) override { (void) key; }
        void print(const char* text) override { (void) text; }
    };

    class IndependentActionsTest : public ::testing::Test
    {
    protected:
        MockLEDController led1, led2, led3;
        ProfileManager profileManager{std::vector<ILEDController*>{&led1, &led2, &led3}};
        StubKeyboard keyboard;
        ConfigLoader configLoader;

        void SetUp() override
        {
            fake_gpio::written_pin = -1;
            fake_gpio::written_value = -1;
        }
    };

    // ---------------------------------------------------------------------------
    // Parsing
    // ---------------------------------------------------------------------------

    TEST_F(IndependentActionsTest, AbsentBlockYieldsNoIndependentActions)
    {
        std::string json = R"json({
        "profiles": [{
            "name": "P1",
            "buttons": { "A": { "type": "SendStringAction", "value": "x" } }
        }]
    })json";

        ASSERT_TRUE(configLoader.loadFromString(profileManager, &keyboard, json));
        EXPECT_EQ(profileManager.getIndependentActions(), nullptr);
    }

    TEST_F(IndependentActionsTest, PresentBlockProducesIndependentActions)
    {
        std::string json = R"json({
        "profiles": [{
            "name": "P1",
            "buttons": { "A": { "type": "SendStringAction", "value": "x" } }
        }],
        "independentActions": {
            "A": { "type": "PinHighWhilePressedAction", "pin": 5 }
        }
    })json";

        ASSERT_TRUE(configLoader.loadFromString(profileManager, &keyboard, json));
        const Profile* independent = profileManager.getIndependentActions();
        ASSERT_NE(independent, nullptr);
        Action* indAction = independent->getAction(Btn::A);
        ASSERT_NE(indAction, nullptr);
        EXPECT_EQ(indAction->getType(), Action::Type::PinHighWhilePressed);
    }

    TEST_F(IndependentActionsTest, EmptyBlockProducesEmptyIndependentSet)
    {
        std::string json = R"json({
        "profiles": [{
            "name": "P1",
            "buttons": { "A": { "type": "SendStringAction", "value": "x" } }
        }],
        "independentActions": {}
    })json";

        ASSERT_TRUE(configLoader.loadFromString(profileManager, &keyboard, json));
        const Profile* independent = profileManager.getIndependentActions();
        ASSERT_NE(independent, nullptr);
        EXPECT_EQ(independent->getAction(Btn::A), nullptr);
    }

    // ---------------------------------------------------------------------------
    // Profile switching does not touch independent actions
    // ---------------------------------------------------------------------------

    TEST_F(IndependentActionsTest, ProfileSwitchLeavesIndependentActionsIntact)
    {
        std::string json = R"json({
        "profiles": [
            { "name": "P1", "buttons": { "A": { "type": "SendStringAction", "value": "p1" } } },
            { "name": "P2", "buttons": { "A": { "type": "SendStringAction", "value": "p2" } } }
        ],
        "independentActions": {
            "A": { "type": "PinHighWhilePressedAction", "pin": 7 }
        }
    })json";

        ASSERT_TRUE(configLoader.loadFromString(profileManager, &keyboard, json));
        const Profile* before = profileManager.getIndependentActions();
        ASSERT_NE(before, nullptr);
        Action* beforeAction = before->getAction(Btn::A);

        profileManager.switchProfile();

        const Profile* after = profileManager.getIndependentActions();
        ASSERT_EQ(after, before);
        EXPECT_EQ(after->getAction(Btn::A), beforeAction);
    }

    // ---------------------------------------------------------------------------
    // Release semantics — executeRelease() reaches PinHighWhilePressed
    // ---------------------------------------------------------------------------

    TEST_F(IndependentActionsTest, IndependentPinHighWhilePressedReleasesLow)
    {
        std::string json = R"json({
        "profiles": [{ "name": "P1", "buttons": {} }],
        "independentActions": {
            "A": { "type": "PinHighWhilePressedAction", "pin": 9 }
        }
    })json";

        ASSERT_TRUE(configLoader.loadFromString(profileManager, &keyboard, json));
        Action* indAction = profileManager.getIndependentActions()->getAction(Btn::A);
        ASSERT_NE(indAction, nullptr);

        indAction->execute();
        EXPECT_EQ(fake_gpio::written_pin, 9);
        EXPECT_EQ(fake_gpio::written_value, HIGH);

        indAction->executeRelease();
        EXPECT_EQ(fake_gpio::written_pin, 9);
        EXPECT_EQ(fake_gpio::written_value, LOW);
    }

    // ---------------------------------------------------------------------------
    // Save round-trip
    // ---------------------------------------------------------------------------

    class CapturingFileSystem : public IFileSystem
    {
    public:
        std::string content;
        bool exists(const char*) override { return false; }
        bool readFile(const char*, std::string&) override { return false; }
        bool writeFile(const char*, const std::string& c) override
        {
            content = c;
            return true;
        }
    };

    TEST_F(IndependentActionsTest, SaveToFileEmitsIndependentActionsBlock)
    {
        std::string json = R"json({
        "profiles": [{ "name": "P1", "buttons": {} }],
        "independentActions": {
            "A": { "type": "PinHighWhilePressedAction", "pin": 11 }
        }
    })json";

        ASSERT_TRUE(configLoader.loadFromString(profileManager, &keyboard, json));

        CapturingFileSystem fs;
        NullLogger logger;
        ConfigLoader saver(&fs, &logger);
        ASSERT_TRUE(saver.saveToFile(profileManager, "any.json"));

        EXPECT_NE(fs.content.find("independentActions"), std::string::npos);
        EXPECT_NE(fs.content.find("PinHighWhilePressedAction"), std::string::npos);
    }

    TEST_F(IndependentActionsTest, SaveToFileOmitsBlockWhenNoIndependentActions)
    {
        std::string json = R"json({
        "profiles": [{ "name": "P1", "buttons": {} }]
    })json";

        ASSERT_TRUE(configLoader.loadFromString(profileManager, &keyboard, json));

        CapturingFileSystem fs;
        NullLogger logger;
        ConfigLoader saver(&fs, &logger);
        ASSERT_TRUE(saver.saveToFile(profileManager, "any.json"));

        EXPECT_EQ(fs.content.find("independentActions"), std::string::npos);
    }

    // ---------------------------------------------------------------------------
    // mergeConfig — block in JSON replaces; absent block keeps existing
    // ---------------------------------------------------------------------------

    TEST_F(IndependentActionsTest, MergeWithoutBlockKeepsExistingIndependentActions)
    {
        std::string initial = R"json({
        "profiles": [{ "name": "P1", "buttons": {} }],
        "independentActions": {
            "A": { "type": "PinHighWhilePressedAction", "pin": 3 }
        }
    })json";
        ASSERT_TRUE(configLoader.loadFromString(profileManager, &keyboard, initial));

        std::string merge = R"json({
        "profiles": [{ "name": "P2", "buttons": {} }]
    })json";
        ASSERT_TRUE(configLoader.mergeConfig(profileManager, &keyboard, merge));

        const Profile* independent = profileManager.getIndependentActions();
        ASSERT_NE(independent, nullptr);
        Action* indAction = independent->getAction(Btn::A);
        ASSERT_NE(indAction, nullptr);
        EXPECT_EQ(indAction->getType(), Action::Type::PinHighWhilePressed);
    }

    TEST_F(IndependentActionsTest, MergeWithBlockReplacesIndependentActions)
    {
        std::string initial = R"json({
        "profiles": [{ "name": "P1", "buttons": {} }],
        "independentActions": {
            "A": { "type": "PinHighWhilePressedAction", "pin": 3 }
        }
    })json";
        ASSERT_TRUE(configLoader.loadFromString(profileManager, &keyboard, initial));

        std::string merge = R"json({
        "profiles": [],
        "independentActions": {
            "B": { "type": "PinHighWhilePressedAction", "pin": 4 }
        }
    })json";
        ASSERT_TRUE(configLoader.mergeConfig(profileManager, &keyboard, merge));

        const Profile* independent = profileManager.getIndependentActions();
        ASSERT_NE(independent, nullptr);
        EXPECT_EQ(independent->getAction(Btn::A), nullptr);
        Action* newAction = independent->getAction(Btn::B);
        ASSERT_NE(newAction, nullptr);
        EXPECT_EQ(newAction->getType(), Action::Type::PinHighWhilePressed);
    }

} // namespace
