#include "DS2480b_event_loop.h"

#include "ds2480b.h"

union ByteFloat 
{
  float float_;
  uint8_t bytes_[sizeof(float)];
};


// TEST BUILD
static uint8_t dscrc_table[] = {
      0, 94,188,226, 97, 63,221,131,194,156,126, 32,163,253, 31, 65,
    157,195, 33,127,252,162, 64, 30, 95,  1,227,189, 62, 96,130,220,
     35,125,159,193, 66, 28,254,160,225,191, 93,  3,128,222, 60, 98,
    190,224,  2, 92,223,129, 99, 61,124, 34,192,158, 29, 67,161,255,
     70, 24,250,164, 39,121,155,197,132,218, 56,102,229,187, 89,  7,
    219,133,103, 57,186,228,  6, 88, 25, 71,165,251,120, 38,196,154,
    101, 59,217,135,  4, 90,184,230,167,249, 27, 69,198,152,122, 36,
    248,166, 68, 26,153,199, 37,123, 58,100,134,216, 91,  5,231,185,
    140,210, 48,110,237,179, 81, 15, 78, 16,242,172, 47,113,147,205,
     17, 79,173,243,112, 46,204,146,211,141,111, 49,178,236, 14, 80,
    175,241, 19, 77,206,144,114, 44,109, 51,209,143, 12, 82,176,238,
     50,108,142,208, 83, 13,239,177,240,174, 76, 18,145,207, 45,115,
    202,148,118, 40,171,245, 23, 73,  8, 86,180,234,105, 55,213,139,
     87,  9,235,181, 54,104,138,212,149,203, 41,119,244,170, 72, 22,
    233,183, 85, 11,136,214, 52,106, 43,117,151,201, 74, 20,246,168,
    116, 42,200,150, 21, 75,169,247,182,232, 10, 84,215,137,107, 53};


//--------------------------------------------------------------------------
// Bit utility to read and write a bit in the buffer 'buf'.
//
// 'op'   - operation (1) to set and (0) to read
// 'state' - set (1) or clear (0) if operation is write (1)
// 'loc'  - bit number location to read or write
// 'buf'  - pointer to array of bytes that contains the bit
//        to read or write
//
// Returns: 1  if operation is set (1)
//       0/1 state of bit number 'loc' if operation is reading
//
static uint8_t bitacc(uint8_t op, uint8_t state, uint8_t loc, uint8_t *buf)
{
  uint8_t nbyt,nbit;

  nbyt = (loc / 8);
  // nbit = (loc % 8);
  nbit = loc - (nbyt * 8);

  if (op == WRITE_FUNCTION)
  {
    if (state)
      buf[nbyt] |= (0x01 << nbit);
    else
      buf[nbyt] &= ~(0x01 << nbit);

    return 1;
  }
  else
    return ((buf[nbyt] >> nbit) & 0x01);
}


//--------------------------------------------------------------------------
// Calculate the CRC8 of the byte value provided with the current 
// Returns current crc8 value
//
static uint8_t docrc8(uint8_t &crc, uint8_t value)
{
  // See Application Note 27
  
  // TEST BUILD
  crc = dscrc_table[crc ^ value];
  return crc;
}


static uint8_t DS2480bTaskCheckMasterResetCycleCB1(DS2480bEventLoop *ow, DS2480bTaskData *data)
{
  if (((data->buffer_[3] & 0xF1) == 0x00) &&
      ((data->buffer_[3] & 0x0E) == PARMSET_9600) &&
      ((data->buffer_[4] & 0xF0) == 0x90) &&
      ((data->buffer_[4] & 0x0C) == PARMSET_9600))
  {
    return 1;
  }
  return 0;
}

static uint8_t DS2480bTaskResetCB1(DS2480bEventLoop *ow, DS2480bTaskData *data)
{
  if (((data->buffer_[0] & RB_RESET_MASK) == RB_PRESENCE) ||
      ((data->buffer_[0] & RB_RESET_MASK) == RB_ALARMPRESENCE))
  {
    return 1;
  }
  return 0;
}

