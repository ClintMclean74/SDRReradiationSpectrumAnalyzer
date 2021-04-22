#ifdef _DEBUG 
 #define _ITERATOR_DEBUG_LEVEL 2 
#endif
#include <string>
#include <windows.h>
#include "CircularDataBuffer.h"
#include "SignalProcessingUtilities.h"
#include "DeviceReceiver.h"

CircularDataBuffer::CircularDataBuffer(long bufferSize, long segmentLength)
{
	size = bufferSize;
	
	circularBuffer = new uint8_t[size];

	segmentSize = segmentLength;

	//segmentsPerSecond = 1000000 / (segmentSize / 2);
	segmentsPerSecond = DeviceReceiver::SAMPLE_RATE / (segmentSize / 2);
}

long CircularDataBuffer::AddIndexes(long index1, long index2)
{
	long result = index1 + index2;

	if (result < 0)
		result += size;
	else
		if (result >= size)
			result -= size;	

	return result;
}

long CircularDataBuffer::GetLength(long startIndex, long endIndex)
{
	if (startIndex < endIndex)
		return endIndex - startIndex;
	else
		return endIndex + (size - startIndex);
}

uint32_t CircularDataBuffer::WriteData(uint8_t *buffer, long length)
{
	if (segmentWriteTime == 0)
		segmentWriteTime = GetTickCount();

	int rightAvailableBytes = size - writeStart;

	if (rightAvailableBytes >= length)
	{
		memcpy(&circularBuffer[writeStart], buffer, length * sizeof(uint8_t));

		writeStart += length;
	}
	else
	{
		memcpy(&circularBuffer[writeStart], buffer, rightAvailableBytes * sizeof(uint8_t));

		int remainingBytesToBeWritten = length - rightAvailableBytes;

		memcpy(&circularBuffer[0], &buffer[rightAvailableBytes], remainingBytesToBeWritten * sizeof(uint8_t));

		writeStart = remainingBytesToBeWritten;
	}

	if (writeStart == size)
		writeStart = 0;
	else
		if (writeStart > size)
			writeStart -= size;

	DWORD currentTime = GetTickCount();

	writes++;

	long segmentsPercentageTime;

	if (writes % segmentsPerSecond == 0)
		segmentsPercentageTime = 1000;
	else
		segmentsPercentageTime = (float)(writes % segmentsPerSecond) / segmentsPerSecond * 1000;

	while (segmentsPercentageTime > (currentTime - segmentWriteTime))
	{
		currentTime = GetTickCount();
	};

	if (writes % segmentsPerSecond == 0)
	{
		segmentTime = GetTickCount() - segmentWriteTime;

		segmentWriteTime = GetTickCount();
	}

	return length;
}

long CircularDataBuffer::ReadMemoryCopy(uint8_t *buffer, long readStart, long length)
{
	int rightAvailableBytes = size - readStart;

	if (rightAvailableBytes >= length)
	{
		memcpy(buffer, &circularBuffer[readStart], length * sizeof(uint8_t));
	}
	else
	{
		memcpy(buffer, &circularBuffer[readStart], rightAvailableBytes * sizeof(uint8_t));

		int remainingBytesToBeWritten = length - rightAvailableBytes;

		memcpy(&buffer[rightAvailableBytes], circularBuffer, remainingBytesToBeWritten * sizeof(uint8_t));
	}

	return length;
}

uint32_t CircularDataBuffer::ReadData(uint8_t* dataBuffer, uint32_t length, int32_t offset, double phaseShift, int32_t readStart)
{
	if (readStart == -1)
		readStart = AddIndexes(writeStart, -length);

	readStart = AddIndexes(readStart, offset);

	if (readStart > -1)
	{
		int rightAvailableBytes = size - readStart;

		if (rightAvailableBytes >= length)
		{
			memcpy(dataBuffer, &circularBuffer[readStart], length * sizeof(uint8_t));
		}
		else
		{
			memcpy(dataBuffer, &circularBuffer[readStart], rightAvailableBytes * sizeof(uint8_t));

			int remainingBytesToBeRead = length - rightAvailableBytes;

			memcpy(&dataBuffer[rightAvailableBytes], circularBuffer, remainingBytesToBeRead * sizeof(uint8_t));
		}

		if (phaseShift!=0)
			SignalProcessingUtilities::Rotate(dataBuffer, length, -phaseShift);			
		
		return length;
	}
	else
		return -1;
}

CircularDataBuffer::~CircularDataBuffer()
{
	delete[] circularBuffer;
}
