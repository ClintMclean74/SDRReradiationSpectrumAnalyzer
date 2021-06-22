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
#include "pstdint.h"

#ifdef _WIN32
    #include <windows.h>
#endif

#include "SignalProcessingUtilities.h"
#include "fftw/fftw3.h"
#include "FFTArrayDataStructure.h"
#include "FrequencyRange.h"
#include "DeviceReceiver.h"

typedef FFTArrayDataStructure* FFTArrayDataStructure_ptr;

class BandwidthFFTBuffer
{
	uint32_t size;

	uint32_t writes;
	uint32_t writeStart;

	FFTArrayDataStructure_ptr* fftArrayDataStructures;

	public:
		static uint32_t FFT_ARRAYS_COUNT;

		FrequencyRange range;
		BandwidthFFTBuffer(uint32_t size);
		BandwidthFFTBuffer(uint32_t lower, uint32_t upper, uint32_t size);
		BandwidthFFTBuffer(FrequencyRange* range, uint32_t size);
		~BandwidthFFTBuffer();
		void ConstructFFTArrayDataStructures(uint32_t size);
		void Set(fftw_complex* fftDurationBuffer, uint32_t length, FrequencyRange* range, uint32_t index);
		uint32_t Add(fftw_complex* fftArray, uint32_t length, DWORD time, FrequencyRange *range, uint32_t ID = 0, uint32_t index = FFT_ARRAYS_COUNT, bool addData = false, bool averageStrengthPhase = false, uint32_t maxFFTArrayLength = DeviceReceiver::FFT_SEGMENT_SAMPLE_COUNT);
		void Reset();
		void SetNewRange(FrequencyRange *range);
		FFTArrayDataStructure* GetFFTArrayData(uint32_t index = FFT_ARRAYS_COUNT);
		double GetStrengthForRange(uint32_t startIndex, uint32_t endIndex, uint32_t bufferIndex);
		SignalProcessingUtilities::Strengths_ID_Time* GetStrengthForRangeOverTime(uint32_t startIndex, uint32_t endIndex, DWORD* duration, unsigned int deviceIndex, uint32_t* resultLength, DWORD currentTime = 0);
		//uint32_t CopyDataIntoBuffer(BandwidthFFTBuffer *dstBuffer, DWORD* duration, unsigned int deviceIndex, uint32_t* resultLength, DWORD currentTime = 0, uint32_t startIndex = 0, uint32_t endIndex = 0);
		uint32_t CopyDataIntoBuffer(BandwidthFFTBuffer *dstBuffer, DWORD* duration, unsigned int deviceIndex, uint32_t* resultLength, DWORD currentTime = 0, uint32_t frequencyStartDataIndex = 0, uint32_t frequencyEndDataIndex = 0, bool addData = false, bool averageStrengthPhase = false);
};
