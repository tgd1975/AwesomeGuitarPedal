#include "status_display.h"

#include "version.h"
#include <Arduino.h>

namespace wiring_test
{

    void
    printStatusBlock(const WiringConfig& cfg, const char* configFilename, const ButtonState* states)
    {
        Serial.println();
        Serial.println("============================================================");
        Serial.println("  AwesomeStudioPedal — wiring test tool");
        Serial.printf("  firmware %s | hardware %s | config %s\n",
                      FIRMWARE_VERSION,
                      cfg.hardware,
                      configFilename != nullptr ? configFilename : "<embedded>");
        Serial.printf("  debounce %lu ms\n", static_cast<unsigned long>(cfg.debounceMs));
        Serial.println("============================================================");

        Serial.println("Buttons:");
        if (cfg.numButtons == 0)
        {
            Serial.println("  (none configured)");
        }
        for (uint8_t i = 0; i < cfg.numButtons; i++)
        {
            const auto& b = cfg.buttons[i];
            uint32_t count = (states != nullptr) ? states[i].pressCount : 0u;
            Serial.printf("  %-20s GPIO %2u  (active-%s%s)  count=%lu\n",
                          b.name,
                          static_cast<unsigned>(b.pin),
                          b.activeLow ? "low" : "high",
                          b.isSelect ? ", select" : "",
                          static_cast<unsigned long>(count));
        }

        Serial.println("LEDs:");
        if (cfg.numLeds == 0)
        {
            Serial.println("  (none configured)");
        }
        for (uint8_t i = 0; i < cfg.numLeds; i++)
        {
            const auto& l = cfg.leds[i];
            Serial.printf("  %-20s GPIO %2u  (active-%s)\n",
                          l.name,
                          static_cast<unsigned>(l.pin),
                          l.activeHigh ? "high" : "low");
        }

        Serial.println("------------------------------------------------------------");
        Serial.println("Keys: 's' summary | '?' help (TASK-387). Press a button to log.");
        Serial.println("------------------------------------------------------------");
    }

} // namespace wiring_test
