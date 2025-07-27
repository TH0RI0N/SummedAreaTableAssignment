#pragma once

#include "string"
#include "DataContainer.h"

/// Parser for test inputs from text files for the summed area table
class InputParser
{
public:
	// Parse the file from input_file into data_out. Can parse text files
	// with numbers separated by any non-number symbol (comma, space, etc.)
	// Will throw a std::runtime_error explaining what went wrong if the
	// parse isn't successful
	static void parse(const std::string& input_file, DataContainer& data_out);
private:
	// Parse the given token, and empty it. The number will be added to the given data container.
	// The current line width will be updated. Will throw a std::runtime_error explaining what went wrong
	// if the parse isn't successful
	static void parse_token(std::string& token, DataContainer& data, int& current_line_width, int current_line);
};