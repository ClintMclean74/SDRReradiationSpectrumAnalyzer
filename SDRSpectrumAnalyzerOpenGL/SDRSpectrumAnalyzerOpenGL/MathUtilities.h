#pragma once
#include "Vector.h"

namespace MathUtilities
{
	const double PI = 3.14159265358979323846;

	const double RadianToDegrees = 180 / PI;

	double Round(double value, uint8_t decimalPoints);
	Vector CalcNormal(Vector vect1, Vector vect2, Vector vect3);
}
