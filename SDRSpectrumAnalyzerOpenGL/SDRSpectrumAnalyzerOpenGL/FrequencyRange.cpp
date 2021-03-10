#include "FrequencyRange.h"

FrequencyRange::FrequencyRange()
{
	lower = 0;
	upper = 0;

	length = 0;

	strength = 0;

	centerFrequency = 0;
}

FrequencyRange::FrequencyRange(uint32_t lower, uint32_t upper, double strengthValue)
{
	Set(lower, upper);

	strength = strengthValue;
}

void FrequencyRange::Set(FrequencyRange *range)
{	
	Set(range->lower, range->upper);
}

void FrequencyRange::Set(uint32_t lower, uint32_t upper)
{
	this->lower = lower;
	this->upper = upper;

	length = (upper - lower);

	centerFrequency = (lower + upper) / 2;
}