#pragma once
#include "i_button.h"
#include <cstdint>

/**
 * @class Button
 * @brief nRF52840-specific implementation of IButton interface
 *
 * Interrupt-driven button with software debouncing. Attach isr() to a
 * CHANGE interrupt on the button pin. digitalRead() is used inside isr()
 * to filter release edges.
 */
class Button : public IButton
{
private:
    uint8_t PIN;
    bool isDebounced(unsigned long now) const;

public:
    volatile uint8_t pressCount = 0;       /**< Incremented by ISR; decremented by event() */
    volatile bool awaitingRelease = false; /**< True after press, until pin goes HIGH */
    unsigned long lastDebounceTime = 0;    /**< Timestamp of last accepted ISR */
    unsigned long debounceDelay = 100;     /**< Minimum ms between accepted events (default 100) */

    /**
     * @brief Constructs a Button for the given pin using the default 100 ms
     *        debounce window.
     * @param PIN GPIO pin number
     */
    explicit Button(uint8_t PIN);

    /**
     * @brief Constructs a Button with an explicit debounce window (EPIC-028).
     *
     * Mirrors the ESP32 2-arg constructor so the shared pedal_app.cpp
     * compiles on both targets. The on-device debounce path on nRF52840
     * is not exercised by any task in EPIC-028; the parity audit
     * (TASK-360) will revisit on-device behaviour when the hardware is
     * reachable again.
     *
     * @param PIN GPIO pin number
     * @param debounceMs Debounce window in milliseconds
     */
    Button(uint8_t PIN, unsigned long debounceMs);

    /** @brief Configures the pin as input with pull-up */
    void setup() override;

    /**
     * @brief Interrupt service routine — call from a CHANGE ISR
     *
     * Filters release edges via digitalRead and applies debouncing.
     */
    void isr();

    /**
     * @brief Consumes and returns a pending press event
     * @return true if a press event was pending
     */
    bool event() override;

    /** @brief Clears any pending press event */
    void reset() override;

    /** @brief Release event detection — stub, not yet implemented for nRF52840 */
    bool releaseEvent() override { return false; }

    /** @brief Hold duration — stub, long/double press not yet implemented for nRF52840 */
    unsigned long holdDurationMs() const override { return 0; }

    /** @brief Double press event — stub, not yet implemented for nRF52840 */
    bool doublePressEvent() override { return false; }
};
