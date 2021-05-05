#pragma once
#include <stdint.h>
#include <windows.h>
#include "SignalProcessingUtilities.h"
#include "fftw3.h"
#include "FFTArrayDataStructure.h"
#include "FrequencyRange.h"

typedef FFTArrayDataStructure* FFTArrayDataStructure_ptr;

class BandwidthFFTBuffer
{
	uint32_t size;

	uint32_t writes = 0;
	uint32_t writeStart = 0;

	FFTArrayDataStructure_ptr* fftArrayDataStructures;

	public:
		static uint32_t FFT_ARRAYS_COUNT;

		FrequencyRange range;
		BandwidthFFTBuffer(uint32_t size);
		BandwidthFFTBuffer(uint32_t lower, uint32_t upper, uint32_t size);
		BandwidthFFTBuffer(FrequencyRange* range, uint32_t size);
		void ConstructFFTArrayDataStructures(uint32_t size);
		uint32_t Add(fftw_complex* fftArray, uint32_t length, DWORD time, FrequencyRange *range, uint32_t ID = 0, uint32_t index = FFT_ARRAYS_COUNT, bool addData = false);
		void Reset();
		void SetNewRange(FrequencyRange *range);
		FFTArrayDataStructure* BandwidthFFTBuffer::GetFFTArrayData(uint32_t index = FFT_ARRAYS_COUNT);
		double GetStrengthForRange(uint32_t startIndex, uint32_t endIndex, uint32_t bufferIndex);
		SignalProcessingUtilities::Strengths_ID_Time* GetStrengthForRangeOverTime(uint32_t startIndex, uint32_t endIndex, DWORD* duration, unsigned int deviceIndex, uint32_t* resultLength, DWORD currentTime = 0);
		//uint32_t CopyDataIntoBuffer(BandwidthFFTBuffer *dstBuffer, DWORD* duration, unsigned int deviceIndex, uint32_t* resultLength, DWORD currentTime = 0, uint32_t startIndex = 0, uint32_t endIndex = 0);
		uint32_t CopyDataIntoBuffer(BandwidthFFTBuffer *dstBuffer, DWORD* duration, unsigned int deviceIndex, uint32_t* resultLength, DWORD currentTime = 0, uint32_t frequencyStartDataIndex = 0, uint32_t frequencyEndDataIndex = 0, bool addData = false);
};