#include <Wire.h>
#include <Arduino.h>

// SCPI configuration
#define SCPI_MAX_TOKENS 32
#define SCPI_MAX_COMMANDS 32
#include <Vrekrer_scpi_parser.h>
// end SCPI configuration

#include "pins.h"


// I2C IO Expander address (example: PCF8574 default address: 0x20)
#define IOEXPANDER_ADDR 0x20
#define IOEXPANDER_ADDR2 0x62
#define IOEXPANDER_ADDR3 0x20

SCPI_Parser scpi;


void scpi_test_dsrm_data_request(SCPI_Commands cmd, SCPI_Parameters params, Stream& interface) {
  interface.println("Starting DSMR Data Request test");
  interface.println("Remark: measure at pin2 of the J37 connector(P1 / DSMR)");
  
  int param = -1;
  if (params.Size() > 0) {
    param = atoi(params[0]);
  }
  if (param == 0) {
    digitalWrite(PIN_P1_DATA_REQUEST, LOW);
    interface.println("P1_DATA_REQUEST set LOW (measure 0V at pin2 of J37)");
  } else if (param == 1) {
    digitalWrite(PIN_P1_DATA_REQUEST, HIGH);
    interface.println("P1_DATA_REQUEST set HIGH (measure 3.3V at pin2 of J37)");
  } else {
    interface.println("Invalid parameter");
  }
}

void scpi_test_dsrm_data_input(SCPI_Commands cmd, SCPI_Parameters params, Stream& interface) {
  interface.println("Starting DSMR Data Input test");
  interface.println("Remark: Run once with no connection at pin5 of the J37 connector(P1 / DSMR); see the state printed here as 0");
  interface.println("Remark: Run another time with GND1 connected to pin5 of the J37 connector(P1 / DSMR); see the state printed here as 1");

  interface.printf("P1_DATA_to_Teensy(%d) = %d\n", PIN_P1_DATA_to_Teensy, digitalRead(PIN_P1_DATA_to_Teensy));
  interface.printf("End of test mode\n"); 
}


void scpi_test_leds(SCPI_Commands cmd, SCPI_Parameters params, Stream& interface) 
{
  const int led_pins[] = {PIN_LED_SPECIAL, PIN_LED_ETH, PIN_LED_WWW, PIN_LED_MQTT, PIN_LED_IO};
  const char* led_names[] = {"LED_SPECIAL", "LED_ETH", "LED_WWW", "LED_MQTT", "LED_IO"};
  for (int i = 0; i < 5; ++i) {
    digitalWrite(led_pins[i], HIGH);
    Serial.printf("%s ON\n", led_names[i]);
    delay(1000);
    digitalWrite(led_pins[i], LOW);
    Serial.printf("%s OFF\n", led_names[i]);
  }
  interface.println("end of LED test");
}

void scpi_test_button(SCPI_Commands cmd, SCPI_Parameters params, Stream& interface) 
{
  int button_state = digitalRead(PIN_SPECIAL_BUTTON);
  if (button_state == HIGH) {
    interface.println("Button is NOT pressed");
  } else {
    interface.println("Button is pressed");
  }
}

void scpi_test_i2c1_ioexpander(SCPI_Commands cmd, SCPI_Parameters params, Stream& interface) {
  interface.println("Starting I2C1 IO Expander test");
  Wire.begin();
  for (int i = 0; i < 10; ++i) { // Toggle 10 times
    // Set all IOs HIGH
    Wire.beginTransmission(IOEXPANDER_ADDR);
    Wire.write(0xFF);
    Wire.endTransmission();
    interface.println("IO Expander: All HIGH");
    delay(1000);
    // Set all IOs LOW
    Wire.beginTransmission(IOEXPANDER_ADDR);
    Wire.write(0x00);
    Wire.endTransmission();
    interface.println("IO Expander: All LOW");
    delay(1000);
  }
  interface.println("End of I2C1 IO Expander test");
}

void scpi_test_i2c1_scan(SCPI_Commands cmd, SCPI_Parameters params, Stream& interface) {
  interface.println("Scanning I2C1 bus for devices...");
  Wire.begin();
  int found = 0;
  for (uint8_t addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0) {
      interface.printf("Found device at 0x%02X\n", addr);
      found++;
    }
    delay(2);
  }
  if (found == 0) {
    interface.println("No I2C devices found.");
  } else {
    interface.printf("Scan complete, %d device(s) found.\n", found);
  }
}

