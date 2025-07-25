#include <cstdio>

#include <ctype.h>
#include <string.h>
#include <Arduino.h>

#include "ow_rom_name.h"

static ow_rom_name_t g_rom_names[OW_MAX_SLAVES];

ow_rom_name_t ow_get_rom_name_by_index(uint8_t index) {
    if (index < OW_MAX_SLAVES) {
        return g_rom_names[index];
    }
    ow_rom_name_t empty = {0};
    return empty;
}

ow_rom_name_t ow_get_rom_name_by_id(uint64_t id) {
    for (uint8_t i = 0; i < OW_MAX_SLAVES; ++i) {
        if (g_rom_names[i].rom.id == id) {
            return g_rom_names[i];
        }
    }
    ow_rom_name_t empty = {0};
    return empty;
}

ow_rom_t ow_get_rom_by_index(uint8_t index) {
    if (index < OW_MAX_SLAVES) {
        return g_rom_names[index].rom;
    }
    ow_rom_t empty = {0};
    return empty;
}

ow_rom_t ow_get_rom_by_name(const char *name) {
    for (uint8_t i = 0; i < OW_MAX_SLAVES; ++i) {
        if (strcmp(g_rom_names[i].name, name) == 0) {
            return g_rom_names[i].rom;
        }
    }
    ow_rom_t empty = {0};
    return empty;
}

const char *ow_get_name_by_rom_id(uint64_t id) {
    for (uint8_t i = 0; i < OW_MAX_SLAVES; ++i) {
        if (g_rom_names[i].rom.id == id) {
            return g_rom_names[i].name;
        }
    }
    return NULL;
}

const char *ow_get_name_by_rom_bytes(uint8_t *bytes) {
    ow_rom_t rom;
    for (uint8_t i = 0; i < 8; ++i) {
        rom.bytes[i] = bytes[i];
    }

    for (uint8_t i = 0; i < OW_MAX_SLAVES; ++i) {
        if (g_rom_names[i].rom.id == rom.id) {
            return g_rom_names[i].name;
        }
    }
    return NULL;
}

const char *ow_get_name_by_index(uint8_t index) {
    if (index < OW_MAX_SLAVES) {
        return g_rom_names[index].name;
    }
    return NULL;
}

const char *ow_get_sanitized_name_by_rom_id(uint64_t id) {
    for (uint8_t i = 0; i < OW_MAX_SLAVES; ++i) {
        if (g_rom_names[i].rom.id == id) {
            return g_rom_names[i].sanitized_name;
        }
    }
    return NULL;
}

const char *ow_get_sanitized_name_by_rom_bytes(uint8_t *bytes) {
    ow_rom_t rom;
    for (uint8_t i = 0; i < 8; ++i) {
        rom.bytes[i] = bytes[i];
    }
    for (uint8_t i = 0; i < OW_MAX_SLAVES; ++i) {
        if (g_rom_names[i].rom.id == rom.id) {
            return g_rom_names[i].sanitized_name;
        }
    }
    return NULL;
}

const char *ow_get_sanitized_name_by_index(uint8_t index) {
    if (index < OW_MAX_SLAVES) {
        return g_rom_names[index].sanitized_name;
    }
    return NULL;
}

void ow_empty_rom_name_table()
{
  memset(g_rom_names, 0, sizeof(g_rom_names));
}

uint8_t ow_add_rom_name(const char *name, ow_rom_t rom)
{
    for (uint8_t i = 0; i < OW_MAX_SLAVES; ++i) {
        if (g_rom_names[i].name[0] == '\0') {
            strncpy(g_rom_names[i].name, name, sizeof(g_rom_names[i].name) - 1);
            memset(g_rom_names[i].sanitized_name, 0, sizeof(g_rom_names[i].sanitized_name));
            for (size_t j = 0; j < sizeof(g_rom_names[i].sanitized_name) - 1 && name[j]; ++j) {
                unsigned char c = name[j];
                if (isalnum(c)) {
                    g_rom_names[i].sanitized_name[j] = tolower(c);
                } else {
                    g_rom_names[i].sanitized_name[j] = '_';
                }
            }
            g_rom_names[i].sanitized_name[sizeof(g_rom_names[i].sanitized_name) - 1] = '\0';
            g_rom_names[i].rom.id = rom.id;
            return 1;
        }
    }
    return 0;
}

/** Convert a hexadecimal string to an ow_rom_t structure 
 * @param hex_string A string representing a hexadecimal number, e.g., "12-34-56-78-9A-BC-DE-F0"
 * @return An ow_rom_t structure with the converted value.
 * If the string is invalid, the id will be set to 0.
 * 'hex_string' can be in the format "0x123456789ABCDEF0" or "12-34-56-78-9A-BC-DE-F0".
 * And it represents a 64-bit value, so it should not exceed 16 hexadecimal digits.
 */
ow_rom_t ow_convert_hex_string(const char *hex_string)
{
    ow_rom_t rom = {0};
    if (hex_string) {
        char buf[32];
        size_t j = 0;
        for (size_t i = 0; hex_string[i] && j < sizeof(buf) - 1; ++i) {
            if (hex_string[i] == '-') continue;
            if (i == 0 && hex_string[0] == '0' && hex_string[1] == 'x') {
            i = 1; // skip '0'
            continue; // skip 'x'
            }
            buf[j++] = hex_string[i];
        }
        buf[j] = '\0';
        hex_string = buf;
        sscanf(hex_string, "%llx", &rom.id);
    }
    return rom;
}

void ow_convert_rom_to_dashed_hex_string(const ow_rom_t *rom, char *buffer, size_t buffer_size)
{
    if (buffer && rom) {
        snprintf(buffer, buffer_size, "%02X-%02X-%02X-%02X-%02X-%02X-%02X-%02X",
                 rom->bytes[7], rom->bytes[6], rom->bytes[5], rom->bytes[4],
                 rom->bytes[3], rom->bytes[2], rom->bytes[1], rom->bytes[0]);
    }
}

void ow_print_rom_name_table()
{
    for (uint8_t i = 0; i < OW_MAX_SLAVES; ++i) {
        if (g_rom_names[i].name[0] != '\0') {
            Serial.printf("ROM[%d]: %02X-%02X-%02X-%02X-%02X-%02X-%02X-%02X, Name: %s\n",
                          i,
                          g_rom_names[i].rom.bytes[7], g_rom_names[i].rom.bytes[6],
                          g_rom_names[i].rom.bytes[5], g_rom_names[i].rom.bytes[4],
                          g_rom_names[i].rom.bytes[3], g_rom_names[i].rom.bytes[2],
                          g_rom_names[i].rom.bytes[1], g_rom_names[i].rom.bytes[0],
                          g_rom_names[i].name);
        }
    }
}


// int main()
// {
//     ow_rom_t rom = ow_convert_hex_string("12-34-56-78-9A-BC-DE-F0");
//     printf("ROM ID: %llx\n", rom.id);

//     char buffer[48];
//     ow_convert_rom_to_dashed_hex_string(&rom, buffer, sizeof(buffer));
//     printf("ROM (dashed): '%s'\n", buffer);
// }
