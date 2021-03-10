#include "Vector.h"

Vector::Vector()
{
	x = y = z = 0;
}

Vector::Vector(double x, double y, double z)
{
	this->x = x;
	this->y = y;
	this->z = z;
}

void Vector::SetVector(double x, double y, double z)
{
	this->x = x;
	this->y = y;
	this->z = z;
}

Vector::~Vector()
{
}
