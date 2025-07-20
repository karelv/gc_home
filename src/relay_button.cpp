#include <string.h>

#include <Arduino.h>
#include <Wire.h>
#include <cppQueue.h>

#include "relay_button.h"
#include "events.h"


ButtonState g_button_states[MAX_BUTTONS];

static uint8_t g_relays_status[MAX_OUTPUT_IOEXPANDERS];
static uint8_t g_relays_status_update[MAX_OUTPUT_IOEXPANDERS];

uint8_t g_input_sa_list[MAX_INPUT_IOEXPANDERS] = {0x50, 0x51, 0x58, 0x59, 0x52, 0x53, 0x5A, 0x5B};
// uint8_t g_input_sa_list[MAX_INPUT_IOEXPANDERS] = {0x50};
uint8_t g_output_sa_list[MAX_OUTPUT_IOEXPANDERS] = {0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F};
uint8_t g_previous_button_state[MAX_INPUT_IOEXPANDERS];

// IO Expander global variables declaration section
uint8_t g_debounced_button_state[MAX_INPUT_IOEXPANDERS];
uint8_t g_input_io_enabled[MAX_INPUT_IOEXPANDERS];

ConnectLink g_connect_links[MAX_CONNECT_LINKS];

void rel_print_status()
{
  Serial.println("Relay status:");
  for (uint8_t i=0; i<4; i++)
  {
    Serial.printf("  %d: %02X vs %02X\n", i, g_relays_status[i], g_relays_status_update[i]);
  }
}

// IO expander section.
uint8_t 
rel_update(uint8_t rel_no, uint8_t value)
{
//  log_queue_data(String("rel_update"), String(rel_no) + String(",") + String(value ? 1 : 0));
  uint8_t rel_group = rel_no / 32;
  rel_no = rel_no % 32;

  uint8_t rel_index = rel_no / 4;
  if (rel_no & 0x10) rel_index = 3-((rel_no & 0x0F) / 4);
  rel_index += (rel_group * 4);

  uint8_t rel_bit_no = rel_no % 4;
  if (rel_no >= 16) rel_bit_no += 4;

  // Serial.printf("rel_no %d => group %d index %d bit %d\n", rel_no, rel_group, rel_index, rel_bit_no);
  if (rel_index < MAX_OUTPUT_IOEXPANDERS)
  {
    if (value == A_ON)
    {
      g_relays_status_update[rel_index] |= (1ULL << (rel_bit_no));
    } else if (value == A_OFF)
    {
      g_relays_status_update[rel_index] &= ~(1ULL << (rel_bit_no));
    } else if (value == A_TOGGLE)
    {
      g_relays_status_update[rel_index] ^= (1ULL << (rel_bit_no));
    }
    return (g_relays_status_update[rel_index] & (1ULL << (rel_bit_no))) ? 1 : 0;
  }
  return 2;
}


uint8_t 
rel_get_state(uint8_t rel_no, bool update_state)
{
  uint8_t rel_group = rel_no / 32;
  rel_no = rel_no % 32;


  uint8_t rel_index = rel_no / 4;
  if (rel_no & 0x10) rel_index = 3-((rel_no & 0x0F) / 4);
  rel_index += (rel_group * 4);

  uint8_t rel_bit_no = rel_no % 4;
  if (rel_no >= 16) rel_bit_no += 4;

  // Serial.printf("rel_no %d => group %d index %d bit %d\n", rel_no, rel_group, rel_index, rel_bit_no);

  if (rel_index >= MAX_OUTPUT_IOEXPANDERS) return 0;

  if (update_state) return (g_relays_status_update[rel_index] & (1ULL << (rel_bit_no))) ? 1 : 0;
  return (g_relays_status[rel_index] & (1ULL << (rel_bit_no))) ? 1 : 0;
}


