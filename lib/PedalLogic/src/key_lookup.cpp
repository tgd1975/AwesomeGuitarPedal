#include "key_lookup.h"
#include <cstring>

// ---- Key/action lookup tables ----

namespace
{

    struct KeyEntry
    {
        const char* name;
        uint8_t code;
    };
    struct MediaKeyEntry
    {
        const char* name;
        const uint8_t* report;
    };
    struct ActionTypeEntry
    {
        const char* name;
        Action::Type type;
    };

    static const KeyEntry KEY_TABLE[] = {
        // Arrow keys
        {"KEY_LEFT_ARROW", KEY_LEFT_ARROW},
        {"KEY_RIGHT_ARROW", KEY_RIGHT_ARROW},
        {"KEY_UP_ARROW", KEY_UP_ARROW},
        {"KEY_DOWN_ARROW", KEY_DOWN_ARROW},
        // Legacy names (no KEY_ prefix)
        {"LEFT_ARROW", KEY_LEFT_ARROW},
        {"RIGHT_ARROW", KEY_RIGHT_ARROW},
        {"UP_ARROW", KEY_UP_ARROW},
        {"DOWN_ARROW", KEY_DOWN_ARROW},
        // Navigation
        {"KEY_PAGE_UP", KEY_PAGE_UP},
        {"KEY_PAGE_DOWN", KEY_PAGE_DOWN},
        {"KEY_HOME", KEY_HOME},
        {"KEY_END", KEY_END},
        {"KEY_INSERT", KEY_INSERT},
        {"KEY_DELETE", KEY_DELETE},
        // Control
        {"KEY_BACKSPACE", KEY_BACKSPACE},
        {"KEY_TAB", KEY_TAB},
        {"KEY_RETURN", KEY_RETURN},
        {"KEY_ENTER", KEY_RETURN},
        {"KEY_ESC", KEY_ESC},
        {"KEY_CAPS_LOCK", KEY_CAPS_LOCK},
        {"KEY_PRINTSCREEN", KEY_PRTSC},
        // Modifiers
        {"KEY_LEFT_CTRL", KEY_LEFT_CTRL},
        {"KEY_LEFT_SHIFT", KEY_LEFT_SHIFT},
        {"KEY_LEFT_ALT", KEY_LEFT_ALT},
        {"KEY_LEFT_GUI", KEY_LEFT_GUI},
        {"KEY_RIGHT_CTRL", KEY_RIGHT_CTRL},
        {"KEY_RIGHT_SHIFT", KEY_RIGHT_SHIFT},
        {"KEY_RIGHT_ALT", KEY_RIGHT_ALT},
        {"KEY_RIGHT_GUI", KEY_RIGHT_GUI},
        // Function keys
        {"KEY_F1", KEY_F1},
        {"KEY_F2", KEY_F2},
        {"KEY_F3", KEY_F3},
        {"KEY_F4", KEY_F4},
        {"KEY_F5", KEY_F5},
        {"KEY_F6", KEY_F6},
        {"KEY_F7", KEY_F7},
        {"KEY_F8", KEY_F8},
        {"KEY_F9", KEY_F9},
        {"KEY_F10", KEY_F10},
        {"KEY_F11", KEY_F11},
        {"KEY_F12", KEY_F12},
        {"KEY_F13", KEY_F13},
        {"KEY_F14", KEY_F14},
        {"KEY_F15", KEY_F15},
        {"KEY_F16", KEY_F16},
        {"KEY_F17", KEY_F17},
        {"KEY_F18", KEY_F18},
        {"KEY_F19", KEY_F19},
        {"KEY_F20", KEY_F20},
        {"KEY_F21", KEY_F21},
        {"KEY_F22", KEY_F22},
        {"KEY_F23", KEY_F23},
        {"KEY_F24", KEY_F24},
        // Legacy F-key names (no KEY_ prefix)
        {"F1", KEY_F1},
        {"F2", KEY_F2},
        {"F3", KEY_F3},
        {"F4", KEY_F4},
        {"F5", KEY_F5},
        {"F6", KEY_F6},
        {"F7", KEY_F7},
        {"F8", KEY_F8},
        {"F9", KEY_F9},
        {"F10", KEY_F10},
        {"F11", KEY_F11},
        {"F12", KEY_F12},
        // Legacy F13-F24 aliases (for consistency with F1-F12 above)
        {"F13", KEY_F13},
        {"F14", KEY_F14},
        {"F15", KEY_F15},
        {"F16", KEY_F16},
        {"F17", KEY_F17},
        {"F18", KEY_F18},
        {"F19", KEY_F19},
        {"F20", KEY_F20},
        {"F21", KEY_F21},
        {"F22", KEY_F22},
        {"F23", KEY_F23},
        {"F24", KEY_F24},
        // Numpad
        {"KEY_NUM_0", KEY_NUM_0},
        {"KEY_NUM_1", KEY_NUM_1},
        {"KEY_NUM_2", KEY_NUM_2},
        {"KEY_NUM_3", KEY_NUM_3},
        {"KEY_NUM_4", KEY_NUM_4},
        {"KEY_NUM_5", KEY_NUM_5},
        {"KEY_NUM_6", KEY_NUM_6},
        {"KEY_NUM_7", KEY_NUM_7},
        {"KEY_NUM_8", KEY_NUM_8},
        {"KEY_NUM_9", KEY_NUM_9},
        {"KEY_NUM_SLASH", KEY_NUM_SLASH},
        {"KEY_NUM_ASTERISK", KEY_NUM_ASTERISK},
        {"KEY_NUM_MINUS", KEY_NUM_MINUS},
        {"KEY_NUM_PLUS", KEY_NUM_PLUS},
        {"KEY_NUM_ENTER", KEY_NUM_ENTER},
        {"KEY_NUM_PERIOD", KEY_NUM_PERIOD},
    };

