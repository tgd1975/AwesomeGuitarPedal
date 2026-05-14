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
            if (runtime->topMode == TopMode::Individual)
            {
                Serial.printf("LED mode: individual  (selected: [%u])\n",
                              static_cast<unsigned>(runtime->selectedLed));
                return;
            }
            if (runtime->groupMode == GroupMode::CycleAll)
            {
                GroupMode active = effectiveMode(*runtime, nowMs);
                Serial.printf("LED mode: group cycle-all -> %s  (sub-mode %u/%u)\n",
                              groupModeName(active),
                              static_cast<unsigned>(runtime->cycleSubModeIndex + 1u),
                              static_cast<unsigned>(kCycleSubModeCount));
            }
            else
            {
                Serial.printf("LED mode: group %s\n", groupModeName(runtime->groupMode));
            }
        }

        bool isSelectedHere(const LedRuntime* runtime, uint8_t i)
        {
            return runtime != nullptr && runtime->topMode == TopMode::Individual &&
                   runtime->selectedLed == i;
        }

        bool individualLitHere(const LedRuntime* runtime, uint8_t i)
        {
            return runtime != nullptr && runtime->topMode == TopMode::Individual &&
                   runtime->individualLit[i];
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
        bool inIndividual = runtime != nullptr && runtime->topMode == TopMode::Individual;
        for (uint8_t i = 0; i < cfg.numLeds; i++)
        {
            const auto& l = cfg.leds[i];
            const char* selectMark = isSelectedHere(runtime, i) ? "*" : " ";
            if (inIndividual)
            {
                Serial.printf("  %s [%u] %-20s GPIO %2u  (active-%s)  state=%s\n",
                              selectMark,
                              static_cast<unsigned>(i),
                              l.name,
                              static_cast<unsigned>(l.pin),
                              l.activeHigh ? "high" : "low",
                              individualLitHere(runtime, i) ? "ON" : "off");
            }
            else
            {
                Serial.printf("    [%u] %-20s GPIO %2u  (active-%s)\n",
                              static_cast<unsigned>(i),
                              l.name,
                              static_cast<unsigned>(l.pin),
                              l.activeHigh ? "high" : "low");
            }
        }

        printModeLine(runtime, nowMs);

        Serial.println("------------------------------------------------------------");
        Serial.println("Press '?' for the keystroke legend.");
        Serial.println("------------------------------------------------------------");
    }

    namespace
    {

        void printGroupLegend()
        {
            Serial.println("--- Group-mode keys ---");
            Serial.println("  o   all LEDs ON");
            Serial.println("  f   all LEDs OFF");
            Serial.println("  b   all LEDs BLINK at ~2 Hz");
            Serial.println("  c   chase-on  (all off, walker lights)");
            Serial.println("  C   chase-off (all on, walker darkens)");
            Serial.println("  a   cycle-all (sweep every mode above on a 3 s cadence)");
            Serial.println("  m   switch to INDIVIDUAL mode");
        }

        void printIndividualLegend()
        {
            Serial.println("--- Individual-mode keys ---");
            Serial.println("  m         switch to GROUP mode");
            Serial.println("  n / p     next / previous LED in the iteration order");
            Serial.println("  g<digit>  go to LED by index (0..numLeds-1)");
            Serial.println("  o / f     selected LED on / off");
            Serial.println("  O / F     every non-selected LED on / off");
            Serial.println("  t         all-toggle (sel = inverted state of others;");
            Serial.println("            successive presses alternate parity)");
        }

        void printMiscLegend()
        {
            Serial.println("--- Always available ---");
            Serial.println("  s   reprint the status block");
            Serial.println("  ?   reprint this legend");
            Serial.println("  q   reset the device");
        }

    } // namespace

    void printHelpLegend(const LedRuntime* runtime)
    {
        Serial.println();
        Serial.println("============================================================");
        Serial.println("  Wiring test tool — keystroke legend");
        Serial.println("============================================================");

        bool inIndividual = (runtime != nullptr) && (runtime->topMode == TopMode::Individual);
        if (runtime == nullptr)
        {
            printGroupLegend();
            Serial.println();
            printIndividualLegend();
        }
        else if (inIndividual)
        {
            printIndividualLegend();
        }
        else
        {
            printGroupLegend();
        }
        Serial.println();
        printMiscLegend();
        Serial.println("============================================================");
    }

} // namespace wiring_test
