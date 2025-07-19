#ifndef __PINS_H__
#define __PINS_H__

// AC Power
#define PIN_AC_DETECTOR 33
#define PIN_ENABLE_POWER 37

// Buttons & LEDs
#define PIN_SPECIAL_BUTTON 4
#define PIN_LED_SPECIAL 1
#define PIN_LED_ETH 38
#define PIN_LED_WWW 39
#define PIN_LED_MQTT 40
#define PIN_LED_IO 26

// DSMR
#define PIN_P1_DATA_to_Teensy 22
#define PIN_P1_DATA_REQUEST 23

// PWM1&2
#define PIN_PWM_OUT1_T 6
#define PIN_PWM_IN1_T 5
#define PIN_PWM_OUT2_T 9
#define PIN_PWM_IN2_T 27

// LIN
#define PIN_LIN_EN 2
#define PIN_LIN_WK 3
#define PIN_LIN_TxD 8
#define PIN_LIN_RxD 7


// CAN
#define PIN_CAN_RS 32
#define PIN_CAN_TxD 31
#define PIN_CAN_RxD 30

// One Wire
#define PIN_1W_TxD 29
#define PIN_1W_RxD 28

#endif // __PINS_H__
