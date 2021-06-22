#ifdef _DEBUG
 #define _ITERATOR_DEBUG_LEVEL 2
#endif #endif

//#include "TimXml\TimXmlRpc.h"

#include "SpectrumAnalyzer.h"
#include "Sound.h"
#include "ThreadUtilities.h"
#include <cstdlib>

SpectrumAnalyzer::SpectrumAnalyzer()
{
    scanFrequencyRangeThreadHandle = NULL;
	playSoundThread = NULL;

	calculateFFTDifferenceBuffer = false;

	spectrumBufferSize = 0;
	spectrumBuffer = NULL;
	deviceReceivers = NULL;
	fftSpectrumBuffers = NULL;
	requiredFramesPerBandwidthScan = 1;
	requiredScanningSequences = 1;
	scanning = false;
	prevFFTBufferIndex = -1;
	scheduledFFTBufferIndex = -1;
	currentFFTBufferIndex = -1;
	useRatios = false;
	usePhase = false;
	eegStrength = false;
	sound = false;

	samp_rate = 1024000;
}

uint8_t SpectrumAnalyzer::InitializeSpectrumAnalyzer(uint32_t bufferSizeInMilliSeconds, uint32_t sampleRate, uint32_t minStartFrequency, uint32_t maxEndFrequency)
{
	DeviceReceiver::SAMPLE_RATE = sampleRate;

    DeviceReceiver::RECEIVING_GNU_DATA = false;

    //Check whether there is a GNU Radio connected device
    //gnuUitilities.CallXMLRPC("<?xml version=\"1.0\"?>\r\n<methodCall><methodName>get_flow_graph_ID</methodName>\r\n<params></params></methodCall>", &function_pt);
    std::string dataStr = gnuUitilities.CallXMLRPC("<?xml version=\"1.0\"?>\r\n<methodCall><methodName>get_flow_graph_ID</methodName>\r\n<params></params></methodCall>");

    if (dataStr.find("GNURADIODEVICE") != std::string::npos)
    {
        DeviceReceiver::RECEIVING_GNU_DATA = true;

		std::cout << "GNU Radio Device: Connected\n";

		//args[0] = (int)(DeviceReceiver::SAMPLE_RATE / 1000);

		//std::string setSampleRateStr = "<?xml version=\"1.0\"?>\r\n<methodCall><methodName>set_samp_rate</methodName>\r\n<params><param><value><i4>1000</i4></value></param></params></methodCall>\r\n";

		std::string setSampleRateStr = "<?xml version=\"1.0\"?>\r\n<methodCall><methodName>set_samp_rate</methodName>\r\n<params><param><value><i4>";


        int sampRate = (int)(DeviceReceiver::SAMPLE_RATE / 1000);

        char sampRateStr[255];

        snprintf(sampRateStr, 255, "%d\0", sampRate);

        setSampleRateStr += sampRateStr;

		//setSampleRateStr += (int)(DeviceReceiver::SAMPLE_RATE / 1000);

		setSampleRateStr += "</i4></value></param></params></methodCall>";


		//char* setSampleRateStr = "<?xml version=\"1.0\"?>\r\n<methodCall><methodName>set_samp_rate</methodName>\r\n<params><param><value><i4>2000</i4></value></param></params></methodCall>";

		gnuUitilities.CallXMLRPC(setSampleRateStr.c_str()); //Check whether there is a GNU Radio connected device

		Timeout(1);
	}


    /*
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
    */

    //DeviceReceiver::RECEIVING_GNU_DATA = false;
    //DeviceReceiver::RECEIVING_GNU_DATA = true;

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

#ifdef _WIN32
    void PlaySoundThread(void *param)
#else
    void* PlaySoundThread(void *param)
#endif
{
	Sound *sound = (Sound *) param;

	#ifdef _WIN32
	Beep(sound->frequency, sound->duration);
	#else

	#endif // _WIN32

	delete sound;

	ThreadUtilities::CloseThread();
}

void SpectrumAnalyzer::PlaySound(DWORD frequency, DWORD duration)
{
	//if (sound && playSoundThread == NULL)
	if (sound)
	{
		Sound *sound = new Sound(frequency, duration);

		ThreadUtilities::CreateThread(PlaySoundThread, (void *)sound);

		////pthread_t playSoundThreadHandle;
		////int result = pthread_create(&playSoundThreadHandle, NULL, PlaySoundThread, (void *)sound);

		//playSoundThread = (HANDLE)_beginthread(PlaySoundThread, 0, (void *) sound);
		//playSoundThread = NULL;
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
	    std::string setFreqStrStr = "<?xml version=\"1.0\"?>\r\n<methodCall><methodName>set_freq</methodName>\r\n<params><param><value><i4>";

        int freq = (int)(centerFrequency/1000);

        char freqStr[255];

        snprintf(freqStr, 255, "%d\0", freq);

        setFreqStrStr += freqStr;

		setFreqStrStr += "</i4></value></param></params></methodCall>";

		gnuUitilities.CallXMLRPC(setFreqStrStr.c_str()); //Check whether there is a GNU Radio connected device

		Timeout(1);
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

void SpectrumAnalyzer::CalculateFFTDifferenceBuffer(uint8_t index1, uint8_t index2)
{
	if (calculateFFTDifferenceBuffer)
		fftSpectrumBuffers->CalculateFFTDifferenceBuffer(index1, index2);
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

#ifdef _WIN32
    void ScanFrequencyRangeThread(void *param)
#else
    void* ScanFrequencyRangeThread(void *param)
#endif

{
	SpectrumAnalyzer* spectrumAnalyzer = (SpectrumAnalyzer*)param;

	spectrumAnalyzer->Scan();

	ThreadUtilities::CloseThread();
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


				#if !defined(_DEBUG) || defined(RELEASE_SETTINGS)
					if (!(currentScanningFrequencyRange.lower == maxFrequencyRange.lower && currentScanningFrequencyRange.upper == maxFrequencyRange.upper))
						Timeout(20000);
					else
						Timeout(1000);
				#else
					if (!(currentScanningFrequencyRange.lower == maxFrequencyRange.lower && currentScanningFrequencyRange.upper == maxFrequencyRange.upper))
						Timeout(6000);
					else
						Timeout(100);
				#endif


				CalculateFFTDifferenceBuffer(0, 1);

				if (deviceReceivers->spectrumRangeGraph)
				{
					if (GetFFTData(spectrumBuffer, spectrumBufferSize, 0, maxFrequencyRange.lower, maxFrequencyRange.upper, AveragedBuffer))
						deviceReceivers->spectrumRangeGraph->SetData((void *)spectrumBuffer, spectrumBufferSize, 0, true, 0, 0, !usePhase);

					if (GetFFTData(spectrumBuffer, spectrumBufferSize, 1, maxFrequencyRange.lower, maxFrequencyRange.upper, AveragedBuffer))
						deviceReceivers->spectrumRangeGraph->SetData((void *)spectrumBuffer, spectrumBufferSize, 1, true, 0, 0, !usePhase);

					if (GetFFTData(spectrumBuffer, spectrumBufferSize, 4, maxFrequencyRange.lower, maxFrequencyRange.upper, AveragedBuffer))
					{
						//deviceReceivers->spectrumRangeGraph->SetData((void *)spectrumBuffer, spectrumBufferSize, 2, true, 0, 0, !usePhase);
						deviceReceivers->spectrumRangeDifGraph->SetData((void *)spectrumBuffer, spectrumBufferSize, 0, true, 0, 0, !usePhase);

						//memset(spectrumBuffer, 0, 4 * sizeof(double));
						spectrumBuffer[0] = 0;
						spectrumBuffer[1] = -100;
						spectrumBuffer[2] = 0;
						spectrumBuffer[3] = -100;


						deviceReceivers->spectrumRangeDifGraph->SetData((void *)spectrumBuffer, 2, 1, true, 0, 0, !usePhase);
					}
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

	scanFrequencyRangeThreadHandle = ThreadUtilities::CreateThread(ScanFrequencyRangeThread, this);
}

bool SpectrumAnalyzer::SetFFTInput(fftw_complex* fftBuffer, FrequencyRange* inputFrequencyRange, uint8_t* samples, uint32_t sampleCount, uint8_t deviceID)
{
	fftBandwidthBuffer->range.lower = inputFrequencyRange->lower;
	fftBandwidthBuffer->range.upper= inputFrequencyRange->upper;

	fftBandwidthBuffer->Add(fftBuffer, sampleCount, GetTime(), inputFrequencyRange, currentFFTBufferIndex);
	//fftBandwidthBuffer->SetFFTInput(fftBuffer, samples, sampleCount, deviceID, inputFrequencyRange, deviceID == 0, currentFFTBufferIndex);

	return fftSpectrumBuffers->SetFFTInput(currentFFTBufferIndex, fftBuffer, samples, sampleCount, deviceID, inputFrequencyRange, deviceID == 0);
}

bool SpectrumAnalyzer::SetFFTInput(fftw_complex* fftBuffer, FrequencyRange* inputFrequencyRange, fftw_complex* samples, uint32_t sampleCount, uint8_t deviceID)
{
	fftBandwidthBuffer->range.lower = inputFrequencyRange->lower;
	fftBandwidthBuffer->range.upper = inputFrequencyRange->upper;

	fftBandwidthBuffer->Add(fftBuffer, sampleCount, GetTime(), inputFrequencyRange, currentFFTBufferIndex);

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
