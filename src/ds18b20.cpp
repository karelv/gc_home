#include "DS2480b_event_loop.h"
#include "ds18b20.h"

union ByteFloat 
{
  float float_;
  uint8_t bytes_[sizeof(float)];
};


void DS2480b_call_back(DS2480bEventLoop *ow, DS2480bTaskData *data);


// Global variables for DS2480b
DS2480bEventLoop g_ow(Serial7, DS2480b_call_back);
ow_rom_name_t g_ow_rom_names[OW_MAX_SLAVES];
uint8_t g_ow_rom_name_count = 0;
uint8_t g_ow_rom_name_index = 0;

// Global variables for DS18B20

uint8_t g_slave_addresses[DS18B20_MAX_SLAVES][8];
uint8_t g_slave_address_index;
uint8_t g_ds18b20_sa_index;
uint8_t g_slave_count = 0;
ds18b20_callback_t g_18b20_callback = nullptr;

DS18b20_t g_ds18b20_sensors[DS18B20_MAX_SLAVES];

void DS2480b_call_back(DS2480bEventLoop *ow, DS2480bTaskData *data)
{
  unsigned long start = micros();
  if (data->status_ != DS2480B_ERROR_OK)
  {
    Serial.printf("DS2480b Error occured ACTION: %d[%d]\n", data->action_, data->task_);
    Serial.printf("  @SA: %02X-%02X-%02X-%02X-%02X-%02X-%02X-%02X\n", ow->slaveAddress_[7], ow->slaveAddress_[6], ow->slaveAddress_[5], ow->slaveAddress_[4], ow->slaveAddress_[3], ow->slaveAddress_[2], ow->slaveAddress_[1], ow->slaveAddress_[0]);
    return;
  }
  // Serial.printf("[%d] status: %d %02X-%02X-%02X-%02X-%02X-%02X-%02X-%02X data..\n", data->action_, data->status_, data->buffer_[7], data->buffer_[6], data->buffer_[5], data->buffer_[4], data->buffer_[3], data->buffer_[2], data->buffer_[1], data->buffer_[0]);
  if (data->action_ == DS2480B_ACTION_MASTER_RESET_CYCLE)
  {
    g_slave_address_index = 0;
    ow->discoverSlaves();
    Serial.printf("DS2480b_call_back: discoverSlaves: %d us\n", micros() - start);
  }
  if (data->action_ == DS2480B_ACTION_DISCOVER_SLAVES)
  {
    for (uint8_t i=0; i<8; i++)
    {
      g_slave_addresses[g_slave_address_index][i] = data->buffer_[i];
    }
    g_slave_address_index++;
    if (data->buffer_[8] == 0)// it is not the last slave!
    {
      ow->discoverSlaves(false);
      Serial.printf("DS2480b_call_back: discoverSlaves2 [%d]: %d us\n", g_slave_address_index, micros() - start);
    } else
    {
      g_slave_count = g_slave_address_index;
      for (uint8_t i=0; i<g_slave_address_index; i++)
      {
        Serial.printf("SA[%d]: %02X-%02X-%02X-%02X-%02X-%02X-%02X-%02X\n", i, g_slave_addresses[i][7], g_slave_addresses[i][6], g_slave_addresses[i][5], g_slave_addresses[i][4], g_slave_addresses[i][3], g_slave_addresses[i][2], g_slave_addresses[i][1], g_slave_addresses[i][0]);
      }
      ow->DS18b20RequestConversionBroadcast();
      Serial.printf("start DS18b20 -- %d us\n", micros() - start);
      g_ds18b20_sa_index = 0;
    }
  }

  if (data->action_ == DS2480B_ACTION_DS18B20_REQUEST_CONVERSION)
  {
    Serial.println("DS18b20 conversion done!");
    uint8_t *sa = &g_slave_addresses[g_ds18b20_sa_index][0];
    ow->DS18b20ReadTemperature(sa);
    Serial.printf("DS2480b_call_back: DS18b20ReadTemperature [%d]: %d us\n", g_ds18b20_sa_index, micros() - start);
  }

  if (data->action_ == DS2480B_ACTION_DS18B20_READ_TEMPERATURE)
  {    
    ByteFloat temperature;
    for (uint8_t i=0; i<4; i++)
    {
      temperature.bytes_[i] = data->buffer_[8+i];
    }    
    Serial.printf("Status %d | SA: %02X-%02X-%02X-%02X-%02X-%02X-%02X-%02X: %f DegC\n", data->status_, data->buffer_[7], data->buffer_[6], data->buffer_[5], data->buffer_[4], data->buffer_[3], data->buffer_[2], data->buffer_[1], data->buffer_[0], temperature.float_);
    g_ds18b20_sa_index++;
    if (g_ds18b20_sa_index < g_slave_address_index)
    { // ok, just read next slave
      uint8_t *sa = &g_slave_addresses[g_ds18b20_sa_index][0];
      ow->DS18b20ReadTemperature(sa);
      Serial.printf("DS2480b_call_back: DS18b20ReadTemperature2 [%d]: %d us\n", g_ds18b20_sa_index, micros() - start);
    } else
    { // trigger a new conversion!
      // ow->DS18b20RequestConversionBroadcast();
      // delay(3000);
      // Serial.printf("start DS18b20\n");

      // delay(3000);
      // Serial.printf("start discovery\n");
      // g_slave_address_index = 0;
      // ow->discoverSlaves();
      //delay(3000);
    //   Serial.println("start master reset cycle");
    //   g_slave_address_index = 0;
      //ow->DS2480bMasterResetCycle();
    }
  }
}

void ds18b20_set_callback(ds18b20_callback_t callback)
{
  g_18b20_callback = callback;
}

void ds18b20_init()
{
  g_ow.begin();
  g_slave_address_index = 0;
  g_ow.DS2480bMasterResetCycle();
}

void ds18b20_loop()
{
  g_ow.loop();
}

void ds18b20_trigger_new_measurement()
{
  g_slave_address_index = 0;
  g_ow.DS2480bMasterResetCycle();
}


bool ds18b20_timer(void *)
{
  ds18b20_trigger_new_measurement();
  return true;
}

