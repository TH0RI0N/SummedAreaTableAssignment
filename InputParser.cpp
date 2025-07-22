#include "InputParser.h"

#include <fstream>

void InputParser::parse(const std::string& input_file, DataContainer& data_out)
{
	// Hardcoded test input to begin with
	data_out.width = 20;
	data_out.height = 20;

	data_out.data.resize(data_out.width * data_out.height, 2);
}
