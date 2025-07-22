#pragma once

#include <cstdint>
#include <string>

static const int DATA_MAX_SIZE = 2048;
typedef uint8_t data_t;
static const int DATA_MAX_VALUE = 255;
static const int DATA_MAX_STRING_LENGTH = std::to_string(DATA_MAX_VALUE).length();

// Max width is 18*4 = 72 characters. Including the 7 character ellipsis at the
// end of the line brings us to 79, which is just under the standard console size of 80
// Height maximum is the same for symmetry
static const int PRINT_MAX_WIDTH = 18;
static const int PRINT_MAX_HEIGHT = 18;