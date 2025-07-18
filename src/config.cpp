#include "config.h"
#include "relay_button.h"

#include <Arduino.h>
#include <SD.h>                     // SD card
#include <LittleFS.h>               // LittleFS 
#include <ArduinoJson.h>            // JSON file format

// SD CARD
const int CHIP_SELECT = BUILTIN_SDCARD;

#if defined(ARDUINO_TEENSY41)
#define BUILTIN_CARD_DETECT_PIN 46
#elif defined(ARDUINO_TEENSY40)
#define BUILTIN_CARD_DETECT_PIN 38
#elif defined(ARDUINO_TEENSY_MICROMOD)
#define BUILTIN_CARD_DETECT_PIN 39
#else
#error "BUILTIN_CARD_DETECT_PIN not available on this hardware!"
#endif


bool g_sd_is_connected;
extern LittleFS_Program g_little_fs;
extern JsonDocument g_json;
extern char g_buffer[2048];
extern uint8_t g_use_power_meter;
extern uint8_t g_use_ac_power_detector;


bool 
builtin_sd_card_begin()
{
  g_sd_is_connected = false;

  if (builtin_sd_card_present())
  {
    if (SD.begin(CHIP_SELECT))
    {
      g_sd_is_connected = true;
    }
  }
  return g_sd_is_connected;
}


bool
builtin_sd_card_present()
{
  pinMode(BUILTIN_CARD_DETECT_PIN, INPUT_PULLDOWN);
  return digitalRead(BUILTIN_CARD_DETECT_PIN) ? true : false;
}


bool 
handle_reconnect_sd(void *)
{
  unsigned long stop_time, start_time = micros();
  bool present = builtin_sd_card_present();
  stop_time = micros();
  Serial.printf("handle_reconnect_sd: %d [%d us]\n", g_sd_is_connected, stop_time - start_time);

  if (g_sd_is_connected && present)
  { // already connected; do nothing!
    return true;
  }
  // see if the card is present and can be initialized:
  Serial.println("check media present");
  if (present)
  {
    Serial.println("check begin");
    if (!SD.begin(CHIP_SELECT)) {
      Serial.println("Card failed, or not present");
      g_sd_is_connected = false;
    } else
    {
      g_sd_is_connected = true;
    }
  } else
  { // no media present
    g_sd_is_connected = false;
  }

  Serial.printf("handle_reconnect_sd (result): %d\n", g_sd_is_connected);

  stop_time = micros();
//  if ((stop_time - start_time) > 200)
  {
    Serial.print("handle_reconnect_sd: ");
    Serial.print(stop_time - start_time);
    Serial.println(" us");
  }
  return true;
}

void
read_connect_links()
{
  const char *filename = "connect_links.bin";
  memset(g_connect_links, 0, sizeof(g_connect_links));

  if (!g_little_fs.exists(filename)) return;
  File f = g_little_fs.open(filename);
  if (!f) return;
  
  uint8_t bytes_per_record = f.read();
  uint8_t bytes_per_input = f.read();
  uint8_t bytes_per_output = f.read();
  for (uint8_t i=0; i<(16-3); i++) // header is 16 bytes
  {
    f.read();
  }

  uint16_t i = 0;
  while (f.available())
  {
    if (i >= MAX_CONNECT_LINKS)
    {
      Serial.println("Connect links out of memeory");
      break;
    }
    g_connect_links[i].input_.cmd_ = f.read();
    g_connect_links[i].input_.nr_ = f.read();
    g_connect_links[i].input_.state_ = f.read();
    g_connect_links[i].input_.action_ = f.read();
    for (uint8_t j=4; j<bytes_per_input; j++) f.read();
    g_connect_links[i].output_.cmd_ = f.read();
    g_connect_links[i].output_.nr_ = f.read();
    g_connect_links[i].output_.state_ = f.read();
    g_connect_links[i].output_.action_ = f.read();
    for (uint8_t j=4; j<bytes_per_output; j++) f.read();
    for (uint8_t j=(bytes_per_input+bytes_per_output); j<bytes_per_record; j++) f.read();
    i++;
  }
  f.close();

  i=0;
  g_connect_links[i].input_.cmd_ = C_BUTTON;
  g_connect_links[i].input_.nr_ = 4;
  g_connect_links[i].input_.state_ = S_SINGLE_CLICK;
  g_connect_links[i].output_.cmd_ = C_RELAY;
  g_connect_links[i].output_.nr_ = 0;
  g_connect_links[i].output_.action_ = A_TOGGLE;
  i++;
  g_connect_links[i].input_.cmd_ = C_BUTTON;
  g_connect_links[i].input_.nr_ = 5;
  g_connect_links[i].input_.state_ = S_SINGLE_CLICK;
  g_connect_links[i].output_.cmd_ = C_RELAY;
  g_connect_links[i].output_.nr_ = 1;
  g_connect_links[i].output_.action_ = A_TOGGLE;
  i++;
  g_connect_links[i].input_.cmd_ = C_BUTTON;
  g_connect_links[i].input_.nr_ = 6;
  g_connect_links[i].input_.state_ = S_SINGLE_CLICK;
  g_connect_links[i].output_.cmd_ = C_RELAY;
  g_connect_links[i].output_.nr_ = 2;
  g_connect_links[i].output_.action_ = A_TOGGLE;
  i++;
  g_connect_links[i].input_.cmd_ = C_BUTTON;
  g_connect_links[i].input_.nr_ = 7;
  g_connect_links[i].input_.state_ = S_SINGLE_CLICK;
  g_connect_links[i].output_.cmd_ = C_RELAY;
  g_connect_links[i].output_.nr_ = 3;
  g_connect_links[i].output_.action_ = A_TOGGLE;
  i++;
  g_connect_links[i].input_.cmd_ = C_BUTTON;
  g_connect_links[i].input_.nr_ = 4;
  g_connect_links[i].input_.state_ = S_DOUBLE_CLICK;
  g_connect_links[i].output_.cmd_ = C_RELAY;
  g_connect_links[i].output_.nr_ = 31;
  g_connect_links[i].output_.action_ = A_TOGGLE;
  i++;
  g_connect_links[i].input_.cmd_ = C_BUTTON;
  g_connect_links[i].input_.nr_ = 5;
  g_connect_links[i].input_.state_ = S_DOUBLE_CLICK;
  g_connect_links[i].output_.cmd_ = C_RELAY;
  g_connect_links[i].output_.nr_ = 30;
  g_connect_links[i].output_.action_ = A_TOGGLE;
  i++;
  g_connect_links[i].input_.cmd_ = C_BUTTON;
  g_connect_links[i].input_.nr_ = 6;
  g_connect_links[i].input_.state_ = S_DOUBLE_CLICK;
  g_connect_links[i].output_.cmd_ = C_RELAY;
  g_connect_links[i].output_.nr_ = 29;
  g_connect_links[i].output_.action_ = A_TOGGLE;
  i++;
  g_connect_links[i].input_.cmd_ = C_BUTTON;
  g_connect_links[i].input_.nr_ = 7;
  g_connect_links[i].input_.state_ = S_DOUBLE_CLICK;
  g_connect_links[i].output_.cmd_ = C_RELAY;
  g_connect_links[i].output_.nr_ = 28;
  g_connect_links[i].output_.action_ = A_TOGGLE;
}


