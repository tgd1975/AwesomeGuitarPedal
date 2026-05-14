#include "status_display.h"

#include "version.h"
#include <Arduino.h>

namespace wiring_test
{

    namespace
    {

        void printModeLine(const LedRuntime* runtime, uint32_t nowMs)
        {
            if (runtime == nullptr)
            {
                Serial.println("LED mode: off (uninit)");
                return;
            }
            GroupMode top = runtime->groupMode;
            if (top == GroupMode::CycleAll)
            {
                GroupMode active = effectiveMode(*runtime, nowMs);
                Serial.printf("LED mode: cycle-all -> %s  (sub-mode %u/%u)\n",
                              groupModeName(active),
                              static_cast<unsigned>(runtime->cycleSubModeIndex + 1u),
                              static_cast<unsigned>(kCycleSubModeCount));
            }
            else
            {
                Serial.printf("LED mode: %s\n", groupModeName(top));
            }
        }

    } // namespace

    void printStatusBlock(const WiringConfig& cfg,
                          const char* configFilename,
                          const ButtonState* states,
                          const LedRuntime* runtime,
                          uint32_t nowMs)
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

        Serial.println("LEDs (chase iteration order top-to-bottom):");
        if (cfg.numLeds == 0)
        {
            Serial.println("  (none configured)");
        }
        for (uint8_t i = 0; i < cfg.numLeds; i++)
        {
            const auto& l = cfg.leds[i];
            Serial.printf("  [%u] %-20s GPIO %2u  (active-%s)\n",
                          static_cast<unsigned>(i),
                          l.name,
                          static_cast<unsigned>(l.pin),
                          l.activeHigh ? "high" : "low");
        }

        printModeLine(runtime, nowMs);

        Serial.println("------------------------------------------------------------");
        Serial.println(
            "Keys: 's' summary | 'o'/'f'/'b'/'c'/'C'/'a' LED group modes | '?' help (TASK-387)");
        Serial.println("------------------------------------------------------------");
    }

} // namespace wiring_test
