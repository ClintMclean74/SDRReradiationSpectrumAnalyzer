/*
 * SDRReradiation - Code system to detect biologically reradiated
 * electromagnetic energy from humans
 * Copyright (C) 2021 by Clint Mclean <clint@getfitnowapps.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once
#include "pstdint.h"
#include "BandwidthFFTBuffer.h"

class Transition
{
	public:
		FrequencyRange range;
		BandwidthFFTBuffer* bandwidthFFTBuffer;
		BandwidthFFTBuffer* bandwidthAverageFFTBuffer;
		uint32_t writes;
		double strengthForMostRecentTransition;
		double averagedStrengthForAllTransitions;

		Transition* previous;
		Transition* next;

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
