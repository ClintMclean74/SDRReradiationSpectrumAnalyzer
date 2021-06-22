#include "NearFarDataAnalyzer.h"
#include "DebuggingUtilities.h"
#include "ArrayUtilities.h"
#include "Graphs.h"
#include "ThreadUtilities.h"
#include "WindowsToLinuxUtilities.h"

#ifdef _WIN32
    #include <direct.h>
#else
    #include <X11/extensions/scrnsaver.h>
    #include <sys/stat.h>
#endif

NearFarDataAnalyzer::NearFarDataAnalyzer()
{
    glutWindowID = -1;
    glutMsgWindowID = -1;
    glutWindowState = -1;

    processingFarDataMessageBoxEventState = CLOSED;

    ignoreNextWindowStateChange = false;

    dataIsNear = true;

    processing = false;

    mode = Near;
    allSessionsSpectrumBuffer = NULL;
    allSessionsSpectrumBufferSize = 0;

    fftBuffer = NULL;
    fftBufferLength = 0;

    addTransition = false;

    automatedDetection = false;
    detectionMode = Normal;
    sessionCount = 1;
    requiredFramesForSessions = 1000;

    checkingReradiatedFrequencyRangeIndex = 0;
    checkingReradiatedFrequencyRanges = false;

    spectrumboardStrengthPoints = NULL;
    spectrumboardStrengthPointsCount = 0;

    leaderboardStrengthPoints = NULL;
    leaderboardStrengthPointsCount = 0;

    transitionboardStrengthPoints = NULL;
    transitionboardStrengthPointsCount = 0;

    strcpy(dataFolder, "results");

    #ifdef _WIN32
        mkdir(dataFolder);
    #else
        const int dir_err = mkdir(dataFolder, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
        if (-1 == dir_err)
        {
            //printf("Error creating directory!n");
        }
    #endif

    sprintf(dataFileName, "results/results%i.txt\0", GetTime());

    graphs = NULL;
}

uint8_t NearFarDataAnalyzer::InitializeNearFarDataAnalyzer(uint32_t bufferSizeInMilliSeconds, uint32_t sampleRate, uint32_t minStartFrequency, uint32_t maxEndFrequency)
{
	spectrumFrequencyRangesBoard = new FrequencyRanges((maxEndFrequency - minStartFrequency) / DeviceReceiver::SEGMENT_BANDWIDTH);
	transitionFrequencyRangesBoard = new FrequencyRanges((maxEndFrequency - minStartFrequency) / DeviceReceiver::SEGMENT_BANDWIDTH);
	leaderboardFrequencyRanges = new FrequencyRanges((maxEndFrequency - minStartFrequency) / DeviceReceiver::SEGMENT_BANDWIDTH);
	reradiatedFrequencyRanges = new FrequencyRanges((maxEndFrequency - minStartFrequency) / DeviceReceiver::SEGMENT_BANDWIDTH);

	FrequencyRange transitionSegmentRange(minStartFrequency, minStartFrequency + DeviceReceiver::SEGMENT_BANDWIDTH);
	do
	{
        transitionFrequencyRangesBoard->Add(transitionSegmentRange.lower, transitionSegmentRange.upper);

        transitionSegmentRange.Set(transitionSegmentRange.lower + DeviceReceiver::SEGMENT_BANDWIDTH, transitionSegmentRange.upper + DeviceReceiver::SEGMENT_BANDWIDTH);
	} while (transitionSegmentRange.lower < maxEndFrequency);


	scanningRange.Set(minStartFrequency, maxEndFrequency);

	spectrumAnalyzer.SetCalculateFFTDifferenceBuffer(true);

	uint8_t deviceCount = spectrumAnalyzer.InitializeSpectrumAnalyzer(bufferSizeInMilliSeconds, sampleRate, minStartFrequency, maxEndFrequency);

	SetMode(Near);

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

		graphs->SetText(4, textBuffer);
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


		spectrumFrequencyRangesBoard->QuickSort(Frequency);

		if (!spectrumboardStrengthPoints || spectrumboardStrengthPointsCount < spectrumFrequencyRangesBoard->count)
		{
			spectrumboardStrengthPoints = new double[spectrumFrequencyRangesBoard->count];

			spectrumboardStrengthPointsCount = spectrumFrequencyRangesBoard->count;
		}

		spectrumFrequencyRangesBoard->GetStrengthValues(spectrumboardStrengthPoints);

		spectrumboardGraph->SetData((void *)spectrumboardStrengthPoints, spectrumboardStrengthPointsCount, 0, false, 0, 0, false, SignalProcessingUtilities::DOUBLE);


		FrequencyRange centerFrequencyrange(spectrumAnalyzer.maxFrequencyRange);

		centerFrequencyrange.lower += 500000;
		centerFrequencyrange.upper -= 500000;

		spectrumboardGraph->SetText(1, "Strongest Near vs Far Ranges:");
		spectrumboardGraph->SetGraphFrequencyRangeText("%.2f-%.2fMHz", &centerFrequencyrange, 2);
	}
	else if (!addTransition) //process data for entire frequency range graphs and ranking boards
	{
		WriteDataToFile(spectrumFrequencyRangesBoard, dataFileName);

		AddTransitionPointsToLeaderboard(transitionFrequencyRangesBoard, leaderboardFrequencyRanges);

		if (detectionMode == Sessions)
		{
			FFTSpectrumBuffer* undeterminedAndNearBuffer = spectrumAnalyzer.GetFFTSpectrumBuffer(NearAndUndetermined);

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

					allSessionsBufferNear->GetFFTData(allSessionsSpectrumBuffer, allSessionsSpectrumBufferSize, scanningRange.lower, scanningRange.upper, AveragedBuffer);

					uint32_t verticesCount = spectrumAnalyzer.deviceReceivers->allSessionsSpectrumRangeGraph->verticesCount;

					spectrumAnalyzer.deviceReceivers->allSessionsSpectrumRangeGraph->SetData((void *)allSessionsSpectrumBuffer, allSessionsSpectrumBufferSize, 0, true, 0, 0, !spectrumAnalyzer.usePhase);

					allSessionsBufferFar->GetFFTData(allSessionsSpectrumBuffer, allSessionsSpectrumBufferSize, scanningRange.lower, scanningRange.upper, AveragedBuffer);

					spectrumAnalyzer.deviceReceivers->allSessionsSpectrumRangeGraph->SetData((void *)allSessionsSpectrumBuffer, allSessionsSpectrumBufferSize, 1, true, 0, 0, !spectrumAnalyzer.usePhase);

					spectrumAnalyzer.deviceReceivers->allSessionsSpectrumRangeGraph->SetGraphFrequencyRangeText("%.2f-%.2fMHz", &spectrumAnalyzer.maxFrequencyRange, 3);

					ClearAllData();
				}

				sessionCount++;
			}
		}
		else
			AddPointsToLeaderboard(spectrumFrequencyRangesBoard, leaderboardFrequencyRanges);


		leaderboardFrequencyRanges->QuickSort(Frequency);

		if (!leaderboardStrengthPoints || leaderboardStrengthPointsCount < leaderboardFrequencyRanges->count)
		{
			leaderboardStrengthPoints = new double[leaderboardFrequencyRanges->count];

			leaderboardStrengthPointsCount = leaderboardFrequencyRanges->count;
		}

		leaderboardFrequencyRanges->GetStrengthValues(leaderboardStrengthPoints);
		leaderboardGraph->SetData((void *)leaderboardStrengthPoints, leaderboardStrengthPointsCount, 0, false, 0, 0, false, SignalProcessingUtilities::DOUBLE);

		FrequencyRange centerFrequencyrange(spectrumAnalyzer.maxFrequencyRange);
		centerFrequencyrange.lower += 500000;
		centerFrequencyrange.upper -= 500000;
		leaderboardGraph->SetGraphFrequencyRangeText("Leaderboard: %.2f-%.2fMHz", &centerFrequencyrange, 1);

		//set currentScanningFrequencyRange to entire scanning range to activate zooming in to SOI frequency range
		spectrumAnalyzer.currentScanningFrequencyRange.Set(scanningRange.lower, scanningRange.upper);
		spectrumAnalyzer.requiredFramesPerBandwidthScan = 1;
	}
}

