#include <QNEthernet.h>             // ethernet/MQTT/webserver
#define MQTT_MAX_RETRY_FOR_AVAILABLE_FOR_WRITE 50
#include <PubSubClient.h>           // ethernet/MQTT/webserver

#include "mqtt.h"
#include "eth.h"
#include "relay_button.h"

using namespace qindesign::network; // ethernet/MQTT/webserver

EthernetClient g_mqtt_eth_client; // mqtt -- only used by g_mqtt_client; do not use directly
PubSubClient g_mqtt_client(g_mqtt_eth_client); // mqtt
uint8_t g_mqtt_is_disconnected = true; // mqtt
uint8_t g_mqtt_is_reconnecting = false;

bool mqtt_begin()
{
  g_mqtt_eth_client.setConnectionTimeout(MQTT_CLIENT_CONNECTION_TIMEOUT_MS); // in ms (timeout for connecting)

  g_mqtt_client.setServer(MQTT_SERVER_IP, MQTT_SERVER_PORT);
  g_mqtt_client.setCallback(mqtt_callback);
  
  return true;
}

void mqtt_disconnect()
{
  g_mqtt_eth_client.setConnectionTimeout(MQTT_CLIENT_DISCONNECT_TIMEOUT_MS);
  g_mqtt_client.disconnect();
  g_mqtt_is_disconnected = true;
}



void mqtt_loop()
{
  unsigned long start_time = micros();
  if (g_mqtt_is_disconnected)
  {
    return;
  }

  if (!g_mqtt_client.connected())
  {
    if (micros() - start_time > 100000)
    {
      Serial.printf("MQTT: not connected... time: %d us (g is disconnected = %d)\n", micros() - start_time, g_mqtt_is_disconnected);
    }
  } else
  {
    if (micros() - start_time > 100000)
    {
      Serial.printf("MQTT: connected... time: %d us (g is disconnected = %d); now loop\n", micros() - start_time, g_mqtt_is_disconnected);
    }
    g_mqtt_client.loop(); // MQTT
    if (micros() - start_time > 100000)
    {
      Serial.printf("MQTT: looped... time: %d us\n", micros() - start_time);
    }
  }
  Ethernet.loop();
}


bool
mqtt_reconnect_handler(void *)
{ 
  static uint8_t first_run = 1;
  unsigned long stop_time, start_time = micros();
  Serial.printf("MQTT: Check if connected..."); Serial.flush();

  if (!g_mqtt_is_disconnected && g_mqtt_client.connected())
  { 
    Serial.printf("connected!\n");
  } else
  {
    Serial.printf("not connected, try reconnect (%d us)\n", micros() - start_time);
    g_mqtt_is_reconnecting = true;
    g_mqtt_eth_client.setConnectionTimeout(MQTT_CLIENT_CONNECTION_TIMEOUT_MS); // in ms (timeout for connecting)

    if (g_mqtt_client.connect(TEENSY_NAME, MQTT_USER, MQTT_PWD, 
                              "gc_home/status", 1, true, "offline")) {
      char topic[64];
      g_mqtt_is_disconnected = false;

      g_mqtt_eth_client.setConnectionTimeout(MQTT_CLIENT_DISCONNECT_TIMEOUT_MS); // no timeout when going to 'disconnect' state...
      // Once connected, publish an announcement...
      Serial.printf("MQTT: connected!; reconnect @ %d us; now resubscribing to topics; publishing states\n", micros() - start_time);
      // ... and resubscribe
      for (uint8_t rel_nr=0; rel_nr<MAX_RELAYS; rel_nr++)
      {
        sprintf(topic, "gc_home/relays/%03d/set", rel_nr);
        g_mqtt_client.subscribe(topic);
        // at every reconnect inform about all of relay states 
        sprintf(topic, "gc_home/relays/%03d/state", rel_nr);        
        g_mqtt_client.publish(topic, rel_get_state(rel_nr, false) ? "ON" : "OFF");
        // Serial.printf("MQTT: publish %s = %s\n", topic, rel_get_state(rel_nr, false) ? "ON" : "OFF");
      }
      for (uint8_t but_nr=0; but_nr<MAX_BUTTONS; but_nr++)
      {
        sprintf(topic, "gc_home/buttons/%03d/state", but_nr);
        g_mqtt_client.publish(topic, "idle");
        // Serial.printf("MQTT: publish %s = %s\n", topic, "idle");
      }

      if (first_run) // publish home assistant config only once
      {
        first_run = 0;
        // publish home assistant config
        Serial.printf("mqtt_reconnect_handler: publish home assistant config\n");
        mqtt_publish_config();
      }

      // Publish 'online' status
      g_mqtt_client.publish("gc_home/status", "online", true);
    } else
    {
      Serial.printf("MQTT: No connection :-( state: %d; try reconnect: %d us\n", g_mqtt_client.state(), micros() - start_time);
    }
  }
  stop_time = micros();
  if ((stop_time - start_time) > 500)
  { 
    Serial.printf("mqtt_reconnect_handler: %d us\n", stop_time - start_time);
  }
  g_mqtt_is_reconnecting = false;
  return true;
}


