#pragma once
#include "SpectrumAnalyzer.h"
#include "FrequencyRanges.h"

enum ReceivingDataMode{ Near, Far, Undetermined, NearAndUndetermined, Paused = -1 };
enum DetectionMode { Normal, Sessions };

class NearFarDataAnalyzer
{	
	private:		
		bool dataIsNear = true;		
		DWORD dataIsNearTimeStamp = 0; 
		bool processing = false;		
		static const uint32_t INACTIVE_DURATION_UNDETERMINED = 4000;
		static const uint32_t INACTIVE_DURATION_FAR = 60000;
		//static const uint32_t INACTIVE_DURATION_FAR = 30000;
		//static const uint32_t INACTIVE_DURATION_FAR = 10000;
		HANDLE processingThreadHandle;				
		ReceivingDataMode mode = ReceivingDataMode::Near;
		ReceivingDataMode prevReceivingDataMode;
		FFTSpectrumBuffer *SessionsBufferNear;
		FFTSpectrumBuffer *SessionsBufferFar;

	public:
		SpectrumAnalyzer spectrumAnalyzer;
		FrequencyRange scanningRange;
		bool automatedDetection = false;
		DetectionMode detectionMode = DetectionMode::Normal;
		uint32_t sessionCount = 1;
		uint32_t requiredFramesForSessions = 1000;

		FrequencyRanges* spectrumFrequencyRangesBoard;
		FrequencyRanges* leaderboardFrequencyRanges;

		NearFarDataAnalyzer();
		uint8_t InitializeNearFarDataAnalyzer(uint32_t bufferSizeInMilliSeconds, uint32_t sampleRate, uint32_t minStartFrequency, uint32_t maxEndFrequency);
		void StartProcessing();	
		void ProcessSequenceFinished();
		void AddPointsToLeaderboard(FrequencyRanges *spectrumBoard, FrequencyRanges *leaderboard);
		void SetMode(ReceivingDataMode mode);
		uint8_t GetMode();
		void Process();
		void StartProcessingThread();
		void Pause();
		void Resume();		
		void ClearAllData();
		~NearFarDataAnalyzer();
};