void NearFarDataAnalyzer::ProcessTransitionsAndSetTransitionGraphsData(Transition *transition)
{
	uint32_t transitionLength;
	char textBuffer[255];

	uint32_t nearFarSeconds = Transitions::DURATION / 1000 / 2;

	transitionGraph->SetGraphFrequencyRangeText("Transition: %.2f-%.2fMHz range", &transition->range, 1);
	transitionGraph->SetText(2, "%i seconds Far. %i seconds Near", nearFarSeconds, nearFarSeconds);
	transitionGraph->SetText(3, "Near/Far Strength Ratio: %f", transition->strengthForMostRecentTransition);

	SignalProcessingUtilities::Strengths_ID_Time* transitionStrengths = transition->GetStrengthForRangeOverTime(0, 0, &transitionLength);
	transitionGraph->SetMaxResolution(transitionLength);
	transitionGraph->SetData(transitionStrengths, transitionLength, 0, false, 0, 0, false, SignalProcessingUtilities::STRENGTHS_ID_TIME);
	ArrayUtilities::AverageDataArray(transitionStrengths, transitionLength, 3); //first half and second half average
	transitionGraph->SetData(transitionStrengths, transitionLength, 1, false, 0, 0, false, SignalProcessingUtilities::STRENGTHS_ID_TIME);

	averageTransitionsGraph->SetGraphFrequencyRangeText("Averaged Transitions: %.2f-%.2fMHz", &transition->range, 1);

	sprintf(textBuffer, "%i seconds Far. %i seconds Near", nearFarSeconds, nearFarSeconds);
	averageTransitionsGraph->SetText(2, textBuffer);


	SignalProcessingUtilities::Strengths_ID_Time* averagedTransitionsStrengths = transition->GetAveragedTransitionsStrengthForRangeOverTime(0, 0, &transitionLength);

	averageTransitionsGraph->SetMaxResolution(transitionLength);
	averageTransitionsGraph->SetData(averagedTransitionsStrengths, transitionLength, 0, false, 0, 0, false, SignalProcessingUtilities::STRENGTHS_ID_TIME);
	ArrayUtilities::AverageDataArray(averagedTransitionsStrengths, transitionLength, 3); //first half and second half average
	averageTransitionsGraph->SetData(averagedTransitionsStrengths, transitionLength, 1, false, 0, 0, false, SignalProcessingUtilities::STRENGTHS_ID_TIME);


	sprintf(textBuffer, "%lu transitions\0", (unsigned long) averagedTransitionsStrengths[0].addCount);
	averageTransitionsGraph->SetText(3, textBuffer);
	averageTransitionsGraph->SetText(4, "Near/Far Strength Ratio: %f", transition->averagedStrengthForAllTransitions);

	transitions.GetFrequencyRangesAndTransitionStrengths(transitionFrequencyRangesBoard);

	transitionFrequencyRangesBoard->QuickSort();

	if (!transitionboardStrengthPoints || transitionboardStrengthPointsCount < transitionFrequencyRangesBoard->count)
	{
		transitionboardStrengthPoints = new double[transitionFrequencyRangesBoard->count];

		transitionboardStrengthPointsCount = transitionFrequencyRangesBoard->count;
	}

	transitionFrequencyRangesBoard->QuickSort(Frequency);
	transitionFrequencyRangesBoard->GetStrengthValues(transitionboardStrengthPoints);

	transitionboardGraph->SetData((void *)transitionboardStrengthPoints, transitionboardStrengthPointsCount, 0, false, 0, 0, false, SignalProcessingUtilities::DOUBLE);

	delete[] transitionStrengths;
	delete[] averagedTransitionsStrengths;
}


