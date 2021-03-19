#define _ITERATOR_DEBUG_LEVEL 0
#include <math.h>
#include <algorithm>
#include "ArrayUtilities.h"
#include "SignalProcessingUtilities.h"

namespace ArrayUtilities
{
	uint8_t* AddArrays(uint8_t* array1, long length, uint8_t* array2)
	{
		for (int i = 0; i < length; i++)
		{
			array2[i] += array1[i];
		}

		return array2;
	}

	uint8_t* SubArrays(uint8_t* array1, long length, uint8_t* array2)
	{
		int16_t value1, value2;

		for (int i = 0; i < length; i++)
		{
			value1 = array1[i];
			value2 = array2[i];

			value1 -= value2;

			value1 += 128;

			if (value1 < 0)
				value1 = 0;

			array1[i] = value1;
		}

		return array1;
	}

	uint8_t* SubArraysMagnitudeAndPhase(uint8_t* array1, long length, uint8_t* array2)
	{
		double angle1, angle2, distance1, distance2, resultDistance;

		for (int i = 0; i < length; i++)
		{		
			if (i % 2 == 0)
			{
				array1[i] = array1[i] - array2[i];
			}
			else
			{
				angle1 = array1[i];
				angle2 = array2[i];

				if (angle1 < angle2)
				{
					distance1 = angle1 + (255 - angle2);
					distance2 = angle2 - angle1;

					resultDistance = std::min(distance1, distance2);
				}
				else if (angle2 < angle1)
				{
					distance1 = angle2 + (255 - angle1);
					distance2 = angle1 - angle2;

					resultDistance = std::min(distance1, distance2);
				}
				else
					resultDistance = 0;

				array1[i] = resultDistance;
			}

		}

		return array1;
	}

	fftw_complex* SubArrays(fftw_complex* array1, long length, fftw_complex* array2)
	{
		for (int i = 0; i < length; i++)
		{			
			array1[i][0] = array1[i][0] - array2[i][0];
			////array1[i][1] = array1[i][1] - array2[i][1];

			array1[i][1] = SignalProcessingUtilities::AngleDistance(array1[i][1], array2[i][1]);
			
			////array1[i][0] = -array1[i][0];
			////array1[i][1] = -array1[i][1];
		}

		return array1;
	}

	fftw_complex* AddArrays(fftw_complex* array1, long length, fftw_complex* array2)
	{
		for (int i = 0; i < length; i++)
		{
			array2[i][0] += array1[i][0];
			array2[i][1] += array1[i][1];
		}

		return array2;
	}

	fftw_complex* AddToArray(fftw_complex* array, double value, uint32_t length, bool useI, bool useQ)
	{
		for (int i = 0; i < length; i++)
		{
			if (useI)
				array[i][0] += value;
			if (useQ)
				array[i][1] += value;
		}

		return array;
	}

	uint32_t* AddToArray(uint32_t* array, uint32_t value, uint32_t length)
	{
		for (int i = 0; i < length; i++)
		{
			array[i] += value;
		}

		return array;
	}

	double* DivideArray(double* array1, long length, double divisor)
	{
		for (int i = 0; i < length; i++)
		{
			array1[i] /= divisor;			
		}

		return array1;
	}


	fftw_complex* DivideArray(fftw_complex* array1, long length, double divisor)
	{
		for (int i = 0; i < length; i++)
		{
			array1[i][0] /= divisor;
			array1[i][1] /= divisor;
		}

		return array1;
	}

	fftw_complex* DivideArray(fftw_complex* array1, long length, fftw_complex* array2)
	{
		for (int i = 0; i < length; i++)
		{
			array1[i][0] /= array2[i][0];
			array1[i][1] /= array2[i][1];
		}

		return array1;
	}


	uint8_t* CopyArray(uint8_t* array1, long length, uint8_t* array2)
	{
		for (int i = 0; i < length; i++)
		{
			array2[i] = array1[i];
		}

		return array2;
	}

	double* CopyArray(uint8_t* array1, long length, double* array2)
	{
		for (int i = 0; i < length; i++)
		{
			array2[i] = array1[i];
		}

		return array2;
	}

	fftw_complex* CopyArray(fftw_complex* array1, long length, fftw_complex* array2)
	{
		////length = 1024;
		for (int i = 0; i < length; i++)
		{
			array2[i][0] = array1[i][0];
			array2[i][1] = array1[i][1];
		}

		return array2;
	}

	double* CopyArray(fftw_complex* array1, long length, double* array2)
	{
		for (int i = 0; i < length; i++)
		{
			array2[i * 2] = array1[i][0];
			array2[i * 2 + 1] = array1[i][1];
		}

		return array2;
	}

