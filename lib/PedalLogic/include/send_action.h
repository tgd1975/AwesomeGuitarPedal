#pragma once
#include "action.h"
#include "i_ble_keyboard.h"
#include "key_lookup.h"
#include <string>

/**
 * @class SendAction
 * @brief Base class for all sendable actions
 *
 * Abstract base class that defines the interface for actions
 * that can be sent via BLE keyboard. Inherits from Action.
 */
class SendAction : public Action
{
public:
    /**
     * @brief Executes the send action
     *
     * Sends the configured input to the BLE keyboard.
     */
    void execute() override { send(); }

    /**
     * @brief Checks if this action is a send action
     *
     * @return true since this is a send action
     */
    bool isSendAction() const override { return true; }

    /**
     * @brief Executes the send action
     *
     * Sends the configured input to the BLE keyboard.
     */
    virtual void send() = 0;
    ~SendAction() override = default;

protected:
    IBleKeyboard* bleKeyboard; /**< Pointer to BLE keyboard interface */

    /**
     * @brief Constructs a SendAction
     *
     * @param bleKeyboard Pointer to BLE keyboard interface
     */
    SendAction(IBleKeyboard* bleKeyboard);
};

// Forward declarations
class SendStringAction;
class SendCharAction;
class SendKeyAction;
class SendMediaKeyAction;

/**
 * @class SendCharAction
 * @brief Sends a single character via BLE keyboard
 */
class SendCharAction : public SendAction
{
private:
    char key; /**< Character to send */

public:
    /**
     * @brief Gets the type of this action
     *
     * @return Action type
     */
    Action::Type getType() const override { return Action::Type::SendChar; }

    void getJsonProperties(JsonObject& json) const override
    {
        // SendChar can hold either a HID keycode (e.g. KEY_LEFT_ARROW = 0xD8,
        // stored as a signed char) or a printable ASCII character. The loader
        // (createSendCharActionFromJson) tries lookupKey first and falls back
        // to single-character ASCII; serialise the same way in reverse.
        const char* name = lookupKeyName(static_cast<uint8_t>(key));
        if (name != nullptr)
        {
            json["value"] = name;
        }
        else
        {
            json["value"] = std::string(1, key);
        }
    }

    /**
     * @brief Gets the character to be sent
     *
     * @return The character
     */
    char getKey() const { return key; }
    /**
     * @brief Executes the character send action
     */
    void send() override;

    /**
     * @brief Constructs a SendCharAction
     *
     * @param bleKeyboard Pointer to BLE keyboard interface
     * @param k Character to send
     */
    SendCharAction(IBleKeyboard* bleKeyboard, char k);
};

/**
 * @class SendStringAction
 * @brief Sends a text string via BLE keyboard
 */
class SendStringAction : public SendAction
{
private:
    std::string text; /**< Text string to send */

public:
    /**
     * @brief Gets the type of this action
     *
     * @return Action type
     */
    Action::Type getType() const override { return Action::Type::SendString; }

    void getJsonProperties(JsonObject& json) const override { json["value"] = text.c_str(); }

    /**
     * @brief Gets the text string to be sent
     *
     * @return The text string
     */
    const std::string& getText() const { return text; }
    /**
     * @brief Executes the string send action
     */
    void send() override;

    /**
     * @brief Constructs a SendStringAction
     *
     * @param bleKeyboard Pointer to BLE keyboard interface
     * @param t Text string to send
     */
    SendStringAction(IBleKeyboard* bleKeyboard, std::string t);
};

/**
 * @class SendKeyAction
 * @brief Sends a USB HID key code via BLE keyboard
 */
class SendKeyAction : public SendAction
{
private:
    uint8_t key; /**< USB HID key code to send */

public:
    /**
     * @brief Gets the type of this action
     *
     * @return Action type
     */
    Action::Type getType() const override { return Action::Type::SendKey; }

    void getJsonProperties(JsonObject& json) const override
    {
        const char* name = lookupKeyName(key);
        // The loader rejects unknown SendKey values, so a stored SendKeyAction
        // always corresponds to a known table entry; defensively emit an empty
        // string if that invariant ever breaks instead of writing a stale "KEY".
        json["value"] = (name != nullptr) ? name : "";
    }

    /**
     * @brief Gets the USB HID key code to be sent
     *
     * @return The key code
     */
    uint8_t getKey() const { return key; }
    /**
     * @brief Executes the key send action
     */
    void send() override;

    /**
     * @brief Constructs a SendKeyAction
     *
     * @param bleKeyboard Pointer to BLE keyboard interface
     * @param k USB HID key code to send
     */
    SendKeyAction(IBleKeyboard* bleKeyboard, uint8_t k);
};

/**
 * @class SendMediaKeyAction
 * @brief Sends a media key report via BLE keyboard
 */
class SendMediaKeyAction : public SendAction
{
private:
    MediaKeyReport key{}; /**< Media key report to send */

public:
    /**
     * @brief Gets the type of this action
     *
     * @return Action type
     */
    Action::Type getType() const override { return Action::Type::SendMediaKey; }

    void getJsonProperties(JsonObject& json) const override
    {
        const char* name = lookupMediaKeyName(key);
        json["value"] = (name != nullptr) ? name : "";
    }

    /**
     * @brief Gets the media key report to be sent
     *
     * @return The media key report
     */
    const MediaKeyReport& getKey() const { return key; }
    /**
     * @brief Executes the media key send action
     */
    void send() override;

    /**
     * @brief Constructs a SendMediaKeyAction
     *
     * @param bleKeyboard Pointer to BLE keyboard interface
     * @param k Media key report to send
     */
    SendMediaKeyAction(IBleKeyboard* bleKeyboard, const MediaKeyReport k);
};
