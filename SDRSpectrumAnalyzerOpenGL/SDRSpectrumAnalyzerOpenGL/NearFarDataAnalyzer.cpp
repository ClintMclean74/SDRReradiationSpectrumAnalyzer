#include <process.h>
#include <direct.h>
#include "NearFarDataAnalyzer.h"
#include "DebuggingUtilities.h"

NearFarDataAnalyzer::NearFarDataAnalyzer()
{	
	HANDLE hFind;	
	strcpy(dataFolder, "results/*");	
	WIN32_FIND_DATAA ffd;
	
	int counter = 0;
	hFind = ::FindFirstFileA(dataFolder, &ffd);
	if (hFind != INVALID_HANDLE_VALUE)
	{
		do
		{
			counter++;						
		} while (::FindNextFileA(hFind, &ffd) == TRUE);

		::FindClose(hFind);
	}

	if (counter > 2)
		counter -= 2;

	counter++;

	dataFolder[strlen(dataFolder) - 1] = 0;
	mkdir(dataFolder); //creates if not already existing
	
	strcpy(dataFileName, dataFolder);	
	strcat(dataFileName, "results");

	char strCount[255];
	strcat(dataFileName, itoa(counter, strCount, 10));
	strcat(dataFileName, ".txt");
	
	//DWORD error = GetLastError();
	
}

