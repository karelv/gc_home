#ifndef I2C_INPUT_24V_H
#define I2C_INPUT_24V_H

#include "i2c_input.h"

class I2CInput24V : public I2CInput {
public:
    I2CInput24V();
    ~I2CInput24V() override;

    uint8_t config(uint8_t bus, uint8_t id, const char *AD1, const char *AD2);

    void readInputs() override;
    uint8_t getInput(uint8_t input_nr);
    void writeOutputs() override;
    void setOutput(uint8_t output_nr, uint8_t state) override;
    uint8_t getOutput(uint8_t output_nr) override;
    uint8_t checkAlive() override;

protected:
    uint32_t outputs_ = 0;
    uint32_t inputs_ = 0;
    uint8_t sa_[4] = {0};
    uint8_t index32_ = 0;
};

#endif // I2C_INPUT_24V_H
