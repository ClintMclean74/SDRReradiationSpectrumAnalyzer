#define _ITERATOR_DEBUG_LEVEL 0
#include <string>
#include <math.h>
#include <algorithm>
#include "SignalProcessingUtilities.h"
#include "MathUtilities.h"

namespace SignalProcessingUtilities
{	
	double ConvertToMHz(uint32_t frequency, uint8_t decimalPoints)
	{		
		double mhz = (double) frequency / 1000000;

		mhz = MathUtilities::Round(mhz, decimalPoints);

		return mhz;
	}

	IQ GetIQFromData(uint8_t* data, uint32_t index)
	{
		index = index << 1;

		IQ iq;

		iq.I = data[index];
		iq.Q = data[index + 1];

		return iq;
	}

	IQ GetIQFromData(double* data, uint32_t index)
	{
		index << 1;

		IQ iq;

		iq.I = data[index];
		iq.Q = data[index + 1];

		return iq;
	}

	IQ GetIQFromData(fftw_complex* data, uint32_t index)
	{
		IQ iq;

		iq.I = data[index][0];
		iq.Q = data[index][1];

		return iq;
	}

	uint32_t GetFrequencyFromDataIndex(uint32_t dataIndex, uint32_t lowerDataIndex, uint32_t upperDataIndex, uint32_t lowerFrequency, uint32_t upperFrequency)
	{
		uint32_t dataLength = upperDataIndex - lowerDataIndex;

		uint32_t frequencyLength = upperFrequency - lowerFrequency;

		return (double) (dataIndex - lowerDataIndex) / dataLength * frequencyLength + lowerFrequency;
	}

	FrequencyRange GetSelectedFrequencyRangeFromDataRange(uint32_t startDataIndex, uint32_t endDataIndex, uint32_t lowerDataIndex, uint32_t upperDataIndex, uint32_t lowerFrequency, uint32_t upperFrequency)
	{		
		FrequencyRange selectedFrequencyRange(GetFrequencyFromDataIndex(startDataIndex, lowerDataIndex, upperDataIndex, lowerFrequency, upperFrequency), endDataIndex == 0 ? upperFrequency : GetFrequencyFromDataIndex(endDataIndex, lowerDataIndex, upperDataIndex, lowerFrequency, upperFrequency));
		
		return selectedFrequencyRange;
	}	

	void FFT_BYTES(uint8_t *data, fftw_complex *fftData, int samples, bool inverse, bool inputComplex, bool rotate180)
	{
		double *realArray = NULL;
		fftw_complex *complexArray = NULL;
		fftw_complex *complexArrayFFT = NULL;

		long complexArrayLength = 0, complexArrayFFTLength = 0, realArrayLength = 0;

		fftw_plan complexArrayFFTPlan = NULL, realArrayFFTPlan = NULL;

		bool fftMeasured = false, fftMeasuredComplexArray = false;

		unsigned int measure = FFTW_ESTIMATE;

		if (inputComplex)
		{
			complexArray = (fftw_complex *)fftw_malloc(sizeof(fftw_complex) * samples);

			complexArrayLength = samples;
		}
		else
		{
			unsigned int fftSize = (samples * 2 + 1);

			if (realArrayLength < fftSize)
			{
				realArray = (double *)malloc(sizeof(double) * fftSize);
				realArrayLength = fftSize;
			}
		}

		int j = 0;

		if (inputComplex)
		{			
			if (!fftMeasured && measure == FFTW_MEASURE)
			{
				for (int i = 0; i < samples; i++)
				{
					complexArray[j][0] = data[(int)i * 2];
					complexArray[j][1] = data[(int)i * 2 + 1];

					j++;
				}

				complexArrayFFTPlan = fftw_plan_dft_1d(samples,
					complexArray,
					fftData,
					inverse ? FFTW_BACKWARD : FFTW_FORWARD,
					measure);

				fftw_execute(complexArrayFFTPlan);

				fftMeasured = true;
			}
			
			j = 0;

			for (int i = 0; i < samples; i++)
			{
				complexArray[j][0] = (double)data[(int)i * 2] / 255 - 0.5;
				complexArray[j][1] = (double)data[(int)i * 2 + 1] / 255 - 0.5;

				j++;
			}

			inverse = true;

			complexArrayFFTPlan = fftw_plan_dft_1d(samples,
				complexArray,
				fftData,
				inverse ? FFTW_BACKWARD : FFTW_FORWARD,
				measure);

			fftw_execute(complexArrayFFTPlan);

		}
		else
		{
			for (int i = 0; i < samples; i++)
			{
				realArray[j] = data[i];

				j++;
			}

			realArrayFFTPlan = fftw_plan_dft_r2c_1d(samples,
				realArray,
				fftData,
				measure);

			fftw_execute(realArrayFFTPlan);
		}

		if (rotate180)		
		{
			int length2 = samples / 2;
			fftw_complex temp;

			fftData[0][0] = fftData[1][0];
			fftData[0][1] = fftData[1][1];

			for (int i = 0; i < length2; i++)
			{
				temp[0] = fftData[i][0];
				temp[1] = fftData[i][1];

				fftData[i][0] = fftData[i + length2][0];
				fftData[i][1] = fftData[i + length2][1];

				fftData[i + length2][0] = temp[0];
				fftData[i + length2][1] = temp[1];
			}
		}

		/*if (inputComplex)
		{
			delete complexArray;
		}*/

		if (complexArrayFFTPlan != NULL)
		{
			fftw_destroy_plan(complexArrayFFTPlan);

			complexArrayFFTPlan = NULL;
		}

		if (realArrayFFTPlan != NULL)
		{
			fftw_destroy_plan(realArrayFFTPlan);

			realArrayFFTPlan = NULL;
		}
	}

