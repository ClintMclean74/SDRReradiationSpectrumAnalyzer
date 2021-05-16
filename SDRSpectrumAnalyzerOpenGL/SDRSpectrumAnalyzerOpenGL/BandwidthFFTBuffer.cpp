#include "BandwidthFFTBuffer.h";
#include "ArrayUtilities.h"
#include "DeviceReceiver.h"

BandwidthFFTBuffer::BandwidthFFTBuffer(uint32_t size)
{	
	ConstructFFTArrayDataStructures(size);
}

BandwidthFFTBuffer::BandwidthFFTBuffer(uint32_t lower, uint32_t upper, uint32_t size)
{		
	range.Set(lower, upper);

	ConstructFFTArrayDataStructures(size);
}

BandwidthFFTBuffer::BandwidthFFTBuffer(FrequencyRange* range, uint32_t size)
{	
	this->range.Set(range);
	
	ConstructFFTArrayDataStructures(size);
}

void BandwidthFFTBuffer::ConstructFFTArrayDataStructures(uint32_t size)
{
	this->size = size;

	fftArrayDataStructures = new FFTArrayDataStructure_ptr[this->size];

	for (int i = 0; i < this->size; i++)
	{
		fftArrayDataStructures[i] = NULL;
		//fftArrayDataStructures[i] = new FFTArrayDataStructure(NULL,  DeviceReceiver::FFT_SEGMENT_BUFF_LENGTH_FOR_SEGMENT_BANDWIDTH*40, 0, NULL, 0);
	}
}

/*void BandwidthFFTBuffer::Set(fftw_complex* fftDurationBuffer, uint32_t length, FrequencyRange* range, uint32_t index)
{
	uint32_t writeIndex = writeStart;

	for (uint32_t i = 0; i < length; i++)
	{
		fftArrayDataStructures[i].fftArray[index] = fftDurationBuffer[i];
		
		strengths_IDs[i].range = fftArrayDataStructures[bufferIndex]->range;
		strengths_IDs[i].addCount = fftArrayDataStructures[bufferIndex]->addCount;
		//strengths[i] = GetStrengthForRange(startIndex, endIndex, bufferIndex);

		bufferIndex++;
	}
}*/

uint32_t BandwidthFFTBuffer::Add(fftw_complex* deviceFFTDataBufferArray, uint32_t length, DWORD time, FrequencyRange *range, uint32_t ID, uint32_t arrayIndex, bool addData, bool averageStrengthPhase, uint32_t maxFFTArrayLength)
{	
	if (arrayIndex != FFT_ARRAYS_COUNT)
		writeStart = arrayIndex;
	
	if (fftArrayDataStructures[writeStart] == NULL)
		fftArrayDataStructures[writeStart] = new FFTArrayDataStructure(deviceFFTDataBufferArray, maxFFTArrayLength, time, range, ID, averageStrengthPhase);

	if (averageStrengthPhase)
	{
		double totalStrength = 0, totalPhase = 0;

		for (int i = 0; i < length; i++)
		{
			totalStrength += deviceFFTDataBufferArray[i][0];
			totalPhase += deviceFFTDataBufferArray[i][1];
		}

		if (!addData)
		{
			fftArrayDataStructures[writeStart]->fftArray[0][0] = totalStrength / length;
			fftArrayDataStructures[writeStart]->fftArray[0][1] = totalPhase / length;

			fftArrayDataStructures[writeStart]->addCount = 1;
		}
		else
		{
			fftArrayDataStructures[writeStart]->fftArray[0][0] += totalStrength / length;
			fftArrayDataStructures[writeStart]->fftArray[0][1] += totalPhase / length;

			fftArrayDataStructures[writeStart]->addCount++;
		}
	}
	else
	{		
		if (!addData)
		{
			ArrayUtilities::CopyArray(deviceFFTDataBufferArray, length, fftArrayDataStructures[writeStart]->fftArray);
			fftArrayDataStructures[writeStart]->addCount = 1;
		}
		else
		{
			ArrayUtilities::AddArrays(deviceFFTDataBufferArray, length, fftArrayDataStructures[writeStart]->fftArray);
			fftArrayDataStructures[writeStart]->addCount++;			
		}		
	}

	if (averageStrengthPhase)
		fftArrayDataStructures[writeStart]->length = 1;
	else
		fftArrayDataStructures[writeStart]->length = length;

	fftArrayDataStructures[writeStart]->time = time;
	fftArrayDataStructures[writeStart]->range.Set(range);
	fftArrayDataStructures[writeStart]->ID = ID;

	writeStart++;

	if (writeStart >= size)
		writeStart = 0;

	writes++;

	return writeStart;
}

