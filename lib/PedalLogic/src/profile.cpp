#include "profile.h"

/**
 * @brief Constructs a Profile with a name
 *
 * @param name The name of this profile
 */
Profile::Profile(std::string name) : name(std::move(name)) {}

/**
 * @brief Adds an action to a specific button in this profile
 *
 * Performs bounds checking before storing the action.
 */
void Profile::addAction(uint8_t button, std::unique_ptr<Action> action)
{
    if (button < MAX_BUTTONS)
    {
        actions[button] = std::move(action);
    }
}

/**
 * @brief Gets the action associated with a button in this profile
 *
 * Performs bounds checking and null checking before returning.
 */
Action* Profile::getAction(uint8_t button) const
{
    if (button < MAX_BUTTONS && actions[button])
    {
        return actions[button].get();
    }
    return nullptr;
}

void Profile::addLongPressAction(uint8_t button, std::unique_ptr<Action> action)
{
    if (button < MAX_BUTTONS)
    {
        longPressActions_[button] = std::move(action);
    }
}

void Profile::addDoublePressAction(uint8_t button, std::unique_ptr<Action> action)
{
    if (button < MAX_BUTTONS)
    {
        doublePressActions_[button] = std::move(action);
    }
}

Action* Profile::getLongPressAction(uint8_t button) const
{
    if (button < MAX_BUTTONS && longPressActions_[button])
    {
        return longPressActions_[button].get();
    }
    return nullptr;
}

Action* Profile::getDoublePressAction(uint8_t button) const
{
    if (button < MAX_BUTTONS && doublePressActions_[button])
    {
        return doublePressActions_[button].get();
    }
    return nullptr;
}

/**
 * @brief Gets the name of this profile
 *
 * @return The profile name
 */
const std::string& Profile::getName() const { return name; }

/**
 * @brief Gets the description of this profile
 *
 * @return The profile description
 */
const std::string& Profile::getDescription() const { return description; }

/**
 * @brief Sets the description of this profile
 *
 * @param description The profile description
 */
void Profile::setDescription(const std::string& description) { this->description = description; }