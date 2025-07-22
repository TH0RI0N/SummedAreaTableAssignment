#include "InputParser.h"

#include <fstream>

void InputParser::parse(const std::string& input_file, DataContainer& data_out)
{
	// Hardcoded test input to begin with
	data_out.width = 10;
	data_out.height = 10;

	data_out.data.resize(10 * 10, 1);
}
