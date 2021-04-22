#pragma once
#include "FrequencyRange.h"
#include "FFTSpectrumBuffer.h"

typedef FrequencyRange* FrequencyRangePtr;

enum QuickSortMode {Frequency, Strenth, Frames};

class FrequencyRanges
{
	private:
		FrequencyRangePtr* frequencyRanges;
		uint32_t maxSize;				
		void QuickSortRecursive(int32_t startIndex, int32_t endIndex, QuickSortMode mode);
		uint32_t FrequencyPartition(int32_t startIndex, int32_t endIndex);
		uint32_t StrengthPartition(int32_t startIndex, int32_t endIndex);
		uint32_t FramesPartition(int32_t startIndex, int32_t endIndex);

	public:
		uint32_t count;
		FrequencyRanges();
		FrequencyRanges(uint32_t size);
		uint32_t Add(uint32_t lower, uint32_t upper, double strength = 0, uint32_t frames = 1, bool overwrite = false);
		FrequencyRange* GetFrequencyRangeFromIndex(uint32_t index);
		void QuickSort(QuickSortMode mode = QuickSortMode::Strenth);
		void ProcessFFTSpectrumStrengthDifferenceData(FFTSpectrumBuffer* fftSpectrumBuffer);
		void AverageStrengthValuesForFrames();
		void GetStrengthValues(double *values);
		void GetStrengthValuesAndFrames(double *values);
		
		~FrequencyRanges();
};