void BandwidthFFTBuffer::Reset()
{
	writes = writeStart = 0;
}

void BandwidthFFTBuffer::SetNewRange(FrequencyRange *range)
{
	this->Reset();
	this->range.Set(range);
}

FFTArrayDataStructure* BandwidthFFTBuffer::GetFFTArrayData(uint32_t index)
{
	if (index == this->size)
	{
		if (writeStart == 0)
			index = this->size - 1;
		else
			index = writeStart - 1;
	}
	
	return fftArrayDataStructures[index];
}

double BandwidthFFTBuffer::GetStrengthForRange(uint32_t startIndex, uint32_t endIndex, uint32_t bufferIndex)
{	
	if (endIndex == 0)
		endIndex = fftArrayDataStructures[bufferIndex]->length;

	uint32_t length = endIndex - startIndex;

	double totalStrength = 0;

	for (int i = startIndex; i < endIndex; i++)
	{
		totalStrength += fftArrayDataStructures[bufferIndex]->fftArray[i][0];		
	}

	return totalStrength / length;
}

SignalProcessingUtilities::Strengths_ID_Time* BandwidthFFTBuffer::GetStrengthForRangeOverTime(uint32_t startIndex, uint32_t endIndex, DWORD* duration, unsigned int deviceIndex, uint32_t* resultLength, DWORD currentTime)
{
	uint32_t bufferIndex;

	if (writeStart > 0)
		bufferIndex = writeStart - 1;
	else
		bufferIndex = this->size - 1;

	uint32_t count = 0;

	if (writes > 0 && currentTime == 0)
		currentTime = fftArrayDataStructures[bufferIndex]->time;

	DWORD currentFFTDataStructureTime, prevFFTDataStructureTime;

	while (count < writes && count < size && currentTime - fftArrayDataStructures[bufferIndex]->time <= *duration)
	{	
		count++;

		if (count >= writes)
		{
			*duration = currentTime - fftArrayDataStructures[bufferIndex]->time;
			bufferIndex--;

			break;
		}
		
		currentFFTDataStructureTime = fftArrayDataStructures[bufferIndex]->time;

		if (bufferIndex ==  0)
			bufferIndex = this->size;

		bufferIndex--;

		if (fftArrayDataStructures[bufferIndex])
			prevFFTDataStructureTime = fftArrayDataStructures[bufferIndex]->time;
		else
			prevFFTDataStructureTime = 0;

		if (currentFFTDataStructureTime - prevFFTDataStructureTime > 1000) //non consequitive fft data structures
		{
			*duration = currentTime - currentFFTDataStructureTime;
			break;
		}
	}

	//*duration = currentTime - fftArrayDataStructures[bufferIndex]->time;	

	if (count > 0)
	{	
		SignalProcessingUtilities::Strengths_ID_Time* strengths_IDs = new SignalProcessingUtilities::Strengths_ID_Time[count];
		
		bufferIndex++;

		if (bufferIndex >= this->size)
			bufferIndex = 0;

		int i = 0;

		uint32_t maxAddCount = 0;

		for (i = 0; i < count; i++)
		{
			if (bufferIndex >= this->size)
				bufferIndex = 0;

			strengths_IDs[i].strength = GetStrengthForRange(startIndex, endIndex, bufferIndex) / fftArrayDataStructures[bufferIndex]->addCount;
			strengths_IDs[i].ID = fftArrayDataStructures[bufferIndex]->ID;
			strengths_IDs[i].time = fftArrayDataStructures[bufferIndex]->time;
			strengths_IDs[i].range = fftArrayDataStructures[bufferIndex]->range;
			strengths_IDs[i].addCount = fftArrayDataStructures[bufferIndex]->addCount;

			if (strengths_IDs[i].addCount > maxAddCount || maxAddCount == 0)
				maxAddCount = strengths_IDs[i].addCount;
			else
				if (strengths_IDs[i].addCount < maxAddCount)
					break;

			bufferIndex++;
		}		

		*resultLength = i;

		return strengths_IDs;		
	}
	else
		*duration = 0;

	return NULL;
}