// I2C2 IO Expander address (example: PCF8574 default address)
void scpi_test_i2c2_ioexpander(SCPI_Commands cmd, SCPI_Parameters params, Stream& interface) {
  interface.println("Starting I2C2 IO Expander test");
  Wire1.begin();
  for (int i = 0; i < 10; ++i) {
    Wire1.beginTransmission(IOEXPANDER_ADDR2);
    Wire1.write(0xFF);
    Wire1.endTransmission();
    interface.println("IO Expander: All HIGH");
    delay(1000);
    Wire1.beginTransmission(IOEXPANDER_ADDR2);
    Wire1.write(0x00);
    Wire1.endTransmission();
    interface.println("IO Expander: All LOW");
    delay(1000);
  }
  interface.println("End of I2C2 IO Expander test");
}

void scpi_test_i2c2_scan(SCPI_Commands cmd, SCPI_Parameters params, Stream& interface) {
  interface.println("Scanning I2C2 bus for devices...");
  Wire1.begin();
  int found = 0;
  for (uint8_t addr = 1; addr < 127; addr++) {
    Wire1.beginTransmission(addr);
    if (Wire1.endTransmission() == 0) {
      interface.printf("Found device at 0x%02X\n", addr);
      found++;
    }
    delay(2);
  }
  if (found == 0) {
    interface.println("No I2C devices found.");
  } else {
    interface.printf("Scan complete, %d device(s) found.\n", found);
  }
}

void scpi_test_i2c3_ioexpander(SCPI_Commands cmd, SCPI_Parameters params, Stream& interface) {
  interface.println("Starting I2C3 IO Expander test");
  Wire2.begin();
  for (int i = 0; i < 10; ++i) {
    Wire2.beginTransmission(IOEXPANDER_ADDR3);
    Wire2.write(0xFF);
    Wire2.endTransmission();
    interface.println("IO Expander: All HIGH");
    delay(1000);
    Wire2.beginTransmission(IOEXPANDER_ADDR3);
    Wire2.write(0x00);
    Wire2.endTransmission();
    interface.println("IO Expander: All LOW");
    delay(1000);
  }
  interface.println("End of I2C3 IO Expander test");
}

void scpi_test_i2c3_scan(SCPI_Commands cmd, SCPI_Parameters params, Stream& interface) {
  interface.println("Scanning I2C3 bus for devices...(beaware of jumper UEXT!)");
  Wire2.begin();
  int found = 0;
  for (uint8_t addr = 1; addr < 127; addr++) {
    Wire2.beginTransmission(addr);
    if (Wire2.endTransmission() == 0) {
      interface.printf("Found device at 0x%02X\n", addr);
      found++;
    }
    delay(2);
  }
  if (found == 0) {
    interface.println("No I2C devices found.");
  } else {
    interface.printf("Scan complete, %d device(s) found.\n", found);
  }
}


void scpi_test_pwm_in(SCPI_Commands cmd, SCPI_Parameters params, Stream& interface) {
  interface.println("Starting PWM1 test");
  interface.println("Remark: it needs AC input power! (110V AC or 230V AC, 50 or 60Hz)");
  interface.println("Remark: Connect the +24V to PWMx VDDe");
  interface.println("Remark: Connect the GND3 to PWMx GNDe");
  interface.println("Remark: PWM1 ==> 24V, input is active low");
  interface.println("Remark: PWM2 ==> 24V, input is active high");
  digitalWrite(PIN_ENABLE_POWER, HIGH); // Turn on AC power

  int in1 = digitalRead(PIN_PWM_IN1_T);
  int in2 = digitalRead(PIN_PWM_IN2_T);
  interface.printf("PWM_IN1_T=%d, PWM_IN2_T=%d\n", in1, in2);
  interface.println("End of PWM test");
}


void scpi_test_pwm_out(SCPI_Commands cmd, SCPI_Parameters params, Stream& interface) {
  interface.println("Starting PWM output test");
  interface.println("Remark: it needs AC input power! (110V AC or 230V AC, 50 or 60Hz)");
  interface.println("Remark: Connect the +24V to PWMx VDDe");
  interface.println("Remark: Connect the GND3 to PWMx GNDe");
  interface.println("Remark: PWM1 ==> 24V, output is active low");
  interface.println("Remark: PWM2 ==> 24V, output is active high");

  digitalWrite(PIN_ENABLE_POWER, HIGH); // Turn on AC power
  for (int i = 0; i < 5; ++i) {
    digitalWrite(PIN_PWM_OUT1_T, LOW);
    digitalWrite(PIN_PWM_OUT2_T, LOW);
    interface.printf("Cycle %d: PWM_OUT1_T LOW, PWM_OUT2_T LOW\n", i+1);
    delay(2000);
    digitalWrite(PIN_PWM_OUT1_T, HIGH);
    digitalWrite(PIN_PWM_OUT2_T, HIGH);
    interface.printf("Cycle %d: PWM_OUT1_T HIGH, PWM_OUT2_T HIGH\n", i+1);
    delay(2000);
  }
  interface.println("End of PWM output test");
}


