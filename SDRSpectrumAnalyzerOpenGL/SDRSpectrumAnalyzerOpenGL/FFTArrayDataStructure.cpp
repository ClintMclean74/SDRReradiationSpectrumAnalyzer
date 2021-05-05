#include "FFTArrayDataStructure.h"
#include "ArrayUtilities.h"

FFTArrayDataStructure::FFTArrayDataStructure(fftw_complex* deviceFFTDataBufferArray, uint32_t length, DWORD time, FrequencyRange *range, uint32_t ID)
{
	this->fftArray = new fftw_complex[length];
	ArrayUtilities::CopyArray(deviceFFTDataBufferArray, length, this->fftArray);	
		
	this->length = length;
	this->time = time;
	this->ID = ID;
	this->range.Set(range);

	this->addCount = 1;
}