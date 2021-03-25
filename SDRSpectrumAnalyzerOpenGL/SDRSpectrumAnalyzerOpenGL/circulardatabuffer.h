#pragma once
#define _ITERATOR_DEBUG_LEVEL 0
#include <stdint.h>
#include <windows.h>

class CircularDataBuffer
{
	public:
		uint8_t *circularBuffer;		

		int size;
		int32_t writeStart = 0;

		long bytesAvailableToRead = 0;

		long writes = 0;

		DWORD prevSegmentWriteTime;
		DWORD segmentWriteTime = 0;
		DWORD segmentTime;

		long segmentSize;
		long segmentsPerSecond;
		
		CircularDataBuffer(long bufferSize, long segmentSize);
		long AddIndexes(long index1, long index2);		
		long GetLength(long startIndex, long endIndex);

		uint32_t WriteData(uint8_t *buffer, long length);
		long ReadMemoryCopy(uint8_t *buffer, long readStart, long length);
		uint32_t ReadData(uint8_t* dataBuffer, uint32_t length, int32_t offset, double phaseShift = 0, int32_t readStart = -1);
		
		~CircularDataBuffer();
};