uint8_t NearFarDataAnalyzer::InitializeNearFarDataAnalyzer(uint32_t bufferSizeInMilliSeconds, uint32_t sampleRate, uint32_t minStartFrequency, uint32_t maxEndFrequency)
{		
	//spectrumFrequencyRangesBoard = new FrequencyRanges((maxEndFrequency - minStartFrequency) / sampleRate);
	//leaderboardFrequencyRanges = new FrequencyRanges((maxEndFrequency- minStartFrequency) / sampleRate);

	spectrumFrequencyRangesBoard = new FrequencyRanges((maxEndFrequency - minStartFrequency) / DeviceReceiver::SEGMENT_BANDWIDTH);
	leaderboardFrequencyRanges = new FrequencyRanges((maxEndFrequency - minStartFrequency) / DeviceReceiver::SEGMENT_BANDWIDTH);

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

void NearFarDataAnalyzer::WriteDataToFile(FrequencyRanges* frequencyRanges, const char* fileName)
{
	FILE * pFile;
	char textBuffer[255];
	pFile = fopen(fileName, "w+");

	FrequencyRange *frequencyRange;
	uint32_t length;

	for (uint8_t i = 0; i < frequencyRanges->count; i++)
	{
		frequencyRange = frequencyRanges->GetFrequencyRangeFromIndex(i);
		if (frequencyRange)
		{
			snprintf(textBuffer, sizeof(textBuffer), "%.4d %.4d %lf %i \n", frequencyRange->lower, frequencyRange->upper, frequencyRange->strength, frequencyRange->frames);
			//snprintf(textBuffer, sizeof(textBuffer), "   %.4d", frequencyRange->lower);			

			length = strlen(textBuffer);

			fwrite(textBuffer, sizeof(char), length, pFile);
		}
	}

	fclose(pFile);
}

void NearFarDataAnalyzer::ProcessSequenceFinished()
{
	if (spectrumAnalyzer.currentScanningFrequencyRange.lower == scanningRange.lower && spectrumAnalyzer.currentScanningFrequencyRange.upper == scanningRange.upper)
	{			
		spectrumFrequencyRangesBoard->ProcessFFTSpectrumStrengthDifferenceData(spectrumAnalyzer.GetFFTSpectrumBuffer(4));		

		
		FrequencyRange newScanningFrequencyRange;
		if (spectrumFrequencyRangesBoard->count > 2)
			newScanningFrequencyRange.Set(spectrumFrequencyRangesBoard->GetFrequencyRangeFromIndex(2));
		else if (spectrumFrequencyRangesBoard->count > 1)
			newScanningFrequencyRange.Set(spectrumFrequencyRangesBoard->GetFrequencyRangeFromIndex(1));
		else
			newScanningFrequencyRange.Set(spectrumFrequencyRangesBoard->GetFrequencyRangeFromIndex(0));

		FrequencyRange bandwidthRangeIterator;

		bandwidthRangeIterator.Set(spectrumAnalyzer.currentScanningFrequencyRange.lower, spectrumAnalyzer.currentScanningFrequencyRange.lower + DeviceReceiver::SAMPLE_RATE);

		do
		{
			if (newScanningFrequencyRange.lower >= bandwidthRangeIterator.lower && newScanningFrequencyRange.upper <= bandwidthRangeIterator.upper)
				break;
			bandwidthRangeIterator.Set(bandwidthRangeIterator.lower + DeviceReceiver::SAMPLE_RATE, bandwidthRangeIterator.upper + DeviceReceiver::SAMPLE_RATE);
		} while (bandwidthRangeIterator.lower < bandwidthRangeIterator.upper);

		spectrumAnalyzer.currentScanningFrequencyRange.Set(&bandwidthRangeIterator);

		spectrumAnalyzer.requiredFramesPerBandwidthScan = 10;
		
		
		
		spectrumFrequencyRangesBoard->QuickSort(QuickSortMode::Frequency);

		if (!spectrumboardStrengthPoints || spectrumboardStrengthPointsCount < spectrumFrequencyRangesBoard->count)
		{
			spectrumboardStrengthPoints = new double[spectrumFrequencyRangesBoard->count];

			spectrumboardStrengthPointsCount = spectrumFrequencyRangesBoard->count;
		}

		spectrumFrequencyRangesBoard->GetStrengthValues(spectrumboardStrengthPoints);

		spectrumboardGraph->SetData((void *)spectrumboardStrengthPoints, spectrumboardStrengthPointsCount, 0, false, 0, 0, false, SignalProcessingUtilities::DataType::DOUBLE);
		

		



		FrequencyRange centerFrequencyrange(spectrumAnalyzer.maxFrequencyRange);

		centerFrequencyrange.lower += 500000;
		centerFrequencyrange.upper -= 500000;

		spectrumboardGraph->SetGraphFrequencyRangeText("Spectrumboard: %.2f-%.2fMHz", &centerFrequencyrange, 1);

		//spectrumFrequencyRangesBoard->QuickSort(QuickSortMode::Frames);
	}
	else
	{	
		spectrumAnalyzer.currentScanningFrequencyRange.Set(scanningRange.lower, scanningRange.upper);

		spectrumAnalyzer.requiredFramesPerBandwidthScan = 1;
		
		WriteDataToFile(spectrumFrequencyRangesBoard, dataFileName);

		if (detectionMode == DetectionMode::Sessions)
		{
			FFTSpectrumBuffer* undeterminedAndNearBuffer = spectrumAnalyzer.GetFFTSpectrumBuffer(ReceivingDataMode::NearAndUndetermined);

			undeterminedAndNearBuffer->ClearData();
			
			if (spectrumAnalyzer.GetFFTSpectrumBuffer(0)->GetFrameCountForRange() > requiredFramesForSessions && spectrumAnalyzer.GetFFTSpectrumBuffer(1)->GetFrameCountForRange() > requiredFramesForSessions)			
			{				
				AddPointsToLeaderboard(spectrumFrequencyRangesBoard, leaderboardFrequencyRanges);

				leaderboardFrequencyRanges->QuickSort(QuickSortMode::Frequency);

				if (!leaderboardStrengthPoints || leaderboardStrengthPointsCount < leaderboardFrequencyRanges->count)
				{
					leaderboardStrengthPoints = new double[leaderboardFrequencyRanges->count];

					leaderboardStrengthPointsCount = leaderboardFrequencyRanges->count;
				}
				
				leaderboardFrequencyRanges->GetStrengthValues(leaderboardStrengthPoints);

				leaderboardGraph->SetData((void *)leaderboardStrengthPoints, leaderboardStrengthPointsCount, 0, false, 0, 0, false, SignalProcessingUtilities::DataType::DOUBLE);				

				FrequencyRange centerFrequencyrange(spectrumAnalyzer.maxFrequencyRange);

				centerFrequencyrange.lower += 500000;
				centerFrequencyrange.upper -= 500000;

				leaderboardGraph->SetGraphFrequencyRangeText("Leaderboard: %.2f-%.2fMHz", &centerFrequencyrange, 1);

				if (spectrumAnalyzer.deviceReceivers->allSessionsSpectrumRangeGraph)
				{
					spectrumAnalyzer.deviceReceivers->allSessionsSpectrumRangeGraph->SetText(1, "Averaged Spectrum for");
					spectrumAnalyzer.deviceReceivers->allSessionsSpectrumRangeGraph->SetText(2, "%i Sessions:", sessionCount);
					

					spectrumAnalyzer.GetFFTSpectrumBuffer(0)->TransferDataToFFTSpectrumBuffer(allSessionsBufferNear);
					spectrumAnalyzer.GetFFTSpectrumBuffer(1)->TransferDataToFFTSpectrumBuffer(allSessionsBufferFar);

					allSessionsBufferNear->GetFFTData(allSessionsSpectrumBuffer, allSessionsSpectrumBufferSize, scanningRange.lower, scanningRange.upper, ReceivingDataBufferSpecifier::AveragedBuffer);

					uint32_t verticesCount = spectrumAnalyzer.deviceReceivers->allSessionsSpectrumRangeGraph->verticesCount;

					if (verticesCount == 0) //write the data twice for first write since it's a 3D graph (not lines) and the data gets set only after every scanning sequence is finished
						spectrumAnalyzer.deviceReceivers->allSessionsSpectrumRangeGraph->SetData((void *)allSessionsSpectrumBuffer, allSessionsSpectrumBufferSize, 0, true, 0, 0, !spectrumAnalyzer.usePhase);
					spectrumAnalyzer.deviceReceivers->allSessionsSpectrumRangeGraph->SetData((void *)allSessionsSpectrumBuffer, allSessionsSpectrumBufferSize, 0, true, 0, 0, !spectrumAnalyzer.usePhase);

					allSessionsBufferFar->GetFFTData(allSessionsSpectrumBuffer, allSessionsSpectrumBufferSize, scanningRange.lower, scanningRange.upper, ReceivingDataBufferSpecifier::AveragedBuffer);

					if (verticesCount == 0) //again write the data twice for first write also for the far series since it's a 3D graph (not lines) and the data gets set only after every scanning sequence is finished
						spectrumAnalyzer.deviceReceivers->allSessionsSpectrumRangeGraph->SetData((void *)allSessionsSpectrumBuffer, allSessionsSpectrumBufferSize, 1, true, 0, 0, !spectrumAnalyzer.usePhase);
					spectrumAnalyzer.deviceReceivers->allSessionsSpectrumRangeGraph->SetData((void *)allSessionsSpectrumBuffer, allSessionsSpectrumBufferSize, 1, true, 0, 0, !spectrumAnalyzer.usePhase);

					spectrumAnalyzer.deviceReceivers->allSessionsSpectrumRangeGraph->SetGraphFrequencyRangeText("%.2f-%.2fMHz", &spectrumAnalyzer.maxFrequencyRange, 3);

					//char textBuffer[255];
					//sprintf(textBuffer, "Averaged Spectrum for %i Sessions:", sessionCount);
					//spectrumAnalyzer.deviceReceivers->allSessionsSpectrumRangeGraph->SetText(1, textBuffer);

					ClearAllData();
				}

				sessionCount++;

				//if (sessionCount > 0)
				{
					/*FILE * pFile;
					char buffer[] = { 'x' , 'y' , 'z' };
					pFile = fopen("results.txt", "w+");
					fwrite(buffer, sizeof(char), sizeof(buffer), pFile);
					fclose(pFile);
					*/
					

					/*FILE *pFile = fopen("results.txt", "w+");

					printf("wrote %d\n", fwrite("asdf", 4, 1, pFile));

					fclose(pFile);
					*/
				}				
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

	//leaderboard->QuickSort();
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

				#if !defined(_DEBUG)				
				if (!DebuggingUtilities::DEBUGGING && INACTIVE_DURATION_FAR > 10000)
				{
					if (INACTIVE_DURATION_FAR > INACTIVE_NOTIFICATION_MSG_TIME && inactiveDuration > INACTIVE_DURATION_FAR - INACTIVE_NOTIFICATION_MSG_TIME && inactiveDuration < INACTIVE_DURATION_FAR)
					{
						HWND hWnd = ::FindWindow(NULL, L"Reradiation Spectrum Analyzer");
						if (!hWnd)
						{
							uint32_t time = INACTIVE_NOTIFICATION_MSG_TIME;
							_beginthread(ProcessingFarDataMessageBox, 0, (void *)&time);
						}
					}
				}
				#endif
								
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
	bool result = SetThreadPriority(ProcessThread, THREAD_PRIORITY_HIGHEST);
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
