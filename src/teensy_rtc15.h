#ifndef __TEENSY_RTC15_H__
#define __TEENSY_RTC15_H__

#include <stdint.h>

#include <Timezone.h>    // https://github.com/JChristensen/Timezone

extern Timezone rtc_CE;
extern Timezone rtc_usPT;
extern Timezone *rtc_tz_active;


uint64_t rtc15_get(void);
void rtc15_set(uint64_t t);
void rtc15_offset(int64_t offset);

time_t rtc15_to_sec(uint64_t t);
uint32_t rtc15_to_us(uint64_t t);
uint16_t rtc15_to_fraction(uint64_t t);

void rtc15_set_active_timezone(Timezone &tz);
uint64_t rtc15_to_local(uint64_t rtc15, Timezone &tz = *rtc_tz_active);
void rtc15_to_str(char *time, uint64_t rtc, Timezone &tz = *rtc_tz_active);
void rtc15_now_str(char *now, Timezone &tz = *rtc_tz_active);

uint64_t rtc15_local_epoch_compile_time();

#endif //__TEENSY_RTC15_H__
