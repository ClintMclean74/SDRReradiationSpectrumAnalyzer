#include "FFTSpectrumBuffers.h"
#include "FFTSpectrumBuffer.h"

FFTSpectrumBuffers::FFTSpectrumBuffers(void *parent, uint32_t lower, uint32_t upper, unsigned int fftSpectrumBufferCount, unsigned int deviceCount)
{
	this->parent = parent;

	this->frequencyRange.Set(lower, upper);

	count = fftSpectrumBufferCount;

	fftSpectrumBuffers = new FFTSpectrumBuffersPtr[count];

	for (int i = 0; i < count; i++)
		fftSpectrumBuffers[i] = new FFTSpectrumBuffer(parent, &this->frequencyRange, deviceCount);	
}

uint32_t FFTSpectrumBuffers::GetBinCountForFrequencyRange()
{	
	return fftSpectrumBuffers[0]->GetBinCountForFrequencyRange();	
}

FFTSpectrumBuffer* FFTSpectrumBuffers::GetFFTSpectrumBuffer(unsigned int index)
{
	if (index < count)
		return fftSpectrumBuffers[index];
	else
		if (index == count)
			return fftDifferenceBuffer;

	return NULL;
}

bool FFTSpectrumBuffers::SetFFTInput(uint8_t index, fftw_complex* fftDeviceData, uint8_t* samples, uint32_t sampleCount, unsigned int deviceIndex, FrequencyRange* inputFrequencyRange, bool referenceDevice)
{
	return GetFFTSpectrumBuffer(index)->SetFFTInput(fftDeviceData, samples, sampleCount, deviceIndex, inputFrequencyRange, referenceDevice);
}

bool FFTSpectrumBuffers::SetFFTInput(uint8_t index, fftw_complex* fftDeviceData, fftw_complex* samples, uint32_t sampleCount, unsigned int deviceIndex, FrequencyRange* inputFrequencyRange, bool referenceDevice)
{
	return GetFFTSpectrumBuffer(index)->SetFFTInput(fftDeviceData, samples, sampleCount, deviceIndex, inputFrequencyRange, referenceDevice);
}

bool FFTSpectrumBuffers::ProcessFFTInput(unsigned int index, FrequencyRange* inputFrequencyRange, bool useRatios, bool usePhse)
{
	return GetFFTSpectrumBuffer(index)->ProcessFFTInput(inputFrequencyRange, useRatios, usePhse);
}

uint32_t FFTSpectrumBuffers::GetFFTData(double *dataBuffer, unsigned int dataBufferLength, int fftSpectrumBufferIndex, int startFrequency, int endFrequency, char dataMode)
{
	return GetFFTSpectrumBuffer(fftSpectrumBufferIndex)->GetFFTData(dataBuffer, dataBufferLength, startFrequency, endFrequency, dataMode);
}

void FFTSpectrumBuffers::CalculateFFTDifferenceBuffer(uint8_t index1, uint8_t index2)
{
	if (fftDifferenceBuffer == NULL)
	{
		fftDifferenceBuffer = new FFTSpectrumBuffer(parent, &this->frequencyRange, 1);
	}

	fftDifferenceBuffer->CalculateFFTDifferenceBuffer(GetFFTSpectrumBuffer(index1), GetFFTSpectrumBuffer(index2));	
}

FFTSpectrumBuffers::~FFTSpectrumBuffers()
{	
	for (int i = 0; i < count; i++)
		delete fftSpectrumBuffers[i];

	delete[] fftSpectrumBuffers;

	if (fftDifferenceBuffer)
		delete fftDifferenceBuffer;
}