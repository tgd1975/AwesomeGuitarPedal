#include "pedal_config.h"
#include "action.h"
#include "button_constants.h"
#include "config.h"
#include "config_loader.h"
#include "pin_name_table.h"
#include "profile.h"
#include "send_action.h"
#include "serial_action.h"
#include <ArduinoJson.h>
#include <memory>
#include <string>

// EPIC-029 / TASK-380. Global pin-name lookup table. Populated from
// the hardware config's pinNames map at boot; consumed by ConfigLoader
// when resolving named pin references inside profile Pin*Actions.
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
PinNameTable g_pinNameTable;

// Forward declaration for platform-specific factory (avoids pulling in full DI headers)
IFileSystem* createFileSystem();

namespace
{
    // Apply the pairing_pin field from the parsed config. Out-of-range or
    // absent → pairing disabled. Pulled out of loadHardwareConfigFromJson
    // so the parent stays under clang-tidy's cognitive-complexity threshold.
    void applyPairingPin(const ArduinoJson::JsonDocument& doc, ILogger* logger)
    {
        if (! doc.containsKey("pairing_pin") || doc["pairing_pin"].isNull())
        {
            hardwareConfig.pairingEnabled = false;
            hardwareConfig.pairingPin = 0;
            return;
        }
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

    // Apply the pinNames mapping (EPIC-029 / TASK-380). Rebuilds the
    // global g_pinNameTable from the on-disk pin → name map (inverted
    // to name → pin for lookup speed). Out-of-range pins and over-long
    // names are skipped with a warning. Absent or empty map leaves the
    // table empty, which is the only behaviour pre-EPIC-029 configs
    // observed.
    void applyPinNames(const ArduinoJson::JsonDocument& doc, ILogger* logger)
    {
        g_pinNameTable.clear();
        if (! doc.containsKey("pinNames"))
        {
            return;
        }
        ArduinoJson::JsonObjectConst obj = doc["pinNames"].as<ArduinoJson::JsonObjectConst>();
        for (ArduinoJson::JsonPairConst entry : obj)
        {
            const char* pinKey = entry.key().c_str();
            const char* roleName = entry.value().as<const char*>();
            char* end = nullptr;
            unsigned long pin = std::strtoul(pinKey, &end, 10);
            if (end == pinKey || *end != '\0' || pin > 39)
            {
                std::string msg = std::string("loadHardwareConfig: pinNames pin '") + pinKey +
                                  "' is not 0..39 — skipped";
                logger->log(msg.c_str());
                continue;
            }
            if (roleName == nullptr || roleName[0] == '\0')
            {
                continue;
            }
            if (! g_pinNameTable.insert(roleName, static_cast<uint8_t>(pin)))
            {
                std::string msg = std::string("loadHardwareConfig: pinNames entry '") + pinKey +
                                  " -> " + roleName + "' rejected (table full or name too long)";
                logger->log(msg.c_str());
            }
        }
    }

    // Apply the debounceMs field (EPIC-028). Schema bounds are 1..1000;
    // anything outside that range — or an absent field — falls back to the
    // 100 ms default.
    void applyDebounceMs(const ArduinoJson::JsonDocument& doc, ILogger* logger)
    {
        if (! doc.containsKey("debounceMs"))
        {
            hardwareConfig.debounceMs = 100;
            return;
        }
        uint32_t ms = doc["debounceMs"].as<uint32_t>();
        if (ms >= 1 && ms <= 1000)
        {
            hardwareConfig.debounceMs = ms;
        }
        else
        {
            logger->log("loadHardwareConfig: debounceMs out of range (1–1000) — defaulting to 100");
            hardwareConfig.debounceMs = 100;
        }
    }
} // namespace

/**
 * @brief Core parsing and application of hardware config JSON.
 *
 * Separated from loadHardwareConfig() so it can be called directly in host
 * tests without a real filesystem.
 */
bool loadHardwareConfigFromJson(const std::string& content, ILogger* logger)
{
    // Doc capacity grew from 1024 → 2048 in EPIC-029 / TASK-380 to make
    // room for the pinNames object on top of the existing scalar fields.
    // A fully-populated 16-pin pinNames adds ~400 bytes; 2 KB leaves
    // comfortable headroom and is still small in ESP32 RAM terms.
    ArduinoJson::DynamicJsonDocument doc(2048);
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

    applyPairingPin(doc, logger);
    applyDebounceMs(doc, logger);
    applyPinNames(doc, logger);

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