void scpi_test_ac_detector(SCPI_Commands cmd, SCPI_Parameters params, Stream& interface) {
  interface.println("Starting AC Detector test");
  interface.println("Remark: it needs AC input power! (110V AC or 230V AC, 50 or 60Hz)");
  digitalWrite(PIN_ENABLE_POWER, HIGH); // Turn on AC power
  const int samples = 200;
  char result[samples + 1];
  for (int i = 0; i < samples; ++i) {
    int val = digitalRead(PIN_AC_DETECTOR);
    result[i] = val ? '1' : '0';
    delay(1);
  }
  result[samples] = '\0';
  interface.printf("AC Detector result: %s\n", result);
  interface.println("End of AC Detector test");
}


void scpi_test_enable_ac_power(SCPI_Commands cmd, SCPI_Parameters params, Stream& interface) {
  interface.println("Starting ENABLE AC POWER test");
  interface.println("Remark: it needs AC input power! (110V AC or 230V AC, 50 or 60Hz)");
  interface.println("Remark: it will toggle the AC power on for 1 seconds and power off for 29 sec");
  interface.println("Remark: check that 24V and +5V LEDs are fading out, and kick in again");
  interface.println("Remark: It may take a while for the LED to turn off");
  digitalWrite(PIN_ENABLE_POWER, HIGH); // Turn on AC power

  digitalWrite(PIN_ENABLE_POWER, HIGH); // Power ON
  interface.println("AC Power ON");
  delay(1000);
  digitalWrite(PIN_ENABLE_POWER, LOW); // Power OFF
  interface.println("AC Power OFF");
  delay(29000);

  digitalWrite(PIN_ENABLE_POWER, HIGH); // End with power ON
  interface.println("AC Power ON (final)");
  interface.println("End of ENABLE AC POWER test");
}


void scpi_test_can_rxd(SCPI_Commands cmd, SCPI_Parameters params, Stream& interface) 
{
  interface.println("Starting CAN test");
  interface.println("Remark: it needs AC input power! (110V AC or 230V AC, 50 or 60Hz)");
  interface.println("Remark: run twice; once without anything on the CAN bus ==> expect CAN_RxD state 1");
  interface.println("Remark: and once with a one 120 Ohm resistor from CAN_L to GND and another 120 Ohm resistor from CAN_H to +5V");
  interface.println("      ==> expect CAN_RxD state 0");
  // Enable AC power
  digitalWrite(PIN_ENABLE_POWER, HIGH);
  interface.println("AC Power ON");
  // Set CAN_RS low
  pinMode(PIN_CAN_RS, OUTPUT);
  digitalWrite(PIN_CAN_RS, LOW);
  interface.println("CAN_RS set LOW");
  // Force CAN_TXD low
  pinMode(PIN_CAN_TxD, OUTPUT);
  digitalWrite(PIN_CAN_TxD, LOW);
  interface.println("CAN_TxD set LOW");
  // Print CAN_RXD state
  pinMode(PIN_CAN_RxD, INPUT);
  int rx_state = digitalRead(PIN_CAN_RxD);
  interface.printf("CAN_RxD state: %d\n", rx_state);
  interface.println("End of CAN test");
}


void scpi_test_can_txd_dom(SCPI_Commands cmd, SCPI_Parameters params, Stream& interface) 
{
  interface.println("Starting CAN output test");
  interface.println("Remark: it needs AC input power! (110V AC or 230V AC, 50 or 60Hz)");
  interface.println("Remark: make sure there is no external CAN bus connection during this test");
  interface.println("Remark: this test will toggle CAN_TXD for 1 second, then set it high for 1 second");
  interface.println("Remark: measure CAN_H-CAN_L with a multimeter to see the voltage levels");
  interface.println("Remark: Test is not working, measure at pin1 of U2");

  // Enable AC power
  digitalWrite(PIN_ENABLE_POWER, HIGH);
  interface.println("AC Power ON");
  // Set CAN_RS low
  pinMode(PIN_CAN_RS, OUTPUT);
  digitalWrite(PIN_CAN_RS, LOW);
  interface.println("CAN_RS set LOW");
  pinMode(PIN_CAN_TxD, OUTPUT);
  // Set CAN_TXD high for 1 second
  digitalWrite(PIN_CAN_TxD, HIGH);
  interface.println("CAN_TxD set HIGH (measure >0.9V on CAN_H and CAN_L)");
  delay(1000);
  interface.println("End of CAN output test");
}

