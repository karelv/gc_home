#ifndef __CONFIG_H__
#define __CONFIG_H__

bool builtin_sd_card_begin();
bool builtin_sd_card_present();
bool handle_reconnect_sd(void *);
void read_connect_links();
void store_connect_links();
void store_relay_states();
void read_and_restore_relay_states(); 
void read_config_json();

#endif // __CONFIG_H__
