#ifndef __EVENTS_H__
#define __EVENTS_H__

#include <cppQueue.h>

#include <stdint.h>

#define MAX_EVENT_SIGNALS 32
#define MAX_MQTT_QUEUE_EVENT_SIGNALS 32

enum SignalCommands {
  C_NONE = 0,
  C_BUTTON,
  C_RELAY,
  C_TIMER,
};

enum SignalStates {
  S_NONE = 0,
  S_PRESSED,
  S_RELEASED,
  S_SINGLE_CLICK,
  S_DOUBLE_CLICK,
  S_TRIPLE_CLICK,
  S_LONG_PRESS,
  S_ON,
  S_OFF,
};

enum Actions {
  A_NONE = 0,
  A_TOGGLE,
  A_ON,
  A_OFF,
};

struct EventSignal{
  uint8_t cmd_;
  uint8_t nr_;
  uint8_t state_;
  uint8_t action_;
};


extern EventSignal g_event_signals[MAX_EVENT_SIGNALS];
extern cppQueue g_mqtt_queue_event_signals;


#endif // __EVENTS_H__