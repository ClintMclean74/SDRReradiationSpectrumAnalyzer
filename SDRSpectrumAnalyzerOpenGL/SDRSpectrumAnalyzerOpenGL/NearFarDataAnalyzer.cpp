#include <process.h>
#include "NearFarDataAnalyzer.h"
#include "DebuggingUtilities.h"

NearFarDataAnalyzer::NearFarDataAnalyzer()
{

}

uint8_t NearFarDataAnalyzer::InitializeNearFarDataAnalyzer(uint32_t bufferSizeInMilliSeconds, uint32_t sampleRate, uint32_t minStartFrequency, uint32_t maxEndFrequency)
{	
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
		leaderboardFrequencyRanges->ProcessFFTSpectrumStrengthDifferenceData(spectrumAnalyzer.GetFFTSpectrumBuffer(4));

		spectrumAnalyzer.currentScanningFrequencyRange.Set(leaderboardFrequencyRanges->GetFrequencyRangeFromIndex(0));

		spectrumAnalyzer.requiredFramesPerBandwidthScan = 10;
	}
	else
	{
		spectrumAnalyzer.currentScanningFrequencyRange.Set(scanningRange.lower, scanningRange.upper);

		spectrumAnalyzer.requiredFramesPerBandwidthScan = 1;
	}
}

void NearFarDataAnalyzer::SetMode(ReceivingDataMode mode)
{	
	/*////if (spectrumAnalyzer.currentFFTBufferIndex == 2)
		return;
		*/

	////mode = ReceivingDataMode::Far;

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
		////DebuggingUtilities::DebugPrint("dataIsNearTimeStamp = GetTickCount(); %d\n");

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

	////spectrumAnalyzer.currentFFTBufferIndex = mode;

	spectrumAnalyzer.ChangeBufferIndex(mode);	
}

uint8_t NearFarDataAnalyzer::GetMode()
{
	return spectrumAnalyzer.currentFFTBufferIndex;
}

void ProcessThread(void *param)
{
	NearFarDataAnalyzer* nearFarDataAnalyzer = (NearFarDataAnalyzer*) param;

	//Sleep(10000);

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

				//DebuggingUtilities::DebugPrint("InactiveDuration: %d\n", inactiveDuration);

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
	SetMode(prevReceivingDataMode);
}

/*////uint32_t NearFarDataAnalyzer::GetNearFFTData(double *dataBuffer, uint32_t dataBufferLength, uint32_t startFrequency, uint32_t endFrequency, uint8_t dataMode)
{
	if (dataMode == 0)
	{		
		return spectrumAnalyzer.GetFFTData(dataBuffer, dataBufferLength, spectrumAnalyzer.currentFFTBufferIndex, startFrequency, endFrequency, dataMode);
	}
	else
	{
		FFTSpectrumBuffer* undeterminedAndNearBuffer = spectrumAnalyzer.GetFFTSpectrumBuffer(3);

		undeterminedAndNearBuffer->ClearData();

		spectrumAnalyzer.GetFFTSpectrumBuffer(0)->TransferDataToFFTSpectrumBuffer(undeterminedAndNearBuffer);
		spectrumAnalyzer.GetFFTSpectrumBuffer(2)->TransferDataToFFTSpectrumBuffer(undeterminedAndNearBuffer);


		return spectrumAnalyzer.GetFFTData(dataBuffer, dataBufferLength, 3, startFrequency, endFrequency, dataMode);
	}
}*/

/*////double NearFarDataAnalyzer::GetNearFFTStrengthForRange(uint32_t startFrequency, uint32_t endFrequency, ReceivingDataMode receivingDataMode)
{
	if (receivingDataMode == ReceivingDataMode::Far)
	{
		return spectrumAnalyzer.GetStrengthForRange(startFrequency, endFrequency, receivingDataMode);
	}
	else
	{
		FFTSpectrumBuffer* undeterminedAndNearBuffer = spectrumAnalyzer.GetFFTSpectrumBuffer(3);

		undeterminedAndNearBuffer->ClearData();

		spectrumAnalyzer.GetFFTSpectrumBuffer(0)->TransferDataToFFTSpectrumBuffer(undeterminedAndNearBuffer);
		spectrumAnalyzer.GetFFTSpectrumBuffer(2)->TransferDataToFFTSpectrumBuffer(undeterminedAndNearBuffer);


		return spectrumAnalyzer.GetStrengthForRange(startFrequency, endFrequency, receivingDataMode, 3);
	}
}
*/

NearFarDataAnalyzer::~NearFarDataAnalyzer()
{
	processing = false;	

	spectrumAnalyzer.Stop();

	////Sleep(1000);

	delete leaderboardFrequencyRanges;
}