void scpi_test_can_txd_res(SCPI_Commands cmd, SCPI_Parameters params, Stream& interface) 
{
  interface.println("Starting CAN output test");
  interface.println("Remark: it needs AC input power! (110V AC or 230V AC, 50 or 60Hz)");
  interface.println("Remark: make sure there is no external CAN bus connection during this test");
  interface.println("Remark: this test will toggle CAN_TXD for 1 second, then set it high for 1 second");
  interface.println("Remark: measure CAN_H-CAN_L with a multimeter to see the voltage levels");
  interface.println("Remark: Test is not working, measure at pin1 of U2");
 
  // Enable AC power
  digitalWrite(PIN_ENABLE_POWER, HIGH);
  interface.println("AC Power ON");
  // Set CAN_RS low
  pinMode(PIN_CAN_RS, OUTPUT);
  digitalWrite(PIN_CAN_RS, LOW);
  interface.println("CAN_RS set LOW");
  // Force CAN_TXD low for 1 second
  pinMode(PIN_CAN_TxD, OUTPUT);
  digitalWrite(PIN_CAN_TxD, LOW);
  interface.println("CAN_TxD set LOW (measure 0V on CAN_H and CAN_L)");
  delay(1000);
  // Set CAN_TXD high for 1 second
  // digitalWrite(PIN_CAN_TxD, HIGH);
  // interface.println("CAN_TxD set HIGH (measure >0.9V on CAN_H and CAN_L)");
  // delay(1000);
  interface.println("End of CAN output test");
}


void scpi_test_lin_control(SCPI_Commands cmd, SCPI_Parameters params, Stream& interface) {
  interface.println("Starting LIN test");
  interface.println("Remark: it needs AC input power! (110V AC or 230V AC, 50 or 60Hz)");
  interface.println("Remark: Measure on Pin 2 of U3");
  interface.println("Remark: Measure on Pin 3 of U3");

  digitalWrite(PIN_ENABLE_POWER, HIGH); // Turn on AC power
  for (int i = 0; i < 5; ++i) {
    digitalWrite(PIN_LIN_WK, HIGH);
    digitalWrite(PIN_LIN_EN, HIGH);
    interface.printf("Cycle %d: LIN_WK and LIN_EN ON\n", i+1);
    delay(2000);
    digitalWrite(PIN_LIN_WK, LOW);
    digitalWrite(PIN_LIN_EN, LOW);
    interface.printf("Cycle %d: LIN_WK and LIN_EN OFF\n", i+1);
    delay(2000);
  }
  digitalWrite(PIN_LIN_WK, HIGH);
  digitalWrite(PIN_LIN_EN, HIGH);
  interface.println("End of LIN test (LIN_WK and LIN_EN ON)");
}


void scpi_test_lin_rxd(SCPI_Commands cmd, SCPI_Parameters params, Stream& interface) {
  interface.println("Starting LIN RxD test");
  interface.println("Remark: it needs AC input power! (110V AC or 230V AC, 50 or 60Hz)");
  interface.println("Remark: Force Pin 1 of U3 to GND2 or leave it floating");
  interface.println("Remark: And run this test; check if the state changes");
  digitalWrite(PIN_ENABLE_POWER, HIGH); // Turn on AC power
  pinMode(PIN_LIN_RxD, INPUT);
  int rxd = digitalRead(PIN_LIN_RxD);
  interface.printf("LIN_RxD state: %d\n", rxd);
  interface.println("End of LIN RxD test");
}


void scpi_test_lin_txd(SCPI_Commands cmd, SCPI_Parameters params, Stream& interface) {
  interface.println("Starting LIN TxD test");
  interface.println("Remark: it needs AC input power! (110V AC or 230V AC, 50 or 60Hz)");
  interface.println("Remark: Measure on Pin 4 of U3 (use GND2)");
  digitalWrite(PIN_ENABLE_POWER, HIGH); // Turn on AC power
  pinMode(PIN_LIN_TxD, OUTPUT);
  for (int i = 0; i < 5; ++i) {
    digitalWrite(PIN_LIN_TxD, HIGH);
    interface.printf("Cycle %d: LIN_TxD HIGH\n", i+1);
    delay(2000);
    digitalWrite(PIN_LIN_TxD, LOW);
    interface.printf("Cycle %d: LIN_TxD LOW\n", i+1);
    delay(2000);
  }
  digitalWrite(PIN_LIN_TxD, HIGH);
  interface.println("End of LIN TxD test (LIN_TxD HIGH)");
}


