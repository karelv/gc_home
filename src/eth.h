#ifndef __ETH_H__
#define __ETH_H__

#include <stdint.h>

#include <QNEthernet.h>             // ethernet/MQTT/webserver


// The link timeout, in milliseconds. Set to zero to not wait and
// instead rely on the listener to inform us of a link.
constexpr uint32_t ETH_LINK_TIMEOUT = 0;//5'000;  // 5 seconds

// Set the static IP to something other than INADDR_NONE (all zeros)
#define TEENSY_NAME "teensy41_3"
const IPAddress TEENSY_STATIC_IP{192, 168, 68, 113}; // was 11
const IPAddress SUBNETMASK{255, 255, 255, 0};
const IPAddress GATEWAY{192, 168, 68, 1};


bool init_ethernet();

#endif // __ETH_H__