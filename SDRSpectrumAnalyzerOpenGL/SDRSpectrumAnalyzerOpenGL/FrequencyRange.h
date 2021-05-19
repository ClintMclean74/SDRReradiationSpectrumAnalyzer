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

	uint8_t flags[4] = { 0, 0, 0, 0 };

	FrequencyRange();
	FrequencyRange(uint32_t lower, uint32_t upper, double strengthValue = 0, uint32_t frames = 1, uint8_t* flags = NULL);	
	int operator ==(FrequencyRange range);
	int operator !=(FrequencyRange range);
	void Set(FrequencyRange *range);
	void Set(uint32_t lower, uint32_t upper);
};