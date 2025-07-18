#include <Arduino.h>
#include <QNEthernet.h>
#include <qnethernet/QNDNSClient.h>
#include <stdint.h>
#include "ntp_lib.h"

using namespace qindesign::network;

constexpr uint16_t NTP_PORT = 123;

// 01-Jan-1900 00:00:00 -> 01-Jan-1970 00:00:00
constexpr uint32_t NTP_EPOCH_DIFF = 2'208'988'800;

// Epoch -> 07-Feb-2036 06:28:16
constexpr uint32_t NTP_BREAK_TIME = 2'085'978'496;

#define NTP_SERVER_URL "pool.ntp.org"

// UDP port.
EthernetUDP udp;
IPAddress g_ntp_server_ip;


uint8_t
ntp_begin()
{
  // Start UDP listening on the NTP port
  Serial.printf("udp.begin(NTP_PORT);\n");
  udp.begin(NTP_PORT);
  if ((g_ntp_server_ip[0] == 0) &&
      (g_ntp_server_ip[1] == 0) &&
      (g_ntp_server_ip[2] == 0) &&
      (g_ntp_server_ip[3] == 0))
  {    
    Serial.printf("getHostByName...\n");
    DNSClient::getHostByName(NTP_SERVER_URL, g_ntp_server_ip, 1000);
  }
  Serial.printf("udp.begin(NTP_PORT); end\n");
  return 0;
}


uint8_t 
ntp_server_url(const char *url)
{
  if (url == NULL)
  {
    return DNSClient::getHostByName(NTP_SERVER_URL, g_ntp_server_ip, 0);
  }
  return DNSClient::getHostByName(url, g_ntp_server_ip, 0);
}


void ntp_bytes2data(NTPData &data, const uint8_t *bytes)
{
  uint8_t i=0;
  data.livnmode_ = bytes[i++];
  data.stratum_ = bytes[i++];
  data.poll_ = bytes[i++];
  data.precision_ = bytes[i++];
  data.delay_ = bytes[i++];
  data.delay_ = (data.delay_<<8) | bytes[i++];
  data.delay_ = (data.delay_<<8) | bytes[i++];
  data.delay_ = (data.delay_<<8) | bytes[i++];
  data.dispersion_ = bytes[i++];
  data.dispersion_ = (data.dispersion_<<8) | bytes[i++];
  data.dispersion_ = (data.dispersion_<<8) | bytes[i++];
  data.dispersion_ = (data.dispersion_<<8) | bytes[i++];
  data.identifier_ = bytes[i++];
  data.identifier_ = (data.identifier_<<8) | bytes[i++];
  data.identifier_ = (data.identifier_<<8) | bytes[i++];
  data.identifier_ = (data.identifier_<<8) | bytes[i++];
  data.reference_t_sec_ = bytes[i++];
  data.reference_t_sec_ = (data.reference_t_sec_<<8) | bytes[i++];
  data.reference_t_sec_ = (data.reference_t_sec_<<8) | bytes[i++];
  data.reference_t_sec_ = (data.reference_t_sec_<<8) | bytes[i++];
  data.reference_t_frac_ = bytes[i++];
  data.reference_t_frac_ = (data.reference_t_frac_<<8) | bytes[i++];
  data.reference_t_frac_ = (data.reference_t_frac_<<8) | bytes[i++];
  data.reference_t_frac_ = (data.reference_t_frac_<<8) | bytes[i++];
  data.originate_t_sec_ = bytes[i++];
  data.originate_t_sec_ = (data.originate_t_sec_<<8) | bytes[i++];
  data.originate_t_sec_ = (data.originate_t_sec_<<8) | bytes[i++];
  data.originate_t_sec_ = (data.originate_t_sec_<<8) | bytes[i++];
  data.originate_t_frac_ = bytes[i++];
  data.originate_t_frac_ = (data.originate_t_frac_<<8) | bytes[i++];
  data.originate_t_frac_ = (data.originate_t_frac_<<8) | bytes[i++];
  data.originate_t_frac_ = (data.originate_t_frac_<<8) | bytes[i++];
  data.receive_t_sec_ = bytes[i++];
  data.receive_t_sec_ = (data.receive_t_sec_<<8) | bytes[i++];
  data.receive_t_sec_ = (data.receive_t_sec_<<8) | bytes[i++];
  data.receive_t_sec_ = (data.receive_t_sec_<<8) | bytes[i++];
  data.receive_t_frac_ = bytes[i++];
  data.receive_t_frac_ = (data.receive_t_frac_<<8) | bytes[i++];
  data.receive_t_frac_ = (data.receive_t_frac_<<8) | bytes[i++];
  data.receive_t_frac_ = (data.receive_t_frac_<<8) | bytes[i++];
  data.transmit_t_sec_ = bytes[i++];
  data.transmit_t_sec_ = (data.transmit_t_sec_<<8) | bytes[i++];
  data.transmit_t_sec_ = (data.transmit_t_sec_<<8) | bytes[i++];
  data.transmit_t_sec_ = (data.transmit_t_sec_<<8) | bytes[i++];
  data.transmit_t_frac_ = bytes[i++];
  data.transmit_t_frac_ = (data.transmit_t_frac_<<8) | bytes[i++];
  data.transmit_t_frac_ = (data.transmit_t_frac_<<8) | bytes[i++];
  data.transmit_t_frac_ = (data.transmit_t_frac_<<8) | bytes[i++];
}


