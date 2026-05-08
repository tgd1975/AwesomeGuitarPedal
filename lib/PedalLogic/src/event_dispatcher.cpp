#include "event_dispatcher.h"

/**
 * @brief Registers a callback function for a specific button
 *
 * Performs bounds checking to ensure valid button index.
 */
void EventDispatcher::registerHandler(uint8_t button, EventCallback callback)
{
    if (button < handlers.size())
    {
        handlers[button] = std::move(callback);
    }
}

/**
 * @brief Executes the callback associated with a button
 *
 * Performs bounds checking and null checking before execution.
 */
void EventDispatcher::dispatch(uint8_t button)
{
    if (button < handlers.size() && handlers[button])
    {
        handlers[button]();
    }
}

void EventDispatcher::registerReleaseHandler(uint8_t button, EventCallback callback)
{
    if (button < releaseHandlers.size())
    {
        releaseHandlers[button] = std::move(callback);
    }
}

void EventDispatcher::dispatchRelease(uint8_t button)
{
    if (button < releaseHandlers.size() && releaseHandlers[button])
    {
        releaseHandlers[button]();
    }
}

void EventDispatcher::registerLongPressHandler(uint8_t button,
                                               EventCallback cb,
                                               uint32_t thresholdMs)
{
    if (button < longPressHandlers_.size())
    {
        longPressHandlers_[button] = std::move(cb);
        longPressThresholds_[button] = thresholdMs;
    }
}

void EventDispatcher::registerDoublePressHandler(uint8_t button, EventCallback cb)
{
    if (button < doublePressHandlers_.size())
    {
        doublePressHandlers_[button] = std::move(cb);
    }
}

void EventDispatcher::dispatchLongPress(uint8_t button)
{
    if (button < longPressHandlers_.size() && longPressHandlers_[button])
    {
        longPressHandlers_[button]();
    }
}

void EventDispatcher::dispatchDoublePress(uint8_t button)
{
    if (button < doublePressHandlers_.size() && doublePressHandlers_[button])
    {
        doublePressHandlers_[button]();
    }
}

uint32_t EventDispatcher::getLongPressThreshold(uint8_t button) const
{
    if (button < longPressThresholds_.size())
    {
        return longPressThresholds_[button];
    }
    return 0;
}

/**
 * @brief Clears all registered event handlers (press and release)
 */
void EventDispatcher::clearHandlers()
{
    for (auto& handler : handlers)
    {
        handler = nullptr;
    }
    for (auto& handler : releaseHandlers)
    {
        handler = nullptr;
    }
    for (auto& handler : longPressHandlers_)
    {
        handler = nullptr;
    }
    for (auto& handler : doublePressHandlers_)
    {
        handler = nullptr;
    }
    for (auto& t : longPressThresholds_)
    {
        t = 0;
    }
}
