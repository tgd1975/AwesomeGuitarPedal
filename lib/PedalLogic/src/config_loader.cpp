#include "config_loader.h"
#include "ble_config_reassembler.h" // kJsonDocCapacity
#include "button_constants.h"
#include "config.h"
#include "delayed_action.h"
#include "file_system.h"
#include "i_logger.h"
#include "key_lookup.h"
#include "macro_action.h"
#include "non_send_action.h"
#include "pin_action.h"
#include "pin_name_table.h"
#include "send_action.h"
#include "serial_action.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#include <cstdlib>
#include <cstring>
#include <memory>

using namespace ArduinoJson;

// Forward declaration for platform-specific factory (createLogger comes from i_logger.h)
IFileSystem* createFileSystem();

namespace
{
    // Parse a SendKey "value" string into a HID keycode in [1, 255].
    // Order of attempts: (1) named-key lookup ("KEY_RETURN"), (2) numeric parse
    // ("0x28", "0X28", "40"). Returns 0 if all attempts fail or the result is
    // out of HID range.
    uint8_t parseSendKeyValue(const char* value)
    {
        if (value == nullptr || value[0] == '\0')
        {
            return 0;
        }
        uint8_t named = lookupKey(value);
        if (named != 0)
        {
            return named;
        }
        char* end = nullptr;
        // base = 0 lets strtoul auto-detect "0x"/"0X" hex and decimal
        unsigned long parsed = std::strtoul(value, &end, 0);
        if (end == value || *end != '\0' || parsed == 0 || parsed > 0xFF)
        {
            return 0;
        }
        return static_cast<uint8_t>(parsed);
    }
} // namespace

/**
 * @brief Default configuration JSON string
 *
 * Contains a single "Navigation" profile with media control button mappings.
 * Used as fallback when no custom configuration file is available.
 */
const char* ConfigLoader::DEFAULT_CONFIG =
    "{\n"
    "  \"profiles\": [\n"
    "    {\n"
    "      \"name\": \"Navigation\",\n"
    "      \"description\": \"Profile for controlling media playback\",\n"
    "      \"buttons\": {\n"
    "        \"A\": {\"type\": \"SendStringAction\", \"name\": \"Space\", \"value\": \" \"},\n"
    "        \"B\": {\"type\": \"SendMediaKeyAction\", \"name\": \"Stop\", \"value\": "
    "\"MEDIA_STOP\"},\n"
    "        \"C\": {\"type\": \"SendCharAction\", \"name\": \"Rewind\", \"value\": "
    "\"LEFT_ARROW\"},\n"
    "        \"D\": {\"type\": \"SendCharAction\", \"name\": \"Forward\", \"value\": "
    "\"RIGHT_ARROW\"}\n"
    "      }\n"
    "    }\n"
    "  ]\n"
    "}\n";

// ---- Constructors ----

/**
 * @brief Default constructor
 *
 * Creates a ConfigLoader using production singletons for file system and logging.
 * Used in firmware builds.
 */
ConfigLoader::ConfigLoader() : fileSystem_(createFileSystem()), logger_(createLogger()) {}

/**
 * @brief Dependency injection constructor
 *
 * Creates a ConfigLoader with injected dependencies for testing or custom implementations.
 *
 * @param fs File system implementation
 * @param logger Logger implementation
 */
ConfigLoader::ConfigLoader(IFileSystem* fs, ILogger* logger) : fileSystem_(fs), logger_(logger) {}

// ---- Public API ----

/**
 * @brief Loads configuration from a file
 *
 * Reads the configuration file from the specified path and parses it using loadFromString().
 *
 * @param profileManager The profile manager to populate with loaded profiles
 * @param keyboard The BLE keyboard interface for creating keyboard actions
 * @param configPath Path to the configuration file
 * @return true if loading succeeded, false if file read failed
 */
bool ConfigLoader::loadFromFile(ProfileManager& profileManager,
                                IBleKeyboard* keyboard,
                                const std::string& configPath)
{
    std::string content{};
    if (! fileSystem_->readFile(configPath.c_str(), content))
    {
        logger_->log("Failed to read config file");
        return false;
    }
    return loadFromString(profileManager, keyboard, content);
}

/**
 * @brief Loads configuration from a JSON string
 *
 * Parses the JSON configuration and populates the profile manager with the defined profiles.
 * Clears existing profiles before loading new ones.
 *
 * @param profileManager The profile manager to populate
 * @param keyboard The BLE keyboard interface for creating keyboard actions
 * @param jsonConfig JSON string containing the configuration
 * @return true if parsing and loading succeeded, false if JSON parsing failed
 */
