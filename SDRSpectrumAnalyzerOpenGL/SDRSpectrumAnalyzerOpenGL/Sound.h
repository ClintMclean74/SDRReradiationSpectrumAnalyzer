#pragma once

class Sound
{
	public:
		DWORD frequency;
		DWORD duration;

	
		Sound(DWORD frequency, DWORD duration)
		{
			this->frequency = frequency;

			this->duration = duration;
		};
};