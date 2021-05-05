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
	uint32_t frames;

	FrequencyRange();
	FrequencyRange(uint32_t lower, uint32_t upper, double strengthValue = 0, uint32_t frames = 1);	
	int operator ==(FrequencyRange range);
	int operator !=(FrequencyRange range);
	void Set(FrequencyRange *range);
	void Set(uint32_t lower, uint32_t upper);
};