void
ntp_data2bytes(uint8_t *bytes, const NTPData &data)
{
  // uint_t i=0;
  bytes[0] = data.livnmode_;
  bytes[1] = data.stratum_;
  bytes[2] = data.poll_;
  bytes[3] = data.precision_;
  bytes[7] = data.delay_;
  bytes[6] = (data.delay_ >> 8);
  bytes[5] = (data.delay_ >> 16);
  bytes[4] = (data.delay_ >> 24);
  bytes[11] = data.dispersion_;
  bytes[10] = (data.dispersion_ >> 8);
  bytes[9] = (data.dispersion_ >> 16);
  bytes[8] = (data.dispersion_ >> 24);
  bytes[15] = data.identifier_;
  bytes[14] = (data.identifier_ >> 8);
  bytes[13] = (data.identifier_ >> 16);
  bytes[12] = (data.identifier_ >> 24);
  bytes[19] = data.reference_t_sec_;
  bytes[18] = (data.reference_t_sec_ >> 8);
  bytes[17] = (data.reference_t_sec_ >> 16);
  bytes[16] = (data.reference_t_sec_ >> 24);
  bytes[23] = data.reference_t_frac_;
  bytes[22] = (data.reference_t_frac_ >> 8);
  bytes[21] = (data.reference_t_frac_ >> 16);
  bytes[20] = (data.reference_t_frac_ >> 24);
  bytes[27] = data.originate_t_sec_;
  bytes[26] = (data.originate_t_sec_ >> 8);
  bytes[25] = (data.originate_t_sec_ >> 16);
  bytes[24] = (data.originate_t_sec_ >> 24);
  bytes[31] = data.originate_t_frac_;
  bytes[30] = (data.originate_t_frac_ >> 8);
  bytes[29] = (data.originate_t_frac_ >> 16);
  bytes[28] = (data.originate_t_frac_ >> 24);
  bytes[35] = data.receive_t_sec_;
  bytes[34] = (data.receive_t_sec_ >> 8);
  bytes[33] = (data.receive_t_sec_ >> 16);
  bytes[32] = (data.receive_t_sec_ >> 24);
  bytes[39] = data.receive_t_frac_;
  bytes[38] = (data.receive_t_frac_ >> 8);
  bytes[37] = (data.receive_t_frac_ >> 16);
  bytes[36] = (data.receive_t_frac_ >> 24);
  bytes[43] = data.transmit_t_sec_;
  bytes[42] = (data.transmit_t_sec_ >> 8);
  bytes[41] = (data.transmit_t_sec_ >> 16);
  bytes[40] = (data.transmit_t_sec_ >> 24);
  bytes[47] = data.transmit_t_frac_;
  bytes[46] = (data.transmit_t_frac_ >> 8);
  bytes[45] = (data.transmit_t_frac_ >> 16);
  bytes[44] = (data.transmit_t_frac_ >> 24);
}


uint64_t
ntp_time2epoch(uint64_t time)
{
  uint32_t frac = time;
  uint32_t sec = (time >> 32);
  if ((sec & 0x80000000U) == 0) {
    // See: Section 3, "NTP Timestamp Format"
    // https://www.rfc-editor.org/rfc/rfc2030.txt
    sec += NTP_BREAK_TIME;
  } else {
    sec -= NTP_EPOCH_DIFF;
  }
  uint64_t result = sec;
  result <<= 32;
  result |= frac;
  return result;
}


uint64_t
ntp_epoch2ntptime(uint64_t epoch)
{
  uint32_t frac = epoch;
  uint32_t sec = (epoch >> 32);
  // See: Section 3, "NTP Timestamp Format"
  // https://www.rfc-editor.org/rfc/rfc2030.txt
  if (sec >= NTP_BREAK_TIME) {
    sec -= NTP_BREAK_TIME;
  } else {
    sec += NTP_EPOCH_DIFF;
  }
  uint64_t result = sec;
  result <<= 32;
  result |= frac;
  return result;
}


void
ntp_print(NTPData &ntp_data)
{
  Serial.printf("- livnmode_: %02X\n", ntp_data.livnmode_);
  Serial.printf("- stratum_: %d\n", ntp_data.stratum_);
  Serial.printf("- poll_: %d\n", ntp_data.poll_);
  Serial.printf("- precision_: %d\n", ntp_data.precision_);
  Serial.printf("- delay_: %d\n", ntp_data.delay_);
  Serial.printf("- dispersion_: %d\n", ntp_data.dispersion_);
  Serial.printf("- identifier_: %d\n", ntp_data.identifier_);
  Serial.printf("- reference_t_: %u - %u\n", ntp_data.reference_t_sec_, ntp_data.reference_t_frac_);
  Serial.printf("- originate_t_: %u - %u\n", ntp_data.originate_t_sec_, ntp_data.originate_t_frac_);
  Serial.printf("- receive_t_: %u - %u\n", ntp_data.receive_t_sec_, ntp_data.receive_t_frac_);
  Serial.printf("- transmit_t_: %u - %u\n", ntp_data.transmit_t_sec_, ntp_data.transmit_t_frac_);
}


