#include <time.h>
#include <math.h>
#include <windows.h>
#include "RateCounter.h"

RateCounter::RateCounter()
{
	memset(&timestamps, 0, maxValues * sizeof(uint32_t));
}

double RateCounter::Add()
{			
	timestamps[currentIndex] = GetTickCount();				

	count++;
	currentIndex++;

	if (currentIndex >= maxValues)
		currentIndex = 0;

	return CountPerSecond();
}

double RateCounter::Add(double value)
{
	values[currentIndex] = value;
	timestamps[currentIndex] = GetTickCount();

	count++;
	currentIndex++;

	if (currentIndex >= maxValues)
		currentIndex = 0;

	return CountPerSecond();
}

uint32_t RateCounter::CountPerSecond()
{
	double delay;
	
	int32_t mostRecentIndex = currentIndex - 1;
	if (mostRecentIndex < 0)
		mostRecentIndex = maxValues - 1;

	int32_t i = mostRecentIndex - 1;
	if (i < 0)
		i = maxValues - 1;

	double countsPerSecond = 1;
	
	double totalValue = values[mostRecentIndex];

	do
	{
		delay = timestamps[mostRecentIndex] - timestamps[i];
		totalValue += values[i];

		countsPerSecond++;

		i--;
		if (i < 0)
			i = maxValues - 1;

	} while (delay < 1000 && countsPerSecond < maxValues);
		

	totalValue = 1000 / delay * totalValue;
	
	return round(totalValue);
}
