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
#include "DeviceReceiver.h"
#include "fftw/fftw3.h"
#include "FrequencyRange.h"
#include "BandwidthFFTBuffer.h"

typedef uint8_t* uint8_t_ptr;
typedef fftw_complex* fftw_complex_ptr;
typedef BandwidthFFTBuffer* BandwidthFFTBuffer_ptr;


class FFTSpectrumBuffer
{
	private:
		uint8_t** deviceDataBuffers;
		fftw_complex** deviceDataBuffers_Complex;
		//fftw_complex_ptr* deviceFFTDataBuffers;

		char *fftDeviceDataBufferFlags;
		fftw_complex* mostRecentFrameBuffer;
		fftw_complex* totalFrameBuffer;
		uint32_t *totalFrameCountForBins;

		unsigned int deviceBuffersCount;
		unsigned int referenceDeviceIndex;

		uint32_t binsForEntireFrequencyRange;
		void *parent;

	public:
		FrequencyRange* frequencyRange;
		BandwidthFFTBuffer_ptr *deviceFFTDataBuffers;

		uint32_t maxFFTArrayLength;// = DeviceReceiver::FFT_SEGMENT_SAMPLE_COUNT;
		double binFrequencySize;

		FFTSpectrumBuffer(void *parent, FrequencyRange* frequencyRange, unsigned int deviceBuffersCount, uint32_t maxFFTArrayLength = DeviceReceiver::FFT_SEGMENT_SAMPLE_COUNT, uint32_t binFrequencySize = 0);
		uint32_t GetBinCountForFrequencyRange();
		uint32_t GetIndexFromFrequency(uint32_t frequency);
		void ClearDeviceBufferFlags();
		bool SetFFTInput(fftw_complex* fftDeviceData, uint8_t* samples, uint32_t sampleCount, unsigned int deviceIndex, FrequencyRange* inputFrequencyRange, bool referenceDevice, uint32_t ID = 0);
		bool SetFFTInput(fftw_complex* fftDeviceData, fftw_complex* samples, uint32_t sampleCount, unsigned int deviceIndex, FrequencyRange* inputFrequencyRange, bool referenceDevice, uint32_t ID = 0);
		bool ProcessFFTInput(FrequencyRange* inputFrequencyRange, bool useRatios = false, bool usePhase = false);
		uint8_t* GetSampleDataForDevice(int deviceIndex);
		uint32_t GetFFTData(double *dataBuffer, unsigned int dataBufferLength, int startFrequency, int endFrequency, uint8_t dataMode);
		uint32_t GetFFTData(fftw_complex *dataBuffer, unsigned int dataBufferLength, int startFrequency, int endFrequency, uint8_t dataMode);
		double GetStrengthForRange(uint32_t startFrequency, uint32_t endFrequency, uint8_t dataMode, bool useI = true, bool useQ = false, bool useAverage = true);
		SignalProcessingUtilities::Strengths_ID_Time* GetStrengthForRangeOverTimeFromCurrentBandwidthBuffer(uint32_t startIndex, uint32_t endIndex, DWORD* duration, unsigned int deviceIndex, uint32_t* resultLength, DWORD currentTime = 0);
		uint32_t GetFrameCountForRange(uint32_t startFrequency = 0, uint32_t endFrequency = 0);
		void CalculateFFTDifferenceBuffer(FFTSpectrumBuffer* buffer1, FFTSpectrumBuffer* buffer2);
		void AddToBuffer(double value, FrequencyRange* inputFrequencyRange, uint8_t dataMode);
		void TransferDataToFFTSpectrumBuffer(FFTSpectrumBuffer *fftBuffer);
		void SetTestData();
		void ClearData();

		~FFTSpectrumBuffer();
};
