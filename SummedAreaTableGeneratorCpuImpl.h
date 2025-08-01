#pragma once

#include "SummedAreaTableGenerator.h"

/// Summed area table generator using the CPU
class SummedAreaTableGeneratorCpuImpl : public SummedAreaTableGenerator
{
public:
	virtual float generate(const DataContainer& data_in, DataContainer& data_out) override;
};