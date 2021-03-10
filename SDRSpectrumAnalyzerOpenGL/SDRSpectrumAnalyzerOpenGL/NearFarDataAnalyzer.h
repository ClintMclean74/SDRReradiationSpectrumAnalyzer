#pragma once
#include "SpectrumAnalyzer.h"
#include "FrequencyRanges.h"

enum ReceivingDataMode{ Near, Far, Undetermined, Paused = -1 };

class NearFarDataAnalyzer
{	
	private:		
		bool dataIsNear = true;		
		DWORD dataIsNearTimeStamp = 0; 
		bool processing = false;		
		static const uint32_t INACTIVE_DURATION_UNDETERMINED = 4000;
		static const uint32_t INACTIVE_DURATION_FAR = 60000;
		////static const uint32_t INACTIVE_DURATION_FAR = 10000;
		HANDLE processingThreadHandle;				
		ReceivingDataMode mode;
		ReceivingDataMode prevReceivingDataMode;

	public:
		SpectrumAnalyzer spectrumAnalyzer;
		FrequencyRange scanningRange;
		bool automatedDetection = false;

		FrequencyRanges* leaderboardFrequencyRanges;

		NearFarDataAnalyzer();
		uint8_t InitializeNearFarDataAnalyzer(uint32_t bufferSizeInMilliSeconds, uint32_t sampleRate, uint32_t minStartFrequency, uint32_t maxEndFrequency);
		void StartProcessing();
		void ProcessSequenceFinished();
		void SetMode(ReceivingDataMode mode);
		uint8_t GetMode();
		void Process();
		void StartProcessingThread();
		void Pause();
		void Resume();
		////uint32_t GetNearFFTData(double *dataBuffer, uint32_t dataBufferLength, uint32_t startFrequency, uint32_t endFrequency, uint8_t dataMode);
		////double GetNearFFTStrengthForRange(uint32_t startFrequency, uint32_t endFrequency, uint8_t dataMode);
		~NearFarDataAnalyzer();
};

////static const DebuggingUtilities::DEBUGGING = false;