static uint8_t DS2480bTaskDecodeSearchCB(DS2480bEventLoop *ow, DS2480bTaskData *data)
{
  uint8_t last_zero = 0;
  uint8_t i;
  uint8_t tmp_rom[8];
  for (i = 0; i < 64; i++)
  {
    // get the ROM bit
    bitacc(WRITE_FUNCTION,
          bitacc(READ_FUNCTION,0,(short)(i * 2 + 1),&data->buffer_[1]),i,
          &tmp_rom[0]);
    // check LastDiscrepancy
    if ((bitacc(READ_FUNCTION,0,(short)(i * 2),&data->buffer_[1]) == 1) &&
        (bitacc(READ_FUNCTION,0,(short)(i * 2 + 1),&data->buffer_[1]) == 0))
    {
      last_zero = i + 1;
      // check LastFamilyDiscrepancy
      if (i < 8)
        ow->lastFamilyDiscrepancy_ = i + 1;
    }
  }

  // do dowcrc
  uint8_t crc = 0;
  for (i = 0; i < 8; i++)
    docrc8(crc, tmp_rom[i]);

  // check results
  if ((crc != 0) || (ow->lastDiscrepancy_ == 63) || (tmp_rom[0] == 0))
  {
    // error during search
    // reset the search
    ow->lastDiscrepancy_ = 0;
    ow->lastDeviceFlag_ = 0;
    ow->lastFamilyDiscrepancy_ = 0;
    data->status_ = DS2480B_ERROR_DURING_DISCOVER;
    return 0;
  }


  // set the last discrepancy
  ow->lastDiscrepancy_ = last_zero;

  // check for last device
  if (ow->lastDiscrepancy_ == 0)
    ow->lastDeviceFlag_ = 1;

  memset(data->buffer_, 0, sizeof(data->buffer_));
  // copy the ROM to the buffer
  for (i = 0; i < 8; i++)
  {
    ow->slaveAddress_[i] = tmp_rom[i];
    data->buffer_[i] = tmp_rom[i];
  }
  data->buffer_[8] = ow->lastDeviceFlag_;
  data->count_ = 9;
  return 1;
}


static uint8_t DS18b20TaskReadTemperatureCB(DS2480bEventLoop *ow, DS2480bTaskData *data)
{
  // 1. check crc on scratch pad
  // 2. convert temperature
  // 3. store slave address in task.buffer
  // 4. store temp in task.buffer

  // 1. check crc on scratch pad
  uint8_t crc = 0;
  for (uint8_t i = 0; i < 9; i++)
    docrc8(crc, data->buffer_[i+1]);
  if (crc != 0)
  {
    data->status_ = DS2480B_ERROR_DATA_INVALID;
    return 0;
  }

  // 2. convert temperature
  int16_t temp = data->buffer_[1];
  temp |= int16_t(data->buffer_[2]) << 8;
  ByteFloat temperature;
  temperature.float_ = float(temp)/16;

  // 3. store slave address in task.buffer
  // 3.1 shift 12 bytes at start of buffer for slave address and float temp storage
  for (uint8_t i=9; i>0; i--)
  {
    data->buffer_[i+11] = data->buffer_[i];
  }
  // 3.2 copy the slave address
  for (uint8_t i = 0; i < 8; i++)
  {
    data->buffer_[i] = ow->slaveAddress_[i];
  }

  // 4. store temp in task.buffer
  for (uint8_t i = 0; i < 4; i++)
  {
    data->buffer_[i+8] = temperature.bytes_[i];
  }  
  return 1; // ok
}


DS2480bEventLoop::DS2480bEventLoop(HardwareSerial &serial, void (*call_back)(DS2480bEventLoop *ow, DS2480bTaskData *data))
    : lastDiscrepancy_(0),
      lastFamilyDiscrepancy_(0),
      lastDeviceFlag_(0),
      serial_(&serial),
      call_back_(call_back),
      q_(sizeof(DS2480bTaskData), DS2480B_MAX_TASKS, FIFO, false, taskData_, sizeof(taskData_)),
      lastTaskStartMillis_(0)
{
  memset(this->taskData_, 0, sizeof(this->taskData_));
  memset(&this->currentTask_, 0, sizeof(this->currentTask_));
  memset(&this->slaveAddress_, 0, sizeof(this->slaveAddress_));
}


DS2480bEventLoop::~DS2480bEventLoop()
{ // not supposed to be called, but let's close the serial port
  if (this->serial_ != NULL)
  {
    this->serial_->end();
    this->serial_ = NULL;
  }
}


void DS2480bEventLoop::setDS2480bHardwareSerial(HardwareSerial &serial)
{
  if (this->serial_ != NULL)
  {
    this->serial_->end();
  }
  this->serial_ = &serial;
  this->begin();
}


