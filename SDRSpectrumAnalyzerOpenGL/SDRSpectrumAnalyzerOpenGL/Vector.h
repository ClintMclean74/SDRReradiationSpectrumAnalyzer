#pragma once

class Vector
{
	public:
		double x, y, z;

		Vector();

		Vector(double x, double y, double z);

		void SetVector(double x, double y, double z);

		~Vector();
};

typedef Vector* VectorPtr;
