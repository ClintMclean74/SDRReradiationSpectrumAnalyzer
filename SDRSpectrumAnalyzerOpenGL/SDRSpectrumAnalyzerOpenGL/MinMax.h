#pragma once
class MinMax
{
	public:
		double min = 999999999, max = -999999999, range = 0;

		void CalculateRange()
		{
			range = max - min;
		}
};