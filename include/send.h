#include <Arduino.h>
#include <BleKeyboard.h>

class Send {
  private: 
    void send() {Serial.println("hae?");};

  public:
    BleKeyboard* bleKeyboard;

    Send(BleKeyboard* bleKeyboard);
};

class SendChar : public Send {
  private:
    char key;

  public:
    void send();
    SendChar(BleKeyboard* bleKeyboard, char k);
};


class SendString : public Send {
  private:
    String text;

  public:
    void send();
    SendString(BleKeyboard* bleKeyboard, String t);
};

class SendKey : public Send {
  private:
    uint8_t key;

  public:
    void send();
    SendKey(BleKeyboard* bleKeyboard, uint8_t k);
};

class SendMediaKey : public Send {
  private:
    MediaKeyReport key;

  public: 
    void send();
    SendMediaKey(BleKeyboard* bleKeyboard, const MediaKeyReport k);
};
