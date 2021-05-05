#include "FFTSpectrumBuffer.h"
#include "ArrayUtilities.h"
#include "SignalProcessingUtilities.h"
#include "DebuggingUtilities.h"
#include "SpectrumAnalyzer.h"

FFTSpectrumBuffer::FFTSpectrumBuffer(void *parent, FrequencyRange* frequencyRange, unsigned int deviceBuffersCount)
{
	this->parent = parent;

	this->frequencyRange = frequencyRange;

	binFrequencySize = (double) DeviceReceiver::SAMPLE_RATE / (DeviceReceiver::FFT_SEGMENT_BUFF_LENGTH / 2);

	this->deviceBuffersCount = deviceBuffersCount;

	deviceDataBuffers = new uint8_t_ptr[this->deviceBuffersCount];
	deviceDataBuffers_Complex = new fftw_complex_ptr[this->deviceBuffersCount];
	//deviceFFTDataBuffers = new fftw_complex_ptr[this->deviceBuffersCount];
	deviceFFTDataBuffers = new BandwidthFFTBuffer_ptr[this->deviceBuffersCount];

	
	fftDeviceDataBufferFlags = new char[this->deviceBuffersCount];

	////long binsForEntireFrequencyRange = DeviceReceiver::FFT_SEGMENT_SAMPLE_COUNT * (this->frequencyRange->length / DeviceReceiver::SAMPLE_RATE);
	uint32_t binsForCurrentFrequencyRange = DeviceReceiver::FFT_SEGMENT_SAMPLE_COUNT;

	for (int i = 0; i < this->deviceBuffersCount; i++)
	{
		deviceDataBuffers[i] = new uint8_t[DeviceReceiver::FFT_SEGMENT_BUFF_LENGTH];	

		deviceDataBuffers_Complex[i] = new fftw_complex[DeviceReceiver::FFT_SEGMENT_SAMPLE_COUNT];
		//deviceFFTDataBuffers[i] = new fftw_complex[DeviceReceiver::FFT_SEGMENT_SAMPLE_COUNT];

		deviceFFTDataBuffers[i] = new BandwidthFFTBuffer(0, 0, BandwidthFFTBuffer::FFT_ARRAYS_COUNT);
	}


	binsForEntireFrequencyRange = DeviceReceiver::FFT_SEGMENT_SAMPLE_COUNT * (this->frequencyRange->length / (double) DeviceReceiver::SAMPLE_RATE);

	if (binsForEntireFrequencyRange < DeviceReceiver::FFT_SEGMENT_SAMPLE_COUNT)
		binsForEntireFrequencyRange = DeviceReceiver::FFT_SEGMENT_SAMPLE_COUNT;

	//binsForEntireFrequencyRange = 8192;

	mostRecentFrameBuffer = new fftw_complex[binsForEntireFrequencyRange];

	ArrayUtilities::ZeroArray(mostRecentFrameBuffer, 0, binsForEntireFrequencyRange);

	totalFrameBuffer = new fftw_complex[binsForEntireFrequencyRange];

	ArrayUtilities::ZeroArray(totalFrameBuffer, 0, binsForEntireFrequencyRange);

	totalFrameCountForBins = new uint32_t[binsForEntireFrequencyRange];

	ArrayUtilities::ZeroArray(totalFrameCountForBins, 0, binsForEntireFrequencyRange);
}

uint32_t FFTSpectrumBuffer::GetBinCountForFrequencyRange()
{
	return binsForEntireFrequencyRange;
}

uint32_t FFTSpectrumBuffer::GetIndexFromFrequency(uint32_t frequency)
{
	return (frequency - frequencyRange->lower) / binFrequencySize;
}

void FFTSpectrumBuffer::ClearDeviceBufferFlags()
{
	for (int i = 0; i < deviceBuffersCount; i++)
		fftDeviceDataBufferFlags[i] = 1;
}

bool FFTSpectrumBuffer::SetFFTInput(fftw_complex* fftDeviceData, uint8_t* samples, uint32_t sampleCount, unsigned int deviceIndex, FrequencyRange* inputFrequencyRange, bool referenceDevice, uint32_t ID)
{
	if (deviceIndex < deviceBuffersCount)
	{		
		ArrayUtilities::CopyArray(samples, sampleCount*2, deviceDataBuffers[deviceIndex]);

		if (*inputFrequencyRange != deviceFFTDataBuffers[deviceIndex]->range)
		{
			deviceFFTDataBuffers[deviceIndex]->SetNewRange(inputFrequencyRange);
		}

		//ArrayUtilities::CopyArray(fftDeviceData, DeviceReceiver::FFT_SEGMENT_SAMPLE_COUNT, deviceFFTDataBuffers[deviceIndex]);
		deviceFFTDataBuffers[deviceIndex]->Add(fftDeviceData, DeviceReceiver::FFT_SEGMENT_SAMPLE_COUNT, GetTickCount(), inputFrequencyRange, ID);

		fftDeviceDataBufferFlags[deviceIndex] = 1;
	
		if (referenceDevice)
			referenceDeviceIndex = deviceIndex;

		return true;
	}

	return false;
}

