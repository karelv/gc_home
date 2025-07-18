#ifndef __RELAY_BUTTON_H__
#define __RELAY_BUTTON_H__

#include <stdint.h>

#include "events.h"

// IO Expander constant declaration section
#define MAX_INPUT_IOEXPANDERS 8
#define MAX_BUTTONS (MAX_INPUT_IOEXPANDERS * 8)
#define MAX_OUTPUT_IOEXPANDERS 16
#define MAX_RELAYS (MAX_OUTPUT_IOEXPANDERS * 8)
#define MAX_CONNECT_LINKS 256

struct ButtonState{
  uint8_t pressed_timer_;
  uint8_t released_timer_;
  uint8_t clicks_;
  uint8_t enabled_;
};

struct ConnectLink{
  EventSignal input_;
  EventSignal output_;
};


extern ConnectLink g_connect_links[MAX_CONNECT_LINKS];


// IO Expander function declaration section
uint8_t rel_update(uint8_t rel_no, uint8_t value);
uint8_t rel_get_state(uint8_t rel_no, bool update_state = true);
void rel_flush();
void rel_config_slave_addresses(uint16_t index=-1, uint8_t sa=0xFF);

bool pushbutton_get_state(uint8_t button);
void button_config_slave_addresses(uint16_t index=-1, uint8_t sa=0xFF);


bool handle_io_expanders(void *);
bool io_expanders_reconnect_handler(void *);

void rel_but_config_print();

#endif // __RELAY_BUTTON_H__