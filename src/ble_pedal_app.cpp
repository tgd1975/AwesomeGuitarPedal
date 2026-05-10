#include "ble_pedal_app.h"

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

// attachInterrupt() takes a free function pointer (not a member function),
// so each ISR stub forwards through this global to the live BlePedalApp
// instance. Const-ifying it would defeat the constructor's `g_blePedalApp =
// this;` registration (and the destructor's nullptr clear). One process,
// one app — but the symbol must be mutable.
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
BlePedalApp* g_blePedalApp = nullptr;

namespace
{
    constexpr uint32_t LONG_PRESS_THRESHOLD_MS = 500;

    // Individual ISR stubs for up to 26 action buttons. attachInterrupt() takes
    // a free function pointer (not a member function), so each stub forwards to
    // the BlePedalApp instance via the global g_blePedalApp pointer.
    void IRAM_ATTR isr_btn0()
    {
        if (g_blePedalApp)
        {
            g_blePedalApp->onActionButtonInterrupt(0);
        }
    }
    void IRAM_ATTR isr_btn1()
    {
        if (g_blePedalApp)
        {
            g_blePedalApp->onActionButtonInterrupt(1);
        }
    }
    void IRAM_ATTR isr_btn2()
    {
        if (g_blePedalApp)
        {
            g_blePedalApp->onActionButtonInterrupt(2);
        }
    }
    void IRAM_ATTR isr_btn3()
    {
        if (g_blePedalApp)
        {
            g_blePedalApp->onActionButtonInterrupt(3);
        }
    }
    void IRAM_ATTR isr_btn4()
    {
        if (g_blePedalApp)
        {
            g_blePedalApp->onActionButtonInterrupt(4);
        }
    }
    void IRAM_ATTR isr_btn5()
    {
        if (g_blePedalApp)
        {
            g_blePedalApp->onActionButtonInterrupt(5);
        }
    }
    void IRAM_ATTR isr_btn6()
    {
        if (g_blePedalApp)
        {
            g_blePedalApp->onActionButtonInterrupt(6);
        }
    }
    void IRAM_ATTR isr_btn7()
    {
        if (g_blePedalApp)
        {
            g_blePedalApp->onActionButtonInterrupt(7);
        }
    }
    void IRAM_ATTR isr_btn8()
    {
        if (g_blePedalApp)
        {
            g_blePedalApp->onActionButtonInterrupt(8);
        }
    }
    void IRAM_ATTR isr_btn9()
    {
        if (g_blePedalApp)
        {
            g_blePedalApp->onActionButtonInterrupt(9);
        }
    }
    void IRAM_ATTR isr_btn10()
    {
        if (g_blePedalApp)
        {
            g_blePedalApp->onActionButtonInterrupt(10);
        }
    }
    void IRAM_ATTR isr_btn11()
    {
        if (g_blePedalApp)
        {
            g_blePedalApp->onActionButtonInterrupt(11);
        }
    }
    void IRAM_ATTR isr_btn12()
    {
        if (g_blePedalApp)
        {
            g_blePedalApp->onActionButtonInterrupt(12);
        }
    }
    void IRAM_ATTR isr_btn13()
    {
        if (g_blePedalApp)
        {
            g_blePedalApp->onActionButtonInterrupt(13);
        }
    }
    void IRAM_ATTR isr_btn14()
    {
        if (g_blePedalApp)
        {
            g_blePedalApp->onActionButtonInterrupt(14);
        }
    }
    void IRAM_ATTR isr_btn15()
    {
        if (g_blePedalApp)
        {
            g_blePedalApp->onActionButtonInterrupt(15);
        }
    }
    void IRAM_ATTR isr_btn16()
    {
        if (g_blePedalApp)
        {
            g_blePedalApp->onActionButtonInterrupt(16);
        }
    }
    void IRAM_ATTR isr_btn17()
    {
        if (g_blePedalApp)
        {
            g_blePedalApp->onActionButtonInterrupt(17);
        }
    }
    void IRAM_ATTR isr_btn18()
    {
        if (g_blePedalApp)
        {
            g_blePedalApp->onActionButtonInterrupt(18);
        }
    }
    void IRAM_ATTR isr_btn19()
    {
        if (g_blePedalApp)
        {
            g_blePedalApp->onActionButtonInterrupt(19);
        }
    }
    void IRAM_ATTR isr_btn20()
    {
        if (g_blePedalApp)
        {
            g_blePedalApp->onActionButtonInterrupt(20);
        }
    }
    void IRAM_ATTR isr_btn21()
    {
        if (g_blePedalApp)
        {
            g_blePedalApp->onActionButtonInterrupt(21);
        }
    }
    void IRAM_ATTR isr_btn22()
    {
        if (g_blePedalApp)
        {
            g_blePedalApp->onActionButtonInterrupt(22);
        }
    }
    void IRAM_ATTR isr_btn23()
    {
        if (g_blePedalApp)
        {
            g_blePedalApp->onActionButtonInterrupt(23);
        }
    }
    void IRAM_ATTR isr_btn24()
    {
        if (g_blePedalApp)
        {
            g_blePedalApp->onActionButtonInterrupt(24);
        }
    }
    void IRAM_ATTR isr_btn25()
    {
        if (g_blePedalApp)
        {
            g_blePedalApp->onActionButtonInterrupt(25);
        }
    }
    void IRAM_ATTR isr_select()
    {
        if (g_blePedalApp)
        {
            g_blePedalApp->onSelectButtonInterrupt();
        }
    }

