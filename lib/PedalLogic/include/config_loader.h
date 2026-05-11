#pragma once
#include "action.h"
#include "button_constants.h"
#include "file_system.h"
#include "i_ble_keyboard.h"
#include "i_logger.h"
#include "profile_manager.h"
#include <ArduinoJson.h>
#include <string>

/**
 * @class ConfigLoader
 * @brief Loads pedal configuration from JSON files
 *
 * Responsible for parsing JSON configuration files and creating the appropriate
 * Action objects to populate the ProfileManager.
 */
class ConfigLoader
{
public:
    /// Default constructor: uses production singletons (firmware path)
    ConfigLoader();

    /// Injection constructor: use in tests or wherever DI is needed
    ConfigLoader(IFileSystem* fs, ILogger* logger);

    bool loadFromFile(ProfileManager& profileManager,
                      IBleKeyboard* keyboard,
                      const std::string& configPath);
    bool loadFromString(ProfileManager& profileManager,
                        IBleKeyboard* keyboard,
                        const std::string& jsonConfig);
    bool saveToFile(const ProfileManager& profileManager, const std::string& configPath);
    bool mergeConfig(ProfileManager& profileManager,
                     IBleKeyboard* keyboard,
                     const std::string& jsonConfig);
    bool replaceProfile(ProfileManager& profileManager,
                        IBleKeyboard* keyboard,
                        uint8_t profileIndex,
                        const std::string& jsonConfig);

    static const char* getDefaultConfig() { return DEFAULT_CONFIG; }

    static uint8_t getButtonIndex(const char* buttonName);

    /// Serialise *action* into *out* (writes the "type" field plus the
    /// action-specific properties). Public so MacroAction's own serialiser
    /// can recurse into nested step actions.
    static void actionToJson(const Action* action, ArduinoJson::JsonObject& out);

private:
    static const char* DEFAULT_CONFIG;

    IFileSystem* fileSystem_;
    ILogger* logger_;

    /// EPIC-029 / TASK-380. Number of named-pin references in profile
    /// actions that failed to resolve against g_pinNameTable during the
    /// most recent loadFromString() call. Reset at the start of each
    /// load and reported via the post-load summary line.
    uint16_t unresolvedNamedPinRefs_ = 0;

    std::unique_ptr<Action> createActionFromJson(const ArduinoJson::JsonObject& actionJson,
                                                 IBleKeyboard* keyboard);
    std::unique_ptr<Action> createSendCharActionFromJson(const ArduinoJson::JsonObject& actionJson,
                                                         IBleKeyboard* keyboard);
    /// EPIC-029 / TASK-380. Pull the Pin*Action branch out of
    /// createActionFromJson so its cognitive complexity stays under
    /// the clang-tidy threshold. Resolves named-pin refs against
    /// g_pinNameTable; an unresolved name bumps unresolvedNamedPinRefs_
    /// and returns nullptr.
    std::unique_ptr<Action> createPinActionFromJson(const ArduinoJson::JsonObject& actionJson,
                                                    Action::Type type);
    void populateProfileFromJson(Profile& profile,
                                 ArduinoJson::JsonObject buttons,
                                 IBleKeyboard* keyboard);
    void logLoadedConfig(const ProfileManager& profileManager) const;
    static bool profileExistsByName(const ProfileManager& profileManager, const char* name);
    static uint8_t findEmptyProfileSlot(const ProfileManager& profileManager);
};
