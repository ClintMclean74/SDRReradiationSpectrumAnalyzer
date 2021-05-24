#include <process.h>
#include <direct.h>
#include "NearFarDataAnalyzer.h"
#include "DebuggingUtilities.h"
#include "ArrayUtilities.h"
#include "Graphs.h"

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
}

uint8_t NearFarDataAnalyzer::InitializeNearFarDataAnalyzer(uint32_t bufferSizeInMilliSeconds, uint32_t sampleRate, uint32_t minStartFrequency, uint32_t maxEndFrequency)
{		
	spectrumFrequencyRangesBoard = new FrequencyRanges((maxEndFrequency - minStartFrequency) / DeviceReceiver::SEGMENT_BANDWIDTH);
	transitionFrequencyRangesBoard = new FrequencyRanges((maxEndFrequency - minStartFrequency) / DeviceReceiver::SEGMENT_BANDWIDTH);
	leaderboardFrequencyRanges = new FrequencyRanges((maxEndFrequency - minStartFrequency) / DeviceReceiver::SEGMENT_BANDWIDTH);
	reradiatedFrequencyRanges = new FrequencyRanges((maxEndFrequency - minStartFrequency) / DeviceReceiver::SEGMENT_BANDWIDTH);

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

	strengthFFTBuffers = new FFTSpectrumBuffers(this, EEG_MIN, EEG_MAX, 4, deviceCount, MAX_EEG_SAMPLES, 1);

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

			length = strlen(textBuffer);

			fwrite(textBuffer, sizeof(char), length, pFile);
		}
	}

	fclose(pFile);
}

