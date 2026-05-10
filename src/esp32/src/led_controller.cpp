#include "led_controller.h"

LEDController::LEDController(uint8_t pin) : pin(pin) {}

void LEDController::setup(uint32_t initialState)
{
    gpio_pad_select_gpio(static_cast<gpio_num_t>(pin));
    gpio_set_direction(static_cast<gpio_num_t>(pin), GPIO_MODE_OUTPUT);
    setState(initialState != 0);
}

void LEDController::setState(bool state)
{
    if (currentState != state)
    {
        gpio_set_level(static_cast<gpio_num_t>(pin), state ? 1 : 0);
        currentState = state;
    }
}

void LEDController::toggle() { setState(! currentState); }

void LEDController::startBlink(uint32_t intervalMs, int16_t count)
{
    if (blinking)
    {
        return; // already blinking — caller must stopBlink() first
    }
    stateBeforeBlink = currentState;
    blinkInterval = intervalMs;
    // count is full on/off cycles; we track half-cycles internally. Bounded
    // by callers (count fits comfortably in int16_t before doubling), so the
    // narrowing back from int*int promotion is intentional.
    // NOLINTNEXTLINE(bugprone-narrowing-conversions,cppcoreguidelines-narrowing-conversions)
    blinkRemaining = (count < 0) ? -1 : static_cast<int16_t>(count * 2);
    blinking = true;
    lastToggleTime = 0; // will be set on first update() call
}

void LEDController::stopBlink()
{
    if (! blinking)
    {
        return;
    }
    blinking = false;
    setState(stateBeforeBlink);
}

void LEDController::update(uint32_t now)
{
    if (! blinking)
    {
        return;
    }

    // Initialise lastToggleTime on the first update after startBlink
    if (lastToggleTime == 0)
    {
        lastToggleTime = now;
        setState(true); // start with LED on
        return;
    }

    if (now - lastToggleTime < blinkInterval)
    {
        return;
    }

    lastToggleTime = now;
    toggle();

    if (blinkRemaining > 0)
    {
        blinkRemaining--;
        if (blinkRemaining == 0)
        {
            stopBlink();
        }
    }
}
