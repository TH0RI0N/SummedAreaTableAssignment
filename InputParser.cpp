#include "InputParser.h"

#include "constants.h"

#include <fstream>
#include <iostream>
#include <ctype.h>
#include <stdexcept>
#include <filesystem>

void InputParser::parse_command_line_arguments(int argument_count, char* arguments[], 
	std::string& input_file_out, std::string& shader_directory_out, bool& print_help_out)
{
	input_file_out = DEFAULT_INPUT_FILE;
	shader_directory_out = DEFAULT_SHADER_DIRECTORY;
	print_help_out = false;

	std::string argument;
	bool next_argument_is_input_file = false;
	bool next_argument_is_shader_directory = false;

	for (int i = 0; i < argument_count; ++i)
	{
		argument = arguments[i];

		if (next_argument_is_input_file)
		{
			input_file_out = argument;
			next_argument_is_input_file = false;
		}
		if (next_argument_is_shader_directory)
		{
			shader_directory_out = argument;
			next_argument_is_shader_directory = false;
		}

		if (argument == "--s" || argument == "-s" || argument == "-shader_dir" || argument == "--shader_dir")
		{
			next_argument_is_shader_directory = true;
		}
		else if (argument == "--f" || argument == "-f" || argument == "-file" || argument == "--file")
		{
			next_argument_is_input_file = true;
		}
		else if (argument == "--h" || argument == "-h" || argument == "-help" || argument == "--help")
		{
			print_help_out = true;
		}
	}
}

void InputParser::parse_input_file(const std::string& input_file, DataContainer& data_out)
{
	if (!std::filesystem::exists(input_file))
	{
		throw std::runtime_error("Could not find input file: " + input_file);
	}

	data_out.data.reserve(INPUT_DATA_MAX_WIDTH * INPUT_DATA_MAX_HEIGHT);

	std::ifstream file(input_file);
	std::string line;
	std::string token = "";

	int current_line = 0;
	int current_line_width = 0;
	int first_line_width = 0;

	while (getline(file, line))
	{
		++current_line;
		current_line_width = 0;

		for (char symbol : line)
		{
			if (isdigit(symbol))
			{
				token += symbol;
			}
			else // Parse the token when encountering a non-number symbol
			{
				parse_token(token, data_out, current_line_width, current_line);
			}
		}

		// Parse the last token of the line if there was no whitespace or other non-number
		// symbols at the end of the line
		parse_token(token, data_out, current_line_width, current_line);

		if (current_line == 1)
		{
			first_line_width = current_line_width;
		}

		if (current_line_width > first_line_width)
		{
			throw std::runtime_error("Line " + std::to_string(current_line) + " has more data than the others!");
		}
		if (current_line_width < first_line_width)
		{
			throw std::runtime_error("Line " + std::to_string(current_line) + " has less data than the others!");
		}
		if (current_line > INPUT_DATA_MAX_HEIGHT)
		{
			throw std::runtime_error("The given input file has too many lines! The maximum is " + std::to_string(INPUT_DATA_MAX_HEIGHT));
		}
	}

	data_out.width = current_line_width;
	data_out.height = current_line;
}

void InputParser::parse_token(std::string& token, DataContainer& data, int& current_line_width, int current_line)
{
	if (token.empty()) // Ignore consecutive non-number symbols
	{
		return;
	}

	int number;

	try
	{
		number = std::stoi(token);
	}
	catch (std::invalid_argument ex)
	{
		throw std::runtime_error("Unknown input " + token + " at line " + std::to_string(current_line));
	}

	if (number > DATA_MAX_VALUE)
	{
		std::cout << "Noncritical error: Number " << token << " clipped to " << DATA_MAX_VALUE
			<< " at line " << current_line << std::endl;
		number = DATA_MAX_VALUE;
	}

	data.data.emplace_back(number);

	token = "";
	++current_line_width;

	if (current_line_width > INPUT_DATA_MAX_WIDTH)
	{
		throw std::runtime_error("Line " + std::to_string(current_line) + " contains too much data! The maximum is " + std::to_string(INPUT_DATA_MAX_WIDTH));
	}
}
