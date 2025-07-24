#ifndef __CONFIG_H__
#define __CONFIG_H__

bool builtin_sd_card_begin();
bool builtin_sd_card_present();
bool handle_reconnect_sd(void *);
void read_connect_links();
void read_one_wire_rom_id_names();
void store_connect_links();
void store_relay_states();
void read_and_restore_relay_states(); 
void read_config_json();


void sd_card_print_files();

#endif // __CONFIG_H__
