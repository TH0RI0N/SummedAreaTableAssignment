#pragma once

#include "DataContainer.h"

/// A simple interface for a summed area table generator
class SummedAreaTableGenerator
{
public:
	virtual ~SummedAreaTableGenerator() = default; // Virtual destructor needed to destruct inherited classes properly
	virtual void generate(const DataContainer& data_in, DataContainer& data_out) = 0;
};