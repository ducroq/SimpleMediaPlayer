/*
Adapted from https://github.com/enjoyneering/
*/
#ifndef __ROTENC__
#define __ROTENC__

#include <Arduino.h>

class RotaryEncoder
{
public:
  RotaryEncoder(uint8_t encoderA, uint8_t encoderB, uint8_t encoderButton);

  void begin(void);
  void read();

  inline int16_t getPosition(void) { return _counter; }
  inline bool getPushButton(void) { return _buttonState; }
  int8_t buttonChange();
  int16_t positionChange();

private:
  void readAB(void);
  void readPushButton(void);

  uint8_t _encoderA;      //pin "A"
  uint8_t _encoderB;      //pin "B"
  uint8_t _encoderButton; //pin "button"

  uint8_t _prevValueAB = 0;     //previous state of "A"+"B"
  uint8_t _currValueAB = 0;     //current   state of "A"+"B"
  bool _buttonState = true;     //encoder button status, idle value is "true" because internal pull-up resistor is enabled
  bool _prevButtonState = true; //
  bool _lastButtonState = true;
  unsigned long lastDebounceTime = 0; // the last time the output pin was toggled
  unsigned long lastSampleTime;       // previous sample time
  unsigned long debounceDelay = 100;  // the debounce time; increase if the output flickers

  int16_t _counter = 0; //encoder click counter, limits -32768..32767
  int16_t _prevCounter = 0;
};

#endif