	void FFT_COMPLEX_ARRAY(fftw_complex* data, fftw_complex* fftData, int samples, bool inverse, bool rotate180)
	{
		double *realArray;
		fftw_complex *complexArray;
		fftw_complex *complexArrayFFT;

		long complexArrayLength = 0, complexArrayFFTLength = 0, realArrayLength = 0;

		fftw_plan complexArrayFFTPlan = NULL, realArrayFFTPlan = NULL;

		bool fftMeasured = false, fftMeasuredComplexArray = false;

		int j = 0;
		
		////if (!fftMeasuredComplexArray)
		if (false)
		{
			fftw_complex* dataCopy = new fftw_complex[samples];

			memcpy(dataCopy, data, samples * 2 * sizeof(double));

			complexArrayFFTPlan = fftw_plan_dft_1d(samples,
				dataCopy,
				fftData,
				inverse ? FFTW_BACKWARD : FFTW_FORWARD,
				FFTW_ESTIMATE);

			fftw_execute(complexArrayFFTPlan);

			fftMeasuredComplexArray = true;
		}

		complexArrayFFTPlan = fftw_plan_dft_1d(samples,
			data,
			fftData,
			inverse ? FFTW_BACKWARD : FFTW_FORWARD,
			FFTW_ESTIMATE);

		fftw_execute(complexArrayFFTPlan);
		
		if (rotate180)
		{
			int length2 = samples / 2;
			fftw_complex temp;

			fftData[0][0] = fftData[1][0];
			fftData[0][1] = fftData[1][1];

			for (int i = 0; i < length2; i++)
			{
				temp[0] = fftData[i][0];
				temp[1] = fftData[i][1];

				fftData[i][0] = fftData[i + length2][0];
				fftData[i][1] = fftData[i + length2][1];

				fftData[i + length2][0] = temp[0];
				fftData[i + length2][1] = temp[1];
			}
		}

		if (complexArrayFFTPlan != NULL)
		{
			fftw_destroy_plan(complexArrayFFTPlan);

			complexArrayFFTPlan = NULL;
		}

		if (realArrayFFTPlan != NULL)
		{
			fftw_destroy_plan(realArrayFFTPlan);

			realArrayFFTPlan = NULL;
		}
	}