void DS2480bEventLoop::setCallBackFunction(void (*call_back)(DS2480bEventLoop *ow, DS2480bTaskData *data))
{
  this->call_back_ = call_back;
}


void DS2480bEventLoop::begin()
{
  this->serial_->begin(9600);
}


uint8_t DS2480bEventLoop::queueIsEmpty()
{
  return this->q_.isEmpty();
}


uint8_t DS2480bEventLoop::queueCount()
{
  return this->q_.getCount();
}


uint8_t DS2480bEventLoop::queueRemainingCount()
{
  return this->q_.getRemainingCount();
}


uint8_t DS2480bEventLoop::discoverSlaves(uint8_t is_first)
{
  if (!this->queueIsEmpty()) return 0;
  if (is_first)
  {
    this->lastDiscrepancy_ = 0;
    this->lastFamilyDiscrepancy_ = 0;
    this->lastDeviceFlag_ = 0;
    memset(&this->slaveAddress_, 0, sizeof(this->slaveAddress_));
  }

  uint8_t result = 0;
  DS2480bTaskData task_data; 

  memset(&task_data, 0, sizeof(task_data));
  task_data.action_ = DS2480B_ACTION_DISCOVER_SLAVES;
  task_data.task_ = DS2480B_TASK_WRITE;
  task_data.count_ = 0;
  task_data.buffer_[task_data.count_++] = MODE_COMMAND;
  task_data.buffer_[task_data.count_++] = CMD_COMM | FUNCTSEL_RESET | SPEEDSEL_FLEX;
  task_data.waitTime_ = 10;
  if (!this->q_.push(&task_data)) return 0;
  result++;

  memset(&task_data, 0, sizeof(task_data));
  task_data.action_ = DS2480B_ACTION_DISCOVER_SLAVES;
  task_data.task_ = DS2480B_TASK_READ;
  task_data.count_ = 1;
  task_data.waitTime_ = 2;
  task_data.call_back_ = DS2480bTaskResetCB1;
  if (!this->q_.push(&task_data)) return 0;
  result++;

  memset(&task_data, 0, sizeof(task_data));
  task_data.action_ = DS2480B_ACTION_DISCOVER_SLAVES;
  task_data.task_ = DS2480B_TASK_WRITE;
  task_data.count_ = 0;
  task_data.buffer_[task_data.count_++] = MODE_DATA;
  task_data.buffer_[task_data.count_++] = 0xF0; // SEARCH command
  task_data.buffer_[task_data.count_++] = MODE_COMMAND;
  task_data.buffer_[task_data.count_++] = CMD_COMM | FUNCTSEL_SEARCHON | SPEEDSEL_FLEX; // search mode on
  task_data.buffer_[task_data.count_++] = MODE_DATA;
  // add the 16 bytes of the search
  uint8_t pos = task_data.count_;
  task_data.count_ += 16;
  // only modify bits if not the first search
  if (this->lastDiscrepancy_ != 0)
  {
    // set the bits in the added buffer
    for (uint8_t i = 0; i < 64; i++)
    {
      // before last discrepancy
      if (i < (this->lastDiscrepancy_ - 1))
          bitacc(WRITE_FUNCTION,
             bitacc(READ_FUNCTION,0,i,&this->slaveAddress_[0]),
             (short)(i * 2 + 1),
             &task_data.buffer_[pos]);
      // at last discrepancy
      else if (i == (this->lastDiscrepancy_ - 1))
           bitacc(WRITE_FUNCTION,1,(short)(i * 2 + 1),
             &task_data.buffer_[pos]);
      // after last discrepancy so leave zeros
    }
  }
  // change back to command mode
  task_data.buffer_[task_data.count_++] = MODE_COMMAND; 
  // search OFF command
  task_data.buffer_[task_data.count_++] = (uint8_t)(CMD_COMM | FUNCTSEL_SEARCHOFF | SPEEDSEL_FLEX);
  task_data.waitTime_ = 40;
  if (!this->q_.push(&task_data)) return 0;
  result++;

  memset(&task_data, 0, sizeof(task_data));
  task_data.action_ = DS2480B_ACTION_DISCOVER_SLAVES;
  task_data.task_ = DS2480B_TASK_READ | DS2480B_TASK_LAST;
  task_data.count_ = 17;
  memset(task_data.buffer_, 0, sizeof(task_data.buffer_));
  task_data.waitTime_ = 2;
  task_data.call_back_ = DS2480bTaskDecodeSearchCB;
  if (!this->q_.push(&task_data)) return 0;
  result++;

  return result;
}


