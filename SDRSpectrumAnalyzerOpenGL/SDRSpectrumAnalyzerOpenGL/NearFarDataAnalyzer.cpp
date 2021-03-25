#include <process.h>
#include "NearFarDataAnalyzer.h"
#include "DebuggingUtilities.h"

NearFarDataAnalyzer::NearFarDataAnalyzer()
{

}

uint8_t NearFarDataAnalyzer::InitializeNearFarDataAnalyzer(uint32_t bufferSizeInMilliSeconds, uint32_t sampleRate, uint32_t minStartFrequency, uint32_t maxEndFrequency)
{		
	spectrumFrequencyRangesBoard = new FrequencyRanges((maxEndFrequency - minStartFrequency) / sampleRate);
	leaderboardFrequencyRanges = new FrequencyRanges((maxEndFrequency- minStartFrequency) / sampleRate);

	scanningRange.Set(minStartFrequency, maxEndFrequency);

	spectrumAnalyzer.SetCalculateFFTDifferenceBuffer(true);

	uint8_t deviceCount = spectrumAnalyzer.InitializeSpectrumAnalyzer(bufferSizeInMilliSeconds, sampleRate, minStartFrequency, maxEndFrequency);

	SetMode(ReceivingDataMode::Near);	

	return deviceCount;
}

void NearFarDataAnalyzer::StartProcessing()
{
	spectrumAnalyzer.StartReceivingData();

	spectrumAnalyzer.LaunchScanningFrequencyRange(spectrumAnalyzer.maxFrequencyRange);

	StartProcessingThread();		
}

void NearFarDataAnalyzer::ProcessSequenceFinished()
{
	if (spectrumAnalyzer.currentScanningFrequencyRange.lower == scanningRange.lower && spectrumAnalyzer.currentScanningFrequencyRange.upper == scanningRange.upper)
	{	
		spectrumFrequencyRangesBoard->ProcessFFTSpectrumStrengthDifferenceData(spectrumAnalyzer.GetFFTSpectrumBuffer(4));		
				
		FrequencyRange *range1 = spectrumFrequencyRangesBoard->GetFrequencyRangeFromIndex(0);
		FrequencyRange *range2 = spectrumFrequencyRangesBoard->GetFrequencyRangeFromIndex(1);
		FrequencyRange *range3 = spectrumFrequencyRangesBoard->GetFrequencyRangeFromIndex(2);

		if (spectrumFrequencyRangesBoard->count>1)
		{
			if (range1->frames <= range2->frames && range1->frames <= range3->frames)
				spectrumAnalyzer.currentScanningFrequencyRange.Set(range1);
			else
				if (range2->frames <= range1->frames && range2->frames <= range3->frames)
					spectrumAnalyzer.currentScanningFrequencyRange.Set(range2);
				else
					if (range3->frames <= range1->frames && range3->frames <= range2->frames)
						spectrumAnalyzer.currentScanningFrequencyRange.Set(range3);
					else
						spectrumAnalyzer.currentScanningFrequencyRange.Set(range1);
		}
		else
			spectrumAnalyzer.currentScanningFrequencyRange.Set(range1);


		spectrumAnalyzer.requiredFramesPerBandwidthScan = 10;
	}
	else
	{	
		spectrumAnalyzer.currentScanningFrequencyRange.Set(scanningRange.lower, scanningRange.upper);

		spectrumAnalyzer.requiredFramesPerBandwidthScan = 1;

		if (detectionMode == DetectionMode::Sessions)
		{
			FFTSpectrumBuffer* undeterminedAndNearBuffer = spectrumAnalyzer.GetFFTSpectrumBuffer(ReceivingDataMode::NearAndUndetermined);

			undeterminedAndNearBuffer->ClearData();

			spectrumAnalyzer.GetFFTSpectrumBuffer(0)->TransferDataToFFTSpectrumBuffer(undeterminedAndNearBuffer);
			spectrumAnalyzer.GetFFTSpectrumBuffer(2)->TransferDataToFFTSpectrumBuffer(undeterminedAndNearBuffer);

			if (undeterminedAndNearBuffer->GetFrameCountForRange() > requiredFramesForSessions && spectrumAnalyzer.GetFFTSpectrumBuffer(1)->GetFrameCountForRange() > requiredFramesForSessions)
			{
				AddPointsToLeaderboard(spectrumFrequencyRangesBoard, leaderboardFrequencyRanges);

				spectrumAnalyzer.GetFFTSpectrumBuffer(0)->TransferDataToFFTSpectrumBuffer(SessionsBufferNear);
				spectrumAnalyzer.GetFFTSpectrumBuffer(1)->TransferDataToFFTSpectrumBuffer(SessionsBufferFar);

				ClearAllData();

				sessionCount++;
			}
		}
	}
}

