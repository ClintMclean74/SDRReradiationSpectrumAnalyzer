#pragma once
#include "FrequencyRanges.h"

FrequencyRanges::FrequencyRanges()
{
}

FrequencyRanges::FrequencyRanges(uint32_t size)
{
	maxSize = size;
	count = 0;

	frequencyRanges = new FrequencyRangePtr[maxSize];

	for (int i = 0; i < count; i++)
	{
		frequencyRanges[i] = NULL;
	}
}

uint32_t FrequencyRanges::Add(uint32_t lower, uint32_t upper, double strength, uint32_t frames, bool overwrite)
{
	bool createNew = true;
	for (int i = 0; i < count; i++)
	{
		if (frequencyRanges[i]->lower == lower && frequencyRanges[i]->upper == upper)
		{
			if (overwrite)
				frequencyRanges[i]->strength = strength;
			else
				frequencyRanges[i]->strength += strength;

			frequencyRanges[i]->frames = frames;

			createNew = false;

			break;
		}
	}

	if (createNew)
		frequencyRanges[count++] = new FrequencyRange(lower, upper, strength, frames);

	return count;
}

FrequencyRange* FrequencyRanges::GetFrequencyRangeFromIndex(uint32_t index)
{
	return frequencyRanges[index];
}

void FrequencyRanges::QuickSort()
{
	QuickSortRecursive(0, count-1);
}

void FrequencyRanges::QuickSortRecursive(int32_t startIndex, int32_t endIndex)
{	
	if (endIndex > startIndex)
	{
		uint32_t j;

		j = Partition(startIndex, endIndex);

		QuickSortRecursive(startIndex, j - 1);
		QuickSortRecursive(j + 1, endIndex);
	}
}

uint32_t FrequencyRanges::Partition(int32_t startIndex, int32_t endIndex)
{	
	uint32_t i, j;
	FrequencyRange* temp;

	FrequencyRange* firstFrequencyRange = frequencyRanges[startIndex];

	i = startIndex;
	j = endIndex + 1;

	do
	{
		do
		{
			i++;
		}
		while (i <= endIndex && firstFrequencyRange->strength < frequencyRanges[i]->strength);

		do
		{
			j--;
		}
		while (j > 0 && firstFrequencyRange->strength > frequencyRanges[j]->strength);

		if (i<j)
		{
			temp = frequencyRanges[i];
			frequencyRanges[i] = frequencyRanges[j];
			frequencyRanges[j] = temp;
		}
	} while (i<j);

	frequencyRanges[startIndex] = frequencyRanges[j];
	frequencyRanges[j] = firstFrequencyRange;

	return j;
}

void FrequencyRanges::ProcessFFTSpectrumStrengthDifferenceData(FFTSpectrumBuffer* fftSpectrumBuffer)
{
	double strength;
	uint32_t frames;
	FrequencyRange currentBandwidthRange;

	currentBandwidthRange.Set(fftSpectrumBuffer->frequencyRange->lower, fftSpectrumBuffer->frequencyRange->lower + DeviceReceiver::SAMPLE_RATE);

	do
	{		
		strength = fftSpectrumBuffer->GetStrengthForRange(currentBandwidthRange.lower, currentBandwidthRange.upper, 1, 1, 0);

		frames = fftSpectrumBuffer->GetFrameCountForRange(currentBandwidthRange.lower, currentBandwidthRange.upper);

		Add(currentBandwidthRange.lower, currentBandwidthRange.upper, strength, frames, true);

		currentBandwidthRange.Set(currentBandwidthRange.lower + DeviceReceiver::SAMPLE_RATE, currentBandwidthRange.upper + DeviceReceiver::SAMPLE_RATE);
	} while (currentBandwidthRange.lower < fftSpectrumBuffer->frequencyRange->upper);

	QuickSort();
}

FrequencyRanges::~FrequencyRanges()
{

}
