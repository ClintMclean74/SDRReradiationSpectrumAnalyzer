#pragma once
#include <stdint.h>
#include "fftw3.h"
#include "FrequencyRange.h"

namespace SignalProcessingUtilities
{		
	struct IQ
	{
		public:
			double I;
			double Q;

			IQ()
			{
				I = 0;
				Q = 0;
			}

			void Swap()
			{
				double origI = I;
				
				I = Q;
				Q = origI;
			}
	};

	enum DataType { UINT8_T, DOUBLE, FFTW_COMPLEX };

	void ShiftToZero(fftw_complex* data, uint32_t length);
	uint32_t GetFrequencyFromDataIndex(uint32_t dataIndex, uint32_t lowerDataIndex, uint32_t upperDataIndex, uint32_t lowerFrequency, uint32_t upperFrequency);
	FrequencyRange GetSelectedFrequencyRangeFromDataRange(uint32_t startDataIndex, uint32_t endDataIndex, uint32_t lowerDataIndex, uint32_t upperDataIndex, uint32_t lowerFrequency, uint32_t upperFrequency);
	IQ GetIQFromData(uint8_t* data, uint32_t index);
	IQ GetIQFromData(double* data, uint32_t index);
	IQ GetIQFromData(fftw_complex* data, uint32_t index);
	double ConvertToMHz(uint32_t frequency, uint8_t decimalPoints = 2);
	void FFT_BYTES(uint8_t *data, fftw_complex *fftData, int samples, bool inverse = false, bool inputComplex = true, bool rotate180 = false);
	void FFT_COMPLEX_ARRAY(fftw_complex* data, fftw_complex* fftData, int samples, bool inverse = false, bool rotate180 = false);
	uint8_t* CalculateMagnitudesAndPhasesForArray(uint8_t* array, long length);
	uint8_t* SetI(uint8_t* array, long length, uint8_t value);
	uint8_t* SetQ(uint8_t* array, long length, uint8_t value);
	fftw_complex* SetI(fftw_complex* array, long length, uint8_t value);
	fftw_complex* SetQ(fftw_complex* array, long length, uint8_t value);
	fftw_complex* CalculateMagnitudesAndPhasesForArray(fftw_complex* array, long length);
	void Rotate(uint8_t *data, long length, double angle);
	void ConjugateArray(fftw_complex* data, uint32_t length);
	void ComplexMultiplyArrays(fftw_complex* data1, fftw_complex* data2, fftw_complex* result, uint32_t length);
	uint32_t ClosestIntegerMultiple(long value, long multiplier);
	double AngleDistance(double angle1, double angle2);	
}