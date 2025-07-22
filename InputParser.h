#pragma once

#include "string"
#include "DataContainer.h"

/// Parser for test inputs from text files for the summed area table
class InputParser
{
public:
	static void parse(const std::string& input_file, DataContainer& data_out);
};