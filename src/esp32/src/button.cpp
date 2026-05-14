#include "button.h"
#include <Arduino.h>
#if __has_include(<driver/gpio.h>)
#include <driver/gpio.h>
#endif

Button::Button(uint8_t PIN) : PIN(PIN) {}

Button::Button(uint8_t PIN, unsigned long debounceMs) : PIN(PIN), debounceDelay(debounceMs) {}

void Button::setup()
{
    gpio_pad_select_gpio(static_cast<gpio_num_t>(PIN));
    pinMode(PIN, INPUT_PULLUP);
}

bool Button::isDebounced(unsigned long now) const
{
    return (now - lastDebounceTime) > debounceDelay;
}

void Button::isr()
{
    unsigned long now = millis();
    if (digitalRead(PIN) == HIGH)
    {
        if (awaitingRelease && isDebounced(now))
        {
            awaitingRelease = false;
            released = true;
            lastDebounceTime = now;
        }
        return;
    }
    if (! awaitingRelease && isDebounced(now))
    {
        if ((now - lastPressTime_) < doublePressWindow_ && lastPressTime_ != 0)
        {
            doublePressFlag_ = true;
            pressCount = 0; // both taps consumed by the double press
        }
        else
        {
            pressCount++;
        }
        lastPressTime_ = now;
        pressStartTime_ = now;
        awaitingRelease = true;
        lastDebounceTime = now;
    }
}

bool Button::event()
{
    if (pressCount > 0 && (millis() - lastPressTime_) >= doublePressWindow_)
    {
        pressCount--;
        return true;
    }
    return false;
}

void Button::reset()
{
    pressCount = 0;
    awaitingRelease = false;
    released = false;
    lastDebounceTime = 0;
    pressStartTime_ = 0;
    lastPressTime_ = 0;
    doublePressFlag_ = false;
}

void Button::setDoublePressWindow(unsigned long ms) { doublePressWindow_ = ms; }

unsigned long Button::holdDurationMs() const
{
    if (awaitingRelease)
    {
        return millis() - pressStartTime_;
    }
    return 0;
}

bool Button::doublePressEvent()
{
    if (doublePressFlag_)
    {
        doublePressFlag_ = false;
        return true;
    }
    return false;
}

bool Button::releaseEvent()
{
    if (released)
    {
        released = false;
        return true;
    }
    return false;
}
