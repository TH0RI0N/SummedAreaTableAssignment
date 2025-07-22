#include "SummedAreaTableGeneratorCpuImpl.h"

/// Reference for the algorithm: https://en.wikipedia.org/wiki/Summed-area_table
void SummedAreaTableGeneratorCpuImpl::generate(const DataContainer& data_in, DataContainer& data_out)
{
	data_out.data.resize(data_in.data.size());
	data_out.height = data_in.height;
	data_out.width = data_in.width;

	for (int y = 0; y < data_in.height; y++)
	{
		for (int x = 0; x < data_in.width; x++)
		{
			int flat_index = y*10 + x;
			data_out.data[flat_index] = data_in.data[flat_index];

			if (x > 0)
			{
				data_out.data[flat_index] += data_out.data[y*10 + (x-1)];
			}
			if (y > 0)
			{
				data_out.data[flat_index] += data_out.data[(y-1)*10 + x];
			}
			if (x > 0 && y > 0)
			{
				data_out.data[flat_index] -= data_out.data[(y - 1) * 10 + (x - 1)];
			}
		}
	}
}
