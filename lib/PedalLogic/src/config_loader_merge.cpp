#include "ble_config_reassembler.h" // JSON_DOC_CAPACITY
#include "config.h"
#include "config_loader.h"
#include "profile.h"
#include "profile_manager.h"
#include <ArduinoJson.h>
#include <memory>

using namespace ArduinoJson;

// ---- mergeConfig helpers ----

bool ConfigLoader::profileExistsByName(const ProfileManager& profileManager, const char* name)
{
    for (uint8_t i = 0; i < hardwareConfig.numProfiles; i++)
    {
        const Profile* p = profileManager.getProfile(i);
        if (p && p->getName() == name)
        {
            return true;
        }
    }
    return false;
}

uint8_t ConfigLoader::findEmptyProfileSlot(const ProfileManager& profileManager)
{
    for (uint8_t i = 0; i < hardwareConfig.numProfiles; i++)
    {
        if (! profileManager.getProfile(i))
        {
            return i;
        }
    }
    return 255;
}

/**
 * @brief Merges configuration from JSON into existing profiles
 *
 * Adds new profiles from the JSON configuration to the profile manager,
 * skipping any profiles that already exist by name. Only adds profiles
 * to empty slots in the profile manager.
 *
 * @param profileManager The profile manager to merge profiles into
 * @param keyboard The BLE keyboard interface for creating keyboard actions
 * @param jsonConfig JSON string containing profiles to merge
 * @return true if merging succeeded, false if JSON parsing failed
 */
bool ConfigLoader::mergeConfig(ProfileManager& profileManager,
                               IBleKeyboard* keyboard,
                               const std::string& jsonConfig)
{
    DynamicJsonDocument doc(JSON_DOC_CAPACITY);
    DeserializationError error = deserializeJson(doc, jsonConfig);

    if (error)
    {
        logger_->log("JSON parsing failed:", error.c_str());
        return false;
    }

    JsonArray profiles = doc["profiles"];

    for (size_t newIdx = 0; newIdx < profiles.size() && newIdx < hardwareConfig.numProfiles;
         newIdx++)
    {
        JsonObject profileJson = profiles[newIdx];
        const char* profileName = profileJson["name"] | "";
        const char* profileDescription = profileJson["description"] | "";

        if (profileExistsByName(profileManager, profileName))
        {
            logger_->log("Profile already exists, skipping: ", profileName);
            continue;
        }

        uint8_t targetIndex = findEmptyProfileSlot(profileManager);
        if (targetIndex == 255)
        {
            logger_->log("No empty profile slots available for merging");
            continue;
        }

        auto newProfile = std::make_unique<Profile>(profileName);
        newProfile->setDescription(profileDescription);
        populateProfileFromJson(*newProfile, profileJson["buttons"], keyboard);
        profileManager.addProfile(targetIndex, std::move(newProfile));
        logger_->log("Added profile: ", profileName);
    }

    logLoadedConfig(profileManager);
    return true;
}

/**
 * @brief Replaces a specific profile with configuration from JSON
 *
 * Parses the JSON configuration and replaces the profile at the specified index
 * with the first profile found in the JSON. Validates the profile index before
 * attempting replacement.
 *
 * @param profileManager The profile manager containing the profile to replace
 * @param keyboard The BLE keyboard interface for creating keyboard actions
 * @param profileIndex Index of the profile to replace (0-3)
 * @param jsonConfig JSON string containing the replacement profile
 * @return true if replacement succeeded, false if validation failed or JSON parsing failed
 */
bool ConfigLoader::replaceProfile(ProfileManager& profileManager,
                                  IBleKeyboard* keyboard,
                                  uint8_t profileIndex,
                                  const std::string& jsonConfig)
{
    if (profileIndex >= hardwareConfig.numProfiles)
    {
        logger_->log("Invalid profile index");
        return false;
    }

    DynamicJsonDocument doc(JSON_DOC_CAPACITY);
    DeserializationError error = deserializeJson(doc, jsonConfig);

    if (error)
    {
        logger_->log("JSON parsing failed:", error.c_str());
        return false;
    }

    JsonArray profiles = doc["profiles"];
    if (profiles.size() == 0)
    {
        logger_->log("No profiles found in JSON config");
        return false;
    }

    JsonObject profileJson = profiles[0];
    const char* profileName = profileJson["name"] | "";
    const char* profileDescription = profileJson["description"] | "";

    auto newProfile = std::make_unique<Profile>(profileName);
    newProfile->setDescription(profileDescription);
    populateProfileFromJson(*newProfile, profileJson["buttons"], keyboard);
    profileManager.addProfile(profileIndex, std::move(newProfile));
    logger_->log("Replaced profile with: ", profileName);
    logLoadedConfig(profileManager);

    return true;
}