void NearFarDataAnalyzer::ProcessSequenceFinished()
{
	if (checkingReradiatedFrequencyRanges)
	{		
		FrequencyRange newScanningFrequencyRange;

		newScanningFrequencyRange.Set(reradiatedFrequencyRanges->GetFrequencyRangeFromIndex(checkingReradiatedFrequencyRangeIndex));

		FrequencyRange bandwidthRangeIterator;

		bandwidthRangeIterator.Set(scanningRange.lower, scanningRange.lower + DeviceReceiver::SAMPLE_RATE);

		do
		{
			if (newScanningFrequencyRange.lower >= bandwidthRangeIterator.lower && newScanningFrequencyRange.upper <= bandwidthRangeIterator.upper)
				break;
			bandwidthRangeIterator.Set(bandwidthRangeIterator.lower + DeviceReceiver::SAMPLE_RATE, bandwidthRangeIterator.upper + DeviceReceiver::SAMPLE_RATE);
		} while (bandwidthRangeIterator.lower < scanningRange.upper);

		spectrumAnalyzer.currentScanningFrequencyRange.Set(&bandwidthRangeIterator);

		char textBuffer[255];
		snprintf(textBuffer, sizeof(textBuffer), "Checking Reradiated Frequency: %.3f MHz - %.3f MHz", SignalProcessingUtilities::ConvertToMHz(newScanningFrequencyRange.lower, 3), SignalProcessingUtilities::ConvertToMHz(newScanningFrequencyRange.upper, 3));

		graphs->SetText(1, textBuffer);		
	}
	else
	if (spectrumAnalyzer.currentScanningFrequencyRange.lower == scanningRange.lower && spectrumAnalyzer.currentScanningFrequencyRange.upper == scanningRange.upper)
	{
		spectrumFrequencyRangesBoard->ProcessFFTSpectrumStrengthDifferenceData(spectrumAnalyzer.GetFFTSpectrumBuffer(4));

		FrequencyRange newScanningFrequencyRange;

		uint32_t frequencyIndex = ((float)rand() / RAND_MAX) * (spectrumFrequencyRangesBoard->count < 3 ? spectrumFrequencyRangesBoard->count : 3);

		if (frequencyIndex >= spectrumFrequencyRangesBoard->count)
			frequencyIndex = spectrumFrequencyRangesBoard->count - 1;

		newScanningFrequencyRange.Set(spectrumFrequencyRangesBoard->GetFrequencyRangeFromIndex(frequencyIndex));

		FrequencyRange bandwidthRangeIterator;

		bandwidthRangeIterator.Set(spectrumAnalyzer.currentScanningFrequencyRange.lower, spectrumAnalyzer.currentScanningFrequencyRange.lower + DeviceReceiver::SAMPLE_RATE);

		do
		{
			if (newScanningFrequencyRange.lower >= bandwidthRangeIterator.lower && newScanningFrequencyRange.upper <= bandwidthRangeIterator.upper)
				break;
			bandwidthRangeIterator.Set(bandwidthRangeIterator.lower + DeviceReceiver::SAMPLE_RATE, bandwidthRangeIterator.upper + DeviceReceiver::SAMPLE_RATE);
		} while (bandwidthRangeIterator.lower <  scanningRange.upper);

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
	}
	else if (!addTransition)
	{
		spectrumAnalyzer.currentScanningFrequencyRange.Set(scanningRange.lower, scanningRange.upper);
		spectrumAnalyzer.requiredFramesPerBandwidthScan = 1;

		WriteDataToFile(spectrumFrequencyRangesBoard, dataFileName);

		AddTransitionPointsToLeaderboard(transitionFrequencyRangesBoard, leaderboardFrequencyRanges);

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

					spectrumAnalyzer.deviceReceivers->allSessionsSpectrumRangeGraph->SetData((void *)allSessionsSpectrumBuffer, allSessionsSpectrumBufferSize, 0, true, 0, 0, !spectrumAnalyzer.usePhase);

					allSessionsBufferFar->GetFFTData(allSessionsSpectrumBuffer, allSessionsSpectrumBufferSize, scanningRange.lower, scanningRange.upper, ReceivingDataBufferSpecifier::AveragedBuffer);

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

void NearFarDataAnalyzer::ProcessTransitionsAndSetTransitionGraphsData(Transition *transition)
{
	uint32_t transitionLength;

	uint32_t nearFarSeconds = Transitions::DURATION / 1000 / 2;

	transitionGraph->SetGraphFrequencyRangeText("Transition: %.2f-%.2fMHz range", &transition->range, 1);
	transitionGraph->SetText(2, "%i seconds Far. %i seconds Near", nearFarSeconds, nearFarSeconds);
	transitionGraph->SetText(3, "Near/Far Strength Ratio: %f", transition->strengthForMostRecentTransition);

	SignalProcessingUtilities::Strengths_ID_Time* transitionStrengths = transition->GetStrengthForRangeOverTime(0, 0, &transitionLength);
	transitionGraph->SetMaxResolution(transitionLength);
	transitionGraph->SetData(transitionStrengths, transitionLength, 0, false, 0, 0, false, SignalProcessingUtilities::DataType::STRENGTHS_ID_TIME);
	ArrayUtilities::AverageDataArray(transitionStrengths, transitionLength, 3); //first half and second half average
	transitionGraph->SetData(transitionStrengths, transitionLength, 1, false, 0, 0, false, SignalProcessingUtilities::DataType::STRENGTHS_ID_TIME);

	averageTransitionsGraph->SetGraphFrequencyRangeText("Averaged Transitions: %.2f-%.2fMHz", &transition->range, 1);
	averageTransitionsGraph->SetText(2, "%i seconds Far. %i seconds Near", nearFarSeconds, nearFarSeconds);

	SignalProcessingUtilities::Strengths_ID_Time* averagedTransitionsStrengths = transition->GetAveragedTransitionsStrengthForRangeOverTime(0, 0, &transitionLength);

	averageTransitionsGraph->SetMaxResolution(transitionLength);
	averageTransitionsGraph->SetData(averagedTransitionsStrengths, transitionLength, 0, false, 0, 0, false, SignalProcessingUtilities::DataType::STRENGTHS_ID_TIME);
	ArrayUtilities::AverageDataArray(averagedTransitionsStrengths, transitionLength, 3); //first half and second half average
	averageTransitionsGraph->SetData(averagedTransitionsStrengths, transitionLength, 1, false, 0, 0, false, SignalProcessingUtilities::DataType::STRENGTHS_ID_TIME);

	averageTransitionsGraph->SetText(3, "%i transitions", averagedTransitionsStrengths[0].addCount);
	averageTransitionsGraph->SetText(4, "Near/Far Strength Ratio: %f", transition->averagedStrengthForAllTransitions);

	transitions.GetFrequencyRangesAndTransitionStrengths(transitionFrequencyRangesBoard);

	transitionFrequencyRangesBoard->QuickSort();

	if (!transitionboardStrengthPoints || transitionboardStrengthPointsCount < transitionFrequencyRangesBoard->count)
	{
		transitionboardStrengthPoints = new double[transitionFrequencyRangesBoard->count];

		transitionboardStrengthPointsCount = transitionFrequencyRangesBoard->count;
	}

	transitionFrequencyRangesBoard->QuickSort(QuickSortMode::Frequency);
	transitionFrequencyRangesBoard->GetStrengthValues(transitionboardStrengthPoints);

	transitionboardGraph->SetData((void *)transitionboardStrengthPoints, transitionboardStrengthPointsCount, 0, false, 0, 0, false, SignalProcessingUtilities::DataType::DOUBLE);

	delete[] transitionStrengths;
	delete[] averagedTransitionsStrengths;
}


void NearFarDataAnalyzer::OnReceiverDataProcessed()
{
	FFTSpectrumBuffer* undeterminedAndNearBuffer = spectrumAnalyzer.GetFFTSpectrumBuffer(ReceivingDataMode::NearAndUndetermined);

	undeterminedAndNearBuffer->ClearData();

	spectrumAnalyzer.GetFFTSpectrumBuffer(0)->TransferDataToFFTSpectrumBuffer(undeterminedAndNearBuffer);
	spectrumAnalyzer.GetFFTSpectrumBuffer(2)->TransferDataToFFTSpectrumBuffer(undeterminedAndNearBuffer);

	graphs->SetText(0, "Near Frames: %d Far Frames: %d", undeterminedAndNearBuffer->GetFrameCountForRange(), spectrumAnalyzer.GetFFTSpectrumBuffer(1)->GetFrameCountForRange());

	if (strengthGraph)
	{
		DWORD transitionDurationForMode = Transitions::DURATION / 2;

		uint32_t dataLength = 0;
		DWORD duration = Transitions::DURATION + 100; //plus 100 milliseconds buffer to get buffer slightly larger

		SignalProcessingUtilities::Strengths_ID_Time *strengths = spectrumAnalyzer.fftBandwidthBuffer->GetStrengthForRangeOverTime(spectrumAnalyzer.deviceReceivers->combinedFFTGraphForBandwidth->startDataIndex, spectrumAnalyzer.deviceReceivers->combinedFFTGraphForBandwidth->endDataIndex, &duration, 0, &dataLength, 0);

		if (dataLength > 1)
		{
			strengthGraph->SetMaxResolution(dataLength);
			strengthGraph->SetData(strengths, dataLength, 0, false, 0, 0, false, SignalProcessingUtilities::DataType::STRENGTHS_ID_TIME);

			double *rollingAverage = ArrayUtilities::GetRollingAverage(strengths, dataLength);
			strengthGraph->SetData(&rollingAverage[dataLength / 2], dataLength - dataLength / 2 - dataLength / 10, 1, false, 0, 0, false, SignalProcessingUtilities::DataType::DOUBLE);

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
						
			char *text = spectrumAnalyzer.deviceReceivers->combinedFFTGraphForBandwidth->SetGraphFrequencyRangeText("FFT: %.2f-%.2fMHz", &spectrumAnalyzer.fftBandwidthBuffer->range, 1);
			strengthGraph->SetText(2, &text[5]);
			strengthFFTGraph->SetText(2, &text[5]);
			strengthFFTAveragedGraph->SetText(2, &text[5]);
			
			
			uint32_t dataLength2 = dataLength;
			fftw_complex* strengthBufferComplex = ArrayUtilities::ConvertArrayToComplex(strengths, dataLength2);
			SignalProcessingUtilities::SetQ(strengthBufferComplex, dataLength2, 0);

			if (!fftBuffer || fftBufferLength < dataLength2)
			{
				if (fftBuffer)
					delete[] fftBuffer;

				fftBuffer = new fftw_complex[dataLength2];

				fftBufferLength = dataLength2;
			}

			SignalProcessingUtilities::FFT_COMPLEX_ARRAY(strengthBufferComplex, fftBuffer, dataLength2, false, false);
			
			SignalProcessingUtilities::CalculateMagnitudesAndPhasesForArray(fftBuffer, dataLength2);
			SignalProcessingUtilities::SetQ(fftBuffer, dataLength2, 0);

			FrequencyRange strengthFFTRange(0, dataLength2 / 2);

			strengthFFTBuffers->SetFFTInput(mode, fftBuffer, strengthBufferComplex, dataLength2, 0, &strengthFFTRange, true);
			strengthFFTBuffers->ProcessFFTInput(mode, &strengthFFTRange);
			
			uint32_t graphDataLength = dataLength2 - 1;
			strengthFFTGraph->SetMaxResolution(graphDataLength);			
			strengthFFTGraph->SetData(&fftBuffer[1], graphDataLength, 0, true, 0, 0, true, SignalProcessingUtilities::DataType::FFTW_COMPLEX);
			
			if (mode != ReceivingDataMode::Far)
			{
				if (mode == ReceivingDataMode::Near)
					strengthFFTGraph->SetDataSeriesColor(Graphs::nearColor, 0);
				else
					strengthFFTGraph->SetDataSeriesColor(Graphs::undeterminedColor, 0);
			}
			else
			{
				strengthFFTGraph->SetDataSeriesColor(Graphs::farColor, 0);
			}

			FFTSpectrumBuffer* undeterminedAndNearBuffer = strengthFFTBuffers->GetFFTSpectrumBuffer(ReceivingDataMode::NearAndUndetermined);
			undeterminedAndNearBuffer->ClearData();
			strengthFFTBuffers->GetFFTSpectrumBuffer(0)->TransferDataToFFTSpectrumBuffer(undeterminedAndNearBuffer);
			strengthFFTBuffers->GetFFTSpectrumBuffer(2)->TransferDataToFFTSpectrumBuffer(undeterminedAndNearBuffer);

			strengthFFTAveragedGraph->SetMaxResolution(graphDataLength);

			undeterminedAndNearBuffer->GetFFTData(fftBuffer, dataLength2, 0, dataLength2, ReceivingDataBufferSpecifier::AveragedBuffer);
			strengthFFTAveragedGraph->SetData(&fftBuffer[1], graphDataLength, ReceivingDataMode::Near, true, 0, 0, true, SignalProcessingUtilities::DataType::FFTW_COMPLEX);

			strengthFFTBuffers->GetFFTSpectrumBuffer(ReceivingDataMode::Far)->GetFFTData(fftBuffer, dataLength2, 0, dataLength2, ReceivingDataBufferSpecifier::AveragedBuffer);
			strengthFFTAveragedGraph->SetData(&fftBuffer[1], graphDataLength, ReceivingDataMode::Far, true, 0, 0, true, SignalProcessingUtilities::DataType::FFTW_COMPLEX);
			
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

								if (transition->writes >= REQUIRED_TRANSITION_WRITES_FOR_RERADIATED_FREQUENCIES && transition->averagedStrengthForAllTransitions >= REQUIRED_TRANSITION_STRENGTH_FOR_RERADIATED_FREQUENCIES)
								//if (transition->writes >= 1 && transition->averagedStrengthForAllTransitions >= 0.0)
								{
									uint32_t prevCount = reradiatedFrequencyRanges->count;

									if (prevCount != reradiatedFrequencyRanges->Add(transition->range.lower, transition->range.upper, 1000, 1, false))
									{
										wchar_t textBuffer[1000];

										swprintf(textBuffer, L"Frequency range detected that could be reradiating from you\n\n %.2f MHz - %.2f MHz\n\nRefer to Leaderboard list for detected frequency ranges in gold", SignalProcessingUtilities::ConvertToMHz(transition->range.lower), SignalProcessingUtilities::ConvertToMHz(transition->range.upper));

										MessageBox(nullptr, textBuffer, TEXT("Detected Frequency Range"), MB_ICONINFORMATION | MB_SETFOREGROUND | MB_TOPMOST | MB_SYSTEMMODAL);
									}
								}

								if (!maxStrengthTransition || transition->averagedStrengthForAllTransitions > maxStrengthTransition->averagedStrengthForAllTransitions)
									maxStrengthTransition = transition;

								currentBandwidthRange.Set(currentBandwidthRange.lower + DeviceReceiver::SEGMENT_BANDWIDTH, currentBandwidthRange.upper + DeviceReceiver::SEGMENT_BANDWIDTH);
							} while (currentBandwidthRange.lower < spectrumAnalyzer.fftBandwidthBuffer->range.upper);

							ProcessTransitionsAndSetTransitionGraphsData(maxStrengthTransition);
						}
					}

					addTransition = false;
				}
			}			

			if (strengthBufferComplex)
				delete[] strengthBufferComplex;				

			if (strengths)
				delete[] strengths;			

			if (rollingAverage)
				delete[] rollingAverage;
		}
	}
}

