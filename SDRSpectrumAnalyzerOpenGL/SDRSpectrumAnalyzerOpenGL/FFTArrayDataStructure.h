#pragma once
#include <stdint.h>
#include <windows.h>
#include "fftw3.h"
#include "FrequencyRange.h"

class FFTArrayDataStructure
{
	public:
		uint32_t length;
		fftw_complex* fftArray;
		uint32_t ID = 0;
		FrequencyRange range;
		DWORD time;
		uint32_t addCount = 0;
	
		FFTArrayDataStructure(fftw_complex* deviceFFTDataBufferArray, uint32_t length, DWORD time, FrequencyRange *range, uint32_t ID = 0, bool averageStrengthPhase = false);
};