#ifndef __MQTT_H__
#define __MQTT_H__

#include <QNEthernet.h>             // ethernet/MQTT/webserver

#include <stdint.h>

const IPAddress MQTT_SERVER_IP{192, 168, 68, 54};
#define MQTT_SERVER_PORT 1883
// #define MQTT_USER "mqtt_client10"
// #define MQTT_PWD "bdSN32HBagV62sClientGippershoven"
#define MQTT_USER "gc_home"
#define MQTT_PWD "golden_cherry123!"



// #define MQTT_SERVER_IP "test.mosquitto.org"
// #define MQTT_SERVER_PORT 1884
// #define MQTT_USER "rw"
// #define MQTT_PWD "readwrite"
 

#define MQTT_CLIENT_CONNECTION_TIMEOUT_MS 50
#define MQTT_CLIENT_DISCONNECT_TIMEOUT_MS 0


bool mqtt_begin();
void mqtt_loop();
void mqtt_disconnect();
bool mqtt_reconnect_handler(void *);
void mqtt_callback(char* topic, byte* payload, unsigned int length);
bool mqtt_publish_button_idle(void *button_nr);
bool mqtt_publish_button(uint16_t button_nr, const char *state);
bool mqtt_publish_relay(uint16_t relay_nr, const char *state);
void mqtt_publish_ds18b20_temperature_sensor(uint8_t *rom_id, float temperature, uint8_t valid);
bool mqtt_publish_topic_value(const char *topic, const char *value);
void mqtt_publish_config();
void mqtt_publish_relay_button_config();
void mqtt_publish_sensor_config();

#endif // __MQTT_H__