#pragma once
#include "FFTSpectrumBuffer.h"

typedef FFTSpectrumBuffer* FFTSpectrumBuffersPtr;

class FFTSpectrumBuffers
{
	private:		
		unsigned int count;
		FFTSpectrumBuffersPtr* fftSpectrumBuffers;
		FFTSpectrumBuffer* fftDifferenceBuffer = NULL;
		void* parent;

	public:
		FrequencyRange frequencyRange;

		FFTSpectrumBuffers(void *parent, uint32_t lower, uint32_t upper, unsigned int fftSpectrumBufferCount, unsigned int deviceCount, uint32_t maxFFTArrayLength = DeviceReceiver::FFT_SEGMENT_SAMPLE_COUNT, uint32_t binFrequencySize = 0);
		//void SetMaxFFTArrayLength(uint32_t length);
		//void SetBinFrequencySize(uint32_t size);
		uint32_t GetBinCountForFrequencyRange();
		FFTSpectrumBuffer* GetFFTSpectrumBuffer(unsigned int index);
		bool SetFFTInput(uint8_t index, fftw_complex* fftDeviceData, uint8_t* samples, uint32_t sampleCount, unsigned int deviceIndex, FrequencyRange* inputFrequencyRange, bool referenceDevice);
		bool SetFFTInput(uint8_t index, fftw_complex* fftDeviceData, fftw_complex* samples, uint32_t sampleCount, unsigned int deviceIndex, FrequencyRange* inputFrequencyRange, bool referenceDevice);
		bool ProcessFFTInput(unsigned int index, FrequencyRange* inputFrequencyRange, bool useRatios = false, bool usePhase = false);
		uint32_t GetFFTData(double *dataBuffer, unsigned int dataBufferLength, int fftSpectrumBufferIndex, int startFrequency, int endFrequency, char dataMode);
		void CalculateFFTDifferenceBuffer(uint8_t index1, uint8_t index2);

		~FFTSpectrumBuffers();
};