uint8_t DS2480bEventLoop::DS18b20RequestConversionBroadcast()
{
  if (!this->queueIsEmpty()) return 0;

  uint8_t result = 0;
  DS2480bTaskData task_data; 

  memset(&task_data, 0, sizeof(task_data));
  task_data.action_ = DS2480B_ACTION_DS18B20_REQUEST_CONVERSION;
  task_data.task_ = DS2480B_TASK_WRITE;
  task_data.count_ = 0;
  task_data.buffer_[task_data.count_++] = MODE_COMMAND; // command mode
  task_data.buffer_[task_data.count_++] = CMD_COMM | FUNCTSEL_RESET | SPEEDSEL_FLEX;
  task_data.waitTime_ = 10;
  if (!this->q_.push(&task_data)) return 0;
  result++;

  memset(&task_data, 0, sizeof(task_data));
  task_data.action_ = DS2480B_ACTION_DS18B20_REQUEST_CONVERSION;
  task_data.task_ = DS2480B_TASK_READ;
  task_data.count_ = 1;
  task_data.waitTime_ = 2;
  task_data.call_back_ = DS2480bTaskResetCB1;
  if (!this->q_.push(&task_data)) return 0;
  result++;

  memset(&task_data, 0, sizeof(task_data));
  task_data.action_ = DS2480B_ACTION_DS18B20_REQUEST_CONVERSION;
  task_data.task_ = DS2480B_TASK_WRITE;
  task_data.count_ = 0;
  task_data.buffer_[task_data.count_++] = MODE_DATA; // enter data mode
  task_data.buffer_[task_data.count_++] = 0xCC; // skip ROM (slave address matching)
  task_data.waitTime_ = 5;
  if (!this->q_.push(&task_data)) return 0;
  result++;
  
  memset(&task_data, 0, sizeof(task_data));
  task_data.action_ = DS2480B_ACTION_DS18B20_REQUEST_CONVERSION;
  task_data.task_ = DS2480B_TASK_WRITE;
  task_data.count_ = 0;
  task_data.buffer_[task_data.count_++] = 0x44; // start DS18b20 temperature conversion.
  task_data.waitTime_ = 999;
  if (!this->q_.push(&task_data)) return 0;
  result++;

  memset(&task_data, 0, sizeof(task_data));
  task_data.action_ = DS2480B_ACTION_DS18B20_REQUEST_CONVERSION;
  task_data.task_ = DS2480B_TASK_NOP | DS2480B_TASK_LAST;
  task_data.waitTime_ = 1;
  if (!this->q_.push(&task_data)) return 0;
  result++;

  return result;
}


