#include "button_constants.h"
#include "config_loader.h"
#include "file_system.h"
#include "mock_led_controller.h"
#include "null_logger.h"
#include "pin_action.h"
#include "pin_name_table.h"
#include "profile_manager.h"
#include "send_action.h"
#include "serial_action.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <vector>

using ::testing::_;
using ::testing::AnyNumber;

// ---------------------------------------------------------------------------
// Fake filesystem for saveToFile tests
// ---------------------------------------------------------------------------

class FakeFileSystem : public IFileSystem
{
public:
    std::string lastWrittenPath;
    std::string lastWrittenContent;
    bool writeResult = true;

    bool exists(const char*) override { return false; }
    bool readFile(const char*, std::string&) override { return false; }
    bool writeFile(const char* path, const std::string& content) override
    {
        lastWrittenPath = path;
        lastWrittenContent = content;
        return writeResult;
    }
};

class MockKeyboard : public IBleKeyboard
{
public:
    void begin() override {}
    bool isConnected() override { return true; }
    void write(uint8_t key) override { (void) key; }
    void write(const MediaKeyReport key) override { (void) key; }
    void print(const char* text) override { (void) text; }
};

class ConfigLoaderUnitTest : public ::testing::Test
{
protected:
    MockLEDController led1, led2, led3;
    ProfileManager profileManager{std::vector<ILEDController*>{&led1, &led2, &led3}};
    MockKeyboard keyboard;
    ConfigLoader configLoader;
};

TEST_F(ConfigLoaderUnitTest, LoadFromValidJsonString)
{
    std::string validJson = R"json({
        "profiles": [{
            "name": "TestProfile",
            "buttons": {
                "A": {"type": "SendStringAction", "value": "Test"}
            }
        }]
    })json";

    EXPECT_TRUE(configLoader.loadFromString(profileManager, &keyboard, validJson));
    EXPECT_EQ(profileManager.getProfileName(0), "TestProfile");
    EXPECT_NE(profileManager.getAction(0, Btn::A), nullptr);
}

TEST_F(ConfigLoaderUnitTest, LoadFromInvalidJsonReturnsFalse)
{
    std::string invalidJson = "not valid json";
    EXPECT_FALSE(configLoader.loadFromString(profileManager, &keyboard, invalidJson));
}

TEST_F(ConfigLoaderUnitTest, GetDefaultConfigReturnsValidJson)
{
    const char* defaultConfig = configLoader.getDefaultConfig();
    EXPECT_FALSE(defaultConfig == nullptr);
    EXPECT_NE(std::string(defaultConfig).find("profiles"), std::string::npos);
}

TEST_F(ConfigLoaderUnitTest, UnknownActionTypeReturnsNullptr)
{
    std::string json = R"json({
        "profiles": [{
            "name": "Test",
            "buttons": {
                "A": {"type": "UnknownAction", "value": "test"}
            }
        }]
    })json";

    EXPECT_TRUE(configLoader.loadFromString(profileManager, &keyboard, json));
    EXPECT_EQ(profileManager.getAction(0, Btn::A), nullptr);
}

TEST_F(ConfigLoaderUnitTest, DelayedActionParsing)
{
    std::string json = R"json({
        "profiles": [{
            "name": "Test",
            "buttons": {
                "A": {
                    "type": "DelayedAction",
                    "delayMs": 1000,
                    "action": {"type": "SendStringAction", "value": "Delayed"}
                }
            }
        }]
    })json";

    EXPECT_TRUE(configLoader.loadFromString(profileManager, &keyboard, json));
    Action* action = profileManager.getAction(0, Btn::A);
    EXPECT_NE(action, nullptr);
    EXPECT_EQ(action->getDelay(), 1000u);
}

TEST_F(ConfigLoaderUnitTest, SerialOutputActionParsing)
{
    std::string json = R"json({
        "profiles": [{
            "name": "Test",
            "buttons": {
                "A": {"type": "SerialOutputAction", "value": "Debug message"}
            }
        }]
    })json";

    EXPECT_TRUE(configLoader.loadFromString(profileManager, &keyboard, json));
    Action* action = profileManager.getAction(0, Btn::A);
    EXPECT_NE(action, nullptr);
    EXPECT_FALSE(action->isSendAction());
}

