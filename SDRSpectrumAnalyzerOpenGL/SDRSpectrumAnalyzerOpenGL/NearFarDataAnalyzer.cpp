#include <process.h>
#include <direct.h>
#include "NearFarDataAnalyzer.h"
#include "DebuggingUtilities.h"
#include "ArrayUtilities.h"

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

	//strcpy(dataFileName, "results.txt");	
}

uint8_t NearFarDataAnalyzer::InitializeNearFarDataAnalyzer(uint32_t bufferSizeInMilliSeconds, uint32_t sampleRate, uint32_t minStartFrequency, uint32_t maxEndFrequency)
{		
	//spectrumFrequencyRangesBoard = new FrequencyRanges((maxEndFrequency - minStartFrequency) / sampleRate);
	//leaderboardFrequencyRanges = new FrequencyRanges((maxEndFrequency- minStartFrequency) / sampleRate);

	spectrumFrequencyRangesBoard = new FrequencyRanges((maxEndFrequency - minStartFrequency) / DeviceReceiver::SEGMENT_BANDWIDTH);
	transitionFrequencyRangesBoard = new FrequencyRanges((maxEndFrequency - minStartFrequency) / DeviceReceiver::SEGMENT_BANDWIDTH);
	leaderboardFrequencyRanges = new FrequencyRanges((maxEndFrequency - minStartFrequency) / DeviceReceiver::SEGMENT_BANDWIDTH);

	//initialize transitionboard frequency range segments, since each segment is otherwise only added when there's a transition
	FrequencyRange transitionSegmentRange(minStartFrequency, minStartFrequency + DeviceReceiver::SEGMENT_BANDWIDTH);
	do
	{
		transitionFrequencyRangesBoard->Add(transitionSegmentRange.lower, transitionSegmentRange.upper);

		transitionSegmentRange.Set(transitionSegmentRange.lower + DeviceReceiver::SEGMENT_BANDWIDTH, transitionSegmentRange.upper + DeviceReceiver::SEGMENT_BANDWIDTH);
	} while (transitionSegmentRange.lower < maxEndFrequency);


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
		//newScanningFrequencyRange.Set(spectrumFrequencyRangesBoard->GetFrequencyRangeFromIndex(0));

		uint32_t frequencyIndex = ((float)rand() / RAND_MAX) * (spectrumFrequencyRangesBoard->count < 3 ? spectrumFrequencyRangesBoard->count : 3);

		if (frequencyIndex >= spectrumFrequencyRangesBoard->count)
			frequencyIndex = spectrumFrequencyRangesBoard->count - 1;

		newScanningFrequencyRange.Set(spectrumFrequencyRangesBoard->GetFrequencyRangeFromIndex(frequencyIndex));

		/*if (spectrumFrequencyRangesBoard->count > 2)
			newScanningFrequencyRange.Set(spectrumFrequencyRangesBoard->GetFrequencyRangeFromIndex(2));
		else if (spectrumFrequencyRangesBoard->count > 1)
			newScanningFrequencyRange.Set(spectrumFrequencyRangesBoard->GetFrequencyRangeFromIndex(1));
		else
			newScanningFrequencyRange.Set(spectrumFrequencyRangesBoard->GetFrequencyRangeFromIndex(0));
			*/

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

		spectrumboardGraph->SetText(1, "Strongest Near vs Far Ranges:");
		spectrumboardGraph->SetGraphFrequencyRangeText("%.2f-%.2fMHz", &centerFrequencyrange, 2);

		//spectrumFrequencyRangesBoard->QuickSort(QuickSortMode::Frames);
	}
	//else if (!addTransition)
	else
	{	
		spectrumAnalyzer.currentScanningFrequencyRange.Set(scanningRange.lower, scanningRange.upper);
		spectrumAnalyzer.requiredFramesPerBandwidthScan = 1;
		
		WriteDataToFile(spectrumFrequencyRangesBoard, dataFileName);

		AddPointsToLeaderboard(transitionFrequencyRangesBoard, leaderboardFrequencyRanges);

		if (detectionMode == DetectionMode::Sessions)
		{
			FFTSpectrumBuffer* undeterminedAndNearBuffer = spectrumAnalyzer.GetFFTSpectrumBuffer(ReceivingDataMode::NearAndUndetermined);

			undeterminedAndNearBuffer->ClearData();
			
			if (spectrumAnalyzer.GetFFTSpectrumBuffer(0)->GetFrameCountForRange() > requiredFramesForSessions && spectrumAnalyzer.GetFFTSpectrumBuffer(1)->GetFrameCountForRange() > requiredFramesForSessions)			
			{				
				AddPointsToLeaderboard(spectrumFrequencyRangesBoard, leaderboardFrequencyRanges);				
				
				if (spectrumAnalyzer.deviceReceivers->allSessionsSpectrumRangeGraph)
				{
					spectrumAnalyzer.deviceReceivers->allSessionsSpectrumRangeGraph->SetText(1, "Averaged Spectrum for");
					spectrumAnalyzer.deviceReceivers->allSessionsSpectrumRangeGraph->SetText(2, "%i Sessions:", sessionCount);
					

					spectrumAnalyzer.GetFFTSpectrumBuffer(0)->TransferDataToFFTSpectrumBuffer(allSessionsBufferNear);
					spectrumAnalyzer.GetFFTSpectrumBuffer(1)->TransferDataToFFTSpectrumBuffer(allSessionsBufferFar);

					allSessionsBufferNear->GetFFTData(allSessionsSpectrumBuffer, allSessionsSpectrumBufferSize, scanningRange.lower, scanningRange.upper, ReceivingDataBufferSpecifier::AveragedBuffer);

					uint32_t verticesCount = spectrumAnalyzer.deviceReceivers->allSessionsSpectrumRangeGraph->verticesCount;

					//if (verticesCount == 0) //write the data twice for first write since it's a 3D graph (not lines) and the data gets set only after every scanning sequence is finished
						//spectrumAnalyzer.deviceReceivers->allSessionsSpectrumRangeGraph->SetData((void *)allSessionsSpectrumBuffer, allSessionsSpectrumBufferSize, 0, true, 0, 0, !spectrumAnalyzer.usePhase);
					spectrumAnalyzer.deviceReceivers->allSessionsSpectrumRangeGraph->SetData((void *)allSessionsSpectrumBuffer, allSessionsSpectrumBufferSize, 0, true, 0, 0, !spectrumAnalyzer.usePhase);

					allSessionsBufferFar->GetFFTData(allSessionsSpectrumBuffer, allSessionsSpectrumBufferSize, scanningRange.lower, scanningRange.upper, ReceivingDataBufferSpecifier::AveragedBuffer);

					//if (verticesCount == 0) //again write the data twice for first write also for the far series since it's a 3D graph (not lines) and the data gets set only after every scanning sequence is finished
						//spectrumAnalyzer.deviceReceivers->allSessionsSpectrumRangeGraph->SetData((void *)allSessionsSpectrumBuffer, allSessionsSpectrumBufferSize, 1, true, 0, 0, !spectrumAnalyzer.usePhase);
					spectrumAnalyzer.deviceReceivers->allSessionsSpectrumRangeGraph->SetData((void *)allSessionsSpectrumBuffer, allSessionsSpectrumBufferSize, 1, true, 0, 0, !spectrumAnalyzer.usePhase);

					spectrumAnalyzer.deviceReceivers->allSessionsSpectrumRangeGraph->SetGraphFrequencyRangeText("%.2f-%.2fMHz", &spectrumAnalyzer.maxFrequencyRange, 3);

					ClearAllData();
				}

				sessionCount++;
			}
		}
		else
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
	}
}

