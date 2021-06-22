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

#ifdef _WIN32
    #include <windows.h>
#else
    #include "WindowsToLinuxUtilities.h"
#endif // _WIN32

#include "fftw/fftw3.h"
#include "FrequencyRange.h"
#include "pstdint.h"

class FFTArrayDataStructure
{
	public:
		uint32_t length;
		fftw_complex* fftArray;
		uint32_t ID;//;
		FrequencyRange range;
		DWORD time;
		uint32_t addCount;//;

		FFTArrayDataStructure(fftw_complex* deviceFFTDataBufferArray, uint32_t length, DWORD time, FrequencyRange *range, uint32_t ID = 0, bool averageStrengthPhase = false);
};
