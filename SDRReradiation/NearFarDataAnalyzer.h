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
#include "SpectrumAnalyzer.h"
#include "FrequencyRanges.h"
#include "Graphs.h"
#include "Graph.h"
#include "Transitions.h"

enum ReceivingDataMode{ Near, Far, Undetermined, NearAndUndetermined, Paused = -1 };
enum DetectionMode { Normal, Sessions };
enum ProcessingFarDataMessageBoxEventState{ LAUNCH, SHOW, CLOSE, CLOSED };

class NearFarDataAnalyzer
{
	private:
		bool dataIsNear;
		bool processing;

		static const uint8_t EEG_MIN = 0;
		static const uint8_t EEG_MAX = 100;
		static const uint32_t MAX_EEG_SAMPLES = 2000;

		void *processingThreadHandle;

		ReceivingDataMode mode;
		ReceivingDataMode prevReceivingDataMode;
		FFTSpectrumBuffer *allSessionsBufferNear;
		FFTSpectrumBuffer *allSessionsBufferFar;

		double* allSessionsSpectrumBuffer;
		uint32_t allSessionsSpectrumBufferSize;

		char dataFolder[255];
		char dataFileName[255];

		fftw_complex* fftBuffer;
		uint32_t fftBufferLength;

	public:
	    #if !defined(_DEBUG) || defined(RELEASE_SETTINGS)
			//Release settings
			static const uint32_t INACTIVE_DURATION_UNDETERMINED = 4000;
			static const uint32_t INACTIVE_DURATION_FAR = 30000;
			static const uint32_t INACTIVE_NOTIFICATION_MSG_TIME = 10000;
		#else
			//Debug settings
			static const uint32_t INACTIVE_DURATION_UNDETERMINED = 4000;
			//static const uint32_t INACTIVE_DURATION_FAR = 6000;
			static const uint32_t INACTIVE_DURATION_FAR = 30000;
			static const uint32_t INACTIVE_NOTIFICATION_MSG_TIME = 10000;
		#endif


		SpectrumAnalyzer spectrumAnalyzer;
		FrequencyRange scanningRange;
		Transitions transitions;
		bool addTransition;

		bool automatedDetection;
		DetectionMode detectionMode;
		uint32_t sessionCount;
		uint32_t requiredFramesForSessions;

		FrequencyRanges* spectrumFrequencyRangesBoard;
		FrequencyRanges* transitionFrequencyRangesBoard;
		FrequencyRanges* leaderboardFrequencyRanges;
		FrequencyRanges* reradiatedFrequencyRanges;

		uint32_t checkingReradiatedFrequencyRangeIndex;
		bool checkingReradiatedFrequencyRanges;

		double *spectrumboardStrengthPoints;
		uint32_t spectrumboardStrengthPointsCount;

		double *leaderboardStrengthPoints;
		uint32_t leaderboardStrengthPointsCount;

		double *transitionboardStrengthPoints;
		uint32_t transitionboardStrengthPointsCount;

		FFTSpectrumBuffers* strengthFFTBuffers;

		Graphs* graphs;
		Graph* strengthGraph;
		Graph* strengthFFTGraph;
		Graph* strengthFFTAveragedGraph;
		Graph* leaderboardGraph;
		Graph* spectrumboardGraph;
		Graph* transitionGraph;
		Graph* averageTransitionsGraph;
		Graph* transitionboardGraph;

		static const uint32_t REQUIRED_TRANSITION_WRITES_FOR_RERADIATED_FREQUENCIES = 4;
		static double REQUIRED_TRANSITION_STRENGTH_FOR_RERADIATED_FREQUENCIES;

		int glutWindowID;
		int glutMsgWindowID;

		int glutWindowState;
		bool ignoreNextWindowStateChange;

		ProcessingFarDataMessageBoxEventState processingFarDataMessageBoxEventState;

		NearFarDataAnalyzer();
		uint8_t InitializeNearFarDataAnalyzer(uint32_t bufferSizeInMilliSeconds, uint32_t sampleRate, uint32_t minStartFrequency, uint32_t maxEndFrequency);
		void StartProcessing();
		void WriteDataToFile(FrequencyRanges* frequencyRanges, const char* fileName);
		void ProcessSequenceFinished();
		void ProcessTransitionsAndSetTransitionGraphsData(Transition *transition);
		void OnReceiverDataProcessed();
		void AddTransitionPointsToLeaderboard(FrequencyRanges *board, FrequencyRanges *leaderboard);
		void AddPointsToLeaderboard(FrequencyRanges *board, FrequencyRanges *leaderboard);
		void SetMode(ReceivingDataMode mode);
		uint8_t GetMode();
		void SetModeNear();
		void Process();
		void StartProcessingThread();
		void Pause();
		void Resume();
		void CloseProcessingFarDataMessageBox();
		void ClearAllData();
		~NearFarDataAnalyzer();
};