TEST_F(ConfigLoaderUnitTest, SendKeyActionParsing)
{
    std::string json = R"json({
        "profiles": [{
            "name": "Test",
            "buttons": {
                "A": {"type": "SendKeyAction", "value": "KEY_F1"}
            }
        }]
    })json";

    EXPECT_TRUE(configLoader.loadFromString(profileManager, &keyboard, json));
    Action* action = profileManager.getAction(0, Btn::A);
    EXPECT_NE(action, nullptr);
    EXPECT_EQ(action->getType(), Action::Type::SendKey);
}

TEST_F(ConfigLoaderUnitTest, SendKeyActionUnknownValueReturnsNullptr)
{
    std::string json = R"json({
        "profiles": [{
            "name": "Test",
            "buttons": {
                "A": {"type": "SendKeyAction", "value": "NOT_A_KEY"}
            }
        }]
    })json";

    EXPECT_TRUE(configLoader.loadFromString(profileManager, &keyboard, json));
    EXPECT_EQ(profileManager.getAction(0, Btn::A), nullptr);
}

// ---------------------------------------------------------------------------
// SendKey "Key (raw HID)" hex/decimal value parsing (TASK-257)
// ---------------------------------------------------------------------------

class CapturingLogger : public ILogger
{
public:
    std::vector<std::string> messages;
    void log(const char* msg) override { messages.emplace_back(msg ? msg : ""); }
    void log(const char* prefix, const char* msg) override
    {
        messages.emplace_back(std::string(prefix ? prefix : "") + (msg ? msg : ""));
    }
};

class SendKeyValueParsingTest : public ::testing::Test
{
protected:
    MockLEDController led1, led2, led3;
    ProfileManager profileManager{std::vector<ILEDController*>{&led1, &led2, &led3}};
    MockKeyboard keyboard;
    FakeFileSystem fs;
    CapturingLogger logger;
    ConfigLoader configLoader{&fs, &logger};

    uint8_t parseSendKey(const std::string& valueLiteral)
    {
        std::string json =
            R"json({"profiles":[{"name":"T","buttons":{"A":{"type":"SendKeyAction","value":)json" +
            valueLiteral + R"json(}}}]})json";
        EXPECT_TRUE(configLoader.loadFromString(profileManager, &keyboard, json));
        Action* action = profileManager.getAction(0, Btn::A);
        if (action == nullptr)
            return 0;
        return static_cast<SendKeyAction*>(action)->getKey();
    }
};

TEST_F(SendKeyValueParsingTest, NamedKeyStillResolves)
{
    EXPECT_EQ(parseSendKey(R"("KEY_RETURN")"), KEY_RETURN);
}

TEST_F(SendKeyValueParsingTest, LowercaseHexResolvesToSameCodeAsName)
{
    // KEY_RETURN = 0xB0
    EXPECT_EQ(parseSendKey(R"("0xB0")"), KEY_RETURN);
}

TEST_F(SendKeyValueParsingTest, UppercaseHexPrefixWorks)
{
    EXPECT_EQ(parseSendKey(R"("0XB0")"), KEY_RETURN);
}

TEST_F(SendKeyValueParsingTest, DecimalValueResolvesToSameCodeAsName)
{
    // 0xB0 == 176
    EXPECT_EQ(parseSendKey(R"("176")"), KEY_RETURN);
}

TEST_F(SendKeyValueParsingTest, OutOfRangeValueDropsActionAndLogs)
{
    EXPECT_EQ(parseSendKey(R"("0x100")"), 0); // 256 — out of range
    EXPECT_EQ(profileManager.getAction(0, Btn::A), nullptr);
    EXPECT_FALSE(logger.messages.empty());
}

TEST_F(SendKeyValueParsingTest, NonNumericNonNamedValueDropsActionAndLogs)
{
    EXPECT_EQ(parseSendKey(R"("notakey")"), 0);
    EXPECT_EQ(profileManager.getAction(0, Btn::A), nullptr);
    EXPECT_FALSE(logger.messages.empty());
}

TEST_F(SendKeyValueParsingTest, EmptyValueDropsActionAndLogs)
{
    EXPECT_EQ(parseSendKey(R"("")"), 0);
    EXPECT_EQ(profileManager.getAction(0, Btn::A), nullptr);
    EXPECT_FALSE(logger.messages.empty());
}

