/*
 * SDRReradiation - Code system to detect biologically reradiated
 * electromagnetic energy from humans
 * Copyright (C) 2021 by Clint Mclean <clint@getfitnowapps.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

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
		uint32_t Add(uint32_t lower, uint32_t upper, double strength = 0, uint32_t frames = 1, bool overwrite = false, uint8_t* flags = NULL);
		FrequencyRange* GetFrequencyRangeFromIndex(uint32_t index);
		FrequencyRange* GetFrequencyRange(uint32_t lower, uint32_t upper);
		void QuickSort(QuickSortMode mode = Strenth);
		void ProcessFFTSpectrumStrengthDifferenceData(FFTSpectrumBuffer* fftSpectrumBuffer);
		void AverageStrengthValuesForFrames();
		void GetStrengthValues(double *values);
		void GetStrengthValuesAndFrames(double *values);

		~FrequencyRanges();
};
