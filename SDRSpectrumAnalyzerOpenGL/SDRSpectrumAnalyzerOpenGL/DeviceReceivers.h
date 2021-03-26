#pragma once
#include "rtl-sdr.h"
#include "DeviceReceiver.h"
#include "FFTSpectrumBuffers.h"
#include "fftw3.h"
#include "Graph.h"

typedef DeviceReceiver* DeviceReceiversPtr;

typedef fftw_complex* fftw_complex_ptr;

class DeviceReceivers
{
	private:		
		DeviceReceiversPtr* deviceReceivers;

		uint8_t* referenceDataBuffer = NULL;
		uint8_t* dataBuffer = NULL;

		fftw_complex_ptr* fftBuffers = NULL;
		fftw_complex* convolutionFFT;
		fftw_complex* convolution;

		int32_t correlationBufferSamples;
		int32_t correlationBufferLengthZeroPadded;	

		uint32_t correlationCount = 0;

		DeviceReceiver* noiseDevice  = NULL;

	public:
		static uint32_t maxCorrelationCount;
		uint8_t count = 0;
		uint8_t initializedDevices = 0;
		FrequencyRange frequencyRange;		
		static HANDLE startReceivingDataGate;
		HANDLE receiveDataGate1 = NULL;
		HANDLE receiveDataGate2 = NULL;
		HANDLE* receiverGates = NULL;
		HANDLE* receiversFinished = NULL;
		HANDLE fftBytesGate = NULL;
		HANDLE fftComplexGate = NULL;

		void* parent;		

		Graph* dataGraph = NULL;
		Graph* correlationGraph = NULL;
		Graph* fftGraphForDeviceRange = NULL;
		Graph* fftGraphForDevicesRange = NULL;
		Graph* fftGraphStrengthsForDeviceRange = NULL;
		Graph* fftAverageGraphForDeviceRange = NULL;	
		Graph* fftAverageGraphStrengthsForDeviceRange = NULL;		
		Graph* spectrumRangeGraph = NULL;
		Graph* allSessionsSpectrumRangeGraph = NULL;		
		
		DWORD* receivedTime1 = new DWORD[DeviceReceiver::MAXRECEIVELOG];
		uint32_t receivedCount1 = 0;
		DWORD* receivedTime2 = new DWORD[DeviceReceiver::MAXRECEIVELOG];
		uint32_t receivedCount2 = 0;

		bool synchronizing = false;
		bool generatingNoise = false;		

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
