#ifndef __DS18B20_H__
#define __DS18B20_H__

#include <stdint.h>

#include "ow_rom_name.h"

// DS18b20

#define DS18B20_MAX_SLAVES 32

/**
 * @brief Structure representing a DS18B20 temperature sensor
 */
typedef struct {
    ow_rom_t rom;               /**< 64-bit ROM address of the sensor */
    float temperature;          /**< Last measured temperature in Celsius */
    uint32_t sample_age_ms;     /**< Age of the sample in milliseconds */
    uint8_t valid;              /**< 1 if sample is valid, 0 otherwise */
} DS18b20_t;


typedef void (*ds18b20_callback_t)(DS18b20_t *);

void ds18b20_set_callback(ds18b20_callback_t callback);

void ds18b20_init();
void ds18b20_loop();
void ds18b20_trigger_new_measurement();
uint8_t ds18b20_get_temperature(uint8_t index, DS18b20_t *temperature);
uint8_t ds18b20_slave_address_str(const DS18b20_t *temperature, char *slave_address);
bool ds18b20_timer(void *);

#endif // __DS18B20_H__
