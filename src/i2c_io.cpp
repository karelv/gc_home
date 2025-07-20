#include "i2c_io.h"
#include "i2c_input_24v.h"
#include "i2c_io_board.h"

I2C_IO_Board *g_i2c_output_boards[MAX_OUTPUT_BOARDS];
I2C_IO_Board *g_i2c_input_boards[MAX_INPUT_BOARDS];


void i2c_io_init() {
    // Initialize the I2C IO boards
    for (int i = 0; i < MAX_OUTPUT_BOARDS; ++i) {
        g_i2c_output_boards[i] = nullptr;
    }
    for (int i = 0; i < MAX_INPUT_BOARDS; ++i) {
        g_i2c_input_boards[i] = nullptr;
    }
}


void i2c_input_boards_add_at(I2C_IO_Board *board, uint8_t index) {
    if (index < MAX_INPUT_BOARDS) {
        g_i2c_input_boards[index] = board;
    } else {
        Serial.printf("Error: Index %d out of bounds for I2C input boards\n", index);
    }
}

void i2c_output_boards_add_at(I2C_IO_Board *board, uint8_t index) {
    if (index < MAX_OUTPUT_BOARDS) {
        g_i2c_output_boards[index] = board;
    } else {
        Serial.printf("Error: Index %d out of bounds for I2C output boards\n", index);
    }
}

