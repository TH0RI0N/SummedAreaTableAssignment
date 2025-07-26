#pragma once

#include "DataContainer.h"

/// A simple interface for a summed area table generator
class SummedAreaTableGenerator
{
public:
	virtual ~SummedAreaTableGenerator() = default; // Virtual destructor needed to destruct inherited classes properly

	// Generate a summed area table of data_in to data_out. 
	// Returns the elapsed time in microseconds for more accurate benchmarking for just the algorithm 
	virtual int generate(const DataContainer& data_in, DataContainer& data_out) = 0;
};