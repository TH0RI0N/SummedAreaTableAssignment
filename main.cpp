#include "DataContainer.h"
#include "InputParser.h"
#include "SummedAreaTableGenerator.h"
#include "SummedAreaTableGeneratorCpuImpl.h"

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
	std::string token;
	for (int y = 0; y < data.height; y++)
	{
		for (int x = 0; x < data.width; x++)
		{
			token = std::to_string(data.data[y * 10 + x]);

			/// Pad with spaces to align rows, since the maximum value is 255.
			/// Adding the spaces one by one is inefficient, but it doesn't really matter here
			while (token.length() < 4)
			{
				token += " ";
			}

			cout << token;
		}
		cout << endl;
	}
}

int main()
{
	DataContainer input_data;
	InputParser::parse("ones_10_x_10.txt", input_data);

	cout << "Input data: " << endl;
	print_data(input_data);
	cout << endl;

	DataContainer output_data;
	SummedAreaTableGeneratorCpuImpl generator;
	int time = generate_summed_area_table(generator, input_data, output_data);

	cout << "Output data (generated in " << time << "ms): " << endl;
	print_data(output_data);
	cout << endl;

	return 0;
}