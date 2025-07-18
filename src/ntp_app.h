#ifndef __NTP_APP_H__
#define __NTP_APP_H__

void ntp_app_begin();
void ntp_app_loop();
bool ntp_app_timer(void *);
void ntp_app_cron(const void *);
void ntp_app_trigger_synch();
bool ntp_app_LED(void *);

#endif // __NTP_APP_H__
