#pragma once
#include "i_led_controller.h"

/**
 * @class NullLEDController
 * @brief No-op ILEDController for tests that don't care about LED state
 */
class NullLEDController : public ILEDController
{
public:
    void setup(uint32_t) override {}
    void setState(bool) override {}
    void toggle() override {}
    void startBlink(uint32_t, int16_t) override {}
    void stopBlink() override {}
    void update(uint32_t) override {}
    bool isBlinking() const override { return false; }
};