void 
rel_flush()
{
  for (uint8_t rel_nr = 0; rel_nr < MAX_RELAYS; rel_nr++)
  {
    uint8_t new_state = rel_get_state(rel_nr);
    uint8_t old_state = rel_get_state(rel_nr, false);
    if (new_state != old_state)
    {
      EventSignal s;
      memset(&s, 0, sizeof(s));
      s.cmd_ = C_RELAY;
      s.nr_ = rel_nr;
      s.state_ = new_state ? S_ON : S_OFF;
      g_mqtt_queue_event_signals.push(&s);
    }
  }


  for (uint8_t rel_index = 0; rel_index < MAX_OUTPUT_IOEXPANDERS; rel_index++)
  {
    if (g_relays_status_update[rel_index] != g_relays_status[rel_index])
    {
      uint8_t sa = g_output_sa_list[rel_index];
      Wire.beginTransmission(sa);
      Wire.write(~g_relays_status_update[rel_index]);
      Wire.endTransmission();
      Serial.printf("rel_flush: SA:0x%02X => %02X\n", sa, g_relays_status_update[rel_index]);
      g_relays_status[rel_index] = g_relays_status_update[rel_index];
    }
  }
}


void
rel_config_slave_addresses(uint16_t index, uint8_t sa)
{
  if (index < 0)
  {
    for (uint8_t i=0; i<MAX_OUTPUT_IOEXPANDERS; i++)
    {
      rel_config_slave_addresses(i, 0xFF);
    }
    return;
  }
  if (index >= MAX_OUTPUT_IOEXPANDERS) return;

  if (sa < 0x80)
  {
    g_output_sa_list[index] = sa;
  }
}


void
button_config_slave_addresses(uint16_t index, uint8_t sa)
{
  if (index < 0)
  {
    for (uint8_t i=0; i<MAX_INPUT_IOEXPANDERS; i++)
    {
      button_config_slave_addresses(i, 0xFF);
    }
    return;
  }
  if (index >= MAX_INPUT_IOEXPANDERS) return;

  if (sa < 0x80)
  {
    g_input_sa_list[index] = sa;
  }
}


bool
pushbutton_get_state(uint8_t button)
{
  uint8_t index = button / 8;
  if (index >= MAX_INPUT_IOEXPANDERS)
  {
    return false;
  }
  if (index >= MAX_INPUT_IOEXPANDERS)
  {
    return false;
  }
  if (g_previous_button_state[index] & (1<<(button%8)))
  {
    return true;
  }
  return false;
}


