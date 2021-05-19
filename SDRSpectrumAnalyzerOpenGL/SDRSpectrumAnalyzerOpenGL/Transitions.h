#pragma once
#include <stdint.h>
#include "Transition.h"
#include "FrequencyRanges.h"

class Transitions
{
	public:
		static uint32_t DURATION;
		static double Transitions::RERADIATED_STRENGTH_INC;

		uint32_t count = 0;
		Transition* first = NULL;
		Transition* last = NULL;

		Transitions();
		Transition* GetTransition(FrequencyRange *range);
		Transition* Add(BandwidthFFTBuffer* transitionBandwidthFFTBuffer, DWORD transitionDuration);
		Transition* Add(BandwidthFFTBuffer* transitionBandwidthFFTBuffer, DWORD transitionDuration, FrequencyRange* range, uint32_t startIndex, uint32_t endIndex);

		Transition* Add(BandwidthFFTBuffer* nearBandwidthFFTBuffer, BandwidthFFTBuffer* farBandwidthFFTBuffer, DWORD transitionDuration);
		void DetermineTransitionStrengths();
		void GetFrequencyRangesAndTransitionStrengths(FrequencyRanges* frequencyRanges);
};
