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
#include "rtl-sdr.h"
#include "DeviceReceiver.h"
#include "FFTSpectrumBuffers.h"
#include "fftw/fftw3.h"
#include "Graph.h"

typedef DeviceReceiver* DeviceReceiversPtr;

typedef fftw_complex* fftw_complex_ptr;

#ifndef _WIN32
    typedef sem_t* sem_t_ptr;
#endif

class DeviceReceivers
{
	private:
		uint8_t* referenceDataBuffer;
		uint8_t* dataBuffer;

		fftw_complex_ptr* fftBuffers;
		fftw_complex* convolutionFFT;
		fftw_complex* convolution;

		int32_t correlationBufferSamples;
		int32_t correlationBufferLengthZeroPadded;

		uint32_t correlationCount;

		DeviceReceiver* noiseDevice ;

	public:
		DeviceReceiversPtr* deviceReceivers;

		static uint32_t maxCorrelationCount;
		uint8_t count;
		uint8_t initializedDevices;
		FrequencyRange frequencyRange;

		#ifdef _WIN32
            HANDLE startReceivingDataGate;
            HANDLE receiveDataGate1;
            HANDLE receiveDataGate2;
            HANDLE* receiverGates;
            HANDLE* receiversFinished;
            HANDLE fftBytesGate;
            HANDLE fftComplexGate;
        #else
            sem_t startReceivingDataGate;
            sem_t receiveDataGate1;
            sem_t receiveDataGate2;
            sem_t_ptr* receiverGates;
            sem_t_ptr* receiversFinished;
            sem_t fftBytesGate;
            sem_t fftComplexGate;
		#endif // _WIN32


		void* parent;

		Graph* dataGraph;
		Graph* correlationGraph;
		Graph* fftGraphForDevicesBandwidth;
		Graph* combinedFFTGraphForBandwidth;
		Graph* fftGraphStrengthsForDeviceRange;
		Graph* fftAverageGraphForDeviceRange;
		Graph* fftAverageGraphStrengthsForDeviceRange;
		Graph* spectrumRangeGraph;
		Graph* spectrumRangeDifGraph;
		Graph* allSessionsSpectrumRangeGraph;
		Graph* strengthGraph;

		DWORD* receivedTime1;// = new DWORD[DeviceReceiver::MAXRECEIVELOG];
		uint32_t receivedCount1;
		DWORD* receivedTime2;// = new DWORD[DeviceReceiver::MAXRECEIVELOG];
		uint32_t receivedCount2;

		bool synchronizing;
		bool generatingNoise;

		DeviceReceivers(void* parent, long bufferSizeInMilliSeconds, uint32_t sampleRate);
		void InitializeDevices(int* deviceIDs);
		void SetGain(int gain);
		void SetCurrentCenterFrequency(uint32_t centerFrequency);
		void StartReceivingData();
		void GenerateNoise(uint8_t state);
		void Correlate(bool correlate = true);
		void Synchronize();
		bool Correlated();
		uint32_t SynchronizeData(uint8_t* referenceDataBuffer, uint8_t* dataBuffer);
		uint32_t GetDataForDevice(double *dataBuffer, uint8_t deviceIndex);
		uint32_t GetDataForDevice(uint8_t *dataBuffer, uint8_t deviceIndex);
		void ReleaseReceiverGates();
		void TransferDataToFFTSpectrumBuffer(FFTSpectrumBuffers* fftSpectrumBuffers, uint8_t fftSpectrumBufferIndex, bool useRatios = false);
		void GetDeviceCurrentFrequencyRange(uint32_t deviceIndex, uint32_t* startFrequency, uint32_t* endFrequency);

		~DeviceReceivers();
};
