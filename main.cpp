#include "DataContainer.h"
#include "InputParser.h"
#include "SummedAreaTableGenerator.h"
#include "SummedAreaTableGeneratorCpuImpl.h"
#include "constants.h"

#include <iostream>
#include <chrono>

using namespace std;

int generate_summed_area_table(SummedAreaTableGenerator& generator, const DataContainer& data_in, DataContainer& data_out)
{
	auto start = std::chrono::high_resolution_clock::now();
	generator.generate(data_in, data_out);
	auto end = std::chrono::high_resolution_clock::now();
	return std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
}

void print_data(DataContainer& data)
{
	int width = data.width;
	int height = data.height;

	bool width_limited = width > PRINT_MAX_WIDTH;
	bool height_limited = height > PRINT_MAX_HEIGHT;

	if (width_limited)
	{
		width = PRINT_MAX_WIDTH;
	}
	if (height_limited)
	{
		height = PRINT_MAX_HEIGHT;
	}

	std::string token;
	for (int y = 0; y < height; y++)
	{
		for (int x = 0; x < width; x++)
		{
			token = std::to_string(data.data[y*data.width + x]);

			// Pad with spaces to separate data and align rows. 
			// Adding the spaces one by one is inefficient, but it doesn't really matter here
			while (token.length() <= DATA_MAX_STRING_LENGTH)
			{
				token += " ";
			}

			cout << token;
		}
		if (width_limited) 
		{
			cout << ".  .  ."; // Add indication of hidden data
		}
		cout << endl;
	}

	if (height_limited)
	{
		// Add 3 columns of dots for indicating hidden data
		for (int y = 0; y < 3; y++)
		{
			for (int x = 0; x < width; x++)
			{
				cout << " .  ";
			}
			cout << endl;
		}
	}
}

int main()
{
	DataContainer input_data;
	InputParser::parse("ones_10_x_10.txt", input_data);

	cout << "Input (" << input_data.width << " x " << input_data.height << "): " << endl;
	print_data(input_data);
	cout << endl;

	DataContainer output_data;
	SummedAreaTableGeneratorCpuImpl generator;
	int time = generate_summed_area_table(generator, input_data, output_data);

	cout << "CPU Output (generated in " << time << "ms): " << endl;
	print_data(output_data);
	cout << endl;

	return 0;
}