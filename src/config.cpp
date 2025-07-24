#include "config.h"
#include "relay_button.h"

#include <Arduino.h>
#include <SD.h>                     // SD card
#include <LittleFS.h>               // LittleFS 
#include <ArduinoJson.h>            // JSON file format

#include "i2c_io.h"
#include "i2c_input_24v.h"

#include "ow_rom_name.h"

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
char g_buffer[2048];
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
  Serial.printf("read_connect_links...\n");
  const char *filename = "connect_links.bin";
  memset(g_connect_links, 0, sizeof(g_connect_links));

  if (!g_little_fs.exists(filename))
  {
    Serial.printf("Error: read_connect_links: cannot find config file in little_fs\n");
    return;
  }
  File f = g_little_fs.open(filename);
  if (!f) return;
  
  uint16_t value_table_record_count = f.read();
  value_table_record_count |= uint16_t(f.read()) << 8;
  uint16_t data_table_record_count = f.read();
  data_table_record_count |= uint16_t(f.read()) << 8;

  // skip the value_table
  f.seek((value_table_record_count+1) * 64, SEEK_SET);

  uint16_t i = 0;
  while (f.available())
  {
    if (i >= MAX_CONNECT_LINKS)
    {
      Serial.println("Error: connect_links.bin: Connect links out of memeory");
      break;
    }
    g_connect_links[i].input_.cmd_ = f.read();
    g_connect_links[i].input_.state_ = f.read();
    g_connect_links[i].input_.action_ = f.read();
    int32_t nr = 0;
    for (int k = 0; k < 4; ++k) {
      nr |= (uint32_t(f.read()) << (8 * k));
    }
    g_connect_links[i].input_.nr_ = nr;

    g_connect_links[i].output_.cmd_ = f.read();
    g_connect_links[i].output_.state_ = f.read();
    g_connect_links[i].output_.action_ = f.read();
    nr = 0;
    for (int k = 0; k < 4; ++k) {
      nr |= (uint32_t(f.read()) << (8 * k));
    }
    g_connect_links[i].output_.nr_ = nr;

    i++;
    f.seek((value_table_record_count + i + 1) * 64, SEEK_SET);
  }
  f.close();
}


void
read_one_wire_rom_id_names()
{
  Serial.printf("read_one_wire_rom_id_names...\n");
  const char *filename = "one_wire_rom_names.bin";
  
  if (!g_little_fs.exists(filename))
  {
    Serial.printf("Error: read_one_wire_rom_id_names: cannot find config file in little_fs\n");
    return;
  }
  File f = g_little_fs.open(filename);
  if (!f) return;
  
  ow_empty_rom_name_table();

  uint16_t table_record_count = f.read();
  table_record_count |= uint16_t(f.read()) << 8;
  
  // move to the end of the header (first record)
  f.seek(64, SEEK_SET);

  uint16_t i = 0;
  while (f.available())
  {
    uint64_t rom_id = 0;
    for (int k = 0; k < 8; ++k) {
      rom_id |= (uint64_t(f.read()) << (8 * k));
    }
    char name_buffer[57] = {0};
    for (int k = 0; k < 56 && f.available(); ++k) {
      name_buffer[k] = f.read();
    }
    ow_rom_t rom = {0};
    rom.id = rom_id;
    ow_add_rom_name(name_buffer, rom);
    i++;
  }
  f.close();

  ow_print_rom_name_table();
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
    Serial.printf("DeserializationError: '%s'\n", error.c_str());

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
        Serial.printf("config: slave_addresses_input[%d] = 0x%02X\n", i, int(slave_addresses_input[i]) & 0x7F);
        button_config_slave_addresses(i, int(slave_addresses_input[i]) & 0x7F);
      }
    }
    {
      rel_config_slave_addresses(); // reset all!
      JsonArray slave_addresses_output = g_json["slave_addresses"]["output"];
      for (uint8_t i=0; i<slave_addresses_output.size(); i++)
      {
        Serial.printf("config: slave_addresses_output[%d] = 0x%02X\n", i, int(slave_addresses_output[i]) & 0x7F);
        rel_config_slave_addresses(i, int(slave_addresses_output[i]) & 0x7F);
      }
    }
    {
      g_use_ac_power_detector = g_json["use_AC_detector"];
      g_use_power_meter = g_json["use_power_meter"];
      Serial.printf("config: use_AC_detector = %d, use_power_meter = %d\n", g_use_ac_power_detector, g_use_power_meter);
    }
    {
      JsonArray input_boards = g_json["i2c_input_boards"];
      for (uint8_t i=0; i<input_boards.size(); i++)
      {
        Serial.printf("config: input_boards[%d]\n", i);
        const char *board = input_boards[i]["type"].as<const char*>();
        if (!strcasecmp(board, "I2C-INPUT-24V"))
        {
          I2CInput24V *input_board = new I2CInput24V();
          input_board->config(
            input_boards[i]["bus"].as<uint8_t>(),
            input_boards[i]["id"].as<uint8_t>(),
            input_boards[i]["AD1"].as<const char*>(),
            input_boards[i]["AD2"].as<const char*>()
          );
          i2c_input_boards_add_at(input_board, input_boards[i]["id"].as<uint8_t>());
        } else {
          Serial.printf("config: i2c_input_boards: Unknown board type: '%s'\n", board);
        }
      }
    }
    {
      JsonArray output_boards = g_json["i2c_output_boards"];
      for (uint8_t i=0; i<output_boards.size(); i++)
      {
        Serial.printf("config: output_boards[%d]\n", i);
        const char *board = output_boards[i]["type"].as<const char*>();
        if (!strcasecmp(board, "I2C-INPUT-24V"))
        {
          I2CInput24V *output_board = new I2CInput24V();
          output_board->config(
            output_boards[i]["bus"].as<uint8_t>(),
            output_boards[i]["id"].as<uint8_t>(),
            output_boards[i]["AD1"].as<const char*>(),
            output_boards[i]["AD2"].as<const char*>()
          );
          i2c_output_boards_add_at(output_board, output_boards[i]["id"].as<uint8_t>());
        } else {
          Serial.printf("config: i2c_output_boards: Unknown board type: '%s'\n", board);
        }
      }
    }
  }
}


#include "utils.h"

void sd_card_print_files()
{
  Serial.println("SD card print files");
  if (builtin_sd_card_present())
  {
    Serial.println("SD card present");
    if (SD.begin(CHIP_SELECT))
    {
      Serial.println("SD card initialized");
      Serial.printf("SD FS: used %llu / total %llu bytes\n", SD.usedSize(), SD.totalSize());
      {
        File root = SD.open("/");
        print_directory(root, 0);
        root.close();
      }
    }
  }
}