TEST_F(SendKeyValueParsingTest, TrailingGarbageRejected)
{
    // strtoul would parse "0x28abc" up to "abc" — make sure we reject it.
    EXPECT_EQ(parseSendKey(R"("0x28abc")"), 0);
    EXPECT_EQ(profileManager.getAction(0, Btn::A), nullptr);
    EXPECT_FALSE(logger.messages.empty());
}

TEST_F(SendKeyValueParsingTest, ZeroValueDropsAction)
{
    // 0 is the sentinel "not found" — don't accept it as a key code.
    EXPECT_EQ(parseSendKey(R"("0x00")"), 0);
    EXPECT_EQ(profileManager.getAction(0, Btn::A), nullptr);
    EXPECT_FALSE(logger.messages.empty());
}

TEST_F(ConfigLoaderUnitTest, PinHighActionParsing)
{
    std::string json = R"json({
        "profiles": [{
            "name": "Test",
            "buttons": {
                "A": {"type": "PinHighAction", "pin": 5}
            }
        }]
    })json";

    EXPECT_TRUE(configLoader.loadFromString(profileManager, &keyboard, json));
    Action* action = profileManager.getAction(0, Btn::A);
    EXPECT_NE(action, nullptr);
    EXPECT_EQ(action->getType(), Action::Type::PinHigh);
}

TEST_F(ConfigLoaderUnitTest, PinLowActionParsing)
{
    std::string json = R"json({
        "profiles": [{
            "name": "Test",
            "buttons": {
                "A": {"type": "PinLowAction", "pin": 6}
            }
        }]
    })json";

    EXPECT_TRUE(configLoader.loadFromString(profileManager, &keyboard, json));
    Action* action = profileManager.getAction(0, Btn::A);
    EXPECT_NE(action, nullptr);
    EXPECT_EQ(action->getType(), Action::Type::PinLow);
}

TEST_F(ConfigLoaderUnitTest, PinToggleActionParsing)
{
    std::string json = R"json({
        "profiles": [{
            "name": "Test",
            "buttons": {
                "A": {"type": "PinToggleAction", "pin": 7}
            }
        }]
    })json";

    EXPECT_TRUE(configLoader.loadFromString(profileManager, &keyboard, json));
    Action* action = profileManager.getAction(0, Btn::A);
    EXPECT_NE(action, nullptr);
    EXPECT_EQ(action->getType(), Action::Type::PinToggle);
}

TEST_F(ConfigLoaderUnitTest, PinHighWhilePressedActionParsing)
{
    std::string json = R"json({
        "profiles": [{
            "name": "Test",
            "buttons": {
                "A": {"type": "PinHighWhilePressedAction", "pin": 8}
            }
        }]
    })json";

    EXPECT_TRUE(configLoader.loadFromString(profileManager, &keyboard, json));
    Action* action = profileManager.getAction(0, Btn::A);
    EXPECT_NE(action, nullptr);
    EXPECT_EQ(action->getType(), Action::Type::PinHighWhilePressed);
}

TEST_F(ConfigLoaderUnitTest, PinLowWhilePressedActionParsing)
{
    std::string json = R"json({
        "profiles": [{
            "name": "Test",
            "buttons": {
                "A": {"type": "PinLowWhilePressedAction", "pin": 9}
            }
        }]
    })json";

    EXPECT_TRUE(configLoader.loadFromString(profileManager, &keyboard, json));
    Action* action = profileManager.getAction(0, Btn::A);
    EXPECT_NE(action, nullptr);
    EXPECT_EQ(action->getType(), Action::Type::PinLowWhilePressed);
}

TEST_F(ConfigLoaderUnitTest, PinActionMissingPinFieldReturnsNullptr)
{
    std::string json = R"json({
        "profiles": [{
            "name": "Test",
            "buttons": {
                "A": {"type": "PinHighAction"}
            }
        }]
    })json";

    EXPECT_TRUE(configLoader.loadFromString(profileManager, &keyboard, json));
    EXPECT_EQ(profileManager.getAction(0, Btn::A), nullptr);
}

// ---------------------------------------------------------------------------
// EPIC-029 / TASK-380. Named-pin resolution against g_pinNameTable.
// The fixture below isolates each test with a clean lookup table.
// ---------------------------------------------------------------------------

class ConfigLoaderNamedPinsTest : public ConfigLoaderUnitTest
{
protected:
    void SetUp() override { g_pinNameTable.clear(); }
    void TearDown() override { g_pinNameTable.clear(); }
};

