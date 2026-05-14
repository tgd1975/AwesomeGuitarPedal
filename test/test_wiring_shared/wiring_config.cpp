#include "wiring_config.h"

#include <ArduinoJson.h>
#include <stdio.h>
#include <string.h>

namespace wiring_test
{

    namespace
    {

        // Copy a string into a fixed buffer, always null-terminated.
        // Avoids strncpy on purpose — gcc's stringop-truncation warns
        // when src may be exactly dstLen-1, which the parser can hit
        // for long pinNames entries; a hand copy stays clean.
        void copyString(char* dst, size_t dstLen, const char* src)
        {
            if (dstLen == 0)
            {
                return;
            }
            if (src == nullptr)
            {
                dst[0] = '\0';
                return;
            }
            size_t i = 0;
            while (i + 1 < dstLen && src[i] != '\0')
            {
                dst[i] = src[i];
                ++i;
            }
            dst[i] = '\0';
        }

        // Look up the pin's symbolic name in the (optional) pinNames map. The
        // keys in the JSON are decimal pin numbers as strings; ArduinoJson treats
        // them as opaque keys so we format and compare directly.
        const char* lookupPinName(const ArduinoJson::JsonObject& pinNames, uint8_t pin)
        {
            if (pinNames.isNull())
            {
                return nullptr;
            }
            char key[8];
            snprintf(key, sizeof(key), "%u", static_cast<unsigned>(pin));
            if (! pinNames.containsKey(key))
            {
                return nullptr;
            }
            const char* name = pinNames[key];
            return (name != nullptr && name[0] != '\0') ? name : nullptr;
        }

        // Append a button entry, applying the pinNames override when present.
        void appendButton(WiringConfig& out,
                          uint8_t pin,
                          const char* fallbackName,
                          bool isSelect,
                          const ArduinoJson::JsonObject& pinNames)
        {
            if (out.numButtons >= kMaxButtons)
            {
                return;
            }
            WiringButton& b = out.buttons[out.numButtons++];
            const char* name = lookupPinName(pinNames, pin);
            copyString(b.name, sizeof(b.name), name != nullptr ? name : fallbackName);
            b.pin = pin;
            b.activeLow = true;
            b.isSelect = isSelect;
        }

        // Append an LED entry, applying the pinNames override when present.
        void appendLed(WiringConfig& out,
                       uint8_t pin,
                       const char* fallbackName,
                       const ArduinoJson::JsonObject& pinNames)
        {
            if (out.numLeds >= kMaxLeds)
            {
                return;
            }
            WiringLed& l = out.leds[out.numLeds++];
            const char* name = lookupPinName(pinNames, pin);
            copyString(l.name, sizeof(l.name), name != nullptr ? name : fallbackName);
            l.pin = pin;
            l.activeHigh = true;
        }

        ParseResult fail(ParseStatus status, const char* message)
        {
            ParseResult r{};
            r.status = status;
            copyString(r.message, sizeof(r.message), message);
            return r;
        }

    } // namespace

    ParseResult parseWiringConfig(const char* json, WiringConfig& out)
    {
        out = WiringConfig{};

        ArduinoJson::DynamicJsonDocument doc(2048);
        auto err = ArduinoJson::deserializeJson(doc, json);
        if (err)
        {
            char msg[96];
            snprintf(msg, sizeof(msg), "JSON parse error: %s", err.c_str());
            return fail(ParseStatus::InvalidJson, msg);
        }

        auto root = doc.as<ArduinoJson::JsonObject>();
        if (root.isNull())
        {
            return fail(ParseStatus::InvalidJson, "config root is not an object");
        }

        // pinNames is optional (EPIC-029 v1; absent on legacy configs).
        auto pinNames = root["pinNames"].as<ArduinoJson::JsonObject>();

        // EPIC-028 debounce window. Default to 100 ms when absent; clamp
        // to the schema's 1..1000 ms range so a hand-edited bad value
        // can't lock the tool out of reading any presses.
        uint32_t debounce =
            root.containsKey("debounceMs") ? root["debounceMs"].as<uint32_t>() : 100u;
        if (debounce < 1u)
        {
            debounce = 1u;
        }
        if (debounce > 1000u)
        {
            debounce = 1000u;
        }
        out.debounceMs = debounce;

        if (root.containsKey("hardware"))
        {
            copyString(out.hardware, sizeof(out.hardware), root["hardware"].as<const char*>());
        }
        else
        {
            copyString(out.hardware, sizeof(out.hardware), "unknown");
        }

        // Action buttons. buttonPins is required; numButtons (if present)
        // caps the visible set.
        auto buttonPins = root["buttonPins"].as<ArduinoJson::JsonArray>();
        if (buttonPins.isNull())
        {
            return fail(ParseStatus::MissingField, "buttonPins[] missing");
        }
        size_t arrLen = buttonPins.size();
        if (arrLen > 255u)
        {
            arrLen = 255u;
        }
        uint8_t numButtons = static_cast<uint8_t>(arrLen);
        if (root.containsKey("numButtons"))
        {
            uint8_t requested = root["numButtons"].as<uint8_t>();
            if (requested < numButtons)
            {
                numButtons = requested;
            }
        }
        for (uint8_t i = 0; i < numButtons; i++)
        {
            char fallback[4];
            snprintf(fallback, sizeof(fallback), "%c", static_cast<char>('A' + i));
            appendButton(out, buttonPins[i].as<uint8_t>(), fallback, /*isSelect=*/false, pinNames);
        }

        if (root.containsKey("buttonSelect"))
        {
            appendButton(
                out, root["buttonSelect"].as<uint8_t>(), "select", /*isSelect=*/true, pinNames);
        }

        if (root.containsKey("ledPower"))
        {
            appendLed(out, root["ledPower"].as<uint8_t>(), "ledPower", pinNames);
        }
        if (root.containsKey("ledBluetooth"))
        {
            appendLed(out, root["ledBluetooth"].as<uint8_t>(), "ledBluetooth", pinNames);
        }

        auto ledSelect = root["ledSelect"].as<ArduinoJson::JsonArray>();
        if (! ledSelect.isNull())
        {
            size_t selLen = ledSelect.size();
            if (selLen > 255u)
            {
                selLen = 255u;
            }
            uint8_t numSelectLeds = static_cast<uint8_t>(selLen);
            if (root.containsKey("numSelectLeds"))
            {
                uint8_t requested = root["numSelectLeds"].as<uint8_t>();
                if (requested < numSelectLeds)
                {
                    numSelectLeds = requested;
                }
            }
            for (uint8_t i = 0; i < numSelectLeds; i++)
            {
                char fallback[16];
                snprintf(fallback, sizeof(fallback), "ledSelect[%u]", static_cast<unsigned>(i));
                appendLed(out, ledSelect[i].as<uint8_t>(), fallback, pinNames);
            }
        }

        if (out.numButtons == 0 && out.numLeds == 0)
        {
            return fail(ParseStatus::MissingField, "no buttons or LEDs in config");
        }

        return ParseResult{ParseStatus::Ok, ""};
    }

} // namespace wiring_test
