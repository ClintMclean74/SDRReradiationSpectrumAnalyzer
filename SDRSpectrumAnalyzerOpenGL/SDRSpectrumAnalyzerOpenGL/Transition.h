#pragma once
#include <stdint.h>
#include "BandwidthFFTBuffer.h"

class Transition
{	
	public:
		FrequencyRange range;
		BandwidthFFTBuffer* bandwidthFFTBuffer;
		BandwidthFFTBuffer* bandwidthAverageFFTBuffer;
		double strength;

		Transition* next = NULL;

		Transition(BandwidthFFTBuffer* transitionBandwidthFFTBuffer, DWORD transitionDuration);
		Transition(BandwidthFFTBuffer* nearBandwidthFFTBuffer, BandwidthFFTBuffer* farBandwidthFFTBuffer, DWORD transitionDuration);
		Transition(BandwidthFFTBuffer* transitionBandwidthFFTBuffer, DWORD transitionDuration, FrequencyRange* range, uint32_t startIndex = 0, uint32_t endIndex = 0);
		void CopyTransitionFFTDataIntoBuffer(BandwidthFFTBuffer* transitionBandwidthFFTBuffer, DWORD transitionDuration);		
		void CopyNearFarFFTDataIntoBuffer(BandwidthFFTBuffer* nearBandwidthFFTBuffer, BandwidthFFTBuffer* farBandwidthFFTBuffer, DWORD transitionDuration);
		void SetTransitionData(BandwidthFFTBuffer* nearBandwidthFFTBuffer, BandwidthFFTBuffer* farBandwidthFFTBuffer, DWORD transitionDuration);
		void SetTransitionData(BandwidthFFTBuffer* transitionBandwidthFFTBuffer, DWORD transitionDuration, FrequencyRange* range, uint32_t startIndex, uint32_t endIndex);
		void SetTransitionData(BandwidthFFTBuffer* transitionBandwidthFFTBuffer, DWORD transitionDuration);
		SignalProcessingUtilities::Strengths_ID_Time* GetStrengthForRangeOverTime(uint32_t startIndex, uint32_t endIndex, uint32_t* resultLength);
		SignalProcessingUtilities::Strengths_ID_Time* GetAveragedTransitionsStrengthForRangeOverTime(uint32_t startIndex, uint32_t endIndex, uint32_t* resultLength);
		void DetermineTransitionStrength();
};
