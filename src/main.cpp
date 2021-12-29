#include <Arduino.h>
//#include "freertos/FreeRTOS.h"
//#include "freertos/task.h"
#include "driver/gpio.h"
#include "sdkconfig.h"
#include "FunctionalInterrupt.h"

#include <BleKeyboard.h>

#include "button.h"
#include "send.h"

//Se the name of the bluetooth keyboard (that shows up in the bluetooth menu of your device)
BleKeyboard bleKeyboard("Strix-Pedal", "Strix");
boolean connected = false;

#define LED_BLUETOOTH GPIO_NUM_26
#define LED_POWER GPIO_NUM_25

#define LED_SELECT_1 GPIO_NUM_5
#define LED_SELECT_2 GPIO_NUM_18
#define LED_SELECT_3 GPIO_NUM_19

Button BUTTON_SELECT = Button(GPIO_NUM_21);

Button BUTTON_A = Button(GPIO_NUM_13);
Button BUTTON_B = Button(GPIO_NUM_12);
Button BUTTON_C = Button(GPIO_NUM_27);
Button BUTTON_D = Button(GPIO_NUM_14);

#define SHIFT 0x80

//SendMediaKey SEND_A = SendMediaKey(&bleKeyboard, KEY_MEDIA_PLAY_PAUSE);
SendString SEND_A = SendString(&bleKeyboard, " ");
SendMediaKey SEND_B = SendMediaKey(&bleKeyboard, KEY_MEDIA_STOP);
SendChar SEND_C = SendChar(&bleKeyboard, KEY_LEFT_ARROW);  // '-'
SendChar SEND_D = SendChar(&bleKeyboard, KEY_RIGHT_ARROW);  // '+'

void IRAM_ATTR isr_a() {
  if (connected) {BUTTON_A.isr();}
}
void IRAM_ATTR isr_b() {
  if (connected) {BUTTON_B.isr();}
}
void IRAM_ATTR isr_c() {
  if (connected) {BUTTON_C.isr();}
}
void IRAM_ATTR isr_d() {
  if (connected) {BUTTON_D.isr();}
}

void IRAM_ATTR isr_select() {
  BUTTON_SELECT.isr();
}


void setup_button(Button button) {
    gpio_pad_select_gpio(button.PIN);
    pinMode(button.PIN, INPUT_PULLUP);
}

void setup_led(gpio_num_t pin, uint32_t level) {
  gpio_pad_select_gpio(pin);
  gpio_set_direction(pin, GPIO_MODE_OUTPUT);
  gpio_set_level(pin, level);
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial.println("started");

  setup_led(LED_POWER, 1);
  setup_led(LED_BLUETOOTH, 0);
  setup_led(LED_SELECT_1, 1);
  setup_led(LED_SELECT_2, 1);
  setup_led(LED_SELECT_3, 1);

  setup_button(BUTTON_A);
  setup_button(BUTTON_B);
  setup_button(BUTTON_C);
  setup_button(BUTTON_D);
  setup_button(BUTTON_SELECT);

  bleKeyboard.begin();
}

void attachInterrupts() {
  BUTTON_A.pressed = false;
  attachInterrupt(BUTTON_A.PIN, isr_a, FALLING);
  BUTTON_B.pressed = false;
  attachInterrupt(BUTTON_B.PIN, isr_b, FALLING);
  BUTTON_C.pressed = false;
  attachInterrupt(BUTTON_C.PIN, isr_c, FALLING);
  BUTTON_D.pressed = false;
  attachInterrupt(BUTTON_D.PIN, isr_d, FALLING);
  BUTTON_SELECT.pressed = false;
  attachInterrupt(BUTTON_SELECT.PIN, isr_select, FALLING);
}

void detachInterrupts() {
  BUTTON_A.pressed = false;
  detachInterrupt(BUTTON_A.PIN);
  BUTTON_B.pressed = false;
  detachInterrupt(BUTTON_D.PIN);
  BUTTON_C.pressed = false;
  detachInterrupt(BUTTON_C.PIN);
  BUTTON_D.pressed = false;
  detachInterrupt(BUTTON_D.PIN);
  BUTTON_SELECT.pressed = false;
  detachInterrupt(BUTTON_SELECT.PIN);
}

void process_events() {
    if (BUTTON_A.event()) {
      SEND_A.send();
    }
    if (BUTTON_B.event()) {
      SEND_B.send();
    }
    if (BUTTON_C.event()) {
      SEND_C.send();
    }
    if (BUTTON_D.event()) {
      SEND_D.send();
    }

    if (BUTTON_SELECT.event()) {

    }

}

void loop() {
  // put your main code here, to run repeatedly:
  if (bleKeyboard.isConnected()) {
    if (!connected) {
      Serial.println("connected");
      attachInterrupts();
      gpio_set_level(LED_BLUETOOTH, 1);
      connected = true;
    }

    process_events();

  } else {
    if (connected) {
      Serial.println("disconnected");
      connected = false;
      detachInterrupts();
      gpio_set_level(LED_BLUETOOTH, 0);
    }

  }
  delay(10);
}