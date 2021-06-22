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
#include "FFTSpectrumBuffer.h"

typedef FFTSpectrumBuffer* FFTSpectrumBuffersPtr;

class FFTSpectrumBuffers
{
	private:
		unsigned int count;
		FFTSpectrumBuffersPtr* fftSpectrumBuffers;
		FFTSpectrumBuffer* fftDifferenceBuffer;
		void* parent;

	public:
		FrequencyRange frequencyRange;

		FFTSpectrumBuffers(void *parent, uint32_t lower, uint32_t upper, unsigned int fftSpectrumBufferCount, unsigned int deviceCount, uint32_t maxFFTArrayLength = DeviceReceiver::FFT_SEGMENT_SAMPLE_COUNT, uint32_t binFrequencySize = 0);
		uint32_t GetBinCountForFrequencyRange();
		FFTSpectrumBuffer* GetFFTSpectrumBuffer(unsigned int index);
		bool SetFFTInput(uint8_t index, fftw_complex* fftDeviceData, uint8_t* samples, uint32_t sampleCount, unsigned int deviceIndex, FrequencyRange* inputFrequencyRange, bool referenceDevice);
		bool SetFFTInput(uint8_t index, fftw_complex* fftDeviceData, fftw_complex* samples, uint32_t sampleCount, unsigned int deviceIndex, FrequencyRange* inputFrequencyRange, bool referenceDevice);
		bool ProcessFFTInput(unsigned int index, FrequencyRange* inputFrequencyRange, bool useRatios = false, bool usePhase = false);
		uint32_t GetFFTData(double *dataBuffer, unsigned int dataBufferLength, int fftSpectrumBufferIndex, int startFrequency, int endFrequency, char dataMode);
		void CalculateFFTDifferenceBuffer(uint8_t index1, uint8_t index2);

		~FFTSpectrumBuffers();
};
