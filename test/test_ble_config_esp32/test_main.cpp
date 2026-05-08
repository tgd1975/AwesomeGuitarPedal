/**
 * BLE Config Integration Test Harness (ESP32 / NodeMCU-32S)
 *
 * This firmware starts the BLE Config service on real hardware and acts as the
 * System Under Test (SUT) for the Python runner (runner.py). It does NOT use
 * the Unity test framework — assertions live entirely in runner.py, which
 * connects as a BLE client and verifies protocol behaviour.
 *
 * Uses the same production code path as the main firmware (BleKeyboardAdapter +
 * setOnStartedCallback). The hardware config must have pairing_pin absent or
 * null — pairing is not supported in the automated test runner.
 *
 * Serial output format (runner.py parses these):
 *   [BLE_TEST] READY              — advertising started, runner may connect
 *   [BLE_TEST] PROFILE:<name>     — active profile changed (after successful upload)
 *   [BLE_TEST] RESET              — ESP32 is about to soft-reset (persistence test)
 *   [BLE_TEST] ERROR:<msg>        — fatal infrastructure error; runner must abort
 *
 * Run via: make test-esp32-ble-config
 */

#include <Arduino.h>
#include <Preferences.h>
#include <vector>

// ble_keyboard_adapter.h pulls in BleKeyboard.h, which defines the
// ESP32_BLE_KEYBOARD_H guard that suppresses the duplicate KEY_MEDIA_*
// definitions in i_ble_keyboard.h. Must come before ble_config_service.h.
// The blank line below stops clang-format SortIncludes from re-sorting it back.
#include "ble_keyboard_adapter.h"

#include "ble_config_service.h"
#include "config.h"
#include "i_led_controller.h"
#include "led_controller.h"
#include "pedal_config.h"
#include "profile_manager.h"

// ---------------------------------------------------------------------------
// Hardware peripherals
// ---------------------------------------------------------------------------

static LEDController* selectLedObjects[ProfileManager::MAX_SELECT_LEDS] = {};
static std::vector<ILEDController*> selectLeds;

static ProfileManager* pm = nullptr;
static BleConfigService bleConfigService;

static constexpr const char* NVS_NAMESPACE = "ble_test";
static constexpr const char* NVS_KEY_PROFILE = "profile";

static void saveProfile(uint8_t index)
{
    Preferences prefs;
    prefs.begin(NVS_NAMESPACE, false);
    prefs.putUChar(NVS_KEY_PROFILE, index);
    prefs.end();
}

static uint8_t loadProfile()
{
    Preferences prefs;
    prefs.begin(NVS_NAMESPACE, true);
    uint8_t idx = prefs.getUChar(NVS_KEY_PROFILE, 0);
    prefs.end();
    return idx;
}

// ---------------------------------------------------------------------------
// setup / loop
// ---------------------------------------------------------------------------

void setup()
{
    Serial.begin(115200);
    delay(2000);

    loadHardwareConfig();

    // Guard: pairing must be disabled for the automated test runner.
    // Flash a hardware config with pairing_pin absent or null (see
    // test/test_ble_config_esp32/data/config.json). If this fires, the wrong
    // config was flashed — check the data_dir for the nodemcu-32s-ble-config-test env.
    if (hardwareConfig.pairingEnabled)
    {
        Serial.println("[BLE_TEST] ERROR: pairing_pin must be null for integration tests");
        Serial.println(
            "[BLE_TEST] ERROR: reflash with test/test_ble_config_esp32/data/config.json");
        while (true)
            delay(1000);
    }

    for (uint8_t i = 0; i < hardwareConfig.numSelectLeds; i++)
    {
        selectLedObjects[i] = new LEDController(hardwareConfig.ledSelect[i]);
        selectLedObjects[i]->setup(false);
        selectLeds.push_back(selectLedObjects[i]);
    }

    pm = new ProfileManager(selectLeds);

    // Load profiles from filesystem (populated by a previous BLE upload).
    configureProfiles(*pm, nullptr);

    // Production path: BleKeyboardAdapter + setOnStartedCallback.
    // This exercises the same code path as the main firmware.
    BleKeyboardAdapter* adapter = createBleKeyboardAdapter();
    bleConfigService.begin(pm, adapter, selectLeds);
    adapter->begin();

    // Restore last active profile from NVS.
    uint8_t saved = loadProfile();
    if (pm->getProfile(saved) != nullptr)
        pm->setCurrentProfile(saved);

    Serial.println("[BLE_TEST] READY");
}

void loop()
{
    bleConfigService.loop();
    pm->update(millis());

    // Report profile changes to runner.py via serial.
    static uint8_t lastProfile = 0xFF;
    uint8_t cur = pm->getCurrentProfile();
    if (cur != lastProfile)
    {
        lastProfile = cur;
        const Profile* p = pm->getProfile(cur);
        if (p)
        {
            Serial.printf("[BLE_TEST] PROFILE:%s\n", p->getName().c_str());
            saveProfile(cur);
        }
    }

    if (Serial.available())
    {
        String cmd = Serial.readStringUntil('\n');
        cmd.trim();
        if (cmd == "RESET")
        {
            Serial.println("[BLE_TEST] RESET");
            Serial.flush();
            delay(100);
            ESP.restart();
        }
        else if (cmd == "PROFILE?")
        {
            const Profile* p = pm->getProfile(pm->getCurrentProfile());
            if (p)
                Serial.printf("[BLE_TEST] PROFILE:%s\n", p->getName().c_str());
            else
                Serial.println("[BLE_TEST] PROFILE:(none)");
        }
    }
}
