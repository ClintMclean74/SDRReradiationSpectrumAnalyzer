#define _ITERATOR_DEBUG_LEVEL 0
#include <string>
#include <stdint.h>
#include "MathUtilities.h"

namespace MathUtilities
{	
	double Round(double value, uint8_t decimalPoints)
	{
		double shiftValue = pow(10, decimalPoints);

		return ((int32_t)(value * shiftValue + 0.5)) / shiftValue;
	}

	Vector CalcNormal(Vector vect1, Vector vect2, Vector vect3)
	{
		Vector normal;
		double length;
		double x1, y1, z1, x2, y2, z2;

		x1 = (vect2.x) - (vect1.x);
		y1 = (vect2.y) - (vect1.y);
		z1 = (vect2.z) - (vect1.z);
		x2 = (vect3.x) - (vect2.x);
		y2 = (vect3.y) - (vect2.y);
		z2 = (vect3.z) - (vect2.z);

		normal.x = (y1*z2) - (y2*z1);
		normal.y = (x2*z1) - (x1*z2);
		normal.z = (x1*y2) - (x2*y1);

		length = (normal.x*normal.x + normal.y*normal.y + normal.z*normal.z);

		if (length>0)
			length = sqrt(length);
		else
		{
			length = 0; return normal;
		}

		normal.x = (normal.x / length);
		normal.y = (normal.y / length);
		normal.z = (normal.z / length);

		return normal;
	}	
}
