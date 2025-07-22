#pragma once

#include "constants.h"
#include <vector>

// Simple container for input and output data for the summed area table
struct DataContainer
{
	int width;
	int height;
	// A flat vector is for ease of use in this demo. In real use case we would probably only be operating on textures
	std::vector<data_t> data;
};