uint8_t
ntp_request(uint64_t epoch)
{
  // Set the Transmit Timestamp
  uint64_t new_time = ntp_epoch2ntptime(epoch);
  NTPData ntp_data;
  memset(&ntp_data, 0, sizeof(ntp_data));
  ntp_data.livnmode_ = 0b00'100'011;  // LI=0, VN=4, Mode=3 (Client)
  ntp_data.transmit_t_frac_ = new_time;
  ntp_data.transmit_t_sec_ = (new_time >> 32);

  // Serial.printf("NTP sent:\n");
  // ntp_print(ntp_data);

  uint8_t buffer[48];
  ntp_data2bytes(buffer, ntp_data);

  udp.clearReadError();
  udp.clearWriteError();
  
  // Send the packet
  if ((g_ntp_server_ip[0] == 0) &&
      (g_ntp_server_ip[1] == 0) &&
      (g_ntp_server_ip[2] == 0) &&
      (g_ntp_server_ip[3] == 0))
  {
    Serial.printf("udp.send(NTP_SERVER_URL ('%s')...\n", NTP_SERVER_URL);
    if (!udp.send(NTP_SERVER_URL, NTP_PORT, buffer, 48)) {
      return -1;
    }
  } else
  {
    Serial.printf("udp.send(g_ntp_server_ip (%d.%d.%d.%d)...\n", g_ntp_server_ip[0], g_ntp_server_ip[1], g_ntp_server_ip[2], g_ntp_server_ip[3]);
    if (!udp.send(g_ntp_server_ip, NTP_PORT, buffer, 48)) {
      return -1;
    }    
  }
  
  return 0;
}


uint8_t
ntp_parse_answer(int64_t &offset_31, int64_t &delay_31, uint64_t t4_epoch)
{
  int size = udp.parsePacket();

  if (size != 48 && size != 68) {
    return -1;
  }

  uint64_t T4 = ntp_epoch2ntptime(t4_epoch);

  NTPData ntp_data;
  ntp_bytes2data(ntp_data, udp.data());
  // Serial.printf("NTP received:\n");
  // ntp_print(ntp_data);

  //  Timestamp Name          ID   When Generated
  //     ------------------------------------------------------------
  //     Originate Timestamp     T1   time request sent by client
  //     Receive Timestamp       T2   time request received by server
  //     Transmit Timestamp      T3   time reply sent by server
  //     Destination Timestamp   T4   time reply received by client

  //  The roundtrip delay d and local clock offset t are defined as

  //     d = (T4 - T1) - (T2 - T3)     t = ((T2 - T1) + (T3 - T4)) / 2.
  uint64_t T1, T2, T3;
  T1 = ntp_data.originate_t_sec_; T1 <<= 32; T1 |= ntp_data.originate_t_frac_;
  T2 = ntp_data.receive_t_sec_;   T2 <<= 32; T2 |= ntp_data.receive_t_frac_;
  T3 = ntp_data.transmit_t_sec_;  T3 <<= 32; T3 |= ntp_data.transmit_t_frac_;

  // Serial.printf("T1: %"  PRIu64 " > %" PRIu64 " - %" PRIu64 " [" PRIu64 " us]\n", T1, T1 >> 32, T1 & 0x0FFFFFFFF, ((T1 & 0x0FFFFFFFF)*1000000) >> 32);
  // Serial.printf("T2: %"  PRIu64 " > %" PRIu64 " - %" PRIu64 " [" PRIu64 " us]\n", T2, T2 >> 32, T2 & 0x0FFFFFFFF, ((T2 & 0x0FFFFFFFF)*1000000) >> 32);
  // Serial.printf("T3: %"  PRIu64 " > %" PRIu64 " - %" PRIu64 " [" PRIu64 " us]\n", T3, T3 >> 32, T3 & 0x0FFFFFFFF, ((T3 & 0x0FFFFFFFF)*1000000) >> 32);
  // Serial.printf("T4: %"  PRIu64 " > %" PRIu64 " - %" PRIu64 " [" PRIu64 " us]\n", T4, T4 >> 32, T4 & 0x0FFFFFFFF, ((T4 & 0x0FFFFFFFF)*1000000) >> 32);

  int64_t T1_2 = T1 >> 2;
  int64_t T2_2 = T2 >> 2;
  int64_t T3_2 = T3 >> 2;
  int64_t T4_2 = T4 >> 2;

  delay_31 = ((T4_2 - T1_2) - (T2_2 - T3_2)) << 1;
  offset_31 = ((T2_2 - T1_2) + (T3_2 - T4_2)); // do not /2 as we start with 30 bit frac
  // delay & offset are using 31 bit frac; this inorder to get room for the signed bit!

  // printf("delay s: %lld -- %lld\n", delay_31, delay_31>>31);
  // printf("offset s: %lld -- %lld\n", offset_31, offset_31>>31);
  return 0;
}