uint8_t DS2480bEventLoop::DS18b20ReadTemperature(uint8_t *slaveAddress)
{
  if (!this->queueIsEmpty()) return 0;

  uint8_t result = 0;
  DS2480bTaskData task_data; 

  // reset
  memset(&task_data, 0, sizeof(task_data));
  task_data.action_ = DS2480B_ACTION_DS18B20_READ_TEMPERATURE;
  task_data.task_ = DS2480B_TASK_WRITE;
  task_data.count_ = 0;
  task_data.buffer_[task_data.count_++] = MODE_COMMAND; // back to command mode
  task_data.buffer_[task_data.count_++] = CMD_COMM | FUNCTSEL_RESET | SPEEDSEL_FLEX;
  task_data.waitTime_ = 5;
  if (!this->q_.push(&task_data)) return 0;
  result++;

  memset(&task_data, 0, sizeof(task_data));
  task_data.action_ = DS2480B_ACTION_DS18B20_READ_TEMPERATURE;
  task_data.task_ = DS2480B_TASK_READ;
  task_data.count_ = 1;
  task_data.waitTime_ = 10;
  task_data.call_back_ = DS2480bTaskResetCB1;
  if (!this->q_.push(&task_data)) return 0;
  result++;

  // ROM match (slave address match)
  memset(&task_data, 0, sizeof(task_data));
  task_data.action_ = DS2480B_ACTION_DS18B20_READ_TEMPERATURE;
  task_data.task_ = DS2480B_TASK_WRITE;
  task_data.count_ = 0;
  task_data.buffer_[task_data.count_++] = MODE_DATA; // enter data mode
  task_data.buffer_[task_data.count_++] = 0x55; // match command
  for (uint8_t i = 0; i < 8; i++)
  {
    task_data.buffer_[task_data.count_++] = slaveAddress[i];
    if (slaveAddress[i] == MODE_COMMAND)
    { // duplicate data that looks like COMMAND mode
      task_data.buffer_[task_data.count_++] = slaveAddress[i];
    }
  }
  task_data.waitTime_ = 15;
  if (!this->q_.push(&task_data)) return 0;
  result++;

  memset(&task_data, 0, sizeof(task_data));
  task_data.action_ = DS2480B_ACTION_DS18B20_READ_TEMPERATURE;
  task_data.task_ = DS2480B_TASK_READ;
  task_data.count_ = 9;
  task_data.waitTime_ = 2;
  if (!this->q_.push(&task_data)) return 0;
  result++;

  memset(&task_data, 0, sizeof(task_data));
  task_data.action_ = DS2480B_ACTION_DS18B20_READ_TEMPERATURE;
  task_data.task_ = DS2480B_TASK_CACHE_SLAVE_ADDRESS;
  task_data.count_ = 0;
  for (uint8_t i = 0; i < 8; i++)
  {
    task_data.buffer_[task_data.count_++] = slaveAddress[i];
  }
  task_data.waitTime_ = 0;
  if (!this->q_.push(&task_data)) return 0;
  result++;

  // read scratchpad of slave address
  memset(&task_data, 0, sizeof(task_data));
  task_data.action_ = DS2480B_ACTION_DS18B20_READ_TEMPERATURE;
  task_data.task_ = DS2480B_TASK_WRITE;
  task_data.count_ = 0;
  task_data.buffer_[task_data.count_++] = 0xBE; // read scratch pad command
  for (uint8_t i = 0; i < 9; i++)
    task_data.buffer_[task_data.count_++] = 0xFF;
  task_data.waitTime_ = 15;
  if (!this->q_.push(&task_data)) return 0;
  result++;

  memset(&task_data, 0, sizeof(task_data));
  task_data.action_ = DS2480B_ACTION_DS18B20_READ_TEMPERATURE;
  task_data.task_ = DS2480B_TASK_READ | DS2480B_TASK_LAST;
  task_data.count_ = 10;
  task_data.waitTime_ = 2;
  task_data.call_back_ = DS18b20TaskReadTemperatureCB;
  if (!this->q_.push(&task_data)) return 0;
  result++;


  return result;
}


uint8_t DS2480bEventLoop::DS2480bMasterResetCycle()
{
  uint8_t result = 0;
  DS2480bTaskData task_data; 
  memset(&task_data, 0, sizeof(task_data));
  task_data.action_ = DS2480B_ACTION_MASTER_RESET_CYCLE;
  task_data.task_ = DS2480B_TASK_BREAK1;
  task_data.waitTime_ = 5;
  if (!this->q_.push(&task_data)) return 0;
  result++;

  memset(&task_data, 0, sizeof(task_data));
  task_data.action_ = DS2480B_ACTION_MASTER_RESET_CYCLE;
  task_data.task_ = DS2480B_TASK_BREAK2;
  task_data.waitTime_ = 5;
  if (!this->q_.push(&task_data)) return 0;
  result++;

  memset(&task_data, 0, sizeof(task_data));
  task_data.action_ = DS2480B_ACTION_MASTER_RESET_CYCLE;
  task_data.task_ = DS2480B_TASK_WRITE;
  task_data.count_ = 1;
  task_data.buffer_[0] = 0xC1; // send the timing byte
  task_data.waitTime_ = 2;
  if (!this->q_.push(&task_data)) return 0;
  result++;

  memset(&task_data, 0, sizeof(task_data));
  task_data.action_ = DS2480B_ACTION_MASTER_RESET_CYCLE;
  task_data.task_ = DS2480B_TASK_WRITE;
  task_data.count_ = 0;
  // set the FLEX configuration parameters
  // default PDSRC = 1.37Vus
  task_data.buffer_[task_data.count_++] = CMD_CONFIG | PARMSEL_SLEW | PARMSET_Slew1p37Vus;
  // default W1LT = 10us
  task_data.buffer_[task_data.count_++] = CMD_CONFIG | PARMSEL_WRITE1LOW | PARMSET_Write10us;
  // default DSO/WORT = 8us
  task_data.buffer_[task_data.count_++] = CMD_CONFIG | PARMSEL_SAMPLEOFFSET | PARMSET_SampOff8us;

  // construct the command to read the baud rate (to test command block)
  task_data.buffer_[task_data.count_++] = CMD_CONFIG | PARMSEL_PARMREAD | (PARMSEL_BAUDRATE >> 3);

  // also do 1 bit operation (to test 1-Wire block)
  task_data.buffer_[task_data.count_++] = CMD_COMM | FUNCTSEL_BIT | PARMSET_9600 | BITPOL_ONE;

  task_data.waitTime_ = 10;
  if (!this->q_.push(&task_data)) return 0;
  result++;

  memset(&task_data, 0, sizeof(task_data));
  task_data.action_ = DS2480B_ACTION_MASTER_RESET_CYCLE;
  task_data.task_ = DS2480B_TASK_READ | DS2480B_TASK_LAST;
  task_data.count_ = 5;
  memset(task_data.buffer_, 0, sizeof(task_data.buffer_));
  task_data.waitTime_ = 2;
  task_data.call_back_ = DS2480bTaskCheckMasterResetCycleCB1;
  if (!this->q_.push(&task_data)) return 0;
  result++;

  return result;
}


