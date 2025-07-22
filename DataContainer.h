#pragma once

#include <cstdint>
#include <vector>

// Simple container for input and output data for the summed area table
struct DataContainer
{
	int width;
	int height;
	std::vector<uint8_t> data;
};