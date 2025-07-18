#ifndef __NTP_LIB_H__
#define __NTP_LIB_H__


#include <stdint.h>
#include <stdio.h>

struct NTPData
{
  uint8_t livnmode_;
  uint8_t stratum_;
  uint8_t poll_;
  int8_t precision_;
  uint32_t delay_;
  uint32_t dispersion_;
  uint32_t identifier_;
  uint32_t reference_t_sec_;
  uint32_t reference_t_frac_;
  uint32_t originate_t_sec_;
  uint32_t originate_t_frac_;
  uint32_t receive_t_sec_;
  uint32_t receive_t_frac_;
  uint32_t transmit_t_sec_;
  uint32_t transmit_t_frac_;
};

void ntp_bytes2data(NTPData &data, const uint8_t *bytes);
void ntp_data2bytes(uint8_t *bytes, const NTPData &data);

uint64_t ntp_time2epoch(uint64_t time);
uint64_t ntp_epoch2ntptime(uint64_t epoch);

uint8_t ntp_begin();
uint8_t ntp_request(uint64_t epoch);
uint8_t ntp_parse_answer(int64_t &offset_31, int64_t &delay_31, uint64_t t4_epoch);
uint8_t ntp_server_url(const char *url = NULL);

void ntp_print(NTPData &ntp_data);

#endif // __NTP_LIB_H__