	uint8_t* CalculateMagnitudesAndPhasesForArray(uint8_t* array, long length)
	{
		double temp, angle;

		double I, Q;

		for (int i = 0; i < length; i+=2)
		{
			temp = array[i];			

			I = temp - 128;
			Q = array[i + 1];
			Q -= 128;

			array[i] = (float)sqrt(I*I + Q*Q);

			angle = (float)(atan2(Q, I));
			
			if (angle < 0)
				angle = MathUtilities::PI * 2 + angle;			

			array[i + 1] = angle / (MathUtilities::PI * 2) * 255;
		}

		return array;
	}

	fftw_complex* CalculateMagnitudesAndPhasesForArray(fftw_complex* array, long length)
	{
		double temp;

		for (int i = 0; i < length; i++)
		{			
			temp = array[i][0];

			array[i][0] = (float) sqrt(array[i][0] * array[i][0] + array[i][1] * array[i][1]);
			
			array[i][1] = (float)(atan2(array[i][1], temp));
		}

		return array;
	}

	uint8_t* SetI(uint8_t* array, long length, uint8_t value)
	{
		for (int i = 0; i < length; i += 2)
		{
			array[i] = value;
		}

		return array;
	}

	uint8_t* SetQ(uint8_t* array, long length, uint8_t value)
	{
		for (int i = 0; i < length; i += 2)
		{
			array[i+1] = value;
		}

		return array;
	}

	fftw_complex* SetI(fftw_complex* array, long length, uint8_t value)
	{
		for (int i = 0; i < length; i ++)
		{
			array[i][0] = value;
		}

		return array;
	}

	fftw_complex* SetQ(fftw_complex* array, long length, uint8_t value)
	{
		for (int i = 0; i < length; i ++)
		{
			array[i][1] = value;
		}

		return array;
	}

	void Rotate(uint8_t *data, long length, double angle)
	{	
		double origX, origY, newX, newY;

		for (int i = 0; i < length; i+=2)
		{		
			origX = data[i];			
			origY = data[i+1];

			origX -= 128;
			origY -= 128;

			newX = origX * cos(angle) - origY * sin(angle);
			newY = origX * sin(angle) + origY * cos(angle);

			newX += 128;
			newY += 128;

			if (newX < 0)
				newX = 0;

			if (newX >= 255)
				newX = 255;

			if (newY < 0)
				newY = 0;

			if (newY >= 255)
				newY = 255;
				

			data[i] = round(newX);
			data[i+1] = round(newY);
		}
	}

	void ConjugateArray(fftw_complex* data, uint32_t length)
	{
		for (unsigned int i = 0; i < length; i++)
		{			
			data[i][1] = -data[i][1];
		}
	}

	void ShiftToZero(fftw_complex* data, uint32_t length)
	{
		for (unsigned int i = 0; i < length; i++)
		{
			data[i][0] -= 128;
			data[i][1] -= 128;
		}
	}

	void ComplexMultiplyArrays(fftw_complex* data1, fftw_complex* data2, fftw_complex* result, uint32_t length)
	{
		for (unsigned int i = 0; i < length; i++)
		{
			result[i][0] = (float)(data1[i][0] * data2[i][0] - data1[i][1] * data2[i][1]);

			result[i][1] = (float)(data1[i][0] * data2[i][1] + data1[i][1] * data2[i][0]);
		}
	}

	uint32_t GetDelay(fftw_complex* data1, fftw_complex* data2, uint32_t samples)
	{
		SignalProcessingUtilities::ConjugateArray(data1, samples);

		return 0;
	}

	uint32_t ClosestIntegerMultiple(long value, long multiplier)
	{
		return floor ((float)value / multiplier) * multiplier;
	}

	double AngleDistance(double angle1, double angle2)
	{		
		double distance1, distance2, resultDistance;

		if (angle1 < angle2)
		{
			distance1 = (angle1 - -MathUtilities::PI) + (MathUtilities::PI - angle2);
			distance2 = angle2 - angle1;

			resultDistance = std::min(distance1, distance2);
		}
		else if (angle2 < angle1)
		{
			distance1 = (angle2 - -MathUtilities::PI) + (MathUtilities::PI - angle1);
			distance2 = angle1 - angle2;

			resultDistance = std::min(distance1, distance2);
		}
		else
			resultDistance = 0;

		return resultDistance;
	}	
}