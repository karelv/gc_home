
#include <cppQueue.h>

#include <stdint.h>

#include "events.h"


EventSignal g_event_signals[MAX_EVENT_SIGNALS];
EventSignal g_mqtt_event_signals_buffer[MAX_MQTT_QUEUE_EVENT_SIGNALS]; // buffer for queue; do not use this variable directly
cppQueue g_mqtt_queue_event_signals(sizeof(EventSignal), MAX_MQTT_QUEUE_EVENT_SIGNALS, FIFO, false, g_mqtt_event_signals_buffer, sizeof(g_mqtt_event_signals_buffer));  // Instantiate queue with static queue data arguments
