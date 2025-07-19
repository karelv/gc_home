#ifndef I2C_INPUT_H
#define I2C_INPUT_H

#include <Wire.h>
#include <stdint.h>

class I2CInput {
public:
    I2CInput() = default;
    virtual ~I2CInput() = default;

    virtual void setWire(TwoWire* wire) { wire_ = wire; }
    virtual void readInputs() = 0;
    virtual uint8_t getInput(uint8_t input_nr) = 0;
    virtual void writeOutputs() = 0;
    virtual void setOutput(uint8_t output_nr, uint8_t state) = 0;
    virtual uint8_t getOutput(uint8_t output_nr) = 0;
    virtual uint8_t checkAlive() = 0;

protected:
    TwoWire* wire_ = nullptr;
};

#endif // I2C_INPUT_H
