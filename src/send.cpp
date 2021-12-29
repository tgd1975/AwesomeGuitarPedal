#include "send.h"
#include <Arduino.h>

Send::Send(BleKeyboard* bleKeyboard) {
    this->bleKeyboard = bleKeyboard;
}

SendKey::SendKey(BleKeyboard* bleKeyboard, uint8_t k) : Send( bleKeyboard) {
    key = k;
}
void SendKey::send() {
    bleKeyboard->write(key);
}


SendMediaKey::SendMediaKey(BleKeyboard* bleKeyboard, const MediaKeyReport k) : Send(bleKeyboard) {
    key[0]=k[0];
    key[1]=k[1];
}
void SendMediaKey::send() {
    bleKeyboard->write(key);
}


SendChar::SendChar(BleKeyboard* bleKeyboard, char k) : Send(bleKeyboard) {
    key = k;
}
void SendChar::send() {
    bleKeyboard->write(key);
}

SendString::SendString(BleKeyboard* bleKeyboard, String t) : Send(bleKeyboard) {
    text = t;
} 
void SendString::send() {
    bleKeyboard->print(text);
}