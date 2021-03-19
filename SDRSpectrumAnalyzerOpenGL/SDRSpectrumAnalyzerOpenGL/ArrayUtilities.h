#pragma once
#include <stdint.h>
#include "fftw3.h"
#include "Vector.h"

namespace ArrayUtilities
{
	uint8_t* AddArrays(uint8_t* array1, long length, uint8_t* array2);
	uint8_t* SubArrays(uint8_t* array1, long length, uint8_t* array2);
	fftw_complex* SubArrays(fftw_complex* array1, long length, fftw_complex* array2);
	uint8_t* SubArraysMagnitudeAndPhase(uint8_t* array1, long length, uint8_t* array2);
	fftw_complex* AddArrays(fftw_complex* array1, long length, fftw_complex* array2);
	fftw_complex* AddToArray(fftw_complex* array, double value, uint32_t length, bool useI, bool useQ);
	uint32_t* AddToArray(uint32_t* array, uint32_t value, uint32_t length);

	double* DivideArray(double* array1, long length, double divisor);
	fftw_complex* DivideArray(fftw_complex* array1, long length, double divisor);

	fftw_complex* DivideArray(fftw_complex* array1, long length, fftw_complex* array2);

	uint8_t* CopyArray(uint8_t* array1, long arrayLength1, uint8_t* array2);

	double* CopyArray(uint8_t* array1, long length, double* array2);

	fftw_complex* CopyArray(fftw_complex* array1, long length, fftw_complex* array2);

	double* CopyArray(fftw_complex* array1, long length, double* array2);

	double GetAverage(fftw_complex* dataArray, uint32_t length, bool forRealValue, bool forImaginaryValue);

	fftw_complex* ConvertArrayToComplex(uint8_t* dataArray, uint32_t length);
	uint8_t* AverageArray(uint8_t* dataArray, uint32_t length, uint32_t segmentCount, bool forRealValue = true);
	fftw_complex* AverageArray(fftw_complex* dataArray, uint32_t length, uint32_t segmentCount, bool forRealValue = true);

	fftw_complex* DecimateArray(fftw_complex* dataArray, uint32_t length, uint32_t newLength);

	double* GetMinValueAndIndex(fftw_complex* dataArray, uint32_t length, bool forRealValue = true, bool forImaginaryValue = true, bool absolute = false);
	double* GetMaxValueAndIndex(fftw_complex* dataArray, uint32_t length, bool forRealValue = true, bool forImaginaryValue = true, bool absolute = false);

	VectorPtr* ResizeArray(VectorPtr* array, uint32_t prevSize, uint32_t newSize);
	fftw_complex* ResizeArrayAndData(fftw_complex* array, uint32_t prevSize, uint32_t newSize);

	bool InArray(int value, int* array, int length);

	void ZeroArray(uint32_t* array, uint32_t startIndex, uint32_t endIndex);

	void ZeroArray(fftw_complex* array, uint32_t startIndex, uint32_t endIndex);
}