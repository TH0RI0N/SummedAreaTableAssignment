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

	// Add an empty row after the data to make it look cleaner
	std::cout << std::endl;
}

// Compare the CPU and GPU data and check that they match, and
// print statistics
void compare_data(DataContainer& cpu_data, DataContainer& gpu_data, float cpu_time, float gpu_time)
{
	size_t data_size = cpu_data.data.size();

	if (gpu_data.data.size() != data_size)
	{
		std::cout << "GPU and CPU output data size doesn't match!" << std::endl;
		return;
	}

	for (size_t i = 0; i < data_size; i++)
	{
		if (cpu_data.data[i] != gpu_data.data[i])
		{
			std::cout << "GPU and CPU output data doesn't match!" << std::endl;
			return;
		}
	}

	std::cout << "GPU and CPU output data matches!" << std::endl;

	float speed_ratio = cpu_time / gpu_time;
	if (speed_ratio > 1)
	{
		std::cout << "GPU generation was " << speed_ratio << "x faster!" << std::endl;
	}
	else
	{
		std::cout << "CPU generation was " << 1.0f / speed_ratio << "x faster!" << std::endl;
	}
}

int main()
{
	try
	{
		// Read and print the input data
		DataContainer input_data;
		InputParser::parse("ones_10_x_10.txt", input_data);

		std::cout.precision(3);

		std::cout << "Input (" << input_data.width << " x " << input_data.height << "): " << std::endl;
		print_data(input_data);

		// Generate and print the summed area table on the CPU
		DataContainer cpu_output_data;
		SummedAreaTableGeneratorCpuImpl cpu_generator;
		float cpu_time = cpu_generator.generate(input_data, cpu_output_data);
		std::cout << "CPU Output (generated in " << cpu_time << "ms): " << std::endl;
		print_data(cpu_output_data);

		// Generate and print the summed area table on the GPU
		DataContainer gpu_output_data;
		SummedAreaTableGeneratorGpuImpl gpu_generator;
		float gpu_time = gpu_generator.generate(input_data, gpu_output_data);
		std::cout << "GPU Output (generated in " << gpu_time << "ms): " << std::endl;
		print_data(gpu_output_data);

		compare_data(cpu_output_data, gpu_output_data, cpu_time, gpu_time);
	}
	catch (std::runtime_error e)
	{
		std::cout << "Error: " << e.what();
		return -1;
	}
	
	return 0;
}