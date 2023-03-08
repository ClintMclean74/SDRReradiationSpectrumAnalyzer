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

#include <math.h>
#include <stdio.h>

#ifdef _WIN32
    #include <utilapiset.h>
#endif // _WIN32

#include "SoundDevice.h"

SoundDevice::SoundDevice()
{
    #ifndef _WIN32
        /* Create a new playback stream */
        if (!(s = pa_simple_new(NULL, "", PA_STREAM_PLAYBACK, NULL, "playback", &ss, NULL, NULL, &error)))
        {
            fprintf(stderr, __FILE__": pa_simple_new() failed: %s\n", pa_strerror(error));
        }
    #endif // _WIN32
};

void SoundDevice::Play(uint32_t frequency, uint32_t duration)
{
    this->frequency = frequency;
	this->duration = duration;

	Play();
}

void SoundDevice::Play()
{
    #ifdef _WIN32
        Beep(frequency, duration);
    #else
        if (frequency > 0)
        {
            uint32_t segmentFrequency = frequency;

            double secFrac = 0;

            for (int i = 0; i<segmentSize;i++, secFrac++)
            {
                buf[i]=(int)(sin(secFrac/(44100/segmentFrequency))*128);

                if (frequency > segmentFrequency)
                    segmentFrequency = frequency;
            }

            pa_simple_write(s,buf,sizeof(buf),NULL);

            if (pa_simple_drain(s, &error) < 0)
            {
                fprintf(stderr, __FILE__": pa_simple_drain() failed: %s\n", pa_strerror(error));
            }
        }

        frequency = 0;
    #endif // !_WIN32
};
