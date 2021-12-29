#include <Arduino.h>

class Button {
  private: 
    boolean isDebounced();
    void incKeyPresses();

  public:
    uint8_t PIN;
    uint32_t numberKeyPresses = 0;
    boolean pressed = false;
    unsigned long lastDebounceTime = 0;  // the last time the output pin was toggled
    unsigned long debounceDelay = 300;    // the debounce time; increase if the output flickers

    Button(uint8_t PIN);

    void isr();
    boolean event();
};