void
store_connect_links()
{
  const char *filename = "connect_links.bin";
  if (g_little_fs.exists(filename))
  {
    g_little_fs.remove(filename);
  }
  File f = g_little_fs.open(filename, FILE_WRITE);

  f.write(8); // 0
  f.write(4); // 1
  f.write(4); // 2
  f.write(0); // 3
  f.write(0); // 4
  f.write(0); // 5
  f.write(0); // 6
  f.write(0); // 7
  f.write(0); // 8
  f.write(0); // 9
  f.write(0); // 10
  f.write(0); // 11
  f.write(0); // 12
  f.write(0); // 13
  f.write(0); // 14
  f.write(0); // 15
  f.flush();

  for (uint16_t i=0; i< MAX_CONNECT_LINKS; i++)
  {
    f.write(g_connect_links[i].input_.cmd_);
    f.write(g_connect_links[i].input_.nr_);
    f.write(g_connect_links[i].input_.state_);
    f.write(g_connect_links[i].input_.action_);
    f.write(g_connect_links[i].output_.cmd_);
    f.write(g_connect_links[i].output_.nr_);
    f.write(g_connect_links[i].output_.state_);
    f.write(g_connect_links[i].output_.action_);
    f.flush();
  }
  f.close();
}

void
store_relay_states()
{
  const char *filename = "relay_states.bin";
  if (SD.exists(filename))
  {
    SD.remove(filename);
  }
  File f = SD.open(filename, FILE_WRITE);

  for (uint16_t i=0; i<MAX_OUTPUT_IOEXPANDERS; i++)
  {
    // f.write(g_relays_status[i]);
  }
  f.close();
}


void
read_and_restore_relay_states()
{
  const char *filename = "relay_states.bin";
  if (!SD.exists(filename)) return;

  File f = SD.open(filename);

  for (uint16_t i=0; i<MAX_OUTPUT_IOEXPANDERS; i++)
  {
    // g_relays_status_update[i] = f.read();
    // g_relays_status[i] = ~g_relays_status_update[i];
  }
  f.close();
  rel_flush();
}


void read_config_json()
{
  if (g_little_fs.exists("config.json"))
  {
    File config_f = g_little_fs.open("config.json");
    char *p = g_buffer;
    if (config_f)
    {
      while(config_f.available())
      {
        *p = config_f.read();
        p++;
      }
      config_f.close();
    }
    DeserializationError error = deserializeJson(g_json, g_buffer, strlen(g_buffer));

    if (error) {
      Serial.print("read config.json failed: ");
      Serial.println(error.c_str());
      return;
    }

    {
      button_config_slave_addresses(); // reset all!
      JsonArray slave_addresses_input = g_json["slave_addresses"]["input"];
      for (uint8_t i=0; i<slave_addresses_input.size(); i++)
      {
        button_config_slave_addresses(i, int(slave_addresses_input[i]) & 0x7F);
      }
    }
    {
      rel_config_slave_addresses(); // reset all!
      JsonArray slave_addresses_output = g_json["slave_addresses"]["output"];
      for (uint8_t i=0; i<slave_addresses_output.size(); i++)
      {
        rel_config_slave_addresses(i, int(slave_addresses_output[i]) & 0x7F);
      }
    }
    {
      g_use_ac_power_detector = g_json["use_AC_detector"];
      g_use_power_meter = g_json["use_power_meter"];
    }
  }
  g_use_ac_power_detector = true;
}
