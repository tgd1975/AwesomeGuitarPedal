#include "button.h"
#include <Arduino.h>

Button::Button(uint8_t PIN) : PIN(PIN) {}

Button::Button(uint8_t PIN, unsigned long debounceMs) : PIN(PIN), debounceDelay(debounceMs) {}

void Button::setup() { pinMode(PIN, INPUT_PULLUP); }

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
            lastDebounceTime = now;
        }
        return;
    }
    if (! awaitingRelease && isDebounced(now))
    {
        pressCount++;
        awaitingRelease = true;
        lastDebounceTime = now;
    }
}

bool Button::event()
{
    if (pressCount > 0)
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
    lastDebounceTime = 0;
}
