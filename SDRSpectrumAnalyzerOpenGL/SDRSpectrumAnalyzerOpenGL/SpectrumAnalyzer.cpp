#include <process.h>
#include "SpectrumAnalyzer.h"
#include "Sound.h"

SpectrumAnalyzer::SpectrumAnalyzer()
{	
}

uint8_t SpectrumAnalyzer::InitializeSpectrumAnalyzer(uint32_t bufferSizeInMilliSeconds, uint32_t sampleRate, uint32_t minStartFrequency, uint32_t maxEndFrequency)
{
	/*////
	x = &SpectrumAnalyzer::StartReceivingData;

	(this->*(this->x))();
	*/

	maxFrequencyRange.Set(minStartFrequency, maxEndFrequency);

	deviceReceivers = new DeviceReceivers(this, bufferSizeInMilliSeconds, sampleRate);

	////int deviceIDs[] = { 3, 4 };
	////int deviceIDs[] = { 1, 2 };
	////int deviceIDs[] = { 2, 3 };

	////int deviceIDs[] = { 3, 4 };

	int deviceIDs[] = { 1};
	////int deviceIDs[] = { 4};

	deviceReceivers->InitializeDevices(deviceIDs);

	fftSpectrumBuffers = new FFTSpectrumBuffers(this, minStartFrequency, maxEndFrequency, 4, deviceReceivers->count);	

	return deviceReceivers->initializedDevices;
}

void PlaySoundThread(void *param)
{
	Sound *sound = (Sound *) param;
	Beep(sound->frequency, sound->duration);

	delete sound;
}

void SpectrumAnalyzer::PlaySound(DWORD frequency, DWORD duration)
{
	if (sound && playSoundThread == NULL)
	{
		Sound *sound = new Sound(frequency, duration);

		playSoundThread = (HANDLE)_beginthread(PlaySoundThread, 0, (void *) sound);

		playSoundThread = NULL;
	}
}

void SpectrumAnalyzer::SetCurrentCenterFrequency(uint32_t centerFrequency)
{
	deviceReceivers->SetCurrentCenterFrequency(centerFrequency);
}

void SpectrumAnalyzer::SetRequiredFramesPerBandwidthScan(uint32_t frames)
{
	requiredFramesPerBandwidthScan = frames;	
}

void SpectrumAnalyzer::SetRequiredScanningSequences(uint32_t frames)
{
	requiredScanningSequences = frames;
}

void SpectrumAnalyzer::SetCalculateFFTDifferenceBuffer(bool value)
{
	calculateFFTDifferenceBuffer = value;
}

void SpectrumAnalyzer::ChangeBufferIndex(int8_t mode)
{
	scheduledFFTBufferIndex = mode;
}

void SpectrumAnalyzer::StartReceivingData()
{		
	SetCurrentCenterFrequency(fftSpectrumBuffers->frequencyRange.lower + DeviceReceiver::SAMPLE_RATE/2);

	deviceReceivers->StartReceivingData();

	////deviceReceivers->Synchronize();
}

void ScanFrequencyRangeThread(void *param)
{
	SpectrumAnalyzer* spectrumAnalyzer = (SpectrumAnalyzer*)param;

	//Sleep(10000);
	spectrumAnalyzer->Scan();
	
	_endthread();
}

