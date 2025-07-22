#include "SummedAreaTableGeneratorGpuImpl.h"

/// Reference for the algorithm: https://en.wikipedia.org/wiki/Summed-area_table
void SummedAreaTableGeneratorGpuImpl::generate(const DataContainer& data_in, DataContainer& data_out)
{
	data_out.data.resize(data_in.data.size());
	data_out.height = data_in.height;
	data_out.width = data_in.width;
}
