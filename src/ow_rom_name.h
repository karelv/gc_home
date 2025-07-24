#ifndef OW_ROM_NAME_H
#define OW_ROM_NAME_H

#include <stdint.h>

#define OW_MAX_SLAVES 32

union ow_rom_t
{
  uint64_t id;
  uint8_t bytes[sizeof(uint64_t)];
};

typedef struct {
    ow_rom_t rom;  /**< 64-bit ROM address of the sensor */
    char name[56]; /**< Name of the sensor */
} ow_rom_name_t;


ow_rom_name_t ow_get_rom_name_by_index(uint8_t index);
ow_rom_name_t ow_get_rom_name_by_id(uint64_t id);
ow_rom_t ow_get_rom_by_index(uint8_t index);
ow_rom_t ow_get_rom_by_name(const char *name);
const char *ow_get_name_by_rom_id(uint64_t id);
const char *ow_get_name_by_rom_bytes(uint8_t *bytes);
const char *ow_get_name_by_index(uint8_t index);

void ow_empty_rom_name_table();
uint8_t ow_add_rom_name(const char *name, ow_rom_t rom);

ow_rom_t ow_convert_hex_string(const char *hex_string);
void ow_convert_rom_to_dashed_hex_string(const ow_rom_t *rom, char *buffer, size_t buffer_size);

void ow_print_rom_name_table();

#endif // OW_ROM_NAME_H
