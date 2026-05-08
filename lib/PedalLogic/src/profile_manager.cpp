#include "profile_manager.h"
#include "config.h"
#include "delayed_action.h"
#include <vector>

ProfileManager::ProfileManager(std::vector<ILEDController*> leds) : selectLeds(std::move(leds))
{
    updateLEDs();
}

void ProfileManager::addProfile(uint8_t profileIndex, std::unique_ptr<Profile> profile)
{
    if (profileIndex < MAX_PROFILES)
    {
        profileSlots[profileIndex] = std::move(profile);
    }
}

Action* ProfileManager::getAction(uint8_t profileIndex, uint8_t button) const
{
    if (profileIndex < MAX_PROFILES && profileSlots[profileIndex])
    {
        return profileSlots[profileIndex]->getAction(button);
    }
    return nullptr;
}

const Profile* ProfileManager::getProfile(uint8_t profileIndex) const
{
    if (profileIndex < MAX_PROFILES && profileSlots[profileIndex])
    {
        return profileSlots[profileIndex].get();
    }
    return nullptr;
}

void ProfileManager::setIndependentActions(std::unique_ptr<Profile> independentActions)
{
    independentActions_ = std::move(independentActions);
}

uint8_t ProfileManager::switchProfile()
{
    uint8_t numProfiles = hardwareConfig.numProfiles;
    uint8_t next = currentProfile;
    for (uint8_t i = 0; i < numProfiles; i++)
    {
        int temp = (next + 1) % numProfiles;
        next = static_cast<uint8_t>(temp);
        if (profileSlots[next])
        {
            break;
        }
    }
    currentProfile = next;
    updateLEDs();

    return currentProfile;
}

void ProfileManager::triggerBlink()
{
    postSwitchBlink = true;
    blinkStarted = false;
    blinkPhase = 0;
    blinkStartTime = 0;
}

/**
 * LED encoding:
 *   One-hot mode  (numProfiles <= numSelectLeds):
 *     Profile N (0-based) lights LED N exclusively.
 *   Binary mode   (numProfiles > numSelectLeds):
 *     1-based profile number encoded in binary across all select LEDs.
 *     LED 0 = bit 0 (LSB), LED 1 = bit 1, etc.
 */
void ProfileManager::updateLEDs()
{
    if (postSwitchBlink)
    {
        return;
    }

    uint8_t numLeds = hardwareConfig.numSelectLeds;
    uint8_t numProfiles = hardwareConfig.numProfiles;

    if (numProfiles <= numLeds)
    {
        // One-hot: exactly one LED on for the current profile
        for (uint8_t i = 0; i < numLeds && i < static_cast<uint8_t>(selectLeds.size()); i++)
        {
            selectLeds[i]->setState(i == currentProfile);
        }
    }
    else
    {
        // Binary: encode 1-based profile number
        auto bits = static_cast<uint8_t>(currentProfile + 1);
        for (uint8_t i = 0; i < numLeds && i < static_cast<uint8_t>(selectLeds.size()); i++)
        {
            selectLeds[i]->setState((bits & (1U << i)) != 0);
        }
    }
}

void ProfileManager::update(uint32_t now)
{
    if (! postSwitchBlink)
    {
        return;
    }

    if (! blinkStarted)
    {
        blinkStarted = true;
        blinkStartTime = now;
        for (auto* led : selectLeds)
        {
            led->setState(true);
        }
        return;
    }

    uint32_t elapsed = now - blinkStartTime;
    auto phase = static_cast<uint8_t>(elapsed / BLINK_INTERVAL);

    if (phase == blinkPhase)
    {
        return;
    }
    blinkPhase = phase;

    uint8_t totalHalfCycles = BLINK_COUNT * 2;
    if (phase >= totalHalfCycles)
    {
        postSwitchBlink = false;
        updateLEDs();
        return;
    }

    bool on = (phase % 2 == 0);
    for (auto* led : selectLeds)
    {
        led->setState(on);
    }
}

void ProfileManager::setCurrentProfile(uint8_t profileIndex)
{
    if (profileIndex < MAX_PROFILES && profileSlots[profileIndex])
    {
        currentProfile = profileIndex;
        updateLEDs();
    }
}

void ProfileManager::resetToFirstProfile()
{
    currentProfile = 0;
    uint8_t numProfiles = hardwareConfig.numProfiles;
    for (uint8_t i = 0; i < numProfiles; i++)
    {
        if (profileSlots[i])
        {
            currentProfile = i;
            break;
        }
    }
    triggerBlink();
}

const std::string& ProfileManager::getProfileName(uint8_t profileIndex) const
{
    if (profileIndex < MAX_PROFILES && profileSlots[profileIndex])
    {
        return profileSlots[profileIndex]->getName();
    }
    static const std::string emptyString;
    return emptyString;
}

bool ProfileManager::hasActiveDelayedAction() const
{
    for (const auto& slot : profileSlots)
    {
        if (! slot)
        {
            continue;
        }
        for (uint8_t b = 0; b < Profile::MAX_BUTTONS; b++)
        {
            const Action* action = slot->getAction(b);
            if (action && action->isInProgress())
            {
                return true;
            }
        }
    }
    if (independentActions_)
    {
        for (uint8_t b = 0; b < Profile::MAX_BUTTONS; b++)
        {
            const Action* action = independentActions_->getAction(b);
            if (action && action->isInProgress())
            {
                return true;
            }
        }
    }
    return false;
}

const char* ProfileManager::getActionTypeString(Action::Type actionType)
{
    switch (actionType)
    {
        case Action::Type::SendString:
            return "SendString";
        case Action::Type::SendChar:
            return "SendChar";
        case Action::Type::SendKey:
            return "SendKey";
        case Action::Type::SendMediaKey:
            return "SendMediaKey";
        case Action::Type::SerialOutput:
            return "SerialOutput";
        case Action::Type::Delayed:
            return "Delayed";
        case Action::Type::PinHigh:
            return "PinHigh";
        case Action::Type::PinLow:
            return "PinLow";
        case Action::Type::PinToggle:
            return "PinToggle";
        case Action::Type::PinHighWhilePressed:
            return "PinHighWhilePressed";
        case Action::Type::PinLowWhilePressed:
            return "PinLowWhilePressed";
        default:
            return "Unknown";
    }
}
