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

#ifdef _DEBUG
 #define _ITERATOR_DEBUG_LEVEL 2
#endif
#pragma once
#include "GNU_Radio_Utilities.h"
#include "DeviceReceivers.h"
#include "FFTSpectrumBuffers.h"
#include "FrequencyRange.h"

enum ReceivingDataBufferSpecifier { CurrentBuffer, AveragedBuffer };

class SpectrumAnalyzer
{
	private:
		////HANDLE scanFrequencyRangeThreadHandle;
		////pthread_t scanFrequencyRangeThreadHandle;

		void* scanFrequencyRangeThreadHandle;

		void (*SequenceFinishedFunction)();
		bool calculateFFTDifferenceBuffer;

		void* playSoundThread;

		GNU_Radio_Utilities gnuUitilities;

	public:
		void(*OnReceiverDataProcessedFunction)();

		uint32_t spectrumBufferSize;
		double* spectrumBuffer;
		DeviceReceivers* deviceReceivers;
		//FFTSpectrumBuffer* fftBandwidthBuffer;
		BandwidthFFTBuffer* fftBandwidthBuffer;
		FFTSpectrumBuffers* fftSpectrumBuffers;
		FrequencyRange maxFrequencyRange;
		FrequencyRange currentScanningFrequencyRange;
		uint32_t requiredFramesPerBandwidthScan;
		uint32_t requiredScanningSequences;
		bool scanning;
		int8_t prevFFTBufferIndex;// = -1;
		int8_t scheduledFFTBufferIndex;// = -1;
		int8_t currentFFTBufferIndex;// = -1;
		bool useRatios;
		bool usePhase;
		bool eegStrength;
		bool sound;

		uint32_t samp_rate;// = 1024000;

		SpectrumAnalyzer();
		uint8_t InitializeSpectrumAnalyzer(uint32_t bufferSizeInMilliSeconds, uint32_t sampleRate, uint32_t minStartFrequency, uint32_t maxEndFrequency);
		uint32_t GetBinCountForFrequencyRange();
		void PlaySound(DWORD frequency, DWORD duration);
		void SetGain(int gain);
		void SetCurrentCenterFrequency(uint32_t centerFrequency);
		void SetRequiredFramesPerBandwidthScan(uint32_t frames);
		void SetRequiredScanningSequences(uint32_t scanningSequences);
		void SetCalculateFFTDifferenceBuffer(bool value);
		void CalculateFFTDifferenceBuffer(uint8_t index1, uint8_t index2);
		void ChangeBufferIndex(int8_t mode);
		void StartReceivingData();
		void Scan();
		void SetSequenceFinishedFunction(void(*func)());
		void SetOnReceiverDataProcessed(void(*func)());
		void LaunchScanningFrequencyRange(FrequencyRange frequencyRange);
		bool SetFFTInput(fftw_complex* fftDeviceData, FrequencyRange* inputFrequencyRange, uint8_t* samples, uint32_t sampleCount, uint8_t deviceID);
		bool SetFFTInput(fftw_complex* fftBuffer, FrequencyRange* inputFrequencyRange, fftw_complex* samples, uint32_t sampleCount, uint8_t deviceID);
		bool ProcessReceiverData(FrequencyRange* inputFrequencyRange, bool useRatios = false);
		FFTSpectrumBuffer* GetFFTSpectrumBuffer(int fftSpectrumBufferIndex);
		uint32_t GetDataForDevice(uint8_t *dataBuffer, uint8_t deviceIndex);
		uint32_t GetDataForDevice(double *dataBuffer, uint8_t deviceIndex);
		uint32_t GetFFTData(double *dataBuffer, unsigned int dataBufferLength, int fftSpectrumBufferIndex, int startFrequency, int endFrequency, ReceivingDataBufferSpecifier receivingDataBufferSpecifier);
		uint32_t GetFFTData(fftw_complex *dataBuffer, unsigned int dataBufferLength, int fftSpectrumBufferIndex, int startFrequency, int endFrequency, ReceivingDataBufferSpecifier receivingDataBufferSpecifier);
		SignalProcessingUtilities::Strengths_ID_Time* GetStrengthForRangeOverTimeFromCurrentBandwidthBuffer(int fftSpectrumBufferIndex, uint32_t startFrequency, uint32_t endFrequency, DWORD* duration, uint32_t* resultLength, DWORD currentTime = 0);
		uint32_t GetFrameCountForRange(uint8_t fftSpectrumBufferIndex, uint32_t startFrequency, uint32_t endFrequency);
		void GetDeviceCurrentFrequencyRange(uint32_t deviceIndex, uint32_t* startFrequency, uint32_t* endFrequency);
		void GetCurrentScanningRange(uint32_t* startFrequency, uint32_t* endFrequency);
		double GetStrengthForRange(uint32_t startFrequency, uint32_t endFrequency, ReceivingDataBufferSpecifier receivingDataBufferSpecifier, int32_t fftSpectrumBufferIndex = -1);
		void Stop();
		~SpectrumAnalyzer();
};