void DS2480bEventLoop::loop()
{
  uint32_t time_since_last_task = millis() - this->lastTaskStartMillis_;
  if (time_since_last_task < this->currentTask_.waitTime_) return;
  // ok, we are ready for the next task
  if (this->q_.isEmpty()) // no next task, set the current task to NOP
  {
    memset(&this->currentTask_, 0, sizeof(this->currentTask_));
    return;
  } 

  if (this->q_.pop(&this->currentTask_))
  {
    this->lastTaskStartMillis_ = millis();

    switch(this->currentTask_.task_ & 0x7F)
    {
      case DS2480B_TASK_NOP:
        break;
      case DS2480B_TASK_BREAK1:
        this->serial_->begin(4800);
        this->serial_->write(uint8_t(0));
        break;
      case DS2480B_TASK_BREAK2:
        this->serial_->begin(9600);
        break;
      case DS2480B_TASK_WRITE:
        // empty the input buffer
        while (this->serial_->available())
        {
          this->serial_->read();
        }
        // Serial.printf("DS2480B_TASK_WRITE: %d ms | ", millis());
        // for (uint8_t i=0; i<this->currentTask_.count_; i++)
        // {
        //   Serial.printf("%02X-", this->currentTask_.buffer_[i]);
        // }
        // Serial.printf("\n");
        this->serial_->write(this->currentTask_.buffer_, this->currentTask_.count_);
        break;
      case DS2480B_TASK_READ:
        if (this->serial_->available() >= this->currentTask_.count_)
        {
          this->serial_->readBytes(this->currentTask_.buffer_, this->currentTask_.count_);

          // Serial.printf("DS2480B_TASK_READ: %d ms | ", millis());
          // for (uint8_t i=0; i<this->currentTask_.count_; i++)
          // {
          //   Serial.printf("%02X-", this->currentTask_.buffer_[i]);
          // }
          // Serial.printf("\n");


        } else
        {
          this->currentTask_.status_ = DS2480B_ERROR_NO_DATA;
          // Serial.printf("ERROR -- DS2480B_TASK_READ: %d ms | ", millis());
          // delay(100);
          // while (this->serial_->available())
          // {
          //   Serial.printf("%02X-", this->serial_->read());
          // }
          // Serial.printf("\n");
        }
        break;
      case DS2480B_TASK_CACHE_SLAVE_ADDRESS:
        for (uint8_t i=0; i<8; i++)
        {
          this->slaveAddress_[i] = this->currentTask_.buffer_[i];
        }
        break;
      default:
        Serial.printf("DS2480B_ERROR_UNKNOWN_TASK: %d ms | ", millis());
        this->currentTask_.status_ = DS2480B_ERROR_UNKNOWN_TASK;
        break;
    }

    if (this->currentTask_.status_ != DS2480B_ERROR_OK)
    { // report the error in the generic call_back function
      this->q_.clean(); // empty the queue
      this->call_back_(this, &this->currentTask_);
      return;
    }

    if (this->currentTask_.call_back_)
    {
      if (!this->currentTask_.call_back_(this, &this->currentTask_))
      { // report the error
        this->q_.clean(); // empty the queue
        this->currentTask_.status_ = DS2480B_ERROR_DATA_INVALID;
        this->call_back_(this, &this->currentTask_);
        return;
      }
    }
    if (this->currentTask_.task_ & DS2480B_TASK_LAST)
    {// report success & results to the generic callback
      this->call_back_(this, &this->currentTask_);
    }
  }
}
