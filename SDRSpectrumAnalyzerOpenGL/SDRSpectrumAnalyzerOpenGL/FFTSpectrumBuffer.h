#pragma once
#include "DeviceReceiver.h"
#include "fftw3.h"
#include "FrequencyRange.h"

typedef uint8_t* uint8_t_ptr;
typedef fftw_complex* fftw_complex_ptr;

class FFTSpectrumBuffer
{
	private:		
		double binFrequencySize;
		uint8_t** deviceDataBuffers;
		fftw_complex** deviceDataBuffers_Complex;
		fftw_complex_ptr* deviceFFTDataBuffers;		
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

		FFTSpectrumBuffer(void *parent, FrequencyRange* frequencyRange, unsigned int deviceBuffersCount);
		uint32_t GetBinCountForFrequencyRange();
		uint32_t GetIndexFromFrequency(uint32_t frequency);
		void ClearDeviceBufferFlags();		
		bool SetFFTInput(fftw_complex* fftDeviceData, uint8_t* samples, uint32_t sampleCount, unsigned int deviceIndex, FrequencyRange* inputFrequencyRange, bool referenceDevice);
		bool SetFFTInput(fftw_complex* fftDeviceData, fftw_complex* samples, uint32_t sampleCount, unsigned int deviceIndex, FrequencyRange* inputFrequencyRange, bool referenceDevice);
		bool ProcessFFTInput(FrequencyRange* inputFrequencyRange, bool useRatios = false, bool usePhase = false);
		uint8_t* GetSampleDataForDevice(int deviceIndex);
		uint32_t GetFFTData(double *dataBuffer, unsigned int dataBufferLength, int startFrequency, int endFrequency, uint8_t dataMode);
		uint32_t GetFFTData(fftw_complex *dataBuffer, unsigned int dataBufferLength, int startFrequency, int endFrequency, uint8_t dataMode);
		double GetStrengthForRange(uint32_t startFrequency, uint32_t endFrequency, uint8_t dataMode, bool useI = true, bool useQ = false, bool useAverage = true);
		uint32_t GetFrameCountForRange(uint32_t startFrequency = 0, uint32_t endFrequency = 0);
		void CalculateFFTDifferenceBuffer(FFTSpectrumBuffer* buffer1, FFTSpectrumBuffer* buffer2);
		void AddToBuffer(double value, FrequencyRange* inputFrequencyRange, uint8_t dataMode);
		void TransferDataToFFTSpectrumBuffer(FFTSpectrumBuffer *fftBuffer);
		void SetTestData();
		void ClearData();

		~FFTSpectrumBuffer();
};
