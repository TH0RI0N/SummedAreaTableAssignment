#include "DataContainer.h"
#include "InputParser.h"
#include "SummedAreaTableGenerator.h"
#include "SummedAreaTableGeneratorCpuImpl.h"
#include "SummedAreaTableGeneratorGpuImpl.h"
#include "constants.h"
#include "DirectXHelper.h"

#include <iostream>
#include <chrono>
#include <stdexcept>

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

			std::cout << token;
		}
		if (width_limited) 
		{
			std::cout << ".  .  ."; // Add indication of hidden data
		}
		std::cout << std::endl;
	}

	if (height_limited)
	{
		// Add 3 columns of dots for indicating hidden data
		for (int y = 0; y < 3; y++)
		{
			for (int x = 0; x < width; x++)
			{
				std::cout << " .  ";
			}
			std::cout << std::endl;
		}
	}
}

int main()
{
	try
	{
		// Read and print the input data
		DataContainer input_data;
		InputParser::parse("ones_10_x_10.txt", input_data);

		std::cout << "Input (" << input_data.width << " x " << input_data.height << "): " << std::endl;
		print_data(input_data);
		std::cout << std::endl;

		// Generate and print the summed area table on the CPU
		DataContainer cpu_output_data;
		SummedAreaTableGeneratorCpuImpl cpu_generator;
		int time = cpu_generator.generate(input_data, cpu_output_data);
		std::cout << "CPU Output (generated in " << time << " microseconds): " << std::endl;
		print_data(cpu_output_data);

		// Generate and print the summed area table on the GPU
		DataContainer gpu_output_data;
		SummedAreaTableGeneratorGpuImpl gpu_generator;
		time = gpu_generator.generate(input_data, gpu_output_data);
		std::cout << "GPU Output (generated in " << time << " microseconds): " << std::endl;
		print_data(gpu_output_data);
	}
	catch (std::runtime_error e)
	{
		std::cout << "Error: " << e.what();
		return -1;
	}
	
	return 0;
}