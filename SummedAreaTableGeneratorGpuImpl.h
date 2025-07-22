#pragma once

#include "SummedAreaTableGenerator.h"

/// Summed area table generator using the GPU
class SummedAreaTableGeneratorGpuImpl : public SummedAreaTableGenerator
{
public:
	virtual void generate(const DataContainer& data_in, DataContainer& data_out) override;
};