#include <Arduino.h>

#include "power.h"
#include "relay_button.h"
#include "config.h"


#define PIN_AC_DETECTOR 27
#define PIN_AC_DC_RELAY_BUTTON 1
#define PIN_ENABLE_POWER 37

void timer_cancel_all_tasks();

// power detector
extern "C" uint32_t set_arm_clock(uint32_t frequency);
uint8_t g_use_ac_power_detector = true;
elapsedMillis g_AC_last_seen;
uint8_t g_use_power_meter = false;


void
power_begin()
{
  pinMode(LED_BUILTIN, OUTPUT); // set LED pin to OUTPUT
  pinMode(PIN_AC_DETECTOR, INPUT_PULLUP);
  pinMode(PIN_ENABLE_POWER, OUTPUT);
  digitalWrite(PIN_ENABLE_POWER, 1); // turn on the trafos
  // be optimistic, let's reset the AC power detector (before we start the g_timer!).
  g_AC_last_seen = 0;
}


void
power_down() // only call this routing from within the g_timer handle routines. (not main loop or g_timer_ms)
{ // goal is that this routine runs without AC, using/depleting the supply capacitor
  // thus this routine should use as less power as possible and as short as possible.
  // therefore the goal is to keep this routine under 100ms and we run with the lowest clock frequency.
  // lets preserve energy
  timer_cancel_all_tasks();
  set_arm_clock(24'000'000);

  // turn off relay power AC/DC

  // Most important task: store the relay states in order to restore at next boot up.
  store_relay_states(); 

  // create some log about the fact there is a power drop

  // turn off all relays (ioexpanders).

}


bool
detect_AC(void *)
{
  if (!g_use_ac_power_detector) return false;
  static elapsedMillis last_run = 0;
  static bool last_power_result = false;
  if (digitalRead(PIN_AC_DETECTOR) == 0)
  {// ok we see the power!
    g_AC_last_seen = 0;
    if (!last_power_result)
    {
      Serial.printf("AC Power back on !!\n");
    }
    last_power_result = true;
  }
  if (int32_t(last_run) > 10)
  {// for some reason the 1ms timer did not run on time..., let's ignore the AC detector and restart the counter.
    g_AC_last_seen = 0;
  }
  last_run = 0;
  if (int32_t(g_AC_last_seen) > 35)
  {
    if (last_power_result)
    {
      Serial.printf("AC power loss!!!\n");
    }
    last_power_result = false;
    return true;
  }
  return true;
}
