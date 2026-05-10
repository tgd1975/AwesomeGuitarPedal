#include "pedal_app.h"

#include <Arduino.h>

#include "button.h"
#include "config.h"
#include "delayed_action.h"
#include "event_dispatcher.h"
#include "i_ble_keyboard.h"
#include "led_controller.h"
#include "macro_action.h"
#include "pedal_config.h"
#include "platform.h"
#include "version.h"

namespace
{
    constexpr uint32_t LONG_PRESS_THRESHOLD_MS = 500;
}

void PedalApp::signalHardwareMismatch()
{
    Serial.println("HARDWARE MISMATCH: config.json targets a different board — halting");
    constexpr int BLINK_ON_MS = 80;
    constexpr int BLINK_OFF_MS = 80;
    constexpr int PAUSE_MS = 600;
    while (true)
    {
        for (int i = 0; i < 3; i++)
        {
            ledPower_->setState(true);
            delay(BLINK_ON_MS);
            ledPower_->setState(false);
            delay(BLINK_OFF_MS);
        }
        delay(PAUSE_MS);
    }
}

void PedalApp::signalLoadError()
{
    Serial.println("CONFIG ERROR: falling back to factory default");
    constexpr int BLINK_COUNT = 5;
    constexpr int BLINK_DURATION = 100;
    for (int i = 0; i < BLINK_COUNT; i++)
    {
        ledPower_->setState(true);
        ledBluetooth_->setState(true);
        for (auto* led : selectLeds_)
        {
            led->setState(true);
        }
        delay(BLINK_DURATION);
        ledPower_->setState(false);
        ledBluetooth_->setState(false);
        for (auto* led : selectLeds_)
        {
            led->setState(false);
        }
        delay(BLINK_DURATION);
    }
    ledPower_->setState(true);
}

void PedalApp::setupHardware()
{
    ledPower_ = new LEDController(hardwareConfig.ledPower);
    ledBluetooth_ = new LEDController(hardwareConfig.ledBluetooth);
    ledPower_->setup(1);
    ledBluetooth_->setup(0);

    for (uint8_t i = 0; i < hardwareConfig.numSelectLeds; i++)
    {
        selectLedObjects_[i] = new LEDController(hardwareConfig.ledSelect[i]);
        selectLedObjects_[i]->setup(i == 0 ? 1 : 0);
        selectLeds_.push_back(selectLedObjects_[i]);
    }

    for (uint8_t i = 0; i < hardwareConfig.numButtons; i++)
    {
        actionButtonObjects_[i] = new Button(hardwareConfig.buttonPins[i]);
        actionButtonObjects_[i]->setup();
    }

    buttonSelect_ = new Button(hardwareConfig.buttonSelect);
    buttonSelect_->setup();
}

void PedalApp::executeActionWithLogging(uint8_t buttonIndex)
{
    char btnLabel[2];
    Btn::name(buttonIndex, btnLabel);

    uint8_t profileIndex = profileManager_->getCurrentProfile();
    const char* profileName = profileManager_->getProfileName(profileIndex).c_str();
    Serial.printf("Button %s pressed (Profile: %s)\n", btnLabel, profileName);

    if (const Profile* independent = profileManager_->getIndependentActions())
    {
        if (Action* indAction = independent->getAction(buttonIndex))
        {
            const char* actionType = ProfileManager::getActionTypeString(indAction->getType());
            Serial.printf("  -> Executing independent %s action\n", actionType);
            indAction->execute();
        }
    }

    if (auto* action = profileManager_->getAction(profileIndex, buttonIndex))
    {
        if (action->isInProgress())
        {
            Serial.println("  -> DelayedAction in progress, ignoring");
            return;
        }

        const char* actionType = ProfileManager::getActionTypeString(action->getType());
        if (action->hasName())
        {
            Serial.printf("  -> Executing %s action [%s]\n", actionType, action->getName().c_str());
        }
        else
        {
            Serial.printf("  -> Executing %s action\n", actionType);
        }
        action->execute();
    }
    else
    {
        Serial.println("  -> no action configured");
    }
}

// Cognitive-complexity score (32) reflects the four nested-lambda branches
// (press/release/long-press/double-press), each with the same defensive shape:
// dispatch the independent-actions profile if present, then the active
// profile. Splitting the lambdas into separate helpers would scatter that
// invariant across the file without simplifying the registration story.
// NOLINTNEXTLINE(readability-function-cognitive-complexity)
void PedalApp::setupEventHandlers()
{
    for (uint8_t i = 0; i < hardwareConfig.numButtons; i++)
    {
        uint8_t idx = i;
        eventDispatcher_->registerHandler(idx, [this, idx]() { executeActionWithLogging(idx); });

        eventDispatcher_->registerReleaseHandler(
            idx,
            [this, idx]()
            {
                if (const Profile* independent = profileManager_->getIndependentActions())
                {
                    if (auto* indAction = independent->getAction(idx))
                    {
                        indAction->executeRelease();
                    }
                }
                uint8_t profile = profileManager_->getCurrentProfile();
                if (auto* action = profileManager_->getAction(profile, idx))
                {
                    action->executeRelease();
                }
            });
    }

    for (uint8_t i = 0; i < hardwareConfig.numButtons; i++)
    {
        uint8_t idx = i;
        eventDispatcher_->registerLongPressHandler(
            idx,
            [this, idx]()
            {
                if (const Profile* independent = profileManager_->getIndependentActions())
                {
                    if (auto* indAction = independent->getLongPressAction(idx))
                    {
                        indAction->execute();
                    }
                }
                uint8_t profile = profileManager_->getCurrentProfile();
                if (auto* action = profileManager_->getProfile(profile)->getLongPressAction(idx))
                {
                    action->execute();
                }
            },
            LONG_PRESS_THRESHOLD_MS);

        eventDispatcher_->registerDoublePressHandler(
            idx,
            [this, idx]()
            {
                if (const Profile* independent = profileManager_->getIndependentActions())
                {
                    if (auto* indAction = independent->getDoublePressAction(idx))
                    {
                        indAction->execute();
                    }
                }
                uint8_t profile = profileManager_->getCurrentProfile();
                if (auto* action = profileManager_->getProfile(profile)->getDoublePressAction(idx))
                {
                    action->execute();
                }
            });
    }

    uint8_t selectHandlerIdx = hardwareConfig.numButtons;
    eventDispatcher_->registerHandler(selectHandlerIdx,
                                      [this]()
                                      {
                                          uint8_t profile = profileManager_->switchProfile();
                                          Serial.printf("Switched to Profile %d\n", profile + 1);
                                          saveProfile(profile);
                                      });
}

void PedalApp::setupCommon()
{
    Serial.begin(115200);
    delay(2000);
    Serial.println("AwesomeStudioPedal " FIRMWARE_VERSION " started");

    bool hwOk = loadHardwareConfig();
    setupHardware();
    if (! hwOk)
    {
        signalHardwareMismatch(); // never returns
    }

    profileManager_ = new ProfileManager(selectLeds_);
    eventDispatcher_ = new EventDispatcher();
    setupEventHandlers();
}