void NearFarDataAnalyzer::AddTransitionPointsToLeaderboard(FrequencyRanges *board, FrequencyRanges *leaderboard)
{
	uint32_t points;

	FrequencyRange *range;

	board->QuickSort();

	for (int i = 0; i < board->count; i++)
	{
		points = board->count - i;

		range = board->GetFrequencyRangeFromIndex(i);

		leaderboard->Add(range->lower, range->upper, points * range->flags[0] * (range->strength - 1), 1, false, range->flags);
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
			FFTSpectrumBuffer *undetermined = spectrumAnalyzer.GetFFTSpectrumBuffer(ReceivingDataMode::Undetermined);
			undetermined->TransferDataToFFTSpectrumBuffer(spectrumAnalyzer.GetFFTSpectrumBuffer(0));
			undetermined->ClearData();		

			undetermined = strengthFFTBuffers->GetFFTSpectrumBuffer(ReceivingDataMode::Undetermined);
			undetermined->TransferDataToFFTSpectrumBuffer(strengthFFTBuffers->GetFFTSpectrumBuffer(ReceivingDataMode::Near));
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

			undetermined = strengthFFTBuffers->GetFFTSpectrumBuffer(ReceivingDataMode::Undetermined);
			undetermined->TransferDataToFFTSpectrumBuffer(strengthFFTBuffers->GetFFTSpectrumBuffer(ReceivingDataMode::Far));
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

				#if !defined(_DEBUG) || defined(RELEASE_SETTINGS)				
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

//double NearFarDataAnalyzer::REQUIRED_TRANSITION_STRENGTH_FOR_RERADIATED_FREQUENCIES = 0;
double NearFarDataAnalyzer::REQUIRED_TRANSITION_STRENGTH_FOR_RERADIATED_FREQUENCIES = 1.01;
//double NearFarDataAnalyzer::REQUIRED_TRANSITION_STRENGTH_FOR_RERADIATED_FREQUENCIES = 1.03;
