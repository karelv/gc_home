#include <Arduino.h>
#include <stdint.h>
#include <Timezone.h>    // https://github.com/JChristensen/Timezone

// Central European Time (Frankfurt, Paris)
TimeChangeRule rtc_CEST = {"CEST", Last, Sun, Mar, 2, 120};     // Central European Summer Time
TimeChangeRule rtc_CET = {"CET ", Last, Sun, Oct, 3, 60};       // Central European Standard Time
Timezone rtc_CE(rtc_CEST, rtc_CET);

// US Pacific Time Zone (Las Vegas, Los Angeles)
TimeChangeRule rtc_usPDT = {"PDT", Second, Sun, Mar, 2, -420};
TimeChangeRule rtc_usPST = {"PST", First, Sun, Nov, 2, -480};
Timezone rtc_usPT(rtc_usPDT, rtc_usPST);

Timezone *rtc_tz_active = &rtc_usPT;

uint64_t
rtc15_get(void)
{
  uint64_t tmp64;
  uint32_t hi1 = SNVS_HPRTCMR;
  uint32_t lo1 = SNVS_HPRTCLR;
  while (1) {
    uint32_t hi2 = SNVS_HPRTCMR;
    uint32_t lo2 = SNVS_HPRTCLR;
    if ((lo1 == lo2) && (hi1 == hi2)) {
      tmp64 = hi1;
      return (tmp64 << 32) | lo1;
    }
    hi1 = hi2;
    lo1 = lo2;
  }
  return 0;
}


void
rtc15_set(uint64_t t)
{
  // stop the RTC
  SNVS_HPCR &= ~(SNVS_HPCR_RTC_EN | SNVS_HPCR_HP_TS);
  while (SNVS_HPCR & SNVS_HPCR_RTC_EN); // wait
  // stop the SRTC
  SNVS_LPCR &= ~SNVS_LPCR_SRTC_ENV;
  while (SNVS_LPCR & SNVS_LPCR_SRTC_ENV); // wait
  // set the SRTC
  SNVS_LPSRTCLR = (t & 0x0FFFFFFFF);
  SNVS_LPSRTCMR = (t >> 32);
  // start the SRTC
  SNVS_LPCR |= SNVS_LPCR_SRTC_ENV;
  while (!(SNVS_LPCR & SNVS_LPCR_SRTC_ENV)); // wait
  // start the RTC and sync it to the SRTC
  SNVS_HPCR |= SNVS_HPCR_RTC_EN | SNVS_HPCR_HP_TS;
}


void
rtc15_offset(int64_t offset)
{
  uint64_t new_time = rtc15_get() - offset;
  rtc15_set(new_time);
}



time_t
rtc15_to_sec(uint64_t t)
{
  return time_t(t>>15);
}


uint32_t 
rtc15_to_us(uint64_t t)
{
  // 15 LSB is 1 sec or 1000000 us
  //              [          32 bit              ]
  //               (   15-bit  )*(     17bit    )
  return ((uint32_t(t) & 0x7FFF)*(1000000/(1<<3)))/(1<<12);
  // // what follows is a long way to say:
  // // t = t * 1000000 / 2^32;
  // // but with 32-bit * & shift operators
  // t >>= 10;
  // t *= 1000;
  // t >>= 10;
  // t *= 1000;
  // t >>= 12;
  // return t;
}


uint16_t 
rtc15_to_fraction(uint64_t t)
{
  return uint32_t(t) & 0x7FFF;
}


void rtc15_set_active_timezone(Timezone &tz)
{
  rtc_tz_active = &tz;
}


void
rtc15_to_str(char *time, uint64_t rtc15, Timezone &tz)
{ // format time in https://www.w3.org/TR/NOTE-datetime (ISO 8601)
  tmElements_t tm;
  time_t utc = rtc15_to_sec(rtc15);
  TimeChangeRule *tcr;
  time_t t = tz.toLocal(utc, &tcr);
  breakTime(t, tm);
  uint32_t us = rtc15_to_us(rtc15);
  sprintf(time, "%04u-%02u-%02uT%02u:%02u:%02u.%06lu%+03d:%02d",
        tm.Year + 1970, tm.Month, tm.Day,
        tm.Hour, tm.Minute, tm.Second, us, tcr->offset/60, tcr->offset%60);
}


uint64_t
rtc15_to_local(uint64_t rtc15, Timezone &tz)
{
  time_t utc = rtc15_to_sec(rtc15);
  TimeChangeRule *tcr;
  time_t t = tz.toLocal(utc, &tcr);
  uint64_t result = rtc15_to_fraction(rtc15);
  return result | (t << 15);
}


void
rtc15_now_str(char *now, Timezone &tz)
{
  uint64_t rtc15 = rtc15_get();
  rtc15_to_str(now, rtc15, tz);
}


uint64_t
rtc15_local_epoch_compile_time()
{
    const time_t FUDGE(10);     // fudge factor to allow for compile time (seconds, YMMV)
    const char *compDate = __DATE__, *compTime = __TIME__, *months = "JanFebMarAprMayJunJulAugSepOctNovDec";
    char chMon[4], *m;
    tmElements_t tm;

    strncpy(chMon, compDate, 3);
    chMon[3] = '\0';
    m = strstr(months, chMon);
    tm.Month = ((m - months) / 3 + 1);

    tm.Day = atoi(compDate + 4);
    tm.Year = atoi(compDate + 7) - 1970;
    tm.Hour = atoi(compTime);
    tm.Minute = atoi(compTime + 3);
    tm.Second = atoi(compTime + 6);
    time_t t = makeTime(tm);
    uint64_t result = t + FUDGE;           // add fudge factor to allow for compile time
    return (result<<15);
}