bool FFTSpectrumBuffer::SetFFTInput(fftw_complex* fftDeviceData, fftw_complex* samples, uint32_t sampleCount, unsigned int deviceIndex, FrequencyRange* inputFrequencyRange, bool referenceDevice, uint32_t ID)
{
	if (deviceIndex < deviceBuffersCount)
	{
		ArrayUtilities::CopyArray(samples, sampleCount, deviceDataBuffers_Complex[deviceIndex]);
		
		//ArrayUtilities::CopyArray(fftDeviceData, sampleCount, deviceFFTDataBuffers[deviceIndex]);
		deviceFFTDataBuffers[deviceIndex]->Add(fftDeviceData, DeviceReceiver::FFT_SEGMENT_SAMPLE_COUNT, GetTickCount(), inputFrequencyRange, ID);

		fftDeviceDataBufferFlags[deviceIndex] = 1;

		if (referenceDevice)
			referenceDeviceIndex = deviceIndex;

		return true;
	}

	return false;
}

bool FFTSpectrumBuffer::ProcessFFTInput(FrequencyRange* inputFrequencyRange, bool useRatios, bool usePhase)
{
	uint32_t startDataIndex = GetIndexFromFrequency(inputFrequencyRange->lower);

	ArrayUtilities::ZeroArray(mostRecentFrameBuffer, startDataIndex, startDataIndex + DeviceReceiver::FFT_SEGMENT_SAMPLE_COUNT);

	int deviceBuffersSetCount = 0;

	for (int i = 0; i < this->deviceBuffersCount; i++)
	{		
		if (((!useRatios && !usePhase) || i != referenceDeviceIndex) && fftDeviceDataBufferFlags[i] > 0)
		{
			//ArrayUtilities::AddArrays(deviceFFTDataBuffers[i], DeviceReceiver::FFT_SEGMENT_SAMPLE_COUNT, &mostRecentFrameBuffer[startDataIndex]);
			
			FFTArrayDataStructure* fftArrayLengthTimeStructure = deviceFFTDataBuffers[i]->GetFFTArrayData();
			ArrayUtilities::AddArrays(fftArrayLengthTimeStructure->fftArray, fftArrayLengthTimeStructure->length, &mostRecentFrameBuffer[startDataIndex]);

			deviceBuffersSetCount++;
		}		
	}

	if (deviceBuffersSetCount>1)
		ArrayUtilities::DivideArray(&mostRecentFrameBuffer[startDataIndex], DeviceReceiver::FFT_SEGMENT_SAMPLE_COUNT, deviceBuffersSetCount);

	if (useRatios && fftDeviceDataBufferFlags[referenceDeviceIndex] > 0)
	{		
		//ArrayUtilities::DivideArray(&mostRecentFrameBuffer[startDataIndex], DeviceReceiver::FFT_SEGMENT_SAMPLE_COUNT, deviceFFTDataBuffers[0]);		

		FFTArrayDataStructure* fftArrayLengthTimeStructure = deviceFFTDataBuffers[0]->GetFFTArrayData();
		ArrayUtilities::DivideArray(&mostRecentFrameBuffer[startDataIndex], fftArrayLengthTimeStructure->length, fftArrayLengthTimeStructure->fftArray);
	}
	else
	if (usePhase && fftDeviceDataBufferFlags[referenceDeviceIndex] > 0)
	{
		//ArrayUtilities::SubArrays(&mostRecentFrameBuffer[startDataIndex], DeviceReceiver::FFT_SEGMENT_SAMPLE_COUNT, deviceFFTDataBuffers[0]);

		FFTArrayDataStructure* fftArrayLengthTimeStructure = deviceFFTDataBuffers[0]->GetFFTArrayData();
		ArrayUtilities::SubArrays(&mostRecentFrameBuffer[startDataIndex], fftArrayLengthTimeStructure->length, fftArrayLengthTimeStructure->fftArray);
	}

	ArrayUtilities::AddArrays(&mostRecentFrameBuffer[startDataIndex], DeviceReceiver::FFT_SEGMENT_SAMPLE_COUNT, &totalFrameBuffer[startDataIndex]);

	ArrayUtilities::AddToArray(&totalFrameCountForBins[startDataIndex], 1, DeviceReceiver::FFT_SEGMENT_SAMPLE_COUNT);

	return true;
}

