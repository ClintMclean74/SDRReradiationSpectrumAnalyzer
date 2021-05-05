#pragma once

class Color
{
	public:
		float r;
		float g;
		float b;
		float a;

		Color()
		{
			r = g = b = a = 1;
		}

		Color(float r, float g, float b, float a)
		{
			this->r = r;
			this->g = g;
			this->b = b;
			this->a = a;				
		}
};