void SpectrumAnalyzer::Scan()
{
	FrequencyRange currentBandwidthRange;

	if (spectrumBuffer == NULL)
	{
		spectrumBufferSize = DeviceReceiver::FFT_SEGMENT_SAMPLE_COUNT * (maxFrequencyRange.length / (double)DeviceReceiver::SAMPLE_RATE);

		spectrumBuffer = new double[spectrumBufferSize * 2 * sizeof(double)];
	}

	////scanning = false;
	while (scanning)
	{		
		for (int j = 0; j < requiredScanningSequences; j++)
		{
			currentBandwidthRange.Set(currentScanningFrequencyRange.lower, currentScanningFrequencyRange.lower + DeviceReceiver::SAMPLE_RATE);
			if (deviceReceivers->spectrumRangeGraph)
				deviceReceivers->spectrumRangeGraph->SetGraphXRange(maxFrequencyRange.lower, maxFrequencyRange.upper);


			/*deviceReceivers->fftGraphForDeviceRange->SetGraphXRange(currentScanningFrequencyRange.lower, currentScanningFrequencyRange.upper);
			deviceReceivers->fftGraphForDevicesRange->SetGraphXRange(currentScanningFrequencyRange.lower, currentScanningFrequencyRange.upper);
			deviceReceivers->fftAverageGraphForDeviceRange->SetGraphXRange(currentScanningFrequencyRange.lower, currentScanningFrequencyRange.upper);
			deviceReceivers->fftAverageGraphStrengthsForDeviceRange->SetGraphXRange(currentScanningFrequencyRange.lower, currentScanningFrequencyRange.upper);
			*/




			do
			{
				deviceReceivers->SetCurrentCenterFrequency(currentBandwidthRange.centerFrequency);

				if (deviceReceivers->fftGraphForDeviceRange)
				////deviceReceivers->spectrumRangeGraph->SetGraphXRange(currentBandwidthRange.lower, currentBandwidthRange.upper);
					deviceReceivers->fftGraphForDeviceRange->SetGraphXRange(currentBandwidthRange.lower, currentBandwidthRange.upper);
				
				if (deviceReceivers->fftGraphForDevicesRange)
					deviceReceivers->fftGraphForDevicesRange->SetGraphXRange(currentBandwidthRange.lower, currentBandwidthRange.upper);
				
				if (deviceReceivers->fftAverageGraphForDeviceRange)
					deviceReceivers->fftAverageGraphForDeviceRange->SetGraphXRange(currentBandwidthRange.lower, currentBandwidthRange.upper);

				if (deviceReceivers->fftAverageGraphStrengthsForDeviceRange)
					deviceReceivers->fftAverageGraphStrengthsForDeviceRange->SetGraphXRange(currentBandwidthRange.lower, currentBandwidthRange.upper);

				//Sleep(10000);
				Sleep(1000);

				/*////for (int i = 0; i < requiredFramesPerBandwidthScan; i++)
				{					
					deviceReceivers->TransferDataToFFTSpectrumBuffer(fftSpectrumBuffers, currentFFTBufferIndex);					
				}

				if (calculateFFTDifferenceBuffer)
					fftSpectrumBuffers->CalculateFFTDifferenceBuffer(0, 1);
					*/

				if (deviceReceivers->spectrumRangeGraph)
				//if (false)
				{					
					GetFFTData(spectrumBuffer, spectrumBufferSize, 0, maxFrequencyRange.lower, maxFrequencyRange.upper, ReceivingDataBufferSpecifier::AveragedBuffer);
					deviceReceivers->spectrumRangeGraph->SetData((void *)spectrumBuffer, spectrumBufferSize, 0, true, 0, 0, true);

					GetFFTData(spectrumBuffer, spectrumBufferSize, 1, maxFrequencyRange.lower, maxFrequencyRange.upper, ReceivingDataBufferSpecifier::AveragedBuffer);
					deviceReceivers->spectrumRangeGraph->SetData((void *)spectrumBuffer, spectrumBufferSize, 1, true, 0, 0, true);


					/*////uint32_t startFrequency = SignalProcessingUtilities::GetFrequencyFromDataIndex(deviceReceivers->spectrumRangeGraph->startDataIndex, 0, deviceReceivers->spectrumRangeGraph->GetPointsCount(), maxFrequencyRange.lower, maxFrequencyRange.upper);

					uint32_t endFrequency;

					if (deviceReceivers->spectrumRangeGraph->endDataIndex == 0)
						endFrequency = maxFrequencyRange.upper;
					else
					{
						endFrequency = SignalProcessingUtilities::GetFrequencyFromDataIndex(deviceReceivers->spectrumRangeGraph->endDataIndex, 0, deviceReceivers->spectrumRangeGraph->GetPointsCount(), maxFrequencyRange.lower, maxFrequencyRange.upper);
					}

					deviceReceivers->spectrumRangeGraph->SetGraphLabelValuesXAxis(SignalProcessingUtilities::ConvertToMHz(startFrequency), SignalProcessingUtilities::ConvertToMHz(endFrequency));
					*/
				}

				currentBandwidthRange.Set(currentBandwidthRange.lower + DeviceReceiver::SAMPLE_RATE, currentBandwidthRange.upper + DeviceReceiver::SAMPLE_RATE);
			} while (currentBandwidthRange.lower < currentScanningFrequencyRange.upper);
		}				

		sequenceFinishedFunction();		
	}
}

void SpectrumAnalyzer::SetSequenceFinishedFunction(void(*func)())
{
	sequenceFinishedFunction = func;	
}

void SpectrumAnalyzer::LaunchScanningFrequencyRange(FrequencyRange frequencyRange)
{
	currentScanningFrequencyRange = frequencyRange;
	scanning = true;

	scanFrequencyRangeThreadHandle = (HANDLE)_beginthread(ScanFrequencyRangeThread, 0, this);
}

bool SpectrumAnalyzer::SetFFTInput(fftw_complex* fftBuffer, FrequencyRange* inputFrequencyRange, uint8_t* samples, uint32_t sampleCount, uint8_t deviceID)
{	
	return fftSpectrumBuffers->SetFFTInput(currentFFTBufferIndex, fftBuffer, samples, sampleCount, deviceID, inputFrequencyRange, deviceID == 0);
}

bool SpectrumAnalyzer::SetFFTInput(fftw_complex* fftBuffer, FrequencyRange* inputFrequencyRange, fftw_complex* samples, uint32_t sampleCount, uint8_t deviceID)
{
	return fftSpectrumBuffers->SetFFTInput(currentFFTBufferIndex, fftBuffer, samples, sampleCount, deviceID, inputFrequencyRange, deviceID == 0);
}

