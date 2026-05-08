#include "pedal_config.h"
#include "action.h"
#include "button_constants.h"
#include "config.h"
#include "config_loader.h"
#include "profile.h"
#include "send_action.h"
#include "serial_action.h"
#include <ArduinoJson.h>
#include <memory>
#include <string>

// Forward declaration for platform-specific factory (avoids pulling in full DI headers)
IFileSystem* createFileSystem();

/**
 * @brief Core parsing and application of hardware config JSON.
 *
 * Separated from loadHardwareConfig() so it can be called directly in host
 * tests without a real filesystem.
 */
bool loadHardwareConfigFromJson(const std::string& content, ILogger* logger)
{
    ArduinoJson::DynamicJsonDocument doc(1024);
    auto err = ArduinoJson::deserializeJson(doc, content);
    if (err)
    {
        logger->log("loadHardwareConfig: JSON parse failed:", err.c_str());
        return true; // unparseable — fall back to compiled-in defaults
    }

    // Hardware identity check — must match this firmware's compiled target.
    // A missing or mismatched field means this config was built for a different
    // board; applying its pin values could drive the wrong GPIOs.
    if (! doc.containsKey("hardware"))
    {
        logger->log("loadHardwareConfig: 'hardware' field missing — rejecting config");
        return false;
    }
    const char* cfgHardware = doc["hardware"];
    if (std::string(cfgHardware) != std::string(hardwareConfig.hardware))
    {
        std::string msg = std::string("loadHardwareConfig: hardware mismatch — config=") +
                          cfgHardware + " firmware=" + hardwareConfig.hardware;
        logger->log(msg.c_str());
        return false;
    }

    if (doc.containsKey("numProfiles"))
    {
        hardwareConfig.numProfiles = doc["numProfiles"];
    }
    if (doc.containsKey("numSelectLeds"))
    {
        hardwareConfig.numSelectLeds = doc["numSelectLeds"];
    }
    if (doc.containsKey("numButtons"))
    {
        hardwareConfig.numButtons = doc["numButtons"];
    }
    if (doc.containsKey("ledBluetooth"))
    {
        hardwareConfig.ledBluetooth = doc["ledBluetooth"];
    }
    if (doc.containsKey("ledPower"))
    {
        hardwareConfig.ledPower = doc["ledPower"];
    }
    if (doc.containsKey("buttonSelect"))
    {
        hardwareConfig.buttonSelect = doc["buttonSelect"];
    }

    if (doc.containsKey("ledSelect"))
    {
        ArduinoJson::JsonArray arr = doc["ledSelect"];
        uint8_t n = arr.size() < 6 ? static_cast<uint8_t>(arr.size()) : 6;
        for (uint8_t i = 0; i < n; i++)
        {
            hardwareConfig.ledSelect[i] = arr[i];
        }
    }

    if (doc.containsKey("buttonPins"))
    {
        ArduinoJson::JsonArray arr = doc["buttonPins"];
        uint8_t n = arr.size() < 26 ? static_cast<uint8_t>(arr.size()) : 26;
        for (uint8_t i = 0; i < n; i++)
        {
            hardwareConfig.buttonPins[i] = arr[i];
        }
    }

    // pairing_pin: integer 0–999999 enables passkey auth; null or absent = no pairing.
    if (doc.containsKey("pairing_pin") && ! doc["pairing_pin"].isNull())
    {
        uint32_t pin = doc["pairing_pin"].as<uint32_t>();
        if (pin <= 999999)
        {
            hardwareConfig.pairingEnabled = true;
            hardwareConfig.pairingPin = pin;
        }
        else
        {
            logger->log(
                "loadHardwareConfig: pairing_pin out of range (0–999999) — pairing disabled");
            hardwareConfig.pairingEnabled = false;
            hardwareConfig.pairingPin = 0;
        }
    }
    else
    {
        hardwareConfig.pairingEnabled = false;
        hardwareConfig.pairingPin = 0;
    }

    logger->log("loadHardwareConfig: overrides applied from /config.json");
    return true;
}

/**
 * @brief Load hardware config overrides from /config.json (LittleFS) if present.
 */
bool loadHardwareConfig()
{
    IFileSystem* fs = createFileSystem();
    ILogger* logger = createLogger();

    std::string content;
    if (! fs->readFile("/config.json", content))
    {
        // File absent — use compiled-in defaults silently
        return true;
    }

    return loadHardwareConfigFromJson(content, logger);
}

/**
 * @brief Load profiles from file; fall back to DEFAULT_CONFIG on failure.
 *
 * Returns false only if DEFAULT_CONFIG itself fails to parse — which indicates
 * a programming error and should never happen at runtime.
 */
bool configureProfiles(ProfileManager& profileManager, IBleKeyboard* keyboard)
{
    ConfigLoader configLoader;

    if (configLoader.loadFromFile(profileManager, keyboard, "/profiles.json"))
    {
        return true;
    }

    // File missing or invalid — load the hardcoded default
    if (! configLoader.loadFromString(profileManager, keyboard, ConfigLoader::getDefaultConfig()))
    {
        // DEFAULT_CONFIG failed to parse: this is a compile-time bug, not a runtime error
        return false;
    }

    // Persist the default so future boots load from file
    configLoader.saveToFile(profileManager, "/profiles.json");
    return true;
}
