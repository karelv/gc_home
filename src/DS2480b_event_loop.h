#ifndef __DS2480B_EVENT_LOOP_H__
#define __DS2480B_EVENT_LOOP_H__

#include <stdint.h>
#include <Arduino.h>
#include <cppQueue.h>

#define DS2480B_MAX_TASKS 16
#define DS2480B_MAX_TASK_BUFFER 32

#define DS2480B_ACTION_NOP 0
#define DS2480B_ACTION_MASTER_RESET_CYCLE 1
#define DS2480B_ACTION_DISCOVER_SLAVES 2
#define DS2480B_ACTION_DS18B20_REQUEST_CONVERSION 3
#define DS2480B_ACTION_DS18B20_READ_TEMPERATURE 4

#define DS2480B_TASK_NOP 0
#define DS2480B_TASK_WRITE 1
#define DS2480B_TASK_READ 2
#define DS2480B_TASK_BREAK1 3
#define DS2480B_TASK_BREAK2 4
#define DS2480B_TASK_CACHE_SLAVE_ADDRESS 5
#define DS2480B_TASK_LAST 128

#define DS2480B_ERROR_OK 0
#define DS2480B_ERROR_NO_DATA 1
#define DS2480B_ERROR_UNKNOWN_TASK 2
#define DS2480B_ERROR_DATA_INVALID 3
#define DS2480B_ERROR_DURING_DISCOVER 4

class DS2480bEventLoop;

struct DS2480bTaskData {
  uint8_t action_;
  uint8_t task_;
  uint8_t count_;
  uint8_t status_;
  uint8_t buffer_[DS2480B_MAX_TASK_BUFFER];
  uint32_t waitTime_; // time to wait after this task has been executed
  uint8_t (*call_back_)(DS2480bEventLoop *ow, DS2480bTaskData *data);
};

class DS2480bEventLoop
{
  public:
    DS2480bEventLoop(HardwareSerial &serial, void (*call_back)(DS2480bEventLoop *ow, DS2480bTaskData *data));
    ~DS2480bEventLoop();
    void setDS2480bHardwareSerial(HardwareSerial &serial);
    void setCallBackFunction(void (*call_back)(DS2480bEventLoop *ow, DS2480bTaskData *data));

    void begin();

    uint8_t queueIsEmpty();
    uint8_t queueCount();
    uint8_t queueRemainingCount();

    // ACTIONS
    uint8_t discoverSlaves(uint8_t is_first = true);
    uint8_t DS18b20RequestConversionBroadcast();
    uint8_t DS18b20ReadTemperature(uint8_t *slaveAddress);
    uint8_t DS2480bMasterResetCycle();

    // loop
    void loop();

    // search variables
    uint8_t lastDiscrepancy_;
    uint8_t lastFamilyDiscrepancy_;
    uint8_t lastDeviceFlag_;
    uint8_t slaveAddress_[8];

//  protected:
  public:
    // variables
    HardwareSerial *serial_;
    void (*call_back_)(DS2480bEventLoop *ow, DS2480bTaskData *data);

    DS2480bTaskData taskData_[DS2480B_MAX_TASKS];
    cppQueue q_;
    uint32_t lastTaskStartMillis_;
    DS2480bTaskData currentTask_;

    // functions
};


#endif // __DS2480B_EVENT_LOOP_H__
