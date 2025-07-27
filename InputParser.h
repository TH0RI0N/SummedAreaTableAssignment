#pragma once

#include "string"
#include "DataContainer.h"

// Parser for program and text file inputs for the summed area table
class InputParser
{
public:
	// Parse the program inputs from the given command line argument list
	static void parse_command_line_arguments(int argument_count, char* arguments[], std::string& input_file_out, std::string& shader_directory_out, bool& print_help_out);

	// Parse the file from input_file into data_out. Can parse text files
	// with numbers separated by any non-number symbol (comma, space, etc.)
	// Will throw a std::runtime_error explaining what went wrong if the
	// parse isn't successful
	static void parse_input_file(const std::string& input_file, DataContainer& data_out);
private:
	// Parse the given token, and empty it. The number will be added to the given data container.
	// The current line width will be updated. Will throw a std::runtime_error explaining what went wrong
	// if the parse isn't successful
	static void parse_token(std::string& token, DataContainer& data, int& current_line_width, int current_line);
};