void 
mqtt_callback(char* topic, byte* payload, unsigned int length) 
{
  // handle message arrived
  char buffer[128];
  memset(buffer, 0, sizeof(buffer));
  strncpy(buffer, (char *)payload, length);

  Serial.printf("<< '%s' = '%s' (%d)\n", topic, buffer, length);
  const char *rel_prefix = "gc_home/relays/";
  int pos = strlen(rel_prefix);
  if (!strncasecmp(topic, rel_prefix, pos))
  { // ok, we have a relay command from home assistant..
    int rel_nr = atoi(topic+pos);
    if (rel_nr == 0)
    {
      if (strncmp(topic + pos, "000", 3)) // check if zero...
      {// only allow relay 0 if we are sure there is 000 in the topic!
        rel_nr = -1; // do not allow relay 0 if there is no zero in the topic
      }
    }
    
    if ((0 <= rel_nr) && (rel_nr < MAX_RELAYS))
    {
      uint8_t action = A_NONE;
      if (!strcasecmp(buffer, "0")) action = A_OFF;
      if (!strcasecmp(buffer, "1")) action = A_ON;
      if (!strcasecmp(buffer, "toggle")) action = A_TOGGLE;
      if (!strcasecmp(buffer, "on")) action = A_ON;
      if (!strcasecmp(buffer, "off")) action = A_OFF;
      if (action != A_NONE)
      {
        rel_update(rel_nr, action);
      }
    }
  }
}


bool
mqtt_publish_button_idle(void *button_nr)
{
  // Serial.printf("mqtt_publish_button_idle: button_nr = %d\n", int(button_nr));
  if (!g_mqtt_client.connected()) return false;
  char topic[32];
  sprintf(topic, "gc_home/buttons/%03d/state", int(button_nr));
  g_mqtt_client.publish(topic, "idle");
  return false;
}


bool mqtt_publish_button(uint16_t button_nr, const char *state)
{
  char topic[32];
  sprintf(topic, "gc_home/buttons/%03d/state", button_nr);
  if (g_mqtt_client.connected())
  {
    g_mqtt_client.publish(topic, state);
  }
  return true;
}


bool mqtt_publish_relay(uint16_t relay_nr, const char *state)
{
  char topic[32];
  sprintf(topic, "gc_home/relays/%03d/state", relay_nr);
  if (g_mqtt_client.connected())
  {
    g_mqtt_client.publish(topic, state);
  }
  return true;
}

bool mqtt_publish_topic_value(const char *topic, const char *value)
{
  if (!g_mqtt_client.connected()) return false;
  return g_mqtt_client.publish(topic, value);
}




void mqtt_publish_config() {
    char topic[128], payload[512];

    // Relays as switches
    for (uint8_t rel_nr = 0; rel_nr < MAX_RELAYS; rel_nr++) {
        sprintf(topic, "homeassistant/switch/gc_home_relay_%03d/config", rel_nr);
        snprintf(payload, sizeof(payload),
            "{"
            "\"name\": \"Relay %03d\","
            "\"unique_id\": \"gc_home_relay_%03d\","
            "\"state_topic\": \"gc_home/relays/%03d/state\","
            "\"command_topic\": \"gc_home/relays/%03d/set\","
            "\"payload_on\": \"ON\","
            "\"payload_off\": \"OFF\","
            "\"optimistic\": false,"
            "\"device\": {"
            "\"identifiers\": [\"gc_home\"],"
            "\"name\": \"Golden Cherry Home: Main Board\","
            "\"model\": \"Golden Cherry Home: Main Board\","
            "\"manufacturer\": \"Golden Cherry\""
            "}"
            "}"
            , rel_nr, rel_nr, rel_nr, rel_nr
        );
        g_mqtt_client.publish(topic, payload, true);
    }

    // Buttons as sensors
    for (uint8_t but_nr = 0; but_nr < MAX_BUTTONS; but_nr++) {
        sprintf(topic, "homeassistant/sensor/gc_home_button_%03d/config", but_nr);
        snprintf(payload, sizeof(payload),
            "{"
            "\"name\": \"Button %03d\","
            "\"unique_id\": \"gc_home_button_%03d_action\","
            "\"state_topic\": \"gc_home/buttons/%03d/state\","
            "\"icon\": \"mdi:gesture-tap-button\","
            "\"device\": {"
            "\"identifiers\": [\"gc_home\"],"
            "\"name\": \"Golden Cherry Home: Main Board\","
            "\"model\": \"Golden Cherry Home: Main Board\","
            "\"manufacturer\": \"Golden Cherry\""
            "}"
            "}"
            , but_nr, but_nr, but_nr
        );
        g_mqtt_client.publish(topic, payload, true);
    }
}
