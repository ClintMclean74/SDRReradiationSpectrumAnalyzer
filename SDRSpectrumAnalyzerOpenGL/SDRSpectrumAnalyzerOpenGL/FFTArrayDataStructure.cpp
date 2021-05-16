#include "FFTArrayDataStructure.h"
#include "ArrayUtilities.h"

FFTArrayDataStructure::FFTArrayDataStructure(fftw_complex* deviceFFTDataBufferArray, uint32_t length, DWORD time, FrequencyRange *range, uint32_t ID, bool averageStrengthPhase)
{		
	if (averageStrengthPhase)
	{
		this->fftArray = new fftw_complex[1];

		this->fftArray[0][0] = 0;
		this->fftArray[0][1] = 0;
	}
	else
	{
		this->fftArray = new fftw_complex[length];

		for (int i = 0; i < length; i++)
		{
			this->fftArray[i][0] = 0;
			this->fftArray[i][1] = 0;
		}
	}
		
	this->length = length;
	this->time = time;
	this->ID = ID;

	if (range)
		this->range.Set(range);

	this->addCount = 1;
}