    using IsrFunc = void (*)();
    const IsrFunc BTN_ISRS[Btn::MAX] = {
        isr_btn0,  isr_btn1,  isr_btn2,  isr_btn3,  isr_btn4,  isr_btn5,  isr_btn6,
        isr_btn7,  isr_btn8,  isr_btn9,  isr_btn10, isr_btn11, isr_btn12, isr_btn13,
        isr_btn14, isr_btn15, isr_btn16, isr_btn17, isr_btn18, isr_btn19, isr_btn20,
        isr_btn21, isr_btn22, isr_btn23, isr_btn24, isr_btn25,
    };
} // namespace

BlePedalApp::BlePedalApp(IBleKeyboard* bleKeyboard) : bleKeyboard_(bleKeyboard)
{
    g_blePedalApp = this;
}

BlePedalApp::~BlePedalApp()
{
    if (g_blePedalApp == this)
    {
        g_blePedalApp = nullptr;
    }
}

void BlePedalApp::onActionButtonInterrupt(uint8_t index)
{
    if (index < Btn::MAX && actionButtonObjects()[index])
    {
        actionButtonObjects()[index]->isr();
    }
}

void BlePedalApp::onSelectButtonInterrupt()
{
    if (buttonSelect())
    {
        buttonSelect()->isr();
    }
}

void BlePedalApp::attachInterrupts()
{
    for (uint8_t i = 0; i < hardwareConfig.numButtons; i++)
    {
        actionButtonObjects()[i]->reset();
        attachInterrupt(hardwareConfig.buttonPins[i], BTN_ISRS[i], CHANGE);
    }
    buttonSelect()->reset();
    attachInterrupt(hardwareConfig.buttonSelect, isr_select, CHANGE);
}

void BlePedalApp::detachInterrupts()
{
    for (uint8_t i = 0; i < hardwareConfig.numButtons; i++)
    {
        actionButtonObjects()[i]->reset();
        detachInterrupt(hardwareConfig.buttonPins[i]);
    }
    buttonSelect()->reset();
    detachInterrupt(hardwareConfig.buttonSelect);
}

void BlePedalApp::processEvents()
{
    for (uint8_t i = 0; i < hardwareConfig.numButtons; i++)
    {
        if (! actionButtonObjects()[i])
        {
            continue;
        }

        if (actionButtonObjects()[i]->doublePressEvent())
        {
            eventDispatcher()->dispatchDoublePress(i);
        }
        else if (actionButtonObjects()[i]->event())
        {
            eventDispatcher()->dispatch(i);
        }

        if (actionButtonObjects()[i]->holdDurationMs() >= LONG_PRESS_THRESHOLD_MS &&
            ! longPressArmed_[i])
        {
            eventDispatcher()->dispatchLongPress(i);
            longPressArmed_[i] = true;
        }
        if (! actionButtonObjects()[i]->awaitingRelease)
        {
            longPressArmed_[i] = false;
        }

        if (actionButtonObjects()[i]->releaseEvent())
        {
            eventDispatcher()->dispatchRelease(i);
        }
    }

    if (buttonSelect()->event())
    {
        eventDispatcher()->dispatch(hardwareConfig.numButtons);
    }
}

void BlePedalApp::setup()
{
    setupCommon();

    platformSetup();
    bleKeyboard_->begin();

    if (! configureProfiles(*profileManager(), bleKeyboard_))
    {
        signalLoadError();
    }

    uint8_t savedProfile = loadProfile();
    if (savedProfile != profileManager()->getCurrentProfile() &&
        profileManager()->getProfile(savedProfile) != nullptr)
    {
        profileManager()->setCurrentProfile(savedProfile);
        Serial.printf("Restored profile %d from NVS\n", savedProfile + 1);
    }

    attachInterrupts();
}

void BlePedalApp::loop()
{
    if (bleKeyboard_->isConnected())
    {
        if (! connected_)
        {
            Serial.println("connected");
            attachInterrupts();
            ledBluetooth()->setState(true);
            connected_ = true;
        }
    }
    else
    {
        if (connected_)
        {
            Serial.println("disconnected");
            connected_ = false;
            detachInterrupts();
            ledBluetooth()->setState(false);
        }
    }
    processEvents();
    platformLoop();

    auto now = static_cast<uint32_t>(millis());

    {
        uint8_t profile = profileManager()->getCurrentProfile();
        for (uint8_t i = 0; i < hardwareConfig.numButtons; i++)
        {
            Action* action = profileManager()->getAction(profile, i);
            if (action && action->isInProgress())
            {
                action->execute();
                // Type tag is checked first; static_cast is the RTTI-free
                // downcast pattern used throughout the codebase (Arduino
                // RTTI is disabled by default to save flash).
                if (action->getType() == Action::Type::Macro)
                {
                    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
                    auto* macro = static_cast<MacroAction*>(action);
                    macro->update();
                }
            }
        }
    }

    ledPower()->update(now);
    ledBluetooth()->update(now);
    for (auto* led : selectLeds())
    {
        led->update(now);
    }

    profileManager()->update(now);

    if (profileManager()->hasActiveDelayedAction())
    {
        if (! ledPower()->isBlinking())
        {
            ledPower()->startBlink(500);
        }
    }
    else
    {
        if (ledPower()->isBlinking())
        {
            ledPower()->stopBlink();
            ledPower()->setState(true);
        }
    }

    delay(10);
}