void scpi_test_1w_txd(SCPI_Commands cmd, SCPI_Parameters params, Stream& interface) {
  interface.println("Starting 1-Wire TxD test");
  interface.println("Remark: it needs AC input power! (110V AC or 230V AC, 50 or 60Hz)");
  interface.println("Remark: Measure on pin 7 of IC1 (GND2)");
  digitalWrite(PIN_ENABLE_POWER, HIGH); // Turn on AC power
  pinMode(PIN_1W_TxD, OUTPUT);
  for (int i = 0; i < 5; ++i) {
    digitalWrite(PIN_1W_TxD, HIGH);
    interface.printf("Cycle %d: 1W_TxD HIGH\n", i+1);
    delay(2000);
    digitalWrite(PIN_1W_TxD, LOW);
    interface.printf("Cycle %d: 1W_TxD LOW\n", i+1);
    delay(2000);
  }
  digitalWrite(PIN_1W_TxD, HIGH);
  interface.println("End of 1-Wire TxD test (1W_TxD HIGH)");
}


void scpi_handle_serial() 
{
  static char buffer[64];
  static size_t idx = 0;
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n' || c == '\r') {
      buffer[idx] = '\0';
      if (idx > 0) {
        Serial.println();
        scpi.last_error = SCPI_Parser::ErrorCode::NoError; // Reset last error
        scpi.Execute(buffer, Serial);
        if (scpi.last_error == SCPI_Parser::ErrorCode::UnknownCommand) {
          Serial.println("Unknown SCPI command");
        }
      }
      idx = 0; // Reset buffer for next command
    } else if (idx < sizeof(buffer) - 1) {
      buffer[idx++] = c;
    }
    // If buffer overflows, reset
    else {
      idx = 0;
      Serial.println("Input too long, buffer cleared.");
    }
  }
}


void scpi_register_commands()
{
  scpi.RegisterCommand("TEST:DSMR:DATA_REQUEST", scpi_test_dsrm_data_request);
  scpi.RegisterCommand("TEST:DSMR:DATA_INPUT", scpi_test_dsrm_data_input);
  scpi.RegisterCommand("TEST:LEDS", scpi_test_leds);
  scpi.RegisterCommand("TEST:BUTTON", scpi_test_button);
  scpi.RegisterCommand("TEST:I2C1:IOEXPANDER", scpi_test_i2c1_ioexpander);
  scpi.RegisterCommand("TEST:I2C1:SCAN", scpi_test_i2c1_scan);
  scpi.RegisterCommand("TEST:I2C2:IOEXPANDER", scpi_test_i2c2_ioexpander);
  scpi.RegisterCommand("TEST:I2C2:SCAN", scpi_test_i2c2_scan);
  scpi.RegisterCommand("TEST:I2C3:IOEXPANDER", scpi_test_i2c3_ioexpander);
  scpi.RegisterCommand("TEST:I2C3:SCAN", scpi_test_i2c3_scan);
  scpi.RegisterCommand("TEST:PWM:INPUT", scpi_test_pwm_in);
  scpi.RegisterCommand("TEST:PWM:OUTPUT", scpi_test_pwm_out);
  scpi.RegisterCommand("TEST:AC_DETECTOR", scpi_test_ac_detector);
  scpi.RegisterCommand("TEST:ENABLE_AC_POWER", scpi_test_enable_ac_power);
  scpi.RegisterCommand("TEST:CAN:RXD", scpi_test_can_rxd);
  scpi.RegisterCommand("TEST:CAN:TXD:DOM", scpi_test_can_txd_dom);
  scpi.RegisterCommand("TEST:CAN:TXD:RES", scpi_test_can_txd_res);
  scpi.RegisterCommand("TEST:LIN:CONTROL", scpi_test_lin_control);
  scpi.RegisterCommand("TEST:LIN:RXD", scpi_test_lin_rxd);
  scpi.RegisterCommand("TEST:LIN:TXD", scpi_test_lin_txd);
  scpi.RegisterCommand("TEST:1W:TXD", scpi_test_1w_txd);
  // use this print to examine how large the buffers needs to be...
  // scpi.PrintDebugInfo(Serial);
}
