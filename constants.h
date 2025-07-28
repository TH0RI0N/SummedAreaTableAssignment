#pragma once

#include <cstdint>
#include <cmath>
#include <string>

// The data type used
// NOTE: If changing to 32 bits, overflow isn't currently handled in the compute shaders!
typedef uint8_t data_t;

// Data properties (updated automatically)
static const int DATA_NUM_OF_BITS = sizeof(data_t) * 8;
static const uint64_t DATA_MAX_VALUE = std::pow(2, DATA_NUM_OF_BITS) - 1;
static const int DATA_MAX_STRING_LENGTH = std::to_string(DATA_MAX_VALUE).length();

// Standard console width is 80 symbols. For larger data we will include a 
// 7 character ellipsis at the end of the line. Calculate how much data fits.
// Height maximum is the same for symmetry
static const int PRINT_TARGET_CONSOLE_WIDTH = 80;
static const int PRINT_ELLIPSIS_SIZE = 7;
static const int PRINT_MAX_WIDTH = std::floor((PRINT_TARGET_CONSOLE_WIDTH - PRINT_ELLIPSIS_SIZE)
											/ (float)DATA_MAX_STRING_LENGTH);
static const int PRINT_MAX_HEIGHT = PRINT_MAX_WIDTH;

// Limit the input size
static const int INPUT_DATA_MAX_WIDTH = 2048;
static const int INPUT_DATA_MAX_HEIGHT = 2048;

// Default values when not given command line arguments
static const std::string DEFAULT_INPUT_FILE = "data/square_10_x_10.txt";
static const std::string DEFAULT_SHADER_DIRECTORY = "shaders";