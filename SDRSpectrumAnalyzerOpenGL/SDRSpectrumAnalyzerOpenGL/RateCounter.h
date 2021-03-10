#pragma once
#include <stdint.h>

class RateCounter
{
	uint32_t count = 0;
	static const uint32_t maxValues = 1000;

	double values[maxValues];
	uint32_t timestamps[maxValues];

	uint32_t currentIndex = 0;

	public:
		RateCounter();
		uint32_t CountPerSecond();
		double Add();
		double Add(double value);
};