bool SpectrumAnalyzer::ProcessReceiverData(FrequencyRange* inputFrequencyRange, bool useRatios)
{		
	if (!deviceReceivers->Correlated() && deviceReceivers->count > 1)
	{		
		////if (!deviceReceivers->generatingNoise)
		if (deviceReceivers->synchronizing)
			deviceReceivers->GenerateNoise(1);

		deviceReceivers->SynchronizeData(GetFFTSpectrumBuffer(currentFFTBufferIndex)->GetSampleDataForDevice(0), GetFFTSpectrumBuffer(currentFFTBufferIndex)->GetSampleDataForDevice(1));
	}
	else
	{
		if (deviceReceivers->generatingNoise)
			deviceReceivers->GenerateNoise(0);
	}	


	return fftSpectrumBuffers->ProcessFFTInput(currentFFTBufferIndex, inputFrequencyRange, useRatios, usePhase);
}

FFTSpectrumBuffer* SpectrumAnalyzer::GetFFTSpectrumBuffer(int fftSpectrumBufferIndex)
{
	return fftSpectrumBuffers->GetFFTSpectrumBuffer(fftSpectrumBufferIndex);	
}

uint32_t SpectrumAnalyzer::GetDataForDevice(uint8_t *dataBuffer, uint8_t deviceIndex)
{
	return deviceReceivers->GetDataForDevice(dataBuffer, deviceIndex);
}

uint32_t SpectrumAnalyzer::GetDataForDevice(double *dataBuffer, uint8_t deviceIndex)
{
	return deviceReceivers->GetDataForDevice(dataBuffer, deviceIndex);
}

uint32_t SpectrumAnalyzer::GetFFTData(double *dataBuffer, unsigned int dataBufferLength, int fftSpectrumBufferIndex, int startFrequency, int endFrequency, ReceivingDataBufferSpecifier receivingDataBufferSpecifier)
{
	FFTSpectrumBuffer* fftBuffer = fftSpectrumBuffers->GetFFTSpectrumBuffer(fftSpectrumBufferIndex);

	if (fftBuffer)
		return fftBuffer->GetFFTData(dataBuffer, dataBufferLength, startFrequency, endFrequency, receivingDataBufferSpecifier);
	else
		return NULL;
}

uint32_t SpectrumAnalyzer::GetFFTData(fftw_complex *dataBuffer, unsigned int dataBufferLength, int fftSpectrumBufferIndex, int startFrequency, int endFrequency, ReceivingDataBufferSpecifier receivingDataBufferSpecifier)
{
	FFTSpectrumBuffer* fftBuffer;
	
	if (fftSpectrumBufferIndex == -1)
		fftSpectrumBufferIndex = currentFFTBufferIndex;
	
	fftBuffer = fftSpectrumBuffers->GetFFTSpectrumBuffer(fftSpectrumBufferIndex);

	////fftBuffer->ClearData();

	if (fftBuffer)
		return fftBuffer->GetFFTData(dataBuffer, dataBufferLength, startFrequency, endFrequency, receivingDataBufferSpecifier);
	else
		return NULL;
}


uint32_t SpectrumAnalyzer::GetBinCountForFrequencyRange()
{
	return fftSpectrumBuffers->GetBinCountForFrequencyRange();
}

uint32_t SpectrumAnalyzer::GetFrameCountForRange(uint8_t fftSpectrumBufferIndex, uint32_t startFrequency, uint32_t endFrequency)
{
	return fftSpectrumBuffers->GetFFTSpectrumBuffer(fftSpectrumBufferIndex)->GetFrameCountForRange(startFrequency, endFrequency);
}

void SpectrumAnalyzer::GetDeviceCurrentFrequencyRange(uint32_t deviceIndex, uint32_t* startFrequency, uint32_t* endFrequency)
{
	deviceReceivers->GetDeviceCurrentFrequencyRange(deviceIndex, startFrequency, endFrequency);
}

void SpectrumAnalyzer::GetCurrentScanningRange(uint32_t* startFrequency, uint32_t* endFrequency)
{
	*startFrequency = currentScanningFrequencyRange.lower;
	*endFrequency = currentScanningFrequencyRange.upper;
}

double SpectrumAnalyzer::GetStrengthForRange(uint32_t startFrequency, uint32_t endFrequency, ReceivingDataBufferSpecifier dataBuffer, int32_t fftSpectrumBufferIndex)
{
	if (fftSpectrumBufferIndex == -1)
		fftSpectrumBufferIndex = currentFFTBufferIndex;

	return fftSpectrumBuffers->GetFFTSpectrumBuffer(fftSpectrumBufferIndex)->GetStrengthForRange(startFrequency, endFrequency, dataBuffer, useRatios, usePhase);
}

void SpectrumAnalyzer::Stop()
{
	scanning = false;

	deviceReceivers->GenerateNoise(0);

	requiredScanningSequences = 0;
	requiredFramesPerBandwidthScan = 0;

	////Sleep(2000);

	delete deviceReceivers;
}

SpectrumAnalyzer::~SpectrumAnalyzer()
{
}
