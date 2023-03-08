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

#include "stdint.h"

#ifndef _WIN32
    #include <pulse/simple.h>
    #include <pulse/error.h>
#endif // !_WIN32

class SoundDevice
{
    private:
        #ifndef _WIN32
            /* The Sample format to use */
            static constexpr pa_sample_spec ss = {
                .format = PA_SAMPLE_S16LE,
                .rate = 44100,
                .channels = 2
            };

            pa_simple *s = NULL;
            int ret = 1;
            int error;

            ////static const uint32_t segmentSize = 44100 / 100;
            ////uint8_t buf[1024];

            ////double segmentSize = 44100 / 100;
            static const uint32_t segmentSize = 44100;
            uint8_t buf[segmentSize];
        #endif // !_WIN32

	public:
		uint32_t frequency = 0;
		uint32_t duration = 100;

		SoundDevice();

		void Play(uint32_t frequency, uint32_t duration);
		void Play();
};
