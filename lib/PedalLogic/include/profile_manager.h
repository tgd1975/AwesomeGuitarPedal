#pragma once
#include "config.h"
#include "i_led_controller.h"
#include "profile.h"
#include <array>
#include <memory>
#include <vector>

/**
 * @class ProfileManager
 * @brief Manages up to MAX_PROFILES profiles with LED feedback
 *
 * LED encoding (chosen at runtime based on numProfiles vs numSelectLeds):
 *   - One-hot mode (numProfiles <= numSelectLeds):
 *     Profile N lights LED N exclusively. Intuitive and direct.
 *   - Binary mode (numProfiles > numSelectLeds):
 *     The 1-based profile number is binary-encoded across all select LEDs.
 *     LED 1 = bit 0 (LSB), LED 2 = bit 1, etc.
 *
 * Maximum profiles for a given wiring: 2^numSelectLeds − 1.
 *
 * After a profile switch all select LEDs blink 3 times, then return to the
 * encoding for the new profile.  Drive this by calling update() every loop.
 */
class ProfileManager
{
public:
    static constexpr uint8_t MAX_PROFILES = 63; /**< 2^6 − 1 (6 select LEDs max) */
    static constexpr uint8_t MAX_SELECT_LEDS = 6;

    /**
     * @param leds   Pointers to LED controllers for profile-select LEDs.
     *               Size must match hardwareConfig.numSelectLeds.
     */
    explicit ProfileManager(std::vector<ILEDController*> leds);

    void addProfile(uint8_t profileIndex, std::unique_ptr<Profile> profile);
    Action* getAction(uint8_t profileIndex, uint8_t button) const;
    const Profile* getProfile(uint8_t profileIndex) const;

    /**
     * @brief Install the profile-independent action set
     *
     * Stores a Profile-shaped action set that fires alongside the active
     * profile's actions on every button event, regardless of which profile
     * is selected. Pass nullptr to clear. Switching profiles never touches
     * this set. See IDEA-024 / TASK-306.
     */
    void setIndependentActions(std::unique_ptr<Profile> independentActions);

    /**
     * @brief Returns the profile-independent action set, or nullptr if none.
     */
    const Profile* getIndependentActions() const { return independentActions_.get(); }

    /**
     * @brief Advance to the next populated profile slot and trigger blink feedback
     *
     * Skips empty slots and wraps around.  Triggers a 3-blink sequence on all
     * select LEDs; the LEDs return to the profile encoding after the sequence completes.
     *
     * @return New current profile index
     */
    uint8_t switchProfile();

    uint8_t getCurrentProfile() const { return currentProfile; }

    /**
     * @brief Directly set the current profile without triggering blink feedback
     *
     * Used to restore a saved profile on boot. Caller must ensure the profile
     * slot is populated before calling.
     */
    void setCurrentProfile(uint8_t profileIndex);

    const std::string& getProfileName(uint8_t profileIndex) const;

    /**
     * @brief Reset to the first populated slot (or slot 0 if all empty)
     *
     * Called after loadFromString clears and repopulates all slots so that
     * currentProfile is never left pointing at an empty slot.
     * Also triggers the blink sequence to confirm the new config was ingested.
     */
    void resetToFirstProfile();

    /**
     * @brief Trigger the select-LED blink sequence
     *
     * Call this after a configuration is ingested (loaded from storage or serial).
     * Normal profile switches via switchProfile() do NOT blink.
     */
    void triggerBlink();

    /**
     * @brief Returns true if any DelayedAction across all profiles is currently running
     */
    bool hasActiveDelayedAction() const;

    /**
     * @brief Drive timed LED behaviour — call every loop iteration
     *
     * @param now Current time in milliseconds (millis())
     */
    void update(uint32_t now);

    static const char* getActionTypeString(Action::Type actionType);

private:
    void updateLEDs();

    std::array<std::unique_ptr<Profile>, MAX_PROFILES> profileSlots;
    std::unique_ptr<Profile> independentActions_; /**< Profile-independent actions (IDEA-024) */
    uint8_t currentProfile = 0;

    std::vector<ILEDController*> selectLeds; /**< Profile-select LED controllers */

    // Post-switch blink state
    bool postSwitchBlink = false;
    bool blinkStarted = false;
    uint32_t blinkStartTime = 0;
    uint8_t blinkPhase = 0;                         // counts half-cycles (on/off)
    static constexpr uint8_t BLINK_COUNT = 3;       // full on/off cycles
    static constexpr uint32_t BLINK_INTERVAL = 150; // ms per half-cycle
};
