#pragma once
#include <stdint.h>
#include "BandwidthFFTBuffer.h"

class Transition
{	
	public:
		FrequencyRange range;
		BandwidthFFTBuffer* bandwidthFFTBuffer;
		BandwidthFFTBuffer* bandwidthAverageFFTBuffer;
		uint32_t writes = 0;
		double strengthForMostRecentTransition;
		double averagedStrengthForAllTransitions;

		Transition* previous = NULL;
		Transition* next = NULL;

		Transition(BandwidthFFTBuffer* transitionBandwidthFFTBuffer, DWORD transitionDuration);
		Transition(BandwidthFFTBuffer* nearBandwidthFFTBuffer, BandwidthFFTBuffer* farBandwidthFFTBuffer, DWORD transitionDuration);
		Transition(BandwidthFFTBuffer* transitionBandwidthFFTBuffer, DWORD transitionDuration, FrequencyRange* range, uint32_t startIndex = 0, uint32_t endIndex = 0);
		//void Set(fftw_complex* transitionFFTBuffer, DWORD transitionDuration, FrequencyRange* range, uint32_t index);
		void CopyTransitionFFTDataIntoBuffer(BandwidthFFTBuffer* transitionBandwidthFFTBuffer, DWORD transitionDuration);		
		void CopyNearFarFFTDataIntoBuffer(BandwidthFFTBuffer* nearBandwidthFFTBuffer, BandwidthFFTBuffer* farBandwidthFFTBuffer, DWORD transitionDuration);
		void SetTransitionData(BandwidthFFTBuffer* nearBandwidthFFTBuffer, BandwidthFFTBuffer* farBandwidthFFTBuffer, DWORD transitionDuration);
		void SetTransitionData(BandwidthFFTBuffer* transitionBandwidthFFTBuffer, DWORD transitionDuration, FrequencyRange* range, uint32_t startIndex, uint32_t endIndex, bool averageStrengthPhase = false);
		void SetTransitionData(BandwidthFFTBuffer* transitionBandwidthFFTBuffer, DWORD transitionDuration);
		SignalProcessingUtilities::Strengths_ID_Time* GetStrengthForRangeOverTime(uint32_t startIndex, uint32_t endIndex, uint32_t* resultLength);
		SignalProcessingUtilities::Strengths_ID_Time* GetAveragedTransitionsStrengthForRangeOverTime(uint32_t startIndex, uint32_t endIndex, uint32_t* resultLength);
		void DetermineTransitionStrength();
};