bool ConfigLoader::loadFromString(ProfileManager& profileManager,
                                  IBleKeyboard* keyboard,
                                  const std::string& jsonConfig)
{
    DynamicJsonDocument doc(kJsonDocCapacity);
    DeserializationError error = deserializeJson(doc, jsonConfig);

    if (error)
    {
        logger_->log("JSON parsing failed:", error.c_str());
        return false;
    }

    // EPIC-029 / TASK-380. Reset counter at the start of each load so
    // the end-of-load summary reflects only this run.
    unresolvedNamedPinRefs_ = 0;

    for (uint8_t i = 0; i < hardwareConfig.numProfiles; i++)
    {
        profileManager.addProfile(i, nullptr);
    }

    JsonArray profiles = doc["profiles"];
    for (size_t i = 0; i < profiles.size() && i < hardwareConfig.numProfiles; i++)
    {
        JsonObject profileJson = profiles[i];
        const char* profileName = profileJson["name"] | "";
        const char* profileDescription = profileJson["description"] | "";

        auto newProfile = std::make_unique<Profile>(profileName);
        newProfile->setDescription(profileDescription);
        populateProfileFromJson(*newProfile, profileJson["buttons"], keyboard);
        profileManager.addProfile(static_cast<uint8_t>(i), std::move(newProfile));
    }

    if (doc.containsKey("independentActions"))
    {
        auto independent = std::make_unique<Profile>("__independent__");
        populateProfileFromJson(*independent, doc["independentActions"], keyboard);
        profileManager.setIndependentActions(std::move(independent));
    }
    else
    {
        profileManager.setIndependentActions(nullptr);
    }

    profileManager.resetToFirstProfile();
    logLoadedConfig(profileManager);
    return true;
}

// ---- Helper methods ----

/**
 * @brief Converts button name to button index
 *
 * @param buttonName The button name ("A", "B", "C", or "D")
 * @return The corresponding Btn:: index, or 255 if not found
 */
uint8_t ConfigLoader::getButtonIndex(const char* buttonName)
{
    if (buttonName[0] >= 'A' && buttonName[0] <= ('A' + Btn::MAX - 1) && buttonName[1] == '\0')
    {
        return static_cast<uint8_t>(buttonName[0] - 'A');
    }
    return 255;
}

/**
 * @brief Populates a profile with actions from JSON configuration
 *
 * Parses the buttons object from JSON and creates appropriate Action objects
 * for each button, adding them to the profile.
 *
 * @param profile The profile to populate
 * @param buttons JSON object containing button configurations
 * @param keyboard The BLE keyboard interface for creating keyboard actions
 */
void ConfigLoader::populateProfileFromJson(Profile& profile,
                                           JsonObject buttons,
                                           IBleKeyboard* keyboard)
{
    // Build an action from a named sub-object (e.g. "longPress", "doublePress")
    // and hand it to the supplied registrar. No-op if the key is absent or the
    // payload does not produce a valid action.
    auto attachVariant = [&](JsonObject actionJson, const char* variantKey, auto registrar)
    {
        if (! actionJson.containsKey(variantKey))
        {
            return;
        }
        JsonObject variantJson = actionJson[variantKey];
        auto variantAction = createActionFromJson(variantJson, keyboard);
        if (! variantAction)
        {
            return;
        }
        const char* variantName = variantJson["name"] | "";
        if (variantName[0] != '\0')
        {
            variantAction->setName(variantName);
        }
        registrar(std::move(variantAction));
    };

    char buttonName[2];
    for (uint8_t b = 0; b < hardwareConfig.numButtons; b++)
    {
        Btn::name(b, buttonName);
        if (! buttons.containsKey(buttonName))
        {
            continue;
        }

        JsonObject actionJson = buttons[buttonName];

        std::unique_ptr<Action> action = createActionFromJson(actionJson, keyboard);
        if (action)
        {
            const char* actionName = actionJson["name"] | "";
            if (actionName[0] != '\0')
            {
                action->setName(actionName);
            }
            profile.addAction(b, std::move(action));
        }

        attachVariant(actionJson,
                      "longPress",
                      [&](std::unique_ptr<Action> a)
                      { profile.addLongPressAction(b, std::move(a)); });
        attachVariant(actionJson,
                      "doublePress",
                      [&](std::unique_ptr<Action> a)
                      { profile.addDoublePressAction(b, std::move(a)); });
    }
}

/**
 * @brief Creates a SendChar action from JSON, handling named keys and raw ASCII
 */
