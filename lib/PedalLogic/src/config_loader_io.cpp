#include "ble_config_reassembler.h" // kJsonDocCapacity
#include "button_constants.h"
#include "config.h"
#include "config_loader.h"
#include "profile_manager.h"
#include <ArduinoJson.h>

using namespace ArduinoJson;

/**
 * @brief Saves configuration to a file
 *
 * Serializes the current profile manager state to JSON and writes it to the specified file.
 * Includes all profiles with their names, descriptions, and button action configurations.
 *
 * @param profileManager The profile manager containing profiles to save
 * @param configPath Path to the configuration file to write
 * @return true if saving succeeded, false if file write failed
 */
static void writeButtonActionsToJson(const Profile& source, JsonObject buttons)
{
    char btnName[2];
    for (uint8_t b = 0; b < hardwareConfig.numButtons; b++)
    {
        Btn::name(b, btnName);
        Action* action = source.getAction(b);
        if (action)
        {
            JsonObject actionObj = buttons.createNestedObject(btnName);
            ConfigLoader::actionToJson(action, actionObj);
        }
    }
}

bool ConfigLoader::saveToFile(const ProfileManager& profileManager, const std::string& configPath)
{
    DynamicJsonDocument doc(kJsonDocCapacity);
    JsonArray profiles = doc.createNestedArray("profiles");

    for (uint8_t profileIndex = 0; profileIndex < hardwareConfig.numProfiles; profileIndex++)
    {
        const Profile* profile = profileManager.getProfile(profileIndex);
        if (! profile)
        {
            continue;
        }

        JsonObject profileObj = profiles.createNestedObject();
        profileObj["name"] = profile->getName().c_str();
        profileObj["description"] = profile->getDescription().c_str();

        JsonObject buttons = profileObj.createNestedObject("buttons");
        writeButtonActionsToJson(*profile, buttons);
    }

    if (const Profile* independent = profileManager.getIndependentActions())
    {
        JsonObject independentObj = doc.createNestedObject("independentActions");
        writeButtonActionsToJson(*independent, independentObj);
    }

    std::string content{};
    serializeJson(doc, content);

    if (! fileSystem_->writeFile(configPath.c_str(), content))
    {
        logger_->log("Failed to write config file");
        return false;
    }

    return true;
}

/**
 * @brief Logs the currently loaded configuration to serial
 *
 * Iterates all populated profile slots and prints each profile name and its
 * button actions (type and optional name) via the logger.
 *
 * @param profileManager The profile manager whose state should be logged
 */
void ConfigLoader::logLoadedConfig(const ProfileManager& profileManager) const
{
    logger_->log("--- Config loaded ---");
    for (uint8_t i = 0; i < hardwareConfig.numProfiles; i++)
    {
        const Profile* profile = profileManager.getProfile(i);
        if (! profile)
        {
            continue;
        }

        logger_->log("Profile: ", profile->getName().c_str());

        char btnLabel[2];
        for (uint8_t b = 0; b < hardwareConfig.numButtons; b++)
        {
            const Action* action = profile->getAction(b);
            if (! action)
            {
                continue;
            }

            Btn::name(b, btnLabel);
            const char* typeStr = ProfileManager::getActionTypeString(action->getType());

            std::string line = "  ";
            line += btnLabel;
            line += ": ";
            line += typeStr;
            if (action->hasName())
            {
                line += " [";
                line += action->getName();
                line += "]";
            }
            logger_->log(line.c_str());
        }
    }
}

void ConfigLoader::actionToJson(const Action* action, JsonObject& out)
{
    if (action->hasName())
    {
        out["name"] = action->getName().c_str();
    }

    switch (action->getType())
    {
        case Action::Type::SendString:
            out["type"] = "SendStringAction";
            action->getJsonProperties(out);
            break;
        case Action::Type::SendChar:
            out["type"] = "SendCharAction";
            action->getJsonProperties(out);
            break;
        case Action::Type::SendKey:
            out["type"] = "SendKeyAction";
            action->getJsonProperties(out);
            break;
        case Action::Type::SendMediaKey:
            out["type"] = "SendMediaKeyAction";
            action->getJsonProperties(out);
            break;
        case Action::Type::SerialOutput:
            out["type"] = "SerialOutputAction";
            action->getJsonProperties(out);
            break;
        case Action::Type::Delayed:
            out["type"] = "DelayedAction";
            action->getJsonProperties(out);
            break;
        case Action::Type::PinHigh:
            out["type"] = "PinHighAction";
            action->getJsonProperties(out);
            break;
        case Action::Type::PinLow:
            out["type"] = "PinLowAction";
            action->getJsonProperties(out);
            break;
        case Action::Type::PinToggle:
            out["type"] = "PinToggleAction";
            action->getJsonProperties(out);
            break;
        case Action::Type::PinHighWhilePressed:
            out["type"] = "PinHighWhilePressedAction";
            action->getJsonProperties(out);
            break;
        case Action::Type::PinLowWhilePressed:
            out["type"] = "PinLowWhilePressedAction";
            action->getJsonProperties(out);
            break;
        case Action::Type::Macro:
            out["type"] = "MacroAction";
            action->getJsonProperties(out);
            break;
        default:
            out["type"] = "UnknownAction";
            break;
    }
}
