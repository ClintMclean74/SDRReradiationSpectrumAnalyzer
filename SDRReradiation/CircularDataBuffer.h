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
#ifdef _DEBUG
 #define _ITERATOR_DEBUG_LEVEL 2
#endif

/*#ifdef _WIN32
    #include <windows.h>
#else
    #include "wintypes.h"
#endif // _WIN32
*/

#include "WindowsToLinuxUtilities.h"
#include "pstdint.h"

class CircularDataBuffer
{
	public:
		uint8_t *circularBuffer;

		int size;
		int32_t writeStart;//;

		long bytesAvailableToRead;

		long writes;

		DWORD prevSegmentWriteTime;
		DWORD segmentWriteTime;
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

