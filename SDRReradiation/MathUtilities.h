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
#include "Vector.h"

namespace MathUtilities
{
	const double PI = 3.14159265358979323846;

	const double RadianToDegrees = 180 / PI;

	double Round(double value, uint8_t decimalPoints = 0);
	Vector CalcNormal(Vector vect1, Vector vect2, Vector vect3);
}