/*uint32_t BandwidthFFTBuffer::CopyDataIntoBuffer(BandwidthFFTBuffer *dstBuffer, DWORD* duration, unsigned int deviceIndex, uint32_t* resultLength, DWORD currentTime = 0, uint32_t startIndex, uint32_t endIndex)
{

}*/

uint32_t BandwidthFFTBuffer::CopyDataIntoBuffer(BandwidthFFTBuffer *dstBuffer, DWORD* duration, unsigned int deviceIndex, uint32_t* resultLength, DWORD currentTime, uint32_t frequencyStartDataIndex, uint32_t frequencyEndDataIndex, bool addData, bool averageStrengthPhase)
{
	uint32_t bufferIndex;

	if (writeStart > 0)
		bufferIndex = writeStart - 1;
	else
		bufferIndex = this->size - 1;

	uint32_t count = 0;

	if (writes > 0 && currentTime == 0)
		currentTime = fftArrayDataStructures[bufferIndex]->time;

	while (count < writes && count < size && currentTime - fftArrayDataStructures[bufferIndex]->time < *duration)
	{
		if (count + 1 >= writes)
			*duration = currentTime - fftArrayDataStructures[bufferIndex]->time;

		count++;

		if (bufferIndex == 0)
			bufferIndex = this->size;

		bufferIndex--;
	}

	*resultLength = count;

	if (count > 0)
	{
		bufferIndex++;

		//double* strengths = new double[count];

		if (bufferIndex >= this->size)
			bufferIndex = 0;

		int i = 0;

		uint32_t length;

		if (frequencyEndDataIndex != 0 || frequencyStartDataIndex != 0)
			length = frequencyEndDataIndex - frequencyStartDataIndex;
		else
			length = fftArrayDataStructures[0]->length;

		for (i = 0; i < count; i++)
		{
			if (bufferIndex >= this->size)
				bufferIndex = 0;			

			dstBuffer->Add(&fftArrayDataStructures[bufferIndex]->fftArray[frequencyStartDataIndex], length, fftArrayDataStructures[bufferIndex]->time, &fftArrayDataStructures[bufferIndex]->range, fftArrayDataStructures[bufferIndex]->ID, i, addData, averageStrengthPhase);
			//dstBuffer->Add(fftArrayDataStructures[bufferIndex]->fftArray, fftArrayDataStructures[bufferIndex]->length, fftArrayDataStructures[bufferIndex]->time, &fftArrayDataStructures[bufferIndex]->range, fftArrayDataStructures[bufferIndex]->ID);
			//dstBuffer->Add(fftArrayDataStructures[bufferIndex]->fftArray, fftArrayDataStructures[bufferIndex]->length, fftArrayDataStructures[bufferIndex]->time, &fftArrayDataStructures[bufferIndex]->range, fftArrayDataStructures[bufferIndex]->ID);
		
			bufferIndex++;
		}

		return count;
	}
	else
		*duration = 0;

	return NULL;
}

uint32_t BandwidthFFTBuffer::FFT_ARRAYS_COUNT = 2000;