uint8_t* FFTSpectrumBuffer::GetSampleDataForDevice(int deviceIndex)
{
	if (deviceIndex < this->deviceBuffersCount)
		return deviceDataBuffers[deviceIndex];
	else
		return NULL;
}

uint32_t FFTSpectrumBuffer::GetFFTData(double *dataBuffer, unsigned int dataBufferLength, int startFrequency, int endFrequency, uint8_t dataMode)
{	
	if (startFrequency == 0)
	{
		startFrequency = frequencyRange->lower;
	}

	if (endFrequency == 0)
	{
		endFrequency = frequencyRange->upper;
	}

	uint32_t startDataIndex = GetIndexFromFrequency(startFrequency);
	uint32_t endIndex = GetIndexFromFrequency(endFrequency);

	uint32_t length = endIndex - startDataIndex;

	uint32_t complexLength = length * 2;

	uint32_t lengthBytes = complexLength * sizeof(double);

	if (dataMode == 0)
	{
		memcpy(dataBuffer, &mostRecentFrameBuffer[startDataIndex], lengthBytes);
	}
	else
	{
		memcpy(dataBuffer, &totalFrameBuffer[startDataIndex], lengthBytes);

		int j = 0;

		for (int i = startDataIndex; i < endIndex; i++)
		{
			if (totalFrameCountForBins[i]!=0)
			{
				dataBuffer[j++] /= totalFrameCountForBins[i];
				dataBuffer[j++] /= totalFrameCountForBins[i];
			}
			else
			{
				j += 2;
			}
		}		
	}

	/*for (int i = 0; i < complexLength; i++)
	{
		if (dataBuffer[i] != 0)
			return complexLength;
	}*/
	
	return complexLength;
}

uint32_t FFTSpectrumBuffer::GetFFTData(fftw_complex *dataBuffer, unsigned int dataBufferLength, int startFrequency, int endFrequency, uint8_t dataMode)
{
	if (startFrequency == 0)
	{
		startFrequency = frequencyRange->lower;
	}

	if (endFrequency == 0)
	{
		endFrequency = frequencyRange->upper;
	}

	uint32_t startDataIndex = GetIndexFromFrequency(startFrequency);
	uint32_t endIndex = GetIndexFromFrequency(endFrequency);

	uint32_t length = endIndex - startDataIndex;

	uint32_t lengthBytes = length * 2 * sizeof(double);

	if (dataMode == 0)
	{
		memcpy(dataBuffer, &mostRecentFrameBuffer[startDataIndex], lengthBytes);
	}
	else
	{
		memcpy(dataBuffer, &totalFrameBuffer[startDataIndex], lengthBytes);

		int j = 0;

		for (int i = startDataIndex; i < endIndex; i++)
		{
			if (totalFrameCountForBins[i] != 0)
			{
				dataBuffer[j][0] /= totalFrameCountForBins[i];
				dataBuffer[j][1] /= totalFrameCountForBins[i];
			}
			
			j++;
		}
	}

	return length * 2;
}

double FFTSpectrumBuffer::GetStrengthForRange(uint32_t startFrequency, uint32_t endFrequency, uint8_t dataMode, bool useI, bool useQ, bool useAverage)
{
	if (startFrequency == 0)
	{
		startFrequency = frequencyRange->lower;
	}

	if (endFrequency == 0)
	{
		endFrequency = frequencyRange->upper;
	}

	if (!useI && !useQ)
		useI = true;

	uint32_t startDataIndex = GetIndexFromFrequency(startFrequency);
	uint32_t endIndex = GetIndexFromFrequency(endFrequency);

	uint32_t length = endIndex - startDataIndex;

	double totalStrength = 0;

	double avgTotalStrengthForIndex;

	if (dataMode == 0)
	{
		for (int i = startDataIndex; i < endIndex; i++)
		{						
			totalStrength += mostRecentFrameBuffer[i][useI ? 0 : 1];
		}
	}
	else
	{
		for (int i = startDataIndex; i < endIndex; i++)
		{
			if (totalFrameCountForBins[i] != 0)
			{
				if (useAverage)
					avgTotalStrengthForIndex = totalFrameBuffer[i][useI ? 0 : 1] / totalFrameCountForBins[i];
				else
					avgTotalStrengthForIndex = totalFrameBuffer[i][useI ? 0 : 1];
			}
			else
				avgTotalStrengthForIndex = 0;

			totalStrength += avgTotalStrengthForIndex;
		}
	}

	return totalStrength / length;
}

SignalProcessingUtilities::Strengths_ID_Time* FFTSpectrumBuffer::GetStrengthForRangeOverTimeFromCurrentBandwidthBuffer(uint32_t startIndex, uint32_t endIndex, DWORD* duration, unsigned int deviceIndex, uint32_t* resultLength, DWORD currentTime)
{
	return deviceFFTDataBuffers[deviceIndex]->GetStrengthForRangeOverTime(startIndex, endIndex, duration, deviceIndex, resultLength, currentTime);
}

