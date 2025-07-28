#include "SummedAreaTableGeneratorCpuImpl.h"

#include <algorithm>
#include <chrono>

#include "constants.h"

/// Reference for the algorithm: https://en.wikipedia.org/wiki/Summed-area_table
float SummedAreaTableGeneratorCpuImpl::generate(const DataContainer& data_in, DataContainer& data_out)
{
	data_out.data.resize(data_in.data.size());
	data_out.height = data_in.height;
	data_out.width = data_in.width;

	auto start = std::chrono::high_resolution_clock::now();

	for (int y = 0; y < data_in.height; ++y)
	{
		for (int x = 0; x < data_in.width; ++x)
		{
			uint64_t output_value = data_in.data[y * data_in.width + x];

			if (x > 0)
			{
				output_value += data_out.data[y*data_in.width + (x-1)];
			}
			if (y > 0)
			{
				output_value += data_out.data[(y-1)*data_in.width + x];
			}
			if (x > 0 && y > 0)
			{
				output_value -= data_out.data[(y - 1)*data_in.width + (x - 1)];
			}

			data_out.data[y * data_in.width + x] = std::min(output_value, DATA_MAX_VALUE);
		}
	}

	auto end = std::chrono::high_resolution_clock::now();
	return std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() / 1000.0f;
}