	double GetAverage(fftw_complex* dataArray, uint32_t length, bool forRealValue, bool forImaginaryValue)
	{
		double total = 0;

		for (int i = 0; i < length; i++)
		{
			if (forRealValue && forImaginaryValue)
			{
				total += dataArray[i][0]/2 + dataArray[i][1]/2;
			}
			else
			{
				if (!forRealValue && !forImaginaryValue)
				{
					total += (float)sqrt(dataArray[i][0] * dataArray[i][0] + dataArray[i][1] * dataArray[i][1]);
				}
				else if (forRealValue)
					total += dataArray[i][0];
				else if (forImaginaryValue)
					total += dataArray[i][1];
			}			
		}
		
		return total / length;
	}

	fftw_complex* ConvertArrayToComplex(uint8_t* dataArray, uint32_t length)
	{
		uint32_t sampleCount = length / 2;
		fftw_complex* newArray = new fftw_complex[sampleCount];
		for (int i = 0; i < sampleCount; i++)
		{
			newArray[i][0] = dataArray[i*2];
			newArray[i][1] = dataArray[i * 2 + 1];
		}

		return newArray;
	}

	uint8_t* AverageArray(uint8_t* dataArray, uint32_t length, uint32_t segmentCount, bool forRealValue)
	{
		uint32_t sampleCount = length / 2;

		uint32_t segmentsLength = sampleCount / segmentCount;
		uint8_t* segmentAvgs = new uint8_t[segmentCount];
		////uint32_t segmentCount = 0;

		uint32_t start = 0, end = segmentsLength;

		uint32_t avg;



		while (start * 2 < length)
		{
			avg = 0;

			if (forRealValue)
			{
				for (int i = start; i < end; i++)
				{
					avg += dataArray[i*2];
				}

				avg /= (end - start);
			}
			else
			{
				for (int i = start; i < end; i++)
				{
					avg += dataArray[i*2 + 1];
				}

				avg /= (end - start);
			}

			if (forRealValue)
			{
				for (int i = start; i < end; i++)
				{
					dataArray[i*2] = avg;
				}
			}
			else
			{
				for (int i = start; i < end; i++)
				{
					dataArray[i * 2 + i] = avg;
				}
			}

			start += segmentsLength;
			end += segmentsLength;

			if (end * 2 > length)
				end = length / 2;

			////segmentCount++;
		}

		return dataArray;
	}

	fftw_complex* AverageArray(fftw_complex* dataArray, uint32_t length, uint32_t segmentCount, bool forRealValue)
	{		
		uint32_t segmentsLength = length / segmentCount;

		fftw_complex* averagedArray = new fftw_complex[segmentCount];

		uint32_t start = 0, end = segmentsLength, segmentIndex = 0;

		double avg;

		while (start < length)
		{
			avg = 0;

			if (forRealValue)
			{
				for (int i = start; i < end; i++)
				{
					avg += dataArray[i][0];
				}

				avg /= segmentsLength;
			}
			else
			{
				for (int i = start; i < end; i++)
				{
					avg += dataArray[i][1];
				}

				avg /= segmentsLength;
			}

			if (forRealValue)
			{
				averagedArray[segmentIndex][0] = avg;
				/*////for (int i = start; i < end; i++)
				{
					dataArray[i][0] = avg;
				}*/
			}
			else
			{
				averagedArray[segmentIndex][1] = avg;
				/*////for (int i = start; i < end; i++)
				{
					dataArray[i][1] = avg;
				}*/
			}

			start += segmentsLength;
			end += segmentsLength;

			if (end > length)
				end = length;

			segmentIndex++;
		}

		return averagedArray;
	}

	fftw_complex* DecimateArray(fftw_complex* dataArray, uint32_t length, uint32_t newLength)
	{		
		fftw_complex* newArray = new fftw_complex[length];
		
		double inc = length / newLength;

		uint32_t newArrayIndex = 0;
		for (int i = 0; i < length; i+=inc)
		{
			newArray[newArrayIndex][0] = dataArray[i][0];
			newArray[newArrayIndex][1] = dataArray[i][1];

			newArrayIndex++;
		}

		return newArray;
	}

	/*////
	double* GetMinValueAndIndex(fftw_complex* dataArray, uint32_t length, bool forRealValue, bool forImaginaryValue)
	{
		int minIndex = -1;

		double minValue = 0, value = 0;

		for (int i = 0; i < length; i++)
		{
			if (forRealValue && forImaginaryValue)
			{
				value = std::min(dataArray[i][0], dataArray[i][1]);
			}
			else
			{
				if (!forRealValue && !forImaginaryValue)
				{
					value = (float)sqrt(dataArray[i][0] * dataArray[i][0] + dataArray[i][1] * dataArray[i][1]);
				}
				else if (forRealValue)
					value = dataArray[i][0];
				else if (forImaginaryValue)
					value = dataArray[i][1];
			}

			if (i == 0 || value < minValue)
			{
				minIndex = i;
				minValue = value;
			}
		}


		double *result = new double[2];

		result[0] = minIndex;
		result[1] = minValue;

		return result;
	}*/