uint32_t FFTSpectrumBuffer::GetFrameCountForRange(uint32_t startFrequency, uint32_t endFrequency)
{
	if (startFrequency == 0)
	{
		startFrequency = frequencyRange->lower;
	}

	if (endFrequency == 0)
	{
		endFrequency = frequencyRange->upper;
	}

	uint32_t startDataIndex = GetIndexFromFrequency(startFrequency);
	uint32_t endIndex = GetIndexFromFrequency(endFrequency);

	uint32_t length = endIndex - startDataIndex;

	uint32_t totalFrameCount = 0;

	for (int i = startDataIndex; i < endIndex; i++)
	{
		totalFrameCount += totalFrameCountForBins[i];
	}

	return totalFrameCount / length;
}

void FFTSpectrumBuffer::CalculateFFTDifferenceBuffer(FFTSpectrumBuffer* buffer1, FFTSpectrumBuffer* buffer2)
{	
	for (int i = 0; i < buffer1->binsForEntireFrequencyRange; i++)
	{
		mostRecentFrameBuffer[i][0] = buffer1->mostRecentFrameBuffer[i][0] - buffer2->mostRecentFrameBuffer[i][0];

		if (mostRecentFrameBuffer[i][0] < 0)
			mostRecentFrameBuffer[i][0] = 0;		

		if (buffer1->totalFrameCountForBins[i] > 0 && buffer2->totalFrameCountForBins[i] > 0)
		{
			totalFrameBuffer[i][0] = (buffer1->totalFrameBuffer[i][0] / buffer1->totalFrameCountForBins[i] - buffer2->totalFrameBuffer[i][0] / buffer2->totalFrameCountForBins[i]);
			totalFrameBuffer[i][1] = (buffer1->totalFrameBuffer[i][1] / buffer1->totalFrameCountForBins[i] - buffer2->totalFrameBuffer[i][1] / buffer2->totalFrameCountForBins[i]);

			if (totalFrameBuffer[i][0] < 0)
				totalFrameBuffer[i][0] = 0;			

			totalFrameCountForBins[i] = (buffer1->totalFrameCountForBins[i] + buffer2->totalFrameCountForBins[i]) /2;
			//totalFrameCountForBins[i] = 1;
		}
		else
		{
			totalFrameBuffer[i][0] = 0;
			totalFrameBuffer[i][1] = 0;

			totalFrameCountForBins[i] = 1;
		}
	}
}

void FFTSpectrumBuffer::AddToBuffer(double value, FrequencyRange* inputFrequencyRange, uint8_t dataMode)
{
	uint32_t startDataIndex = GetIndexFromFrequency(inputFrequencyRange->lower);

	if (dataMode == 0)
	{
		for (int i = startDataIndex; i < startDataIndex + DeviceReceiver::FFT_SEGMENT_SAMPLE_COUNT; i++)
		{
			mostRecentFrameBuffer[i][0] += value;
		}
	}
	else
	{
		for (int i = startDataIndex; i < startDataIndex + DeviceReceiver::FFT_SEGMENT_SAMPLE_COUNT; i++)
		{
			totalFrameBuffer[i][0] += value;
		}
	}
}

void FFTSpectrumBuffer::TransferDataToFFTSpectrumBuffer(FFTSpectrumBuffer *fftBuffer)
{	
	for (int i = 0; i < binsForEntireFrequencyRange; i++)
	{
		fftBuffer->totalFrameBuffer[i][0] += totalFrameBuffer[i][0];
		fftBuffer->totalFrameBuffer[i][1] += totalFrameBuffer[i][1];

		fftBuffer->totalFrameCountForBins[i] += totalFrameCountForBins[i];
	}
}

void FFTSpectrumBuffer::SetTestData()
{
	ArrayUtilities::ZeroArray(totalFrameBuffer, 0, binsForEntireFrequencyRange);
	ArrayUtilities::ZeroArray(totalFrameCountForBins, 0, binsForEntireFrequencyRange);

	for (int i = 0; i < binsForEntireFrequencyRange; i++)
	{
		totalFrameBuffer[i][0] = 1;
		totalFrameBuffer[i][1] = 0;
		
		totalFrameCountForBins[i] = 1;
	}
}

void FFTSpectrumBuffer::ClearData()
{
	ArrayUtilities::ZeroArray(totalFrameBuffer, 0, binsForEntireFrequencyRange);
	ArrayUtilities::ZeroArray(totalFrameCountForBins, 0, binsForEntireFrequencyRange);
}

FFTSpectrumBuffer::~FFTSpectrumBuffer()
{

}
