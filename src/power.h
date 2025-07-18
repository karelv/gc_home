#ifndef __POWER_H__
#define __POWER_H__

#include <stdint.h>


void power_begin();
void power_down();
bool detect_AC(void *);

#endif // __POWER_H__