    static const MediaKeyEntry MEDIA_KEY_TABLE[] = {
        {"MEDIA_NEXT_TRACK", KEY_MEDIA_NEXT_TRACK},
        {"MEDIA_PREVIOUS_TRACK", KEY_MEDIA_PREVIOUS_TRACK},
        {"MEDIA_STOP", KEY_MEDIA_STOP},
        {"MEDIA_PLAY_PAUSE", KEY_MEDIA_PLAY_PAUSE},
        {"MEDIA_MUTE", KEY_MEDIA_MUTE},
        {"MEDIA_VOLUME_UP", KEY_MEDIA_VOLUME_UP},
        {"MEDIA_VOLUME_DOWN", KEY_MEDIA_VOLUME_DOWN},
        // KEY_-prefixed aliases
        {"KEY_MEDIA_NEXT_TRACK", KEY_MEDIA_NEXT_TRACK},
        {"KEY_MEDIA_PREVIOUS_TRACK", KEY_MEDIA_PREVIOUS_TRACK},
        {"KEY_MEDIA_STOP", KEY_MEDIA_STOP},
        {"KEY_MEDIA_PLAY_PAUSE", KEY_MEDIA_PLAY_PAUSE},
        {"KEY_MEDIA_MUTE", KEY_MEDIA_MUTE},
        {"KEY_MEDIA_VOLUME_UP", KEY_MEDIA_VOLUME_UP},
        {"KEY_MEDIA_VOLUME_DOWN", KEY_MEDIA_VOLUME_DOWN},
        // Short aliases used in config
        {"KEY_VOLUME_UP", KEY_MEDIA_VOLUME_UP},
        {"KEY_VOLUME_DOWN", KEY_MEDIA_VOLUME_DOWN},
        // Extended media keys
        {"KEY_MEDIA_WWW_HOME", KEY_MEDIA_WWW_HOME},
        {"KEY_MEDIA_WWW_BACK", KEY_MEDIA_WWW_BACK},
        {"KEY_MEDIA_WWW_STOP", KEY_MEDIA_WWW_STOP},
        {"KEY_MEDIA_WWW_SEARCH", KEY_MEDIA_WWW_SEARCH},
        {"KEY_MEDIA_WWW_BOOKMARKS", KEY_MEDIA_WWW_BOOKMARKS},
        {"KEY_MEDIA_CALCULATOR", KEY_MEDIA_CALCULATOR},
        {"KEY_MEDIA_EMAIL_READER", KEY_MEDIA_EMAIL_READER},
        {"KEY_MEDIA_LOCAL_MACHINE_BROWSER", KEY_MEDIA_LOCAL_MACHINE_BROWSER},
        {"KEY_MEDIA_CONSUMER_CONTROL_CONFIGURATION", KEY_MEDIA_CONSUMER_CONTROL_CONFIGURATION},
    };

    static const ActionTypeEntry ACTION_TYPE_TABLE[] = {
        {"SendStringAction", Action::Type::SendString},
        {"SendCharAction", Action::Type::SendChar},
        {"SendKeyAction", Action::Type::SendKey},
        {"SendMediaKeyAction", Action::Type::SendMediaKey},
        {"SerialOutputAction", Action::Type::SerialOutput},
        {"DelayedAction", Action::Type::Delayed},
        {"PinHighAction", Action::Type::PinHigh},
        {"PinLowAction", Action::Type::PinLow},
        {"PinToggleAction", Action::Type::PinToggle},
        {"PinHighWhilePressedAction", Action::Type::PinHighWhilePressed},
        {"PinLowWhilePressedAction", Action::Type::PinLowWhilePressed},
        {"MacroAction", Action::Type::Macro},
    };

} // namespace

Action::Type lookupActionType(const char* name)
{
    for (const auto& e : ACTION_TYPE_TABLE)
    {
        if (strcmp(e.name, name) == 0)
        {
            return e.type;
        }
    }
    return Action::Type::Unknown;
}

uint8_t lookupKey(const char* name)
{
    for (const auto& e : KEY_TABLE)
    {
        if (strcmp(e.name, name) == 0)
        {
            return e.code;
        }
    }
    return 0;
}

const uint8_t* lookupMediaKey(const char* name)
{
    for (const auto& e : MEDIA_KEY_TABLE)
    {
        if (strcmp(e.name, name) == 0)
        {
            return e.report;
        }
    }
    return nullptr;
}

const char* lookupKeyName(uint8_t code)
{
    for (const auto& e : KEY_TABLE)
    {
        if (e.code == code)
        {
            return e.name;
        }
    }
    return nullptr;
}

const char* lookupMediaKeyName(const uint8_t* report)
{
    if (! report)
    {
        return nullptr;
    }
    for (const auto& e : MEDIA_KEY_TABLE)
    {
        if (e.report[0] == report[0] && e.report[1] == report[1])
        {
            return e.name;
        }
    }
    return nullptr;
}
