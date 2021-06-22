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
#include "pstdint.h"
#include <cstddef>

class FrequencyRange
{
public:
	uint32_t lower;
	uint32_t upper;
	uint32_t length;
	uint32_t centerFrequency;
	double strength;
	uint32_t frames;

	uint8_t flags[4];

	FrequencyRange();
	FrequencyRange(uint32_t lower, uint32_t upper, double strengthValue = 0, uint32_t frames = 1, uint8_t* flags = NULL);
	int operator ==(FrequencyRange range);
	int operator !=(FrequencyRange range);
	void Set(FrequencyRange *range);
	void Set(uint32_t lower, uint32_t upper);
};
