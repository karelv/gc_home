#include "ntp_lib.h"
#include "teensy_rtc15.h"

#include <Arduino.h>
#include <limits.h>


#define NTP_STATE_IDLE 0
#define NTP_STATE_START 1
#define NTP_STATE_DNS 2
#define NTP_STATE_SYNCHING 3
#define NTP_STATE_SETTING 4


#define PIN_LED_NTP 39

uint8_t g_ntp_state = NTP_STATE_IDLE;
uint8_t g_ntp_counter = 0;
int64_t g_ntp_delays[8];
int64_t g_ntp_offsets[8];
uint64_t g_ntp_last_synch;


bool ntp_app_timer(void *)
{
  if (g_ntp_state == NTP_STATE_SYNCHING)
  {
    Serial.printf("NTP: request!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
    ntp_request(rtc15_get()<<17);
  }
  return true;
}


void ntp_app_cron(const void *)
{
  // Serial.printf("NTP: request!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
  //ntp_request(rtc15_get() << 17);
  g_ntp_state = NTP_STATE_START;
}


void ntp_app_trigger_synch()
{
  g_ntp_state = NTP_STATE_START;
}


bool ntp_app_LED(void *)
{
  static uint8_t state;
  // pinMode(PIN_LED_NTP, OUTPUT);
  // digitalWrite(PIN_LED_NTP, state % 2);    

  uint32_t ntp_synch_age = (rtc15_get() - g_ntp_last_synch) >> 15;
  if (g_ntp_state != NTP_STATE_IDLE)
  { // display synch is running...
    digitalWrite(PIN_LED_NTP, state % 2);    
  } else if (ntp_synch_age > (7*24*3600))
  { // last NTP synch is older than 7 days...
    if (state == 0)
    {
      Serial.printf("NTP STATUS: not synched in 7+ days\n");
    }
    digitalWrite(PIN_LED_NTP, 0);
  } else 
  { // all ok; just blink once in a while
    digitalWrite(PIN_LED_NTP, ((state % 10) == 0) ? 1 : 0);
  }

  state++;
  if (state == 100) state = 0;
  return true;
}


void dns_cb(bool success)
{
  if (success)
  {
    Serial.printf("NTP: DNS lookup succeeded...\n");
    g_ntp_state = NTP_STATE_SYNCHING;
  } else
  {
    Serial.printf("NTP: DNS lookup failed...waiting for retry...\n");    
  }
}


void ntp_app_begin()
{
  pinMode(PIN_LED_NTP, OUTPUT);
  digitalWrite(PIN_LED_NTP, 0);
  ntp_set_server_cb(dns_cb);
}


void ntp_app_loop()
{
  static uint32_t last_ms = 0;
  if (g_ntp_state == NTP_STATE_IDLE) return;
  if (g_ntp_state == NTP_STATE_START)
  {
    g_ntp_state = NTP_STATE_DNS;
    ntp_begin();
    ntp_server_url();
    last_ms = millis();
    return;
  }
  if (g_ntp_state == NTP_STATE_DNS)
  { // the DNS lookup is in progress...
    // the callback function will change the state to NTP_STATE_SYNCHING
    if ((millis() - last_ms) > 5000)
    { // when longer than 5 second, then we assume that DNS is not working...
      // and we start again.
      g_ntp_state = NTP_STATE_START;
      last_ms = millis();
    }
    return;
  }
  if (g_ntp_state != NTP_STATE_SYNCHING)
  {
    return;
  }



  int64_t offset_31 = 0;
  int64_t delay_31 = 0;
  uint64_t t4_epoch = rtc15_get();
  t4_epoch <<= 17;
  if (ntp_parse_answer(offset_31, delay_31, t4_epoch) == 0)
  {
    if (g_ntp_state == NTP_STATE_IDLE)
    { // hmmm, strange normally no NTP packages are requested... 
      // but lets initiate sequential series of requests
      g_ntp_counter = 0;
      for (uint8_t i=0; i<8; i++)
      {
        g_ntp_delays[i] = LONG_LONG_MAX;
        g_ntp_offsets[i] = 0LL;
      }
      g_ntp_state = NTP_STATE_SYNCHING;
    }
    if (g_ntp_state == NTP_STATE_SYNCHING)
    {
      Serial.printf("NTP: offset_31: %lld\n", offset_31);
      Serial.printf("NTP: delay_31: %lld - %lld us\n", delay_31, (delay_31*1000000>>31));

      uint8_t spot = g_ntp_counter;
      if (spot >= 8)
      {
        // find largest spot
        int64_t max = g_ntp_delays[0];
        spot = 0;
        for (uint8_t i=1; i<8; i++)
        {          
          if (max < g_ntp_delays[i])
          { // ok we found a 'spot' which has a lower delay.
            spot = i;
            max = g_ntp_delays[i];
          }
        }
        if (delay_31 >= max)
        { // new synch is not faster ==> skip
          spot = 8;
        }
      }
      if (spot < 8)
      {
        g_ntp_offsets[spot] = offset_31;
        g_ntp_delays[spot] = delay_31;
      }

      g_ntp_counter++;
      if (g_ntp_counter >= 16)
      {
        g_ntp_state = NTP_STATE_SETTING;
      }
    }
    if (g_ntp_state == NTP_STATE_SETTING)
    {
      offset_31 = 0;
      for (uint8_t i=0; i<8; i++)
      {
        Serial.printf("NTP results[%d]: offset %lld us -- delay %lld us\n", i, (g_ntp_offsets[i]*1000000>>31), (g_ntp_delays[i]*1000000>>31));
      }
      for (uint8_t i=0; i<8; i++)
      {
        offset_31 += (g_ntp_offsets[i]>>3);
      }
      Serial.printf("NTP averaged: offset_31: %lld - %lld us\n", offset_31, (offset_31*1000000>>31));

      // ajust only half of it when it is a small correction
      if (offset_31 < 0x7FFFFFFF)
      {
        Serial.printf("NTP Small offset\n");
        offset_31 >>= 1;
      }

      // compensate for 'updating-time' of the RTC...
      offset_31 += 2000000LL; // about 1 ms.

      Serial.printf("NTP result: offset_31: %lld - %lld us\n", offset_31, (offset_31*1000000>>31));

      rtc15_offset(-(offset_31 >> 16));
      g_ntp_last_synch = rtc15_get();
      g_ntp_counter = 0;
      g_ntp_state = NTP_STATE_IDLE;
      for (uint8_t i=0; i<8; i++)
      {
        g_ntp_delays[i] = LONG_LONG_MAX;
        g_ntp_offsets[i] = 0LL;
      }
    }
  }  
}
