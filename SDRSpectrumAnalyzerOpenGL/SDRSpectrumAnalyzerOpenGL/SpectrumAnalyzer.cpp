#ifdef _DEBUG #ifdef _DEBUG 
 #define _ITERATOR_DEBUG_LEVEL 2 
#endif #endif

#include "..\..\XmlRpcC4Win1.0.15\TimXml\TimXmlRpc.h"
#include <process.h>
#include "SpectrumAnalyzer.h"
#include "Sound.h"
#include "GNU_Radio_Utilities.h"

SpectrumAnalyzer::SpectrumAnalyzer()
{	
}

uint8_t SpectrumAnalyzer::InitializeSpectrumAnalyzer(uint32_t bufferSizeInMilliSeconds, uint32_t sampleRate, uint32_t minStartFrequency, uint32_t maxEndFrequency)
{	
	DeviceReceiver::SAMPLE_RATE = sampleRate;

	XmlRpcClient connection(GNU_Radio_Utilities::GNU_RADIO_XMLRPC_SERVER_ADDRESS);
	connection.setIgnoreCertificateAuthority();
	XmlRpcValue args, result;
	
	if (!connection.execute("get_flow_graph_ID", args, result))
	{
		//std::cout << connection.getError();
		std::cout << "GNU Radio Device: Not connected\n";
		std::cout << "Run the provided GNU Radio Flowgraph to connect GNU Radio compatible devices\n\n";
	}

	if (strcmp(result, "GNURADIODEVICE") == 0)	
	{
		DeviceReceiver::RECEIVING_GNU_DATA = true;		

		std::cout << "GNU Radio Device: Connected\n";

		args[0] = (int)(DeviceReceiver::SAMPLE_RATE / 1000);
		if (!connection.execute("set_samp_rate", args, result))
		{
			std::cout << connection.getError();
		}
		else
		{			
			std::cout << "Sample rate: " << SignalProcessingUtilities::ConvertToMHz(sampleRate, 3) << " MHz\n";
		}
	}
	else
		DeviceReceiver::RECEIVING_GNU_DATA = false;

	DeviceReceiver::FFT_BUFF_LENGTH_FOR_DEVICE_BANDWIDTH = DeviceReceiver::SAMPLE_RATE / DeviceReceiver::SEGMENT_BANDWIDTH * DeviceReceiver::FFT_SEGMENT_BUFF_LENGTH_FOR_SEGMENT_BANDWIDTH;

	maxFrequencyRange.Set(minStartFrequency, maxEndFrequency);

	deviceReceivers = new DeviceReceivers(this, bufferSizeInMilliSeconds, sampleRate);

	//int deviceIDs[] = { 1, 2 };	
	int deviceIDs[] = { -1 };

	deviceReceivers->InitializeDevices(deviceIDs);

	//fftBandwidthBuffer = new FFTSpectrumBuffer(this, new FrequencyRange(), deviceReceivers->count);

	fftBandwidthBuffer = new BandwidthFFTBuffer(0, 0, BandwidthFFTBuffer::FFT_ARRAYS_COUNT);

	fftSpectrumBuffers = new FFTSpectrumBuffers(this, minStartFrequency, maxEndFrequency, 4, deviceReceivers->count);	
	
	std::cout << "\nFrequency range: " << SignalProcessingUtilities::ConvertToMHz(minStartFrequency, 3) << " MHz to " << SignalProcessingUtilities::ConvertToMHz(maxEndFrequency, 3) << "MHz \n";

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

void SpectrumAnalyzer::SetGain(int gain)
{
	deviceReceivers->SetGain(gain);
}

void SpectrumAnalyzer::SetCurrentCenterFrequency(uint32_t centerFrequency)
{	
	if (DeviceReceiver::RECEIVING_GNU_DATA)
	{
		//XmlRpcClient connection("http://10.0.0.4:12345");
		XmlRpcClient connection(GNU_Radio_Utilities::GNU_RADIO_XMLRPC_SERVER_ADDRESS);
		
		connection.setIgnoreCertificateAuthority();
		
		XmlRpcValue args, result;

		args[0] = (int)(centerFrequency/1000);

		if (!connection.execute("set_freq", args, result))
		{
			////std::cout << connection.getError();
			std::cout << "Connecting to GNU Radio....";
		}
		else
		{
			std::cout << "Frequency set to: " << SignalProcessingUtilities::ConvertToMHz(centerFrequency, 3) << "\n";
		}

		
		/*DeviceReceiver::SAMPLE_RATE *= 2;

		if (DeviceReceiver::SAMPLE_RATE >= 20480000)
			DeviceReceiver::SAMPLE_RATE = 20480000;
			
		args[0] = (int)(DeviceReceiver::SAMPLE_RATE / 1000);
		if (!connection.execute("set_samp_rate", args, result))
		{
			std::cout << connection.getError();
		}
		else
		{			
			//DeviceReceiver::FFT_BUFF_LENGTH_FOR_DEVICE_BANDWIDTH = samp_rate / 1024000;
			DeviceReceiver::FFT_BUFF_LENGTH_FOR_DEVICE_BANDWIDTH = DeviceReceiver::SAMPLE_RATE / DeviceReceiver::SEGMENT_BANDWIDTH * DeviceReceiver::FFT_SEGMENT_BUFF_LENGTH_FOR_SEGMENT_BANDWIDTH;

			if (DeviceReceiver::SAMPLE_RATE >= 20480000)
				DeviceReceiver::SAMPLE_RATE = 512000;
		}
		*/		

		/*args[0] = 0;

		if (!connection.execute("get_samp_rate", NULL, result))
		{
			std::cout << connection.getError();
		}*/
	}
	
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

	if (deviceReceivers->count > 1)
		deviceReceivers->Synchronize();
}

void ScanFrequencyRangeThread(void *param)
{
	SpectrumAnalyzer* spectrumAnalyzer = (SpectrumAnalyzer*)param;

	spectrumAnalyzer->Scan();
	
	_endthread();
}

void SpectrumAnalyzer::Scan()
{
	FrequencyRange currentBandwidthRange;
	int gain = 0;
	char textBuffer[255];

	if (spectrumBuffer == NULL)
	{
		spectrumBufferSize = DeviceReceiver::FFT_SEGMENT_SAMPLE_COUNT * (maxFrequencyRange.length / (double)DeviceReceiver::SAMPLE_RATE);

		spectrumBuffer = new double[spectrumBufferSize * 2 * sizeof(double)];
	}

	while (scanning)
	{		
		for (int j = 0; j < requiredScanningSequences; j++)
		{
			currentBandwidthRange.Set(currentScanningFrequencyRange.lower, currentScanningFrequencyRange.lower + DeviceReceiver::SAMPLE_RATE);
			
			do
			{				
				SetCurrentCenterFrequency(currentBandwidthRange.centerFrequency);
				//deviceReceivers->SetCurrentCenterFrequency(currentBandwidthRange.centerFrequency);

				/*code for calculating resonant frequencies: H = height, W = weight
				float f, wavelength, c, H, W;
				c = 300000000;				

				f = c / (H * 2);
				wavelength = H * 2;

				f = (c / (4 * MathUtilities::PI)) * (4.4923*sqrt((MathUtilities::PI*H) / W) + sqrt(20.181 * (MathUtilities::PI*H) / W + 0.25*((MathUtilities::PI / H)*(MathUtilities::PI / H))));

				f = (c / (4 * MathUtilities::PI)) * (1.742*sqrt((MathUtilities::PI*H) / W) + sqrt(3.0345 * (MathUtilities::PI*H) / W + 4/(H*H)));
				*/

				if (deviceReceivers->fftGraphForDevicesBandwidth)				
					deviceReceivers->fftGraphForDevicesBandwidth->SetGraphXRange(currentBandwidthRange.lower, currentBandwidthRange.upper);
				
				if (deviceReceivers->combinedFFTGraphForBandwidth)
					deviceReceivers->combinedFFTGraphForBandwidth->SetGraphXRange(currentBandwidthRange.lower, currentBandwidthRange.upper);
				
				if (deviceReceivers->fftAverageGraphForDeviceRange)
					deviceReceivers->fftAverageGraphForDeviceRange->SetGraphXRange(currentBandwidthRange.lower, currentBandwidthRange.upper);

				if (deviceReceivers->fftAverageGraphStrengthsForDeviceRange)
					deviceReceivers->fftAverageGraphStrengthsForDeviceRange->SetGraphXRange(currentBandwidthRange.lower, currentBandwidthRange.upper);

				/*if (!(currentScanningFrequencyRange.lower == maxFrequencyRange.lower && currentScanningFrequencyRange.upper == maxFrequencyRange.upper))					
					Sleep(800000);
				else					
					Sleep(600000);
					*/

				#if !defined(_DEBUG)				
					if (!(currentScanningFrequencyRange.lower == maxFrequencyRange.lower && currentScanningFrequencyRange.upper == maxFrequencyRange.upper))
						Sleep(20000);
					else
						Sleep(1000);
				#else
					if (!(currentScanningFrequencyRange.lower == maxFrequencyRange.lower && currentScanningFrequencyRange.upper == maxFrequencyRange.upper))
						Sleep(10000);
					else
						Sleep(100);
				#endif

				if (calculateFFTDifferenceBuffer)
					fftSpectrumBuffers->CalculateFFTDifferenceBuffer(0, 1);
					
				if (deviceReceivers->spectrumRangeGraph)				
				{					
					if (GetFFTData(spectrumBuffer, spectrumBufferSize, 0, maxFrequencyRange.lower, maxFrequencyRange.upper, ReceivingDataBufferSpecifier::AveragedBuffer))
						deviceReceivers->spectrumRangeGraph->SetData((void *)spectrumBuffer, spectrumBufferSize, 0, true, 0, 0, !usePhase);

					if (GetFFTData(spectrumBuffer, spectrumBufferSize, 1, maxFrequencyRange.lower, maxFrequencyRange.upper, ReceivingDataBufferSpecifier::AveragedBuffer))
						deviceReceivers->spectrumRangeGraph->SetData((void *)spectrumBuffer, spectrumBufferSize, 1, true, 0, 0, !usePhase);
				}

				currentBandwidthRange.Set(currentBandwidthRange.lower + DeviceReceiver::SAMPLE_RATE, currentBandwidthRange.upper + DeviceReceiver::SAMPLE_RATE);
			} while (currentBandwidthRange.lower < currentScanningFrequencyRange.upper);
		}				

 		SequenceFinishedFunction();		
	}
}

void SpectrumAnalyzer::SetSequenceFinishedFunction(void(*func)())
{
	SequenceFinishedFunction = func;	
}

void SpectrumAnalyzer::SetOnReceiverDataProcessed(void(*func)())
{
	OnReceiverDataProcessedFunction = func;
}

void SpectrumAnalyzer::LaunchScanningFrequencyRange(FrequencyRange frequencyRange)
{
	currentScanningFrequencyRange = frequencyRange;
	scanning = true;

	scanFrequencyRangeThreadHandle = (HANDLE)_beginthread(ScanFrequencyRangeThread, 0, this);
	bool result = SetThreadPriority(ScanFrequencyRangeThread, THREAD_PRIORITY_HIGHEST);
}

bool SpectrumAnalyzer::SetFFTInput(fftw_complex* fftBuffer, FrequencyRange* inputFrequencyRange, uint8_t* samples, uint32_t sampleCount, uint8_t deviceID)
{		
	fftBandwidthBuffer->range.lower = inputFrequencyRange->lower;
	fftBandwidthBuffer->range.upper= inputFrequencyRange->upper;

	fftBandwidthBuffer->Add(fftBuffer, sampleCount, GetTickCount(), inputFrequencyRange, currentFFTBufferIndex);
	//fftBandwidthBuffer->SetFFTInput(fftBuffer, samples, sampleCount, deviceID, inputFrequencyRange, deviceID == 0, currentFFTBufferIndex);

	return fftSpectrumBuffers->SetFFTInput(currentFFTBufferIndex, fftBuffer, samples, sampleCount, deviceID, inputFrequencyRange, deviceID == 0);
}

bool SpectrumAnalyzer::SetFFTInput(fftw_complex* fftBuffer, FrequencyRange* inputFrequencyRange, fftw_complex* samples, uint32_t sampleCount, uint8_t deviceID)
{
	fftBandwidthBuffer->range.lower = inputFrequencyRange->lower;
	fftBandwidthBuffer->range.upper = inputFrequencyRange->upper;

	fftBandwidthBuffer->Add(fftBuffer, sampleCount, GetTickCount(), inputFrequencyRange, currentFFTBufferIndex);

	return fftSpectrumBuffers->SetFFTInput(currentFFTBufferIndex, fftBuffer, samples, sampleCount, deviceID, inputFrequencyRange, deviceID == 0);
}

bool SpectrumAnalyzer::ProcessReceiverData(FrequencyRange* inputFrequencyRange, bool useRatios)
{		
	if (!deviceReceivers->Correlated() && deviceReceivers->count > 1)
	{		
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

	if (fftBuffer)
		return fftBuffer->GetFFTData(dataBuffer, dataBufferLength, startFrequency, endFrequency, receivingDataBufferSpecifier);
	else
		return NULL;
}

SignalProcessingUtilities::Strengths_ID_Time* SpectrumAnalyzer::GetStrengthForRangeOverTimeFromCurrentBandwidthBuffer(int fftSpectrumBufferIndex, uint32_t startFrequency, uint32_t endFrequency, DWORD* duration, uint32_t* resultLength, DWORD currentTime)
{
	FFTSpectrumBuffer* fftBuffer;

	if (fftSpectrumBufferIndex == -1)
		fftSpectrumBufferIndex = currentFFTBufferIndex;

	fftBuffer = fftSpectrumBuffers->GetFFTSpectrumBuffer(fftSpectrumBufferIndex);

	if (fftBuffer)
		return fftBuffer->GetStrengthForRangeOverTimeFromCurrentBandwidthBuffer(startFrequency, endFrequency, duration, 0, resultLength, currentTime);
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

	delete deviceReceivers;
}

SpectrumAnalyzer::~SpectrumAnalyzer()
{
}