void NearFarDataAnalyzer::OnReceiverDataProcessed()
{
	if (strengthGraph)
	{
		DWORD transitionDurationForMode = Transitions::DURATION / 2;

		uint32_t dataLength = 0;
		DWORD duration = Transitions::DURATION + 100; //plus 100 milliseconds buffer to get buffer slightly larger
		//DWORD duration = Transitions::DURATION + 2000; //plus 2000 milliseconds buffer to get buffer slightly larger

		SignalProcessingUtilities::Strengths_ID_Time *strengths = spectrumAnalyzer.fftBandwidthBuffer->GetStrengthForRangeOverTime(spectrumAnalyzer.deviceReceivers->combinedFFTGraphForBandwidth->startDataIndex, spectrumAnalyzer.deviceReceivers->combinedFFTGraphForBandwidth->endDataIndex, &duration, 0, &dataLength, 0);

		strengthGraph->SetMaxResolution(dataLength);
		strengthGraph->SetData(strengths, dataLength, 0, false, 0, 0, false, SignalProcessingUtilities::DataType::STRENGTHS_ID_TIME);

		double *rollingAverage = ArrayUtilities::GetRollingAverage(strengths, dataLength);
		strengthGraph->SetData(&rollingAverage[dataLength / 2], dataLength - dataLength / 2 - dataLength / 10, 1, false, 0, 0, false, SignalProcessingUtilities::DataType::DOUBLE);

		if (dataLength > 0)
		{
			if (spectrumAnalyzer.currentFFTBufferIndex == ReceivingDataMode::Near)
			{
				strengthGraph->SetText(1, "FFT Range Strength: Near");
				strengthGraph->SetDataSeriesColor(1, 0, 0, 1, 0, spectrumAnalyzer.currentFFTBufferIndex);
			}
			else
				if (spectrumAnalyzer.currentFFTBufferIndex == ReceivingDataMode::Far)
				{
					strengthGraph->SetText(1, "FFT Range Strength: Far");
					strengthGraph->SetDataSeriesColor(0, 1, 0, 1, 0, spectrumAnalyzer.currentFFTBufferIndex);
				}
				else
					if (spectrumAnalyzer.currentFFTBufferIndex == ReceivingDataMode::Undetermined)
					{
						strengthGraph->SetText(1, "FFT Range Strength: Near/Far?");
						strengthGraph->SetDataSeriesColor(1, 1, 0, 1, 0, spectrumAnalyzer.currentFFTBufferIndex);
					}

			strengthGraph->SetGraphFrequencyRangeText("%.2f-%.2fMHz", &spectrumAnalyzer.fftBandwidthBuffer->range, 2, false);

			//transitions.DetermineTransitionStrengths();

			if (addTransition)
			{
				uint32_t nearDataLength = 0;
				uint32_t farDataLength = 0;

				DWORD transitionDurationForMode = Transitions::DURATION / 2;
				DWORD nearDuration;
				DWORD farDuration;

				int i = dataLength - 1;

				uint32_t startTime = strengths[i].time;
				uint32_t currentTime = startTime;

				do
				{
					nearDataLength++;
					i--;
				} while (i > 0 && strengths[i].ID == ReceivingDataMode::Near && strengths[i].range == spectrumAnalyzer.fftBandwidthBuffer->range);

				//if (nearDataLength > 100 && i > 0)
				{
					if (i < 0)
						nearDuration = 0;
					else
						nearDuration = currentTime - strengths[i].time;

					if (nearDuration >= transitionDurationForMode)
					{
						currentTime = strengths[i].time;
						do
						{
							farDataLength++;

							i--;
						} while (i > 0 && strengths[i].ID == ReceivingDataMode::Far && strengths[i].range == spectrumAnalyzer.fftBandwidthBuffer->range && currentTime - strengths[i].time < transitionDurationForMode);

						if (farDataLength)
						{
							if (i < 0)
								farDuration = 0;
							else
								farDuration = currentTime - strengths[i].time;

							if (farDuration >= transitionDurationForMode)
							{
								BandwidthFFTBuffer* transitionBandwidthFFTBuffer = spectrumAnalyzer.fftBandwidthBuffer;

								FrequencyRange currentBandwidthRange;

								currentBandwidthRange.Set(spectrumAnalyzer.fftBandwidthBuffer->range.lower, spectrumAnalyzer.fftBandwidthBuffer->range.lower + DeviceReceiver::SEGMENT_BANDWIDTH);

								Transition* transition;
								Transition* maxStrengthTransition = NULL;

								do
								{
									transition = transitions.Add(transitionBandwidthFFTBuffer, Transitions::DURATION, &currentBandwidthRange, SignalProcessingUtilities::GetDataIndexFromFrequency(currentBandwidthRange.lower, spectrumAnalyzer.fftBandwidthBuffer->range.lower, spectrumAnalyzer.fftBandwidthBuffer->range.upper, 0, DeviceReceiver::FFT_SEGMENT_SAMPLE_COUNT), SignalProcessingUtilities::GetDataIndexFromFrequency(currentBandwidthRange.upper, spectrumAnalyzer.fftBandwidthBuffer->range.lower, spectrumAnalyzer.fftBandwidthBuffer->range.upper, 0, DeviceReceiver::FFT_SEGMENT_SAMPLE_COUNT));

									transition->DetermineTransitionStrength();

									if (!maxStrengthTransition || transition->strength > maxStrengthTransition->strength)
										maxStrengthTransition = transition;

									currentBandwidthRange.Set(currentBandwidthRange.lower + DeviceReceiver::SEGMENT_BANDWIDTH, currentBandwidthRange.upper + DeviceReceiver::SEGMENT_BANDWIDTH);
								} while (currentBandwidthRange.lower < spectrumAnalyzer.fftBandwidthBuffer->range.upper);


								uint32_t transitionLength;

								uint32_t nearFarSeconds = Transitions::DURATION / 1000 / 2;

								transitionGraph->SetGraphFrequencyRangeText("Transition: %.2f-%.2fMHz range", &maxStrengthTransition->range, 1);
								transitionGraph->SetText(2, "%i seconds Far. %i seconds Near", nearFarSeconds, nearFarSeconds);
								
								SignalProcessingUtilities::Strengths_ID_Time* transitionStrengths = maxStrengthTransition->GetStrengthForRangeOverTime(0, 0, &transitionLength);
								transitionGraph->SetMaxResolution(transitionLength);
								transitionGraph->SetData(transitionStrengths, transitionLength, 0, false, 0, 0, false, SignalProcessingUtilities::DataType::STRENGTHS_ID_TIME);
								ArrayUtilities::AverageDataArray(transitionStrengths, transitionLength, 3); //first half and second half average
								transitionGraph->SetData(transitionStrengths, transitionLength, 1, false, 0, 0, false, SignalProcessingUtilities::DataType::STRENGTHS_ID_TIME);

								averageTransitionsGraph->SetGraphFrequencyRangeText("Averaged Transitions: %.2f-%.2fMHz", &maxStrengthTransition->range, 1);
								averageTransitionsGraph->SetText(2, "%i seconds Far. %i seconds Near", nearFarSeconds, nearFarSeconds);

								SignalProcessingUtilities::Strengths_ID_Time* averagedTransitionsStrengths = maxStrengthTransition->GetAveragedTransitionsStrengthForRangeOverTime(0, 0, &transitionLength);

								averageTransitionsGraph->SetMaxResolution(transitionLength);
								averageTransitionsGraph->SetData(averagedTransitionsStrengths, transitionLength, 0, false, 0, 0, false, SignalProcessingUtilities::DataType::STRENGTHS_ID_TIME);
								ArrayUtilities::AverageDataArray(averagedTransitionsStrengths, transitionLength, 3); //first half and second half average
								averageTransitionsGraph->SetData(averagedTransitionsStrengths, transitionLength, 1, false, 0, 0, false, SignalProcessingUtilities::DataType::STRENGTHS_ID_TIME);

								averageTransitionsGraph->SetText(3, "%i transitions", averagedTransitionsStrengths[0].addCount);

								transitions.GetFrequencyRangesAndTransitionStrengths(transitionFrequencyRangesBoard);

								transitionFrequencyRangesBoard->QuickSort();

								//AddPointsToLeaderboard(transitionFrequencyRangesBoard, leaderboardFrequencyRanges);

								if (!transitionboardStrengthPoints || transitionboardStrengthPointsCount < transitionFrequencyRangesBoard->count)
								{
									transitionboardStrengthPoints = new double[transitionFrequencyRangesBoard->count];

									transitionboardStrengthPointsCount = transitionFrequencyRangesBoard->count;
								}

								transitionFrequencyRangesBoard->QuickSort(QuickSortMode::Frequency);
								transitionFrequencyRangesBoard->GetStrengthValues(transitionboardStrengthPoints);

								transitionboardGraph->SetData((void *)transitionboardStrengthPoints, transitionboardStrengthPointsCount, 0, false, 0, 0, false, SignalProcessingUtilities::DataType::DOUBLE);

								addTransition = false;
							}
						}
					}
				}
			}

			if (strengths)
				delete strengths;

			if (rollingAverage)
				delete rollingAverage;
		}
	}
}
void NearFarDataAnalyzer::AddPointsToLeaderboard(FrequencyRanges *board, FrequencyRanges *leaderboard)
{
	uint32_t points;

	FrequencyRange *range;

	board->QuickSort();

	for (int i = 0; i < board->count; i++)
	{
		points = board->count - i;

		range = board->GetFrequencyRangeFromIndex(i);

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
			FFTSpectrumBuffer *undetermined = spectrumAnalyzer.GetFFTSpectrumBuffer(spectrumAnalyzer.currentFFTBufferIndex);
			undetermined->TransferDataToFFTSpectrumBuffer(spectrumAnalyzer.GetFFTSpectrumBuffer(0));
			undetermined->ClearData();		
		}
		
		dataIsNearTimeStamp = GetTickCount();
	}
	else if (mode == ReceivingDataMode::Far)
	{
		if (spectrumAnalyzer.currentFFTBufferIndex == 2)
		{
			FFTSpectrumBuffer *undetermined = spectrumAnalyzer.GetFFTSpectrumBuffer(spectrumAnalyzer.currentFFTBufferIndex);

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
