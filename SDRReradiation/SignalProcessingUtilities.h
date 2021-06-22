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
#include "fftw/fftw3.h"
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
				I;
				Q;
			}

			void Swap()
			{
				double origI = I;

				I = Q;
				Q = origI;
			}
	};

	struct Strengths_ID_Time
	{
		double strength;//;
		uint32_t ID;//;
		uint32_t time;
		FrequencyRange range;
		uint32_t addCount;
	};

	enum DataType { UINT8_T, DOUBLE, FFTW_COMPLEX, STRENGTHS_ID_TIME};

	void ShiftToZero(fftw_complex* data, uint32_t length);
	uint32_t GetFrequencyFromDataIndex(uint32_t dataIndex, uint32_t lowerDataIndex, uint32_t upperDataIndex, uint32_t lowerFrequency, uint32_t upperFrequency);
	uint32_t GetDataIndexFromFrequency(uint32_t frequency, uint32_t lowerFrequency, uint32_t upperFrequency, uint32_t lowerDataIndex, uint32_t upperDataIndex);
	FrequencyRange GetSelectedFrequencyRangeFromDataRange(uint32_t startDataIndex, uint32_t endDataIndex, uint32_t lowerDataIndex, uint32_t upperDataIndex, uint32_t lowerFrequency, uint32_t upperFrequency);
	IQ GetIQFromData(uint8_t* data, uint32_t index);
	IQ GetIQFromData(double* data, uint32_t index);
	IQ GetIQFromData(fftw_complex* data, uint32_t index);
	IQ GetIQFromData(Strengths_ID_Time* data, uint32_t index);
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
