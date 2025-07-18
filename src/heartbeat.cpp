#include <Arduino.h>
#include <Wire.h>

#include "heartbeat.h"


// heartbeat implementation section.
bool heartbeat_led(void *) 
{
  unsigned long start_time = micros();  

  static uint8_t state;
  switch (state)
  {
    case 2:
    case 3:
    case 5:
    case 6:
      digitalWrite(LED_BUILTIN, HIGH);
      break;
    default:
      digitalWrite(LED_BUILTIN, LOW);
      break;
  }
  state++;
  if (state >= 10) state = 0;

  unsigned long stop_time = micros();
  if ((stop_time - start_time) > 100)
  {
    Serial.print("heartbeat loop: ");
    Serial.print(stop_time - start_time);
    Serial.println(" us");
  }

  return true; // repeat? true
}


bool
handle_brownout_detector(void *)
{ // todo: copy from original project
  static uint8_t state;
  if (state % 2)
  {
    Wire1.beginTransmission(0x62);
    Wire1.write(0xEF);
    Wire1.endTransmission();
  } else
  {
    Wire1.beginTransmission(0x62);
    Wire1.write(0xFF);
    Wire1.endTransmission();
  }
  state++;
  return true;
}
