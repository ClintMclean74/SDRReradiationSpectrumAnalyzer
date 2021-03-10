#pragma once
#include <stdint.h>

class FrequencyRange
{
public:
	uint32_t lower;
	uint32_t upper;
	uint32_t length;
	uint32_t centerFrequency;
	double strength;

	FrequencyRange();
	FrequencyRange(uint32_t lower, uint32_t upper, double strengthValue = 0);
	void Set(FrequencyRange *range);
	void Set(uint32_t lower, uint32_t upper);

};