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

	allSessionsSpectrumBufferSize = DeviceReceiver::FFT_SEGMENT_SAMPLE_COUNT * (scanningRange.length / (double)DeviceReceiver::SAMPLE_RATE);
	allSessionsSpectrumBuffer = new double[allSessionsSpectrumBufferSize * 2 * sizeof(double)];
	
	allSessionsBufferNear = new FFTSpectrumBuffer(NULL, &scanningRange, deviceCount);
	allSessionsBufferFar = new FFTSpectrumBuffer(NULL, &scanningRange, deviceCount);

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
			
			if (spectrumAnalyzer.GetFFTSpectrumBuffer(0)->GetFrameCountForRange() > requiredFramesForSessions && spectrumAnalyzer.GetFFTSpectrumBuffer(1)->GetFrameCountForRange() > requiredFramesForSessions)			
			{
				AddPointsToLeaderboard(spectrumFrequencyRangesBoard, leaderboardFrequencyRanges);

				if (spectrumAnalyzer.deviceReceivers->allSessionsSpectrumRangeGraph)
				{
					spectrumAnalyzer.GetFFTSpectrumBuffer(0)->TransferDataToFFTSpectrumBuffer(allSessionsBufferNear);
					spectrumAnalyzer.GetFFTSpectrumBuffer(1)->TransferDataToFFTSpectrumBuffer(allSessionsBufferFar);

					allSessionsBufferNear->GetFFTData(allSessionsSpectrumBuffer, allSessionsSpectrumBufferSize, scanningRange.lower, scanningRange.upper, ReceivingDataBufferSpecifier::AveragedBuffer);
					spectrumAnalyzer.deviceReceivers->allSessionsSpectrumRangeGraph->SetData((void *)allSessionsSpectrumBuffer, allSessionsSpectrumBufferSize, 0, true, 0, 0, !spectrumAnalyzer.usePhase);

					allSessionsBufferFar->GetFFTData(allSessionsSpectrumBuffer, allSessionsSpectrumBufferSize, scanningRange.lower, scanningRange.upper, ReceivingDataBufferSpecifier::AveragedBuffer);
					spectrumAnalyzer.deviceReceivers->allSessionsSpectrumRangeGraph->SetData((void *)allSessionsSpectrumBuffer, allSessionsSpectrumBufferSize, 1, true, 0, 0, !spectrumAnalyzer.usePhase);

					ClearAllData();
				}

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
		HWND hWnd = ::FindWindow(NULL, L"Reradiation Spectrum Analyzer");
		if (hWnd)
		{
			::PostMessage(hWnd, WM_CLOSE, 0, 0);
		}

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

void ProcessingFarDataMessageBox(void *time)
{
	wchar_t textBuffer[255];
	
	swprintf(textBuffer, L"Processing data as far series in %i seconds\n\nPress a key/button or move the mouse if near", *((uint32_t *) time) / 1000);

	MessageBox(nullptr, textBuffer, TEXT("Reradiation Spectrum Analyzer"), MB_ICONINFORMATION | MB_SETFOREGROUND | MB_TOPMOST | MB_SYSTEMMODAL);
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

				if (inactiveDuration > INACTIVE_DURATION_FAR - INACTIVE_NOTIFICATION_MSG_TIME && inactiveDuration < INACTIVE_DURATION_FAR)
				{
					HWND hWnd = ::FindWindow(NULL, L"Reradiation Spectrum Analyzer");
					if (!hWnd)
					{						
						uint32_t time = INACTIVE_NOTIFICATION_MSG_TIME;
						_beginthread(ProcessingFarDataMessageBox, 0, (void *) &time);
					}
				}
								
				if (inactiveDuration > INACTIVE_DURATION_FAR)
				{					
					HWND hWnd = ::FindWindow(NULL, L"Reradiation Spectrum Analyzer");
					if (hWnd)
					{
						::PostMessage(hWnd, WM_CLOSE, 0, 0);
					}

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
