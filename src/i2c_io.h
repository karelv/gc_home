#ifndef __I2C_IO_H__
#define __I2C_IO_H__    

#include "i2c_io_board.h"

#define MAX_OUTPUT_BOARDS 8
#define MAX_INPUT_BOARDS 8



void i2c_io_init();
void i2c_input_boards_add_at(I2C_IO_Board *board, uint8_t index);
void i2c_output_boards_add_at(I2C_IO_Board *board, uint8_t index);

#endif // __I2C_IO_H__