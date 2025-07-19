#include "i2c_input_24v.h"

I2CInput24V::I2CInput24V() : I2CInput()
{
}

I2CInput24V::~I2CInput24V()
{
    // Cleanup if necessary, currently nothing to clean up
    Serial.printf("I2CInput24V: Destructor called for id %d\n", index32_ / 32);
}


uint8_t
I2CInput24V::config(uint8_t bus, uint8_t id, const char *AD1, const char *AD2)
{
    switch(bus) {
        case 1:
            wire_ = &Wire;  // Wire is the I2C bus for bus 0
            break;
        case 2:
            wire_ = &Wire1; // Wire1 is the I2C bus for bus 1
            break;
        case 3:
            wire_ = &Wire2; // Wire2 is the I2C bus for bus 2
            break;
        default:
            wire_ = &Wire; // Default to Wire if no valid bus is specified
            break;
    }
    // Additional initialization logic can be added here
    uint8_t sa_base = 255;
    if (!strcasecmp(AD2, "gnd")) {
        sa_base = 0x10;
    }
    if (!strcasecmp(AD2, "vdd")) {
        sa_base = 0x14;
    }
    if (!strcasecmp(AD2, "scl")) {
        sa_base = 0x50;
    }
    if (!strcasecmp(AD2, "sda")) {
        sa_base = 0x54;
    }
    if (sa_base == 255) {
        Serial.printf("I2CInput24V: Invalid AD2 configuration '%s'\n", AD2);
        return 1; // Invalid address, do not proceed
    }
    uint8_t ad1_value = 255;
    if (!strcasecmp(AD1, "gnd")) {
        ad1_value = 0x10;
    }
    if (!strcasecmp(AD1, "vdd")) {
        ad1_value = 0x12;
    }
    if (!strcasecmp(AD1, "scl")) {
        ad1_value = 0x00;
    }
    if (!strcasecmp(AD1, "sda")) {
        ad1_value = 0x02;
    }
    if (ad1_value == 255) {
        Serial.printf("I2CInput24V: Invalid AD1 configuration '%s'\n", AD1);
        return 1; // Invalid address, do not proceed
    }
    sa_base += ad1_value;
    this->sa_[0] = sa_base;
    this->sa_[1] = sa_base + 1;
    this->sa_[2] = sa_base + 8;
    this->sa_[3] = sa_base + 9;
    for (int i = 0; i < 4; ++i) 
    {
        if (this->sa_[i] >= 104) this->sa_[i] += 8;
    }
    this->index32_ = id*32;

    return 0; // Success
}


void
I2CInput24V::readInputs() 
{
    // Example: Read inputs from I2C device and store in inputs_
    // Replace with actual I2C read logic
    if (this->wire_) 
    {
        this->inputs_ = 0;
        for (int i = 0; i < 4; ++i) 
        {
            this->wire_->requestFrom(this->sa_[i], uint8_t(1));
            uint32_t read_value = this->wire_->read();
            this->inputs_ |= (read_value << (8 * i));
        }
    }
}

uint8_t I2CInput24V::getInput(uint8_t input_nr) {
    // Return the state of the requested input
    input_nr -= (this->index32_ * 32);
    if (input_nr < 32) {
        return (this->inputs_ >> input_nr) & 0x01;
    }
    return 0;
}

void I2CInput24V::writeOutputs() {
    if (wire_) {
        for (int i = 0; i < 4; ++i) 
        {
            wire_->beginTransmission(this->sa_[i]); // Example address
            wire_->write((outputs_ >> (8 * i)) & 0xFF);
            wire_->endTransmission();
        }
    }
}

void I2CInput24V::setOutput(uint8_t output_nr, uint8_t state) {
    // Set or clear the bit for the requested output
    output_nr -= (this->index32_ * 32);
    if (output_nr < 32) {
        if (state)
            outputs_ |= (1UL << output_nr);
        else
            outputs_ &= ~(1UL << output_nr);
    }
}

uint8_t I2CInput24V::getOutput(uint8_t output_nr) {
    // Return the state of the requested output
    output_nr -= (this->index32_ * 32);
    if (output_nr < 32) {
        return (outputs_ >> output_nr) & 0x01;
    }
    return 0;
}

uint8_t I2CInput24V::checkAlive() {
    
    if (wire_) 
    {
        uint8_t result = 0;
        for (int i = 0; i < 4; ++i) 
        {
            wire_->beginTransmission(this->sa_[i]);
            uint8_t error = wire_->endTransmission();
            if (error != 0) {
                result |= (1 << i); // Device is not present..
            }
        }
        return result;
    }
    return 0xFF;
}