bool
handle_io_expanders(void *)
{ // Note: this routine is called every 10ms; make sure it is as optimized as possible.
  //
  // procedure:
  // 1. Read inputs(buttons)
  // 2. compute and emit signals on queue
  // 3. set the relmem
  // 4. do set the relays (flush)

  unsigned long start_time = micros();

  // 1. Read inputs(buttons)
  static uint8_t debug = 0;
  uint8_t current_button_state[MAX_INPUT_IOEXPANDERS] = {};
  for (uint8_t i=0; i<MAX_INPUT_IOEXPANDERS; i++)
  {
    if (!g_input_io_enabled[i]) continue;
    int count = Wire1.requestFrom(g_input_sa_list[i], uint8_t(1));
    if (count != 1) 
    {
      g_input_io_enabled[i] = false;
      continue;
    }
    if (!Wire1.available()) continue;
    current_button_state[i] = ~Wire1.read();
    
    if (debug)
    {
      unsigned long stop_time = micros();
      Serial.printf("IO input[%d]: %02X -- %d us\n", i, current_button_state[i], stop_time - start_time);
    }    
  }
  unsigned long stop_time = micros();
  if ((stop_time - start_time) > 1000)
  {
    Serial.print("ioexpanders end:");
    Serial.print(stop_time - start_time);
    Serial.println(" us");
  }

  // 2. compute and emit signals on queue
  // 2.A. detect changes
  for (uint8_t i=0; i<MAX_INPUT_IOEXPANDERS; i++)
  { // exor; something changed?
    uint8_t x = current_button_state[i] ^ g_previous_button_state[i];
    // See truth table to formula: (for update variable)
    // https://tma.main.jp/logic/logic.php?lang=en&type=3&v0=D&v1=X&v2=R&00=0&01=1&02=0&03=0&04=0&05=1&06=1&07=1
    uint8_t update = (~x & current_button_state[i]) | (x & g_debounced_button_state[i]);
    if (update != g_debounced_button_state[i])
    {
      uint8_t updated_bit = update ^ g_debounced_button_state[i];
      for (uint8_t b=0; b<8; b++)
      {
        if (updated_bit & 0x01)
        { 
          uint8_t button = i*8 + b;
          uint8_t new_state = (update >> b) & 1;
          // analyse button clicks / long press
          ButtonState *but = &g_button_states[button];
          if (but->enabled_)
          {
            if (new_state == 0) but->clicks_++;
            if (new_state == 0) but->released_timer_ = 0;
            if (new_state == 1) but->pressed_timer_ = 0;
          }
          if (new_state == 1)
          {
            if (but->released_timer_ == 0)
            {
              but->enabled_ = 1;
            }
          } 

          // report to main-loop (MQTT)
          EventSignal s;
          memset(&s, 0, sizeof(s));
          s.cmd_ = C_BUTTON;
          s.nr_ = button;
          s.state_ = new_state ? S_PRESSED : S_RELEASED;
          g_mqtt_queue_event_signals.push(&s);
          
          // add to local event signals cache
          int16_t sig_index = -1;
          for (int16_t i=0; i<MAX_EVENT_SIGNALS; i++)
          {
            if (g_event_signals[i].cmd_ == C_NONE) // found free spot
            {
              sig_index = i;
              g_event_signals[i] = s;
              break;
            }
          }
          if (sig_index < 0)
          {
            // todo: make a error message in log file
            Serial.printf("ERROR: g_event_signals buffer too small [1]\n");
          }
        }
        updated_bit >>= 1;
      }

    }

    g_debounced_button_state[i] = update;
  }
  stop_time = micros();
  if ((stop_time - start_time) > 1000)
  {
    Serial.print("ioexpanders after 2A: ");
    Serial.print(stop_time - start_time);
    Serial.println(" us");
  }
  
  // 2.B. detect Single/double/triple click and long press signals from buttons.
  for (uint8_t i=0; i<MAX_INPUT_IOEXPANDERS; i++)
  {
    for (uint8_t b=0; b<8; b++)
    {
      uint8_t button = i*8 + b;
      ButtonState *but = &g_button_states[button];
      if (!but->enabled_)
      { 
        if (but->released_timer_ > 0) but->released_timer_--;
        continue;
      }

      uint8_t button_current_state = (g_debounced_button_state[i] >> b) & 1;
      if (button_current_state == 0) but->released_timer_++;
      if (button_current_state == 1) but->pressed_timer_++;
      if (but->pressed_timer_ > 250)
      { // we have a 'long press'!
        // report to main-loop (MQTT)
        EventSignal s;
        memset(&s, 0, sizeof(s));
        s.cmd_ = C_BUTTON;
        s.nr_ = button;
        s.state_ = S_LONG_PRESS;
        g_mqtt_queue_event_signals.push(&s);

        // add to local event signals cache
        int16_t sig_index = -1;
        for (int16_t i=0; i<MAX_EVENT_SIGNALS; i++)
        {
          if (g_event_signals[i].cmd_ == C_NONE) // found free spot
          {
            sig_index = i;
            g_event_signals[i] = s;
            break;
          }
        }
        if (sig_index < 0)
        {
          // todo: make a error message in log file
          Serial.printf("ERROR: g_event_signals buffer too small [2]\n");
        }

        memset(but, 0, sizeof(ButtonState));
        but->released_timer_ = 100;
        continue;
      }

      if ((but->released_timer_ > 16) || (but->clicks_ >= 3))
      { // we have a finished sequence!
        // report to main-loop (MQTT)
        EventSignal s;
        memset(&s, 0, sizeof(s));
        s.cmd_ = C_BUTTON;
        s.nr_ = button;
        s.state_ = S_SINGLE_CLICK;
        if (but->clicks_ == 2) s.state_ = S_DOUBLE_CLICK;
        if (but->clicks_ == 3) s.state_ = S_TRIPLE_CLICK;

        g_mqtt_queue_event_signals.push(&s);

        // add to local event signals cache
        int16_t sig_index = -1;
        for (int16_t i=0; i<MAX_EVENT_SIGNALS; i++)
        {
          if (g_event_signals[i].cmd_ == C_NONE) // found free spot
          {
            sig_index = i;
            g_event_signals[i] = s;
            break;
          }
        }
        if (sig_index < 0)
        {
          // todo: make a error message in log file
          Serial.printf("ERROR: g_event_signals buffer too small [3]\n");
        }

        memset(but, 0, sizeof(ButtonState));
        but->released_timer_ = 50;
      }
    }
  }
  stop_time = micros();
  if ((stop_time - start_time) > 1000)
  {
    Serial.print("ioexpanders after 2B: ");
    Serial.print(stop_time - start_time);
    Serial.println(" us");
  }

  // 2.C. store current button state as previous button state.
  memcpy(g_previous_button_state, current_button_state, sizeof(current_button_state));

  // 3. set the relmem

  for (int16_t i=0; i<MAX_EVENT_SIGNALS; i++)
  {    
    if (g_event_signals[i].cmd_ == C_NONE) continue; // nothing there!
    EventSignal &event = g_event_signals[i];
    for (int16_t c = 0; c<MAX_CONNECT_LINKS; c++)
    {
      if (g_connect_links[c].input_.cmd_ == C_NONE) continue; // nothing there!

      if ((event.cmd_     == g_connect_links[c].input_.cmd_) &&
          (event.nr_      == g_connect_links[c].input_.nr_) &&
          (event.state_   == g_connect_links[c].input_.state_) &&
          (event.action_  == g_connect_links[c].input_.action_))
      { // we have found an action!
        // execute the action...
        EventSignal &action = g_connect_links[c].output_;
        if (action.cmd_ == C_RELAY)
        {
          rel_update(action.nr_, action.action_);
        }
      }
    }
  }

  // ok, here all event signals should have been processed
  memset(g_event_signals, 0, sizeof(g_event_signals));

  // RelaySignal rel_signal;
  // while (g_queue_relay_signals.pop(&rel_signal))
  // {
  //   uint8_t new_state = 0;
  //   if (rel_signal.action_ == A_ON) new_state = 1;
  //   if (rel_signal.action_ == A_TOGGLE)
  //   {
  //     new_state = rel_get_state(rel_signal.rel_nr_) ? 0 : 1;
  //   }
  //   rel_update(rel_signal.rel_nr_, new_state);
  // }
  // stop_time = micros();
  // if ((stop_time - start_time) > 1000)
  // {
  //   Serial.print("ioexpanders after 3: ");
  //   Serial.print(stop_time - start_time);
  //   Serial.println(" us");
  // }

  // 4. do set the relays (flush)
  rel_flush();

  // unsigned long stop_time = micros();
  stop_time = micros();
  if ((stop_time - start_time) > 1000)
  {
    Serial.print("ioexpanders loop: ");
    Serial.print(stop_time - start_time);
    Serial.println(" us");
  }

  return true;
}


bool
io_expanders_reconnect_handler(void *)
{
  for (uint8_t i=0; i<MAX_INPUT_IOEXPANDERS; i++)
  {
    if (g_input_io_enabled[i]) continue;
    Wire1.beginTransmission(g_input_sa_list[i]);
    byte result = Wire1.endTransmission();     // stop transmitting

    if (result == 0)
    {
      g_input_io_enabled[i] = true;
      Serial.printf("io_expanders_reconnect_handler: sa:0x%02X enabled!\n", g_input_sa_list[i]);
    }
  }
  return true;
}


void rel_but_config_print()
{
  Serial.printf("input: ");    
  for (int i = 0; i<MAX_INPUT_IOEXPANDERS; i++)
  {
    Serial.printf("0x%02X, ", g_input_sa_list[i]);
  }
  Serial.printf("\n");    
  Serial.printf("output: ");    
  for (int i = 0; i<MAX_OUTPUT_IOEXPANDERS; i++)
  {
    Serial.printf("0x%02X, ", g_output_sa_list[i]);
  }
  Serial.printf("\n");  
}