TEST_F(ConfigLoaderNamedPinsTest, NamedPinResolvesAgainstPinNameTable)
{
    // Hardware config maps GPIO 27 to "button_a"; a profile that
    // references "button_a" should fire pin 27.
    g_pinNameTable.insert("button_a", 27);

    std::string json = R"json({
        "profiles": [{
            "name": "Test",
            "buttons": {
                "A": {"type": "PinHighAction", "pin": "button_a"}
            }
        }]
    })json";

    EXPECT_TRUE(configLoader.loadFromString(profileManager, &keyboard, json));
    Action* a = profileManager.getAction(0, Btn::A);
    ASSERT_NE(a, nullptr);
    EXPECT_EQ(a->getType(), Action::Type::PinHigh);
}

TEST_F(ConfigLoaderNamedPinsTest, MixedDirectAndNamedPinsBothResolve)
{
    g_pinNameTable.insert("led_power", 2);

    std::string json = R"json({
        "profiles": [{
            "name": "Test",
            "buttons": {
                "A": {"type": "PinHighAction", "pin": 27},
                "B": {"type": "PinLowAction",  "pin": "led_power"}
            }
        }]
    })json";

    EXPECT_TRUE(configLoader.loadFromString(profileManager, &keyboard, json));
    EXPECT_NE(profileManager.getAction(0, Btn::A), nullptr);
    EXPECT_NE(profileManager.getAction(0, Btn::B), nullptr);
}

TEST_F(ConfigLoaderNamedPinsTest, UnresolvedNamedPinDropsActionButLoadContinues)
{
    // No entry for "button_a" in the table — the named action drops.
    g_pinNameTable.insert("led_power", 2);

    std::string json = R"json({
        "profiles": [{
            "name": "Test",
            "buttons": {
                "A": {"type": "PinHighAction",  "pin": "button_a"},
                "B": {"type": "PinLowAction",   "pin": "led_power"},
                "C": {"type": "SendStringAction", "value": "ok"}
            }
        }]
    })json";

    EXPECT_TRUE(configLoader.loadFromString(profileManager, &keyboard, json));
    EXPECT_EQ(profileManager.getAction(0, Btn::A), nullptr);
    // Sibling actions inside the same profile still bind.
    EXPECT_NE(profileManager.getAction(0, Btn::B), nullptr);
    EXPECT_NE(profileManager.getAction(0, Btn::C), nullptr);
}

TEST_F(ConfigLoaderNamedPinsTest, AllDirectProfileWithEmptyPinNameTableStillLoads)
{
    // No pinNames mapping in the hardware config — pre-EPIC-029
    // builders' all-direct profiles must continue to work unchanged.
    g_pinNameTable.clear();

    std::string json = R"json({
        "profiles": [{
            "name": "Test",
            "buttons": {
                "A": {"type": "PinHighAction", "pin": 27},
                "B": {"type": "PinLowAction",  "pin": 14}
            }
        }]
    })json";

    EXPECT_TRUE(configLoader.loadFromString(profileManager, &keyboard, json));
    EXPECT_NE(profileManager.getAction(0, Btn::A), nullptr);
    EXPECT_NE(profileManager.getAction(0, Btn::B), nullptr);
}

TEST_F(ConfigLoaderNamedPinsTest, TypoedNameDropsOnlyBadActionWithinProfile)
{
    // The builder typoed "buton_a" (missing letter). Only that action
    // drops; the rest of the profile, including the correctly-named
    // sibling, still resolves.
    g_pinNameTable.insert("button_a", 13);
    g_pinNameTable.insert("button_b", 12);

    std::string json = R"json({
        "profiles": [{
            "name": "Test",
            "buttons": {
                "A": {"type": "PinHighAction", "pin": "buton_a"},
                "B": {"type": "PinLowAction",  "pin": "button_b"}
            }
        }]
    })json";

    EXPECT_TRUE(configLoader.loadFromString(profileManager, &keyboard, json));
    EXPECT_EQ(profileManager.getAction(0, Btn::A), nullptr);
    EXPECT_NE(profileManager.getAction(0, Btn::B), nullptr);
}

TEST_F(ConfigLoaderUnitTest, DelayedActionMissingInnerActionReturnsNullptr)
{
    std::string json = R"json({
        "profiles": [{
            "name": "Test",
            "buttons": {
                "A": {"type": "DelayedAction", "delayMs": 500}
            }
        }]
    })json";

    EXPECT_TRUE(configLoader.loadFromString(profileManager, &keyboard, json));
    EXPECT_EQ(profileManager.getAction(0, Btn::A), nullptr);
}

TEST_F(ConfigLoaderUnitTest, SendMediaKeyUnknownValueReturnsNullptr)
{
    std::string json = R"json({
        "profiles": [{
            "name": "Test",
            "buttons": {
                "A": {"type": "SendMediaKeyAction", "value": "NOT_A_MEDIA_KEY"}
            }
        }]
    })json";

    EXPECT_TRUE(configLoader.loadFromString(profileManager, &keyboard, json));
    EXPECT_EQ(profileManager.getAction(0, Btn::A), nullptr);
}

// ---------------------------------------------------------------------------
// saveToFile tests — use injection constructor with FakeFileSystem
// ---------------------------------------------------------------------------

class ConfigLoaderSaveTest : public ::testing::Test
{
protected:
    FakeFileSystem fs;
    NullLogger logger;
    ConfigLoader loader{&fs, &logger};
    MockLEDController led1, led2, led3;
    MockKeyboard keyboard;
    ProfileManager pm{std::vector<ILEDController*>{&led1, &led2, &led3}};

    void SetUp() override
    {
        EXPECT_CALL(led1, setState(_)).Times(AnyNumber());
        EXPECT_CALL(led2, setState(_)).Times(AnyNumber());
        EXPECT_CALL(led3, setState(_)).Times(AnyNumber());
        EXPECT_CALL(led1, startBlink(_, _)).Times(AnyNumber());
        EXPECT_CALL(led2, startBlink(_, _)).Times(AnyNumber());
        EXPECT_CALL(led3, startBlink(_, _)).Times(AnyNumber());
        EXPECT_CALL(led1, stopBlink()).Times(AnyNumber());
        EXPECT_CALL(led2, stopBlink()).Times(AnyNumber());
        EXPECT_CALL(led3, stopBlink()).Times(AnyNumber());
        EXPECT_CALL(led1, update(_)).Times(AnyNumber());
        EXPECT_CALL(led2, update(_)).Times(AnyNumber());
        EXPECT_CALL(led3, update(_)).Times(AnyNumber());
        EXPECT_CALL(led1, isBlinking()).Times(AnyNumber());
        EXPECT_CALL(led2, isBlinking()).Times(AnyNumber());
        EXPECT_CALL(led3, isBlinking()).Times(AnyNumber());
    }
};

TEST_F(ConfigLoaderSaveTest, SaveToFileWritesJsonToCorrectPath)
{
    loader.loadFromString(pm, &keyboard, R"json({
        "profiles":[{"name":"MyProfile","buttons":{"A":{"type":"SendStringAction","value":"hi"}}}]
    })json");

    EXPECT_TRUE(loader.saveToFile(pm, "/config.json"));
    EXPECT_EQ(fs.lastWrittenPath, "/config.json");
    EXPECT_NE(fs.lastWrittenContent.find("MyProfile"), std::string::npos);
}

TEST_F(ConfigLoaderSaveTest, SaveToFileReturnsFalseOnWriteFailure)
{
    fs.writeResult = false;
    loader.loadFromString(pm, &keyboard, R"json({
        "profiles":[{"name":"P","buttons":{}}]
    })json");
    EXPECT_FALSE(loader.saveToFile(pm, "/config.json"));
}

TEST_F(ConfigLoaderSaveTest, SaveToFileSerializesAllActionTypes)
{
    loader.loadFromString(pm, &keyboard, R"json({
        "profiles":[{
            "name":"Multi",
            "buttons":{
                "A":{"type":"SendStringAction","value":"hello"},
                "B":{"type":"SendKeyAction","value":"KEY_A"},
                "C":{"type":"SerialOutputAction","value":"ping"},
                "D":{"type":"SendMediaKeyAction","value":"MEDIA_STOP"}
            }
        }]
    })json");

    EXPECT_TRUE(loader.saveToFile(pm, "/multi.json"));
    EXPECT_NE(fs.lastWrittenContent.find("SendStringAction"), std::string::npos);
}

TEST_F(ConfigLoaderSaveTest, SaveToFileWithEmptyProfileManager)
{
    EXPECT_TRUE(loader.saveToFile(pm, "/empty.json"));
    EXPECT_FALSE(fs.lastWrittenContent.empty());
}
