#include "button.h"
#include <Arduino.h>

Button::Button(uint8_t PIN) {
    this->PIN = PIN;
}

boolean Button::isDebounced() {
    if ((millis() - lastDebounceTime) > debounceDelay) {
    lastDebounceTime = millis();
    return true;
    }
    return false;
}

void Button::incKeyPresses() {
    numberKeyPresses++;
}

void Button::isr() {
    if (isDebounced()) {
        incKeyPresses();
        Serial.printf("PIN %d pressed %d-times\n", PIN, numberKeyPresses);
        pressed = true;
    } 
}

boolean Button::event() {
    if (pressed) {
        pressed = false;
        return true;
    }
    return false;
}