	double* GetMinValueAndIndex(fftw_complex* dataArray, uint32_t length, bool forRealValue, bool forImaginaryValue, bool absolute)
	{
		int minIndex = -1;

		double minValue = 0, value = 0;

		for (int i = 0; i < length; i++)
		{
			if (forRealValue && forImaginaryValue)
			{
				if (!absolute)
					value = std::min(dataArray[i][0], dataArray[i][1]);
				else
					value = std::min(abs(dataArray[i][0]), abs(dataArray[i][1]));

			}
			else
			{
				if (!forRealValue && !forImaginaryValue)
				{
					value = (float)sqrt(dataArray[i][0] * dataArray[i][0] + dataArray[i][1] * dataArray[i][1]);
				}
				else if (forRealValue)
				{
					if (!absolute)
						value = dataArray[i][0];
					else
						value = abs(dataArray[i][0]);
				}
				else if (forImaginaryValue)
				{
					if (!absolute)
						value = dataArray[i][1];
					else
						value = abs(dataArray[i][1]);
				}
			}

			if (i == 0 || value < minValue)
			{
				minIndex = i;
				minValue = value;
			}
		}

		double *result = new double[2];

		result[0] = minIndex;
		result[1] = minValue;

		return result;
	}

	double* GetMaxValueAndIndex(fftw_complex* dataArray, uint32_t length, bool forRealValue, bool forImaginaryValue, bool absolute)
	{
		int maxIndex = -1;

		double maxValue = 0, value = 0;
		
		for (int i = 0; i < length; i++)
		{
			if (forRealValue && forImaginaryValue)
			{				
				if (!absolute)
					value = std::max(dataArray[i][0], dataArray[i][1]);
				else
					value = std::max(abs(dataArray[i][0]), abs(dataArray[i][1]));

			}
			else
			{
				if (!forRealValue && !forImaginaryValue)
				{					
					value = (float) sqrt(dataArray[i][0] * dataArray[i][0] + dataArray[i][1] * dataArray[i][1]);
				}
				else if (forRealValue)
				{
					if (!absolute)
						value = dataArray[i][0];
					else
						value = abs(dataArray[i][0]);
				}
				else if (forImaginaryValue)
				{
					if (!absolute)
						value = dataArray[i][1];
					else
						value = abs(dataArray[i][1]);
				}
			}

			if (i == 0 || value > maxValue)
			{
				maxIndex = i;
				maxValue = value;
			}
		}

		double *result = new double[2];

		result[0] = maxIndex;		
		result[1] = maxValue;
		
		return result;
	}

	VectorPtr* ResizeArray(VectorPtr* array, uint32_t prevSize, uint32_t newSize)
	{
		VectorPtr* newArray = new VectorPtr[newSize];

		memcpy(newArray, array, prevSize * sizeof(VectorPtr));

		for (int i = prevSize; i < newSize; i++)
		{
			newArray[i] = NULL;
		}

		delete[] array;

		array = newArray;

		return array;
	}


	fftw_complex* ResizeArrayAndData(fftw_complex* array, uint32_t prevSize, uint32_t newSize)
	{
		fftw_complex* newArray = new fftw_complex[newSize];

		uint32_t dstStartIndex = 0, dstEndIndex;

		double inc = newSize / prevSize;

		for (int i = 0; i< prevSize; i++)
		{			
			dstEndIndex = dstStartIndex + inc;

			for (int j = dstStartIndex; j < dstEndIndex; j++)
			{
				newArray[j][0] = array[i][0];
				newArray[j][1] = array[i][1];
			}

			dstStartIndex += inc;
		}
		
		delete[] array;

		array = newArray;

		return array;
	}

	bool InArray(int value, int* array, int length)
	{
		for (int i = 0; i < length; i++)
		{
			if (array[i] == value)
			{
				return true;
			}
		}

		return false;
	}

	void ZeroArray(uint32_t* array, uint32_t startIndex, uint32_t endIndex)
	{
		for (int i = startIndex; i < endIndex; i++)
		{
			array[i] = 0;			
		}
	}

	void ZeroArray(fftw_complex* array, uint32_t startIndex, uint32_t endIndex)
	{
		for (int i = startIndex; i < endIndex; i++)
		{
			array[i][0] = 0;
			array[i][1] = 0;
		}		
	}

}