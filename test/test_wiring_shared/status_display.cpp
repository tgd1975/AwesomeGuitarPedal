#include "status_display.h"

#include "version.h"
#include <Arduino.h>

namespace wiring_test
{

    void printBanner(const WiringConfig& cfg, const char* configFilename)
    {
        Serial.println();
        Serial.println("============================================================");
        Serial.println("  AwesomeStudioPedal — wiring test tool");
        Serial.printf("  firmware %s | hardware %s | config %s\n",
                      FIRMWARE_VERSION,
                      cfg.hardware,
                      configFilename != nullptr ? configFilename : "<embedded>");
        Serial.println("============================================================");

        Serial.println("Buttons:");
        if (cfg.numButtons == 0)
        {
            Serial.println("  (none configured)");
        }
        for (uint8_t i = 0; i < cfg.numButtons; i++)
        {
            const auto& b = cfg.buttons[i];
            Serial.printf("  %-20s GPIO %2u  (active-%s%s)\n",
                          b.name,
                          static_cast<unsigned>(b.pin),
                          b.activeLow ? "low" : "high",
                          b.isSelect ? ", select" : "");
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
        Serial.println("Press '?' for help. (Bindings land in TASK-387.)");
        Serial.println("------------------------------------------------------------");
    }

} // namespace wiring_test