void NearFarDataAnalyzer::OnReceiverDataProcessed()
{
	FFTSpectrumBuffer* undeterminedAndNearBuffer = spectrumAnalyzer.GetFFTSpectrumBuffer(NearAndUndetermined);

	undeterminedAndNearBuffer->ClearData();

	spectrumAnalyzer.GetFFTSpectrumBuffer(0)->TransferDataToFFTSpectrumBuffer(undeterminedAndNearBuffer);
	spectrumAnalyzer.GetFFTSpectrumBuffer(2)->TransferDataToFFTSpectrumBuffer(undeterminedAndNearBuffer);


	char textBuffer[255];

	sprintf(textBuffer, "Near Frames: %d Far Frames: %d", undeterminedAndNearBuffer->GetFrameCountForRange(), spectrumAnalyzer.GetFFTSpectrumBuffer(1)->GetFrameCountForRange());
	graphs->SetText(5, textBuffer);

	if (strengthGraph)
	{
		DWORD transitionDurationForMode = Transitions::DURATION / 2;

		uint32_t dataLength = 0;
		DWORD duration = Transitions::DURATION + 100; //plus 100 milliseconds buffer to get buffer slightly larger

		SignalProcessingUtilities::Strengths_ID_Time *strengths = spectrumAnalyzer.fftBandwidthBuffer->GetStrengthForRangeOverTime(spectrumAnalyzer.deviceReceivers->combinedFFTGraphForBandwidth->startDataIndex, spectrumAnalyzer.deviceReceivers->combinedFFTGraphForBandwidth->endDataIndex, &duration, 0, &dataLength, 0);

		if (dataLength > 1)
		{
			strengthGraph->SetMaxResolution(dataLength);
			strengthGraph->SetData(strengths, dataLength, 0, false, 0, 0, false, SignalProcessingUtilities::STRENGTHS_ID_TIME);

			double *rollingAverage = ArrayUtilities::GetRollingAverage(strengths, dataLength);
			strengthGraph->SetData(&rollingAverage[dataLength / 2], dataLength - dataLength / 2 - dataLength / 10, 1, false, 0, 0, false, SignalProcessingUtilities::DOUBLE);

			if (spectrumAnalyzer.currentFFTBufferIndex == Near)
			{
				strengthGraph->SetText(1, "FFT Range Strength: Near");
				strengthGraph->SetDataSeriesColor(1, 0, 0, 1, 0, spectrumAnalyzer.currentFFTBufferIndex);
			}
			else
				if (spectrumAnalyzer.currentFFTBufferIndex == Far)
				{
					strengthGraph->SetText(1, "FFT Range Strength: Far");
					strengthGraph->SetDataSeriesColor(0, 1, 0, 1, 0, spectrumAnalyzer.currentFFTBufferIndex);
				}
				else
					if (spectrumAnalyzer.currentFFTBufferIndex == Undetermined)
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


			strengthFFTBuffers->SetFFTInput(spectrumAnalyzer.currentFFTBufferIndex, fftBuffer, strengthBufferComplex, dataLength2, 0, &strengthFFTRange, true);
			strengthFFTBuffers->ProcessFFTInput(spectrumAnalyzer.currentFFTBufferIndex, &strengthFFTRange);

			uint32_t graphDataLength = dataLength2 - 1;
			strengthFFTGraph->SetMaxResolution(graphDataLength);
			strengthFFTGraph->SetData(&fftBuffer[1], graphDataLength, 0, true, 0, 0, true, SignalProcessingUtilities::FFTW_COMPLEX);

			if (mode != Far)
			{
				if (mode == Near)
					strengthFFTGraph->SetDataSeriesColor(Graphs::nearColor, 0);
				else
					strengthFFTGraph->SetDataSeriesColor(Graphs::undeterminedColor, 0);
			}
			else
			{
				strengthFFTGraph->SetDataSeriesColor(Graphs::farColor, 0);
			}

			FFTSpectrumBuffer* undeterminedAndNearBuffer = strengthFFTBuffers->GetFFTSpectrumBuffer(NearAndUndetermined);
			undeterminedAndNearBuffer->ClearData();
			strengthFFTBuffers->GetFFTSpectrumBuffer(0)->TransferDataToFFTSpectrumBuffer(undeterminedAndNearBuffer);
			strengthFFTBuffers->GetFFTSpectrumBuffer(2)->TransferDataToFFTSpectrumBuffer(undeterminedAndNearBuffer);

			strengthFFTAveragedGraph->SetMaxResolution(graphDataLength);

			undeterminedAndNearBuffer->GetFFTData(fftBuffer, dataLength2, 0, dataLength2, AveragedBuffer);
			strengthFFTAveragedGraph->SetData(&fftBuffer[1], graphDataLength, Near, true, 0, 0, true, SignalProcessingUtilities::FFTW_COMPLEX);

			strengthFFTBuffers->GetFFTSpectrumBuffer(Far)->GetFFTData(fftBuffer, dataLength2, 0, dataLength2, AveragedBuffer);
			strengthFFTAveragedGraph->SetData(&fftBuffer[1], graphDataLength, Far, true, 0, 0, true, SignalProcessingUtilities::FFTW_COMPLEX);

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
				} while (i > 0 && strengths[i].ID == Near && strengths[i].range == spectrumAnalyzer.fftBandwidthBuffer->range);

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
					} while (i > 0 && strengths[i].ID == Far && strengths[i].range == spectrumAnalyzer.fftBandwidthBuffer->range && currentTime - strengths[i].time < transitionDurationForMode);

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
								{
									uint32_t prevCount = reradiatedFrequencyRanges->count;

									if (prevCount != reradiatedFrequencyRanges->Add(transition->range.lower, transition->range.upper, 1000, 1, false))
									{
										char textBuffer[1000];

										sprintf(textBuffer, "Frequency range detected that could be reradiating from you:");
										graphs->SetText(0, textBuffer);

										sprintf(textBuffer, "%.2f MHz - %.2f MHz", SignalProcessingUtilities::ConvertToMHz(transition->range.lower), SignalProcessingUtilities::ConvertToMHz(transition->range.upper));
										graphs->SetText(1, textBuffer);

										sprintf(textBuffer, "Refer to Leaderboard list (SHIFT 'l') for detected frequency ranges(gold)", SignalProcessingUtilities::ConvertToMHz(transition->range.lower), SignalProcessingUtilities::ConvertToMHz(transition->range.upper));
										graphs->SetText(2, textBuffer);
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
	if (this->mode == Paused)
		return;

	this->mode = mode;

	if (mode == Near)
	{
		if (spectrumAnalyzer.currentFFTBufferIndex == 2)
		{
			FFTSpectrumBuffer *undetermined = spectrumAnalyzer.GetFFTSpectrumBuffer(Undetermined);
			undetermined->TransferDataToFFTSpectrumBuffer(spectrumAnalyzer.GetFFTSpectrumBuffer(0));
			undetermined->ClearData();

			undetermined = strengthFFTBuffers->GetFFTSpectrumBuffer(Undetermined);
			undetermined->TransferDataToFFTSpectrumBuffer(strengthFFTBuffers->GetFFTSpectrumBuffer(Near));
			undetermined->ClearData();
		}
	}
	else if (mode == Far)
	{
		if (spectrumAnalyzer.currentFFTBufferIndex == 2)
		{
			FFTSpectrumBuffer *undetermined = spectrumAnalyzer.GetFFTSpectrumBuffer(spectrumAnalyzer.currentFFTBufferIndex);

			undetermined->TransferDataToFFTSpectrumBuffer(spectrumAnalyzer.GetFFTSpectrumBuffer(1));
			undetermined->ClearData();

			undetermined = strengthFFTBuffers->GetFFTSpectrumBuffer(Undetermined);
			undetermined->TransferDataToFFTSpectrumBuffer(strengthFFTBuffers->GetFFTSpectrumBuffer(Far));
			undetermined->ClearData();
		}
	}

	spectrumAnalyzer.ChangeBufferIndex(mode);
}

uint8_t NearFarDataAnalyzer::GetMode()
{
	return spectrumAnalyzer.currentFFTBufferIndex;
}

#ifdef _WIN32
    void ProcessThread(void *param)
#else
    void* ProcessThread(void *param)
#endif
{
	NearFarDataAnalyzer* nearFarDataAnalyzer = (NearFarDataAnalyzer*) param;

	nearFarDataAnalyzer->Process();

	ThreadUtilities::CloseThread();
}

#ifdef _WIN32
    void ProcessingFarDataMessageBox(void *param)
#else
    void* ProcessingFarDataMessageBox(void *param)
#endif
{
    NearFarDataAnalyzer* nearFarDataAnalyzer = (NearFarDataAnalyzer*) param;

	char textBuffer[255];

	uint32_t inactiveSecondsMSGTime = NearFarDataAnalyzer::INACTIVE_NOTIFICATION_MSG_TIME / 1000;

	sprintf(textBuffer, "Processing data as far series in %i seconds\n\nPress a key/button or move the mouse if near", inactiveSecondsMSGTime);
	nearFarDataAnalyzer->graphs->SetText(3, textBuffer);

	glutSetWindow(nearFarDataAnalyzer->glutMsgWindowID);

    glutShowWindow();

    glutSetWindow(nearFarDataAnalyzer->glutWindowID);

	ThreadUtilities::CloseThread();
}

void NearFarDataAnalyzer::SetModeNear()
{
	if (automatedDetection)
	{
		if (GetMode() == Far)
			addTransition = true;

		SetMode(Near);

        processingFarDataMessageBoxEventState = CLOSE;
	}
}

void NearFarDataAnalyzer::Process()
{
    DWORD inactiveDuration;

    #ifdef _WIN32
        LASTINPUTINFO info;
        info.cbSize=sizeof(LASTINPUTINFO);
    #else
        XScreenSaverInfo *info = XScreenSaverAllocInfo();
        Display *display = XOpenDisplay(0);
    #endif // _WIN32

	while (processing)
	{
		if (automatedDetection)
		{
		    #ifdef _WIN32
                GetLastInputInfo(&info);
                inactiveDuration = GetTime() - info.dwTime;
            #else
                XScreenSaverQueryInfo(display, DefaultRootWindow(display), info);
                inactiveDuration = info->idle;
		    #endif // _WIN32

            if (INACTIVE_DURATION_FAR > INACTIVE_NOTIFICATION_MSG_TIME && inactiveDuration > INACTIVE_DURATION_FAR - INACTIVE_NOTIFICATION_MSG_TIME && inactiveDuration < INACTIVE_DURATION_FAR)
			{
                if (glutGetWindow() != glutMsgWindowID)
                {
                    processingFarDataMessageBoxEventState = LAUNCH;
                }
            }

            if (inactiveDuration > INACTIVE_DURATION_FAR)
			{
                processingFarDataMessageBoxEventState = CLOSE;

				SetMode(Far);
            }
			else
                if (inactiveDuration > INACTIVE_DURATION_UNDETERMINED)
				{
                    SetMode(Undetermined);
                }
				else
                    if (inactiveDuration < INACTIVE_DURATION_UNDETERMINED)
                    {
                        if (inactiveDuration < INACTIVE_DURATION_FAR - INACTIVE_NOTIFICATION_MSG_TIME)
                        {
                            processingFarDataMessageBoxEventState = CLOSE;
                        }

                        SetModeNear();
                    }
		}

		Timeout(1000);
	}
}

void NearFarDataAnalyzer::StartProcessingThread()
{
	processing = true;

	processingThreadHandle = ThreadUtilities::CreateThread(ProcessThread, (void *)this);
}

void NearFarDataAnalyzer::Pause()
{
	prevReceivingDataMode = mode;

	SetMode(Paused);
}

void NearFarDataAnalyzer::Resume()
{
	this->mode = prevReceivingDataMode;
	SetMode(prevReceivingDataMode);
}

void NearFarDataAnalyzer::CloseProcessingFarDataMessageBox()
{
    processingFarDataMessageBoxEventState = CLOSED;
    glutSetWindow(glutMsgWindowID);
    glutHideWindow();

    glutSetWindow(glutWindowID);
}

void NearFarDataAnalyzer::ClearAllData()
{
	spectrumAnalyzer.GetFFTSpectrumBuffer(0)->ClearData();
	spectrumAnalyzer.GetFFTSpectrumBuffer(1)->ClearData();
	spectrumAnalyzer.GetFFTSpectrumBuffer(2)->ClearData();
	spectrumAnalyzer.GetFFTSpectrumBuffer(NearAndUndetermined)->ClearData();
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
