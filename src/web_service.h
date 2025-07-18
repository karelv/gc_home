#ifndef __WEB_SERVICE_H__
#define __WEB_SERVICE_H__

#include <stdint.h>

constexpr uint16_t WEB_SERVER_PORT = 80;

// Timeout for waiting for input from the client.
constexpr uint32_t WEB_CLIENT_TIMEOUT = 5'000;  // 5 seconds

// Timeout for waiting for a close from the client after a
// half close.
constexpr uint32_t WEB_CLIENT_SHUTDOWN_TIME = 30'000;  // 30 seconds


uint8_t web_service_begin();
void handle_web_requests();


#endif // __WEB_SERVICE_H__
