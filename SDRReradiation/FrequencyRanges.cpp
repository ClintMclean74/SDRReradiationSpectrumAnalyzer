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

uint32_t FrequencyRanges::Add(uint32_t lower, uint32_t upper, double strength, uint32_t frames, bool overwrite, uint8_t* flags)
{
	bool createNew = true;
	for (int i = 0; i < count; i++)
	{
		if (frequencyRanges[i]->lower == lower && frequencyRanges[i]->upper == upper)
		{
			if (overwrite)
			{
				frequencyRanges[i]->strength = strength;
				frequencyRanges[i]->frames = frames;
			}
			else
			{
				frequencyRanges[i]->strength += strength;
				frequencyRanges[i]->frames += frames;
			}

			if (flags)
			{
				for (int j = 0; j < 4; j++)
				{
					frequencyRanges[i]->flags[j] = flags[j];
				}
			}

			createNew = false;

			break;
		}
	}

	if (createNew)
		frequencyRanges[count++] = new FrequencyRange(lower, upper, strength, frames, flags);

	return count;
}

FrequencyRange* FrequencyRanges::GetFrequencyRangeFromIndex(uint32_t index)
{
	return frequencyRanges[index];
}

FrequencyRange* FrequencyRanges::GetFrequencyRange(uint32_t lower, uint32_t upper)
{
	for (int i = 0; i < count; i++)
	{
		if (frequencyRanges[i]->lower == lower && frequencyRanges[i]->upper == upper)
		{
			return frequencyRanges[i];
		}
	}

	return NULL;
}

void FrequencyRanges::QuickSort(QuickSortMode mode)
{
	QuickSortRecursive(0, count-1, mode);
}

void FrequencyRanges::QuickSortRecursive(int32_t startIndex, int32_t endIndex, QuickSortMode mode)
{
	if (endIndex > startIndex)
	{
		uint32_t j;

		switch (mode)
		{
			case(Strenth):
				j = StrengthPartition(startIndex, endIndex);
			break;
			case(Frequency):
				j = FrequencyPartition(startIndex, endIndex);
			break;
			case(Frames):
				j = FramesPartition(startIndex, endIndex);
			break;
		}

		QuickSortRecursive(startIndex, j - 1, mode);
		QuickSortRecursive(j + 1, endIndex, mode);
	}
}

uint32_t FrequencyRanges::FrequencyPartition(int32_t startIndex, int32_t endIndex)
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
		} while (i <= endIndex && firstFrequencyRange->centerFrequency > frequencyRanges[i]->centerFrequency);

		do
		{
			j--;
		} while (j > 0 && firstFrequencyRange->centerFrequency < frequencyRanges[j]->centerFrequency);

		if (i < j)
		{
			temp = frequencyRanges[i];
			frequencyRanges[i] = frequencyRanges[j];
			frequencyRanges[j] = temp;
		}
	} while (i < j);

	frequencyRanges[startIndex] = frequencyRanges[j];
	frequencyRanges[j] = firstFrequencyRange;

	return j;
}

uint32_t FrequencyRanges::StrengthPartition(int32_t startIndex, int32_t endIndex)
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

uint32_t FrequencyRanges::FramesPartition(int32_t startIndex, int32_t endIndex)
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
		} while (i <= endIndex && firstFrequencyRange->frames < frequencyRanges[i]->frames);

		do
		{
			j--;
		} while (j > 0 && firstFrequencyRange->frames > frequencyRanges[j]->frames);

		if (i < j)
		{
			temp = frequencyRanges[i];
			frequencyRanges[i] = frequencyRanges[j];
			frequencyRanges[j] = temp;
		}
	} while (i < j);

	frequencyRanges[startIndex] = frequencyRanges[j];
	frequencyRanges[j] = firstFrequencyRange;

	return j;
}

void FrequencyRanges::GetStrengthValues(double *values)
{
	for (int i = 0; i < count; i++)
	{
		values[i] = frequencyRanges[i]->strength;
	}
}

void FrequencyRanges::GetStrengthValuesAndFrames(double *values)
{
	for (int i = 0; i < count; i++)
	{
		values[i * 2] = frequencyRanges[i]->strength;
		values[i * 2 + 1] = frequencyRanges[i]->frames;
	}
}

void FrequencyRanges::ProcessFFTSpectrumStrengthDifferenceData(FFTSpectrumBuffer* fftSpectrumBuffer)
{
	double strength;
	uint32_t frames;
	FrequencyRange currentBandwidthRange;
	FrequencyRange segmentOfBandwidthRange;

	currentBandwidthRange.Set(fftSpectrumBuffer->frequencyRange->lower, fftSpectrumBuffer->frequencyRange->lower + DeviceReceiver::SAMPLE_RATE);

	if (currentBandwidthRange.length > DeviceReceiver::SEGMENT_BANDWIDTH)
		segmentOfBandwidthRange.Set(fftSpectrumBuffer->frequencyRange->lower, fftSpectrumBuffer->frequencyRange->lower + DeviceReceiver::SEGMENT_BANDWIDTH);
	else
		segmentOfBandwidthRange = currentBandwidthRange;

	do
	{
		strength = fftSpectrumBuffer->GetStrengthForRange(segmentOfBandwidthRange.lower, segmentOfBandwidthRange.upper, 1, 1, 0, false); //doesn't use average because strength value is already averaged (/frame count) frames != 1 because we also use the number of frames here

		frames = fftSpectrumBuffer->GetFrameCountForRange(segmentOfBandwidthRange.lower, segmentOfBandwidthRange.upper);

		Add(segmentOfBandwidthRange.lower, segmentOfBandwidthRange.upper, strength, frames, true);

		segmentOfBandwidthRange.Set(segmentOfBandwidthRange.lower + DeviceReceiver::SEGMENT_BANDWIDTH, segmentOfBandwidthRange.upper + DeviceReceiver::SEGMENT_BANDWIDTH);
	} while (segmentOfBandwidthRange.lower < fftSpectrumBuffer->frequencyRange->upper);

	QuickSort();
}

void FrequencyRanges::AverageStrengthValuesForFrames()
{
	for (int i = 0; i < count; i++)
	{
		frequencyRanges[i]->strength /= frequencyRanges[i]->frames;
	}
}

FrequencyRanges::~FrequencyRanges()
{

}