std::unique_ptr<Action> ConfigLoader::createSendCharActionFromJson(const JsonObject& actionJson,
                                                                   IBleKeyboard* keyboard)
{
    const char* value = actionJson["value"] | "";
    uint8_t code = lookupKey(value);
    if (code != 0)
    {
        return std::make_unique<SendCharAction>(keyboard, static_cast<char>(code));
    }
    // Single printable ASCII character (e.g. "[", "]", "c", " ")
    if (value[0] != '\0' && value[1] == '\0')
    {
        return std::make_unique<SendCharAction>(keyboard, value[0]);
    }
    logger_->log("SendChar: unknown key value: ", value);
    return nullptr;
}

std::unique_ptr<Action> ConfigLoader::createPinActionFromJson(const JsonObject& actionJson,
                                                              Action::Type type)
{
    // EPIC-029 / TASK-380. "pin" accepts two forms (schema:
    // profiles.schema.json#pinRef): an integer GPIO or a standard role
    // name resolved against g_pinNameTable.
    ArduinoJson::JsonVariantConst raw = actionJson["pin"];
    if (raw.is<const char*>())
    {
        const char* name = raw.as<const char*>();
        uint8_t resolved = g_pinNameTable.lookup(name);
        if (resolved == PinNameTable::kUnresolved)
        {
            ++unresolvedNamedPinRefs_;
            std::string msg = std::string("PinAction: unresolved named pin '") +
                              (name ? name : "") +
                              "' — action dropped (hardware config's pinNames has no mapping)";
            logger_->log(msg.c_str());
            return nullptr;
        }
        return std::make_unique<PinAction>(type, resolved);
    }
    int pin = raw | -1;
    if (pin < 0)
    {
        logger_->log("PinAction: missing or invalid 'pin' field");
        return nullptr;
    }
    return std::make_unique<PinAction>(type, static_cast<uint8_t>(pin));
}

/**
 * @brief Creates an Action object from JSON configuration
 *
 * Parses the action JSON object and creates the appropriate Action subclass
 * based on the "type" field. Supports all action types including nested
 * DelayedAction with inner actions.
 *
 * @param actionJson JSON object containing action configuration
 * @param keyboard BLE keyboard interface for keyboard actions
 * @return Unique pointer to the created Action, or nullptr if type is unknown or invalid
 */
std::unique_ptr<Action> ConfigLoader::createActionFromJson(const JsonObject& actionJson,
                                                           IBleKeyboard* keyboard)
{
    const char* typeName = actionJson["type"] | "";
    Action::Type type = lookupActionType(typeName);

    switch (type)
    {
        case Action::Type::SendString:
        {
            const char* value = actionJson["value"] | "";
            return std::make_unique<SendStringAction>(keyboard, value);
        }
        case Action::Type::SendChar:
            return createSendCharActionFromJson(actionJson, keyboard);
        case Action::Type::SendKey:
        {
            const char* value = actionJson["value"] | "";
            uint8_t code = parseSendKeyValue(value);
            if (code != 0)
            {
                return std::make_unique<SendKeyAction>(keyboard, code);
            }
            logger_->log("SendKey: unknown or out-of-range value: ", value);
            break;
        }
        case Action::Type::SendMediaKey:
        {
            const uint8_t* report = lookupMediaKey(actionJson["value"] | "");
            if (report)
            {
                return std::make_unique<SendMediaKeyAction>(keyboard, report);
            }
            break;
        }
        case Action::Type::SerialOutput:
        {
            const char* value = actionJson["value"] | "";
            return std::make_unique<SerialOutputAction>(value);
        }
        case Action::Type::Delayed:
        {
            uint32_t delayMs = actionJson["delayMs"] | 0U;
            JsonObject nestedJson = actionJson["action"];
            std::unique_ptr<Action> inner = createActionFromJson(nestedJson, keyboard);
            if (inner)
            {
                return std::make_unique<DelayedAction>(std::move(inner), delayMs);
            }
            break;
        }
        case Action::Type::Macro:
        {
            auto macro = std::make_unique<MacroAction>();
            JsonArray stepsJson = actionJson["steps"];
            for (JsonArray stepJson : stepsJson)
            {
                MacroAction::Step step;
                for (JsonObject actionObj : stepJson)
                {
                    auto inner = createActionFromJson(actionObj, keyboard);
                    if (inner)
                    {
                        step.push_back(std::move(inner));
                    }
                }
                macro->addStep(std::move(step));
            }
            return macro;
        }
        case Action::Type::PinHigh:
        case Action::Type::PinLow:
        case Action::Type::PinToggle:
        case Action::Type::PinHighWhilePressed:
        case Action::Type::PinLowWhilePressed:
            return createPinActionFromJson(actionJson, type);
        default:
            break;
    }

    return nullptr;
}
