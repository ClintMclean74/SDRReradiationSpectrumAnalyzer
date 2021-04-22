#ifdef _DEBUG 
 #define _ITERATOR_DEBUG_LEVEL 2 
#endif
#pragma once
#include "DeviceReceivers.h"
#include "FFTSpectrumBuffers.h"
#include "FrequencyRange.h"

enum ReceivingDataBufferSpecifier { CurrentBuffer, AveragedBuffer };

class SpectrumAnalyzer
{
	private:		
		HANDLE scanFrequencyRangeThreadHandle = NULL;		
		void (*sequenceFinishedFunction)();
		bool calculateFFTDifferenceBuffer = false;		
		HANDLE playSoundThread = NULL;		

	public:
		uint32_t spectrumBufferSize = 0;
		double* spectrumBuffer = NULL;
		DeviceReceivers* deviceReceivers = NULL;
		FFTSpectrumBuffers* fftSpectrumBuffers = NULL;
		FrequencyRange maxFrequencyRange;
		FrequencyRange currentScanningFrequencyRange;
		uint32_t requiredFramesPerBandwidthScan = 1;
		uint32_t requiredScanningSequences = 1;
		bool scanning = false;
		int8_t scheduledFFTBufferIndex = -1;
		int8_t currentFFTBufferIndex = -1;
		bool useRatios = false;
		bool usePhase = false;
		bool eegStrength = false;
		bool sound = false;

		uint32_t samp_rate = 1024000;

		SpectrumAnalyzer();
		uint8_t InitializeSpectrumAnalyzer(uint32_t bufferSizeInMilliSeconds, uint32_t sampleRate, uint32_t minStartFrequency, uint32_t maxEndFrequency);
		uint32_t GetBinCountForFrequencyRange();
		void PlaySound(DWORD frequency, DWORD duration);
		void SetGain(int gain);
		void SetCurrentCenterFrequency(uint32_t centerFrequency);
		void SetRequiredFramesPerBandwidthScan(uint32_t frames);
		void SetRequiredScanningSequences(uint32_t scanningSequences);
		void SetCalculateFFTDifferenceBuffer(bool value);
		void ChangeBufferIndex(int8_t mode);		
		void StartReceivingData();
		void Scan();
		void SetSequenceFinishedFunction(void(*func)());
		void LaunchScanningFrequencyRange(FrequencyRange frequencyRange);
		bool SetFFTInput(fftw_complex* fftDeviceData, FrequencyRange* inputFrequencyRange, uint8_t* samples, uint32_t sampleCount, uint8_t deviceID);
		bool SetFFTInput(fftw_complex* fftBuffer, FrequencyRange* inputFrequencyRange, fftw_complex* samples, uint32_t sampleCount, uint8_t deviceID);
		bool ProcessReceiverData(FrequencyRange* inputFrequencyRange, bool useRatios = false);
		FFTSpectrumBuffer* GetFFTSpectrumBuffer(int fftSpectrumBufferIndex);
		uint32_t GetDataForDevice(uint8_t *dataBuffer, uint8_t deviceIndex);
		uint32_t GetDataForDevice(double *dataBuffer, uint8_t deviceIndex);
		uint32_t GetFFTData(double *dataBuffer, unsigned int dataBufferLength, int fftSpectrumBufferIndex, int startFrequency, int endFrequency, ReceivingDataBufferSpecifier receivingDataBufferSpecifier);
		uint32_t GetFFTData(fftw_complex *dataBuffer, unsigned int dataBufferLength, int fftSpectrumBufferIndex, int startFrequency, int endFrequency, ReceivingDataBufferSpecifier receivingDataBufferSpecifier);		
		uint32_t GetFrameCountForRange(uint8_t fftSpectrumBufferIndex, uint32_t startFrequency, uint32_t endFrequency);
		void GetDeviceCurrentFrequencyRange(uint32_t deviceIndex, uint32_t* startFrequency, uint32_t* endFrequency);
		void GetCurrentScanningRange(uint32_t* startFrequency, uint32_t* endFrequency);
		double GetStrengthForRange(uint32_t startFrequency, uint32_t endFrequency, ReceivingDataBufferSpecifier receivingDataBufferSpecifier, int32_t fftSpectrumBufferIndex = -1);
		void Stop();
		~SpectrumAnalyzer();
};
