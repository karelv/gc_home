#ifndef __DS18B20_H__
#define __DS18B20_H__

#include <stdint.h>

#define DS18B20_MAX_SLAVES 32

void ds18b20_init();
void ds18b20_loop();
void ds18b20_trigger_new_measurement();
uint8_t ds18b20_get_temperature(uint8_t index);
uint8_t ds18b20_get_slave_id(uint8_t index, uint8_t *slave_id);
bool ds18b20_timer(void *);

#endif // __DS18B20_H__