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
#include <algorithm>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include "rtl-sdr.h"
#include "CircularDataBuffer.h"
#include "FrequencyRange.h"
#include "fftw/fftw3.h"
#include "RateCounter.h"

#ifndef _WIN32
    #include <netinet/in.h>
    #include <sys/socket.h>
    #include <semaphore.h>
#endif // _WIN32

#ifndef _WIN32
    typedef int SOCKET;
#endif // _WIN32

class ReceiveData
{
	public:
		void* deviceReceiver;
		uint8_t *buffer;
		uint32_t length;
		uint32_t processingSegment;
};

class DeviceReceiver
{
	private:
		CircularDataBuffer *circularDataBuffer;
		void* receivingDataThreadHandle;
		void* processingDataThreadHandle;
		//HANDLE receivingDataThreadHandle;
		//HANDLE processingDataThreadHandle;
		////pthread_t receivingDataThreadHandle;
		////pthread_t processingDataThreadHandle;
		uint8_t *transferDataBuffer;
		DeviceReceiver** devicesToSendClonedDataTo;
		uint8_t devicesToSendClonedDataToCount;
		DWORD* receivedDuration;// = new DWORD[MAXRECEIVELOG];
		DWORD graphRefreshTime;
		uint32_t receivedCount;
		uint8_t* buffer2;
		fftw_complex* fftBuffer;
		fftw_complex *complexArray;
		fftw_complex *complexArray2;

		fftw_plan complexArrayFFTPlan;
		fftw_plan complexArrayFFTPlan2;
		fftw_plan complexArrayCorrelationFFTPlan;

		uint8_t *dataBuffer;

		float GetSampleByteAtIndex(long index, long length, long currentTime, bool realValue = true);

		bool strengthDetectionSound;
		bool gradientDetectionSound;
		RateCounter soundRateCounter;
		uint32_t soundThresholdCount;;

	public:
		DWORD prevReceivedTime;
		static bool RECEIVING_GNU_DATA;
		static uint32_t MAXRECEIVELOG;
		uint8_t *gnuReceivedBuffer;
		uint8_t *gnuReceivedBufferForProcessing;
		uint32_t gnuReceivedBufferForProcessingLength = 0;
		uint8_t* prevData;
		uint32_t processingSegment;
		uint32_t prevProcessingSegment;

        SOCKET sd;

		unsigned int server_length;						/* Length of server struct */
		struct sockaddr_in server;				/* Information about the server */

		//void* sd_ptr;								/* The socket descriptor */
		//int server_length;						/* Length of server struct */
		//void* server_ptr;				/* Information about the server */


		rtlsdr_dev_t *device;
		uint8_t deviceID;
		static uint32_t SAMPLE_RATE;
		static uint32_t SEGMENT_BANDWIDTH;
		static long RECEIVE_BUFF_LENGTH;
		static long FFT_SEGMENT_BUFF_LENGTH_FOR_SEGMENT_BANDWIDTH;
		static long FFT_BUFF_LENGTH_FOR_DEVICE_BANDWIDTH;
		static long CORRELATION_BUFF_LENGTH;
		static long FFT_SEGMENT_SAMPLE_COUNT;
		short MAX_BUF_LEN_FACTOR;// = 1000;
		short ASYNC_BUF_NUMBER;
		long RTL_READ_FACTOR;// = 122;
		FrequencyRange* frequencyRange;
		bool referenceDevice;
		DWORD receivedDatatartTime;
		int32_t delayShift;
		double phaseShift;

        #ifdef _WIN32
            HANDLE rtlDataAvailableGate;
            HANDLE receiverBufferDataAvailableGate;

            HANDLE setFFTGate1;
            HANDLE setFFTGate2;

            HANDLE receiverFinished;
            HANDLE receiverGate;
        #else
            sem_t rtlDataAvailableGate;
            sem_t receiverBufferDataAvailableGate;

            sem_t setFFTGate1;
            sem_t setFFTGate2;

            sem_t receiverFinished;
            sem_t receiverGate;
		#endif // _WIN32


		void* parent;

		uint8_t *receivedBuffer;
		uint32_t receivedBufferLength;
		uint32_t receivedLength;

		uint8_t *eegReceivedBuffer;
		uint32_t eegReceivedBufferLength;

		uint8_t *receivedBufferPtr;

		bool receivingData;

		DeviceReceiver(void* parent, long bufferSizeInMilliSeconds, uint32_t sampleRate, uint8_t ID);
		int InitializeDeviceReceiver(int dev_index);
		void SetGain(int gain);
		void SetFrequencyRange(FrequencyRange* frequencyRange);
		void CheckScheduledFFTBufferIndex();
		void FFT_BYTES(uint8_t *data, fftw_complex *fftData, int samples, bool inverse, bool inputComplex, bool rotate180 = false, bool synchronizing = false);
		void FFT_COMPLEX_ARRAY(fftw_complex* data, fftw_complex* fftData, int samples, bool inverse, bool rotate180, bool synchronizing = false);
		void GenerateNoise(uint8_t state);
		void WriteReceivedDataToBuffer(uint8_t *data, uint32_t length);
		void ProcessData(uint8_t *data, uint32_t length);
		void ProcessData(fftw_complex *data, uint32_t length);
		void ReceiveData3(uint8_t* bufffer, long length);
		void ReceiveData2(uint8_t* buffer, long length);
		void ReceiveDataFunction(ReceiveData* receiveData);
		void GetTestData(uint8_t* buffer, uint32_t length);
		void ReceiveTestData(uint32_t length);
		int GetDelayAndPhaseShiftedData(uint8_t* dataBuffer, long dataBufferLength, float durationMilliSeconds = -1, int durationBytes = -1, bool async = true);
		void StartReceivingData();
		void StartProcessingData();
		void StopReceivingData();
		void SetDelayShift(double value);
		void SetPhaseShift(double value);
		void AddDeviceToSendClonedDataTo(DeviceReceiver *deviceReceiver);
		void GetDeviceCurrentFrequencyRange(uint32_t* startFrequency, uint32_t* endFrequency);
		float GetSampleAtIndex(long index, long length, long currentTime, bool realValue = true);

		~DeviceReceiver();
};

typedef DeviceReceiver* DeviceReceiverPtr;
