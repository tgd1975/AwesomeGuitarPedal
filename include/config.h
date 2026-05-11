#pragma once
#include <cstdint>

/**
 * @struct HardwareConfig
 * @brief Platform-independent hardware pin configuration
 *
 * Uses uint8_t for pin numbers so this header compiles on any Arduino
 * target without pulling in ESP-IDF or platform-specific headers.
 * Each hardware package provides its own config.cpp with the concrete
 * pin assignments for that board.
 *
 * Runtime active counts (numProfiles, numSelectLeds, numButtons) control
 * loop bounds throughout the firmware. The compile-time ceilings in
 * ProfileManager and Profile determine array sizes only.
 */
struct HardwareConfig
{
    uint8_t numProfiles;   /**< Number of active profiles (1..MAX_PROFILES) */
    uint8_t numSelectLeds; /**< Profile-select LEDs wired; caps profiles at 2^n − 1 */
    uint8_t numButtons;    /**< Action buttons wired (1..26, A–Z) */

    // LED pins
    uint8_t ledBluetooth; /**< Bluetooth status LED pin */
    uint8_t ledPower;     /**< Power indicator LED pin */
    uint8_t ledSelect[6]; /**< Profile-select LED pins (up to 6) */

    // Button pins
    uint8_t buttonSelect;   /**< Profile-cycle button pin */
    uint8_t buttonPins[26]; /**< Action button pins: index 0 = A, 1 = B, ..., 25 = Z */

    // Board identity — set at compile time from CONF_HARDWARE_TARGET; never overridden at runtime
    const char* hardware; /**< Board identifier string: "esp32" or "nrf52840" */

    // BLE pairing passkey — loaded from config.json at runtime.
    // When pairingEnabled is false, no passkey is set and config characteristics
    // are accessible without pairing (intended for development and test fixtures).
    // When true, NimBLEDevice::setSecurityPasskey(pairingPin) is applied and
    // passkey-entry auth is required before writing config data.
    bool pairingEnabled; /**< true if a passkey is configured; false = open access */
    // "That's the stupidest combination I've ever heard in my life!" — Spaceballs (1987)
    uint32_t pairingPin; /**< 6-digit BLE passkey (0–999999); only used when pairingEnabled */

    // Project-wide button-debounce window. Loaded from config.json at runtime;
    // defaults to 100 when the field is absent. Bounds 1..1000 are enforced by
    // the JSON schema (data/config.schema.json), so firmware trusts the
    // contract — values outside that range from a hand-edited config get
    // rejected at config-load time, not at use.
    uint32_t debounceMs; /**< Button-debounce window in milliseconds (1..1000, default 100) */
};

/**
 * @brief Global hardware configuration instance
 *
 * Defined in the platform-specific config.cpp inside each hardware package.
 */
extern HardwareConfig hardwareConfig;