void NearFarDataAnalyzer::AddPointsToLeaderboard(FrequencyRanges *spectrumBoard, FrequencyRanges *leaderboard)
{
	uint32_t points;

	FrequencyRange *range;

	for (int i = 0; i < spectrumBoard->count; i++)
	{
		points = spectrumBoard->count - i;

		range = spectrumBoard->GetFrequencyRangeFromIndex(i);

		leaderboard->Add(range->lower, range->upper, points, 1, false);
	}

	leaderboard->QuickSort();
}

void NearFarDataAnalyzer::SetMode(ReceivingDataMode mode)
{	
	if (this->mode == ReceivingDataMode::Paused)
		return;

	this->mode = mode;

	if (mode == ReceivingDataMode::Near)
	{
		if (spectrumAnalyzer.currentFFTBufferIndex == 2)
		{
			FFTSpectrumBuffer *undetermined = spectrumAnalyzer.GetFFTSpectrumBuffer(2);
			undetermined->TransferDataToFFTSpectrumBuffer(spectrumAnalyzer.GetFFTSpectrumBuffer(0));
			undetermined->ClearData();		
		}
		
		dataIsNearTimeStamp = GetTickCount();
	}
	else if (mode == ReceivingDataMode::Far)
	{
		if (spectrumAnalyzer.currentFFTBufferIndex == 2)
		{
			FFTSpectrumBuffer *undetermined = spectrumAnalyzer.GetFFTSpectrumBuffer(2);

			undetermined->TransferDataToFFTSpectrumBuffer(spectrumAnalyzer.GetFFTSpectrumBuffer(1));
			undetermined->ClearData();
		}	
	}	

	spectrumAnalyzer.ChangeBufferIndex(mode);	
}

uint8_t NearFarDataAnalyzer::GetMode()
{
	return spectrumAnalyzer.currentFFTBufferIndex;
}

void ProcessThread(void *param)
{
	NearFarDataAnalyzer* nearFarDataAnalyzer = (NearFarDataAnalyzer*) param;
	
	nearFarDataAnalyzer->Process();

	_endthread();
}

void NearFarDataAnalyzer::Process()
{
	dataIsNearTimeStamp = GetTickCount();

	while (processing)
	{
		if (automatedDetection)
		{
			if (dataIsNearTimeStamp != 0)
			{
				DWORD inactiveDuration = GetTickCount() - dataIsNearTimeStamp;

				if (inactiveDuration > INACTIVE_DURATION_FAR)
				{					
					SetMode(ReceivingDataMode::Far);
				}
				else
					if (inactiveDuration > INACTIVE_DURATION_UNDETERMINED)
					{
						SetMode(ReceivingDataMode::Undetermined);
					}					
			}
		}

		Sleep(1000);
	}
}

void NearFarDataAnalyzer::StartProcessingThread()
{	
	processing = true;

	processingThreadHandle = (HANDLE)_beginthread(ProcessThread, 0, this);
}

void NearFarDataAnalyzer::Pause()
{
	prevReceivingDataMode = mode;

	SetMode(ReceivingDataMode::Paused);
}

void NearFarDataAnalyzer::Resume()
{
	this->mode = prevReceivingDataMode;
	SetMode(prevReceivingDataMode);
}

void NearFarDataAnalyzer::ClearAllData()
{
	spectrumAnalyzer.GetFFTSpectrumBuffer(0)->ClearData();
	spectrumAnalyzer.GetFFTSpectrumBuffer(1)->ClearData();
	spectrumAnalyzer.GetFFTSpectrumBuffer(2)->ClearData();
	spectrumAnalyzer.GetFFTSpectrumBuffer(ReceivingDataMode::NearAndUndetermined)->ClearData();
}

NearFarDataAnalyzer::~NearFarDataAnalyzer()
{
	processing = false;	

	spectrumAnalyzer.Stop();

	delete spectrumFrequencyRangesBoard;
}
