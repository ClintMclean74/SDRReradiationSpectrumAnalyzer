#ifdef _DEBUG 
 #define _ITERATOR_DEBUG_LEVEL 2 
#endif
#include <string>
#include <process.h>
#include "SpectrumAnalyzer.h"
#include "DeviceReceivers.h"
#include "DeviceReceiver.h"
#include "convenience.h"
#include "SignalProcessingUtilities.h"
#include "kerberos_functions.h"
#include "NearFarDataAnalyzer.h"
#include "DebuggingUtilities.h"
#include "ArrayUtilities.h"
#include "GNU_Radio_Utilities.h"

DeviceReceiver::DeviceReceiver(void* parent, long bufferSizeInMilliSeconds, uint32_t sampleRate, uint8_t ID)
{
	this->parent = parent;
	this->deviceID = ID;	


	//SAMPLE_RATE = 1000000; //1 second of samples
	//SAMPLE_RATE = 20480000; //1 second of samples	

	uint32_t NUMBER_OF_SAMPLES_FOR_PROCESSING = 1000000; //1 second of samples
	//RECEIVE_BUFF_LENGTH = SignalProcessingUtilities::ClosestIntegerMultiple(NUMBER_OF_SAMPLES_FOR_PROCESSING * 2, 16384);		
	////original RECEIVE_BUFF_LENGTH = FFT_SEGMENT_BUFF_LENGTH;


	if (DebuggingUtilities::RECEIVE_TEST_DATA)
		RECEIVE_BUFF_LENGTH = 200;

	/*original if (RECEIVE_BUFF_LENGTH < FFT_SEGMENT_BUFF_LENGTH)
		FFT_SEGMENT_BUFF_LENGTH = RECEIVE_BUFF_LENGTH;
		*/

	CORRELATION_BUFF_LENGTH = FFT_SEGMENT_BUFF_LENGTH * 2;
	FFT_SEGMENT_SAMPLE_COUNT = FFT_SEGMENT_BUFF_LENGTH / 2;
	
	receiverBufferDataAvailableGate = CreateSemaphore(
		NULL,           // default security attributes
		0,  // initial count
		10,  // maximum count
		NULL);          // unnamed semaphore

	if (receiverBufferDataAvailableGate == NULL)
	{
		if (DebuggingUtilities::DEBUGGING)
		{
			DebuggingUtilities::DebugPrint("CreateSemaphore error: %d\n", GetLastError());
			
		}
	}

	receiverFinished = CreateSemaphore(NULL, 0, 10, NULL);

	if (receiverFinished == NULL)
	{
		if (DebuggingUtilities::DEBUGGING)
		{
			DebuggingUtilities::DebugPrint("CreateSemaphore error: %d\n", GetLastError());

		}
	}

	receiverGate = CreateSemaphore(NULL, 0, 10, NULL);

	if (receiverGate == NULL)
	{
		if (DebuggingUtilities::DEBUGGING)
		{
			DebuggingUtilities::DebugPrint("CreateSemaphore error: %d\n", GetLastError());

		}
	}
	
	circularDataBuffer = new CircularDataBuffer((float) bufferSizeInMilliSeconds / 1000 * sampleRate * 2, FFT_SEGMENT_BUFF_LENGTH);

	devicesToSendClonedDataTo = new DeviceReceiverPtr[4];
	devicesToSendClonedDataToCount = 0;

	transferDataBuffer = new uint8_t[RECEIVE_BUFF_LENGTH];

	gnuReceivedBuffer = new uint8_t[RECEIVE_BUFF_LENGTH];

	/*
	sharedBuffer = std::make_shared<SharedBuffer>();

	auto sharedBuffer = make_shared<SharedBuffer>();

	std::memcpy(sharedBuffer.get(), gnuReceivedBuffer, RECEIVE_BUFF_LENGTH);

	std::shared_ptr<int> foo = std::make_shared<int>(10);
	// same as:
	std::shared_ptr<int> foo2(new int(10));

	auto bar = std::make_shared<int>(20);

	auto baz = std::make_shared<std::pair<int, int>>(30, 40);
	*/
	

	if (DeviceReceiver::RECEIVING_GNU_DATA)
	{
		WSADATA w;								/* Used to open Windows connection */
		unsigned short port_number;				/* The port number to use */				
		struct hostent *hp;						/* Information about the server */		
		struct sockaddr_in client;				/* Information about the client */

		/* Open windows connection */
		if (WSAStartup(0x0101, &w) != 0)
		{
			fprintf(stderr, "Could not open Windows connection.\n");
			exit(0);
		}

		/* Open a datagram socket */
		sd = socket(AF_INET, SOCK_DGRAM, 0);
		if (sd == INVALID_SOCKET)
		{
			fprintf(stderr, "Could not create socket.\n");
			WSACleanup();
			exit(0);
		}

		server_length = sizeof(struct sockaddr_in);

		/* Clear out server struct */
		memset((void *)&server, '\0', server_length);

		/* Set family and port */
		server.sin_family = AF_INET;

		/* Clear out client struct */
		memset((void *)&client, '\0', sizeof(struct sockaddr_in));

		/* Set family and port */
		client.sin_family = AF_INET;
		//client.sin_addr.s_addr = inet_addr("10.0.0.3");
		//client.sin_addr.s_addr = inet_addr("127.0.0.1");
		client.sin_addr.s_addr = inet_addr(GNU_Radio_Utilities::GNU_RADIO_DATA_STREAMING_ADDRESS);
		client.sin_port = htons(1234);

		/* Bind local address to socket */
		if (bind(sd, (struct sockaddr *)&client, sizeof(struct sockaddr_in)) == -1)
		{
			fprintf(stderr, "Cannot bind address to socket.\n");
			closesocket(sd);
			WSACleanup();
			exit(0);
		}
	}		
}

int DeviceReceiver::InitializeDeviceReceiver(int dev_index)
{
	int result = -1;

	result = rtlsdr_open(&device, dev_index);

	if (result < 0)
	{
		fprintf(stderr, "Failed to open rtlsdr device #%d.\n", dev_index);

		return result;
	}
	
	if (rtlsdr_set_dithering(device, 0) != 0) // Only in keenerd's driver
	{
		fprintf(stderr, "[ ERROR ] Failed to disable dithering: %s\n", strerror(errno));
	}

	if (rtlsdr_reset_buffer(device) != 0)
	{
		fprintf(stderr, "[ ERROR ] Failed to reset receiver buffer: %s\n", strerror(errno));
	}
	
	if (rtlsdr_set_tuner_gain_mode(device, 1) != 0)
	//if (rtlsdr_set_tuner_gain_mode(device, 0) != 0)
	{
		fprintf(stderr, "[ ERROR ] Failed to disable AGC: %s\n", strerror(errno));
	}
	
	int gain = 300;
	//int gain = 0;
	
	if (rtlsdr_set_tuner_gain(device, gain) != 0)	
	{
		fprintf(stderr, "[ ERROR ] Failed to set gain value: %s\n", strerror(errno));
	}
	
	rtlsdr_set_center_freq(device, 450000000);

	if (rtlsdr_set_sample_rate(device, SAMPLE_RATE) != 0)
	{
		fprintf(stderr, "[ ERROR ] Failed to set sample rate: %s\n", strerror(errno));
	}

	return dev_index;
}

void DeviceReceiver::SetFrequencyRange(FrequencyRange* frequencyRange)
{
	this->frequencyRange = frequencyRange;
	
	rtlsdr_set_center_freq(device, frequencyRange->centerFrequency);
}


void DeviceReceiver::SetGain(int gain)
{
	if (rtlsdr_set_tuner_gain(device, gain) != 0)
	{
		fprintf(stderr, "[ ERROR ] Failed to set gain value: %s\n", strerror(errno));
	}
}

void DeviceReceiver::FFT_BYTES(uint8_t *data, fftw_complex *fftData, int samples, bool inverse, bool inputComplex, bool rotate180, bool synchronizing)
{	
	unsigned int measure = FFTW_MEASURE;
	int j = 0;	

	DeviceReceivers* deviceReceivers = (DeviceReceivers*)parent;

	if (inputComplex)
	{
		if (complexArray == NULL && samples == DeviceReceiver::FFT_SEGMENT_SAMPLE_COUNT)
		{
			complexArray = (fftw_complex *)fftw_malloc(sizeof(fftw_complex) * samples);
		}
		else
		if (complexArray2 == NULL && samples == DeviceReceiver::FFT_SEGMENT_SAMPLE_COUNT * 2)
		{
			complexArray2 = (fftw_complex *)fftw_malloc(sizeof(fftw_complex) * samples);
		}
		else
			if (complexArray == NULL)
				complexArray = (fftw_complex *)fftw_malloc(sizeof(fftw_complex) * samples);
	}
	else
	{
		unsigned int fftSize = (samples * 2 + 1);
	}

	if (inputComplex)
	{
		if (complexArrayFFTPlan == NULL && samples == DeviceReceiver::FFT_SEGMENT_SAMPLE_COUNT)		
		{
			if (synchronizing && !referenceDevice)
				WaitForSingleObject(deviceReceivers->fftBytesGate, 2000);

			complexArrayFFTPlan = fftw_plan_dft_1d(samples,
					complexArray,
					fftData,
					inverse ? FFTW_BACKWARD : FFTW_FORWARD,
					measure);

			if (synchronizing && referenceDevice)
			{
				if (ReleaseSemaphore(deviceReceivers->fftBytesGate, 1, NULL)==0)
				{
					if (DebuggingUtilities::DEBUGGING)
						DebuggingUtilities::DebugPrint("ReleaseSemaphore error: %d\n", GetLastError());					
				}
			}			
				
		}
		else
		if (complexArrayFFTPlan2 == NULL && samples == DeviceReceiver::FFT_SEGMENT_SAMPLE_COUNT * 2)		
		{					
			if (synchronizing && !referenceDevice)
				WaitForSingleObject(deviceReceivers->fftBytesGate, 2000);
			
			complexArrayFFTPlan2 = fftw_plan_dft_1d(samples,
					complexArray2,
					fftData,
					inverse ? FFTW_BACKWARD : FFTW_FORWARD,
					measure);

			if (synchronizing && referenceDevice)
			{
				if (ReleaseSemaphore(deviceReceivers->fftBytesGate, 1, NULL)==0)
				{
					if (DebuggingUtilities::DEBUGGING)
						DebuggingUtilities::DebugPrint("ReleaseSemaphore error: %d\n", GetLastError());					
				}
			}

		}
		else if (complexArrayFFTPlan == NULL && samples != DeviceReceiver::FFT_SEGMENT_SAMPLE_COUNT && samples != DeviceReceiver::FFT_SEGMENT_SAMPLE_COUNT * 2)
		{			
			if (synchronizing && !referenceDevice)
				WaitForSingleObject(deviceReceivers->fftBytesGate, 2000);
			
			complexArrayFFTPlan = fftw_plan_dft_1d(samples,
					complexArray,
					fftData,
					inverse ? FFTW_BACKWARD : FFTW_FORWARD,
					measure);

			if (synchronizing && referenceDevice)
			{
				if (ReleaseSemaphore(deviceReceivers->fftBytesGate, 1, NULL) == 0)
				{
					if (DebuggingUtilities::DEBUGGING)
						DebuggingUtilities::DebugPrint("ReleaseSemaphore error: %d\n", GetLastError());
				}
			}
		}

		j = 0;

		fftw_complex *array;

		if (samples == DeviceReceiver::FFT_SEGMENT_SAMPLE_COUNT)
		{
			array = complexArray;			
		}
		else
			if (samples == DeviceReceiver::FFT_SEGMENT_SAMPLE_COUNT * 2)
			{
				array = complexArray2;
			}
			else
				array = complexArray;

		for (int i = 0; i < samples; i++)
		{
			array[j][0] = (double)data[(int)i * 2] / 255 - 0.5;
			array[j][1] = (double)data[(int)i * 2 + 1] / 255 - 0.5;

			j++;
		}
	
		if (synchronizing && !referenceDevice)
			WaitForSingleObject(deviceReceivers->fftBytesGate, 2000);

		if (DebuggingUtilities::DEBUGGING)
		{
			DebuggingUtilities::DebugPrint("fft start, referenceDevice: ");
			DebuggingUtilities::DebugPrint(referenceDevice ? "true\n" : "false\n");
		}		

		if (samples == DeviceReceiver::FFT_SEGMENT_SAMPLE_COUNT)
			fftw_execute_dft(complexArrayFFTPlan, array, fftData);
		else
			if (samples == DeviceReceiver::FFT_SEGMENT_SAMPLE_COUNT * 2)
				fftw_execute_dft(complexArrayFFTPlan2, array, fftData);				
			else
				fftw_execute_dft(complexArrayFFTPlan, array, fftData);

		if (DebuggingUtilities::DEBUGGING)
		{
			DebuggingUtilities::DebugPrint("fft end, referenceDevice: ");
			DebuggingUtilities::DebugPrint(referenceDevice ? "true\n" : "false\n");
		}

		if (synchronizing && referenceDevice)
		{
			if (ReleaseSemaphore(deviceReceivers->fftBytesGate, 1, NULL) == 0)
			{
				if (DebuggingUtilities::DEBUGGING)
					DebuggingUtilities::DebugPrint("ReleaseSemaphore error: %d\n", GetLastError());
			}
		}

	}

	fftData[0][0] = fftData[1][0];
	fftData[0][1] = fftData[1][1];
	
	if (rotate180)
	{
		int length2 = samples / 2;
		fftw_complex temp;		

		for (int i = 0; i < length2; i++)
		{
			temp[0] = fftData[i][0];
			temp[1] = fftData[i][1];

			fftData[i][0] = fftData[i + length2][0];
			fftData[i][1] = fftData[i + length2][1];

			fftData[i + length2][0] = temp[0];
			fftData[i + length2][1] = temp[1];
		}
	}
}


void DeviceReceiver::FFT_COMPLEX_ARRAY(fftw_complex* data, fftw_complex* fftData, int samples, bool inverse, bool rotate180, bool synchronizing)
{	
	unsigned int measure = FFTW_MEASURE;	

	DeviceReceivers* deviceReceivers = (DeviceReceivers*)parent;

	if (complexArrayCorrelationFFTPlan == NULL)
	{
		if (synchronizing && !referenceDevice)
			WaitForSingleObject(deviceReceivers->fftComplexGate, 2000);

		fftw_complex* initializationData = (fftw_complex *)fftw_malloc(sizeof(fftw_complex) * samples);
		memcpy(initializationData, data, sizeof(fftw_complex) * samples);

		complexArrayCorrelationFFTPlan = fftw_plan_dft_1d(samples,
			initializationData,
			fftData,
			inverse ? FFTW_BACKWARD : FFTW_FORWARD,
			measure);

		fftw_free(initializationData);

		if (synchronizing && referenceDevice)
		{
			if (ReleaseSemaphore(deviceReceivers->fftComplexGate, 1, NULL) == 0)
			{
				if (DebuggingUtilities::DEBUGGING)
					DebuggingUtilities::DebugPrint("ReleaseSemaphore error: %d\n", GetLastError());
			}
		}
	}

	if (!referenceDevice)
		WaitForSingleObject(deviceReceivers->fftComplexGate, 2000);

	if (DebuggingUtilities::DEBUGGING)
	{
		DebuggingUtilities::DebugPrint("fft start, referenceDevice: ");
		DebuggingUtilities::DebugPrint(referenceDevice ? "true\n" : "false\n");
	}

	fftw_execute_dft(complexArrayCorrelationFFTPlan, data, fftData);

	if (DebuggingUtilities::DEBUGGING)
	{
		DebuggingUtilities::DebugPrint("fft end, referenceDevice: ");
		DebuggingUtilities::DebugPrint(referenceDevice ? "true\n" : "false\n");
	}

	if (referenceDevice)
	{
		if (ReleaseSemaphore(deviceReceivers->fftComplexGate, 1, NULL) == 0)
		{
			if (DebuggingUtilities::DEBUGGING)
				DebuggingUtilities::DebugPrint("ReleaseSemaphore error: %d\n", GetLastError());
		}
	}

	fftData[0][0] = fftData[1][0];
	fftData[0][1] = fftData[1][1];
	
	if (rotate180)
	{
		int length2 = samples / 2;
		fftw_complex temp;		

		for (int i = 0; i < length2; i++)
		{
			temp[0] = fftData[i][0];
			temp[1] = fftData[i][1];

			fftData[i][0] = fftData[i + length2][0];
			fftData[i][1] = fftData[i + length2][1];

			fftData[i + length2][0] = temp[0];
			fftData[i + length2][1] = temp[1];
		}
	}
}

void DeviceReceiver::GenerateNoise(uint8_t state)
{
	if (state == 1)
		rtlsdr_set_gpio(device, 1, 0);
	else
		rtlsdr_set_gpio(device, 0, 0);
}

void DeviceReceiver::WriteReceivedDataToBuffer(uint8_t *data, uint32_t length)
{
	DWORD currentTime = GetTickCount();
	receivedDatatartTime = currentTime;

	if (DebuggingUtilities::TEST_CORRELATION && referenceDevice)
	{
		if (receivedLength == RECEIVE_BUFF_LENGTH)
		{			
			memcpy(&transferDataBuffer[4], receivedBuffer, receivedLength - 4);
		}		
	}

	////circularDataBuffer->WriteData(receivedBuffer, receivedLength);
	circularDataBuffer->WriteData(data, length);

	/*//// original ReceiveData receiveData;

	receiveData.buff

	for (int i = 0; i < devicesToSendClonedDataToCount; i++)
		devicesToSendClonedDataTo[i]->ReceiveData(transferDataBuffer, receivedLength);
		*/

	if (ReleaseSemaphore(receiverBufferDataAvailableGate, 1, NULL)==0)
	{
		if (DebuggingUtilities::DEBUGGING)
		{
			DebuggingUtilities::DebugPrint("ReleaseSemaphore error: %d\n", GetLastError());			
		}
	}

	if (DebuggingUtilities::DEBUGGING)
	{
		DebuggingUtilities::DebugPrint("open receiverBufferDataAvailableGate\n");		
	}
}

void DeviceReceiver::CheckScheduledFFTBufferIndex()
{
	DeviceReceivers* deviceReceivers = (DeviceReceivers*)parent;
	SpectrumAnalyzer* spectrumAnalyzer = ((SpectrumAnalyzer*)deviceReceivers->parent);

	spectrumAnalyzer->prevFFTBufferIndex = spectrumAnalyzer->currentFFTBufferIndex;

	spectrumAnalyzer->currentFFTBufferIndex = spectrumAnalyzer->scheduledFFTBufferIndex;
}

void DeviceReceiver::ProcessData(uint8_t *data, uint32_t length)
{
	DWORD currentTime;

	uint32_t segments, segmentBufferLength;

	bool processAllData = false;

	if (processAllData)
	{
		segments = 1;

		segmentBufferLength = length;
	}
	else
		if (length < DeviceReceiver::FFT_SEGMENT_BUFF_LENGTH)
		{
			segments = 1;

			segmentBufferLength = length;
		}
		else
		{
			segments = length / DeviceReceiver::FFT_SEGMENT_BUFF_LENGTH;

			segmentBufferLength = DeviceReceiver::FFT_SEGMENT_BUFF_LENGTH;
		}

	uint32_t samplesCount = segmentBufferLength / 2;

	if (fftBuffer == NULL)
	{
		if (processAllData)
			fftBuffer = new fftw_complex[samplesCount];
		else
			fftBuffer = new fftw_complex[FFT_SEGMENT_SAMPLE_COUNT];
	}


	uint32_t currentSegmentIndex = 0;

	int i;

	DWORD dwWaitResult;

	DeviceReceivers* deviceReceivers = (DeviceReceivers*)parent;
	SpectrumAnalyzer* spectrumAnalyzer = ((SpectrumAnalyzer*)deviceReceivers->parent);

	int32_t remainingTime = (int32_t)receivedDuration[receivedCount - 1] - (GetTickCount() - receivedDatatartTime);

	double segmentTime = (double)remainingTime / segments;
	DWORD startSegmentTime, segmentDuration;
	int32_t delay;
	
	uint32_t newSegmentsCount;

	uint32_t segmentsProcessed = segments;
	for (i = 0; i < segments; i++)
	{
		if (spectrumAnalyzer->currentFFTBufferIndex != ReceivingDataMode::Paused)
		{
			startSegmentTime = GetTickCount();

			FFT_BYTES(&data[currentSegmentIndex], fftBuffer, samplesCount, false, true, true, true);
			SignalProcessingUtilities::CalculateMagnitudesAndPhasesForArray(fftBuffer, samplesCount);

			if (deviceReceivers->fftGraphForDevicesBandwidth)
				deviceReceivers->fftGraphForDevicesBandwidth->SetData(fftBuffer, samplesCount - 1, referenceDevice ? 0 : 1, true, 0, 0, !((SpectrumAnalyzer*)deviceReceivers->parent)->usePhase);

			spectrumAnalyzer->SetFFTInput(fftBuffer, frequencyRange, &data[currentSegmentIndex], samplesCount, deviceID);

			if (ReleaseSemaphore(receiverFinished, 1, NULL) == 0)
			{
				if (DebuggingUtilities::DEBUGGING)
				{
					DebuggingUtilities::DebugPrint("ReleaseSemaphore error: %d\n", GetLastError());
				}
			}

			if (DebuggingUtilities::DEBUGGING)
			{
				DebuggingUtilities::DebugPrint("ProcessData(): open deviceReceivers->receiversFinished[deviceID]\n");
			}

			if (referenceDevice)
			{
				if (DebuggingUtilities::DEBUGGING)
				{
					DebuggingUtilities::DebugPrint("referenceDevice WaitForMultipleObjects\n");
				}

				uint8_t* referenceDataBuffer = NULL;
				uint8_t* dataBuffer = NULL;

				DWORD dwWaitResult = WaitForMultipleObjects(deviceReceivers->count, deviceReceivers->receiversFinished, true, 2000);

				switch (dwWaitResult)
				{
				case WAIT_OBJECT_0:
					if (DebuggingUtilities::DEBUGGING)
						DebuggingUtilities::DebugPrint("ProcessData(): deviceReceivers->receiversFinished open :-> ProcessReceiverData()\n");

					spectrumAnalyzer->ProcessReceiverData(frequencyRange, spectrumAnalyzer->useRatios);

					spectrumAnalyzer->OnReceiverDataProcessedFunction();

					/*
					if (deviceReceivers->strengthGraph)
					{
						DWORD duration = 1000;
						uint32_t nearDataLength = 0;
						uint32_t farDataLength = 0;
												
						double *nearStrengths = spectrumAnalyzer->GetStrengthForRangeOverTimeFromCurrentBandwidthBuffer(ReceivingDataMode::Near, 0, 0, &duration, &nearDataLength);
						//double *nearStrengths = spectrumAnalyzer->GetStrengthForRangeOverTimeFromCurrentBandwidthBuffer(spectrumAnalyzer->currentFFTBufferIndex, 0, 0, &duration, &nearDataLength);						

						if (duration >= 1000)
						{
							duration = 1000;

							double *farStrengths = spectrumAnalyzer->GetStrengthForRangeOverTimeFromCurrentBandwidthBuffer(ReceivingDataMode::Far, 0, 0, &duration, &farDataLength);

							if (duration >= 1000)
							{
								spectrumAnalyzer->
								deviceReceivers->nearfa
							}

							if (farStrengths)
								delete farStrengths;
						}

						if (nearStrengths)
							delete nearStrengths;

						


						




						if (fftBandwidthStrength)
						{
							double *strengths = spectrumAnalyzer->GetStrengthForRangeOverTimeFromCurrentBandwidthBuffer(spectrumAnalyzer->currentFFTBufferIndex, 0, 0, &duration, &nearDataLength);

							deviceReceivers->strengthGraph->SetData(strengths, nearDataLength, 0, false, 0, 0, false, SignalProcessingUtilities::DataType::DOUBLE);

							if (spectrumAnalyzer->currentFFTBufferIndex == ReceivingDataMode::Near)
							{
								deviceReceivers->strengthGraph->SetText(1, "Range Strength Graph: Near");
								deviceReceivers->strengthGraph->SetDataSeriesColor(1, 0, 0, 1, ReceivingDataMode::Near);
							}
							else
								if (spectrumAnalyzer->currentFFTBufferIndex == ReceivingDataMode::Far)
								{
									deviceReceivers->strengthGraph->SetText(1, "Range Strength Graph: Far");
									deviceReceivers->strengthGraph->SetDataSeriesColor(0, 1, 0, 1, ReceivingDataMode::Near);
								}
								else
									if (spectrumAnalyzer->currentFFTBufferIndex == ReceivingDataMode::Undetermined)
									{
										deviceReceivers->strengthGraph->SetText(1, "Range Strength Graph: Near/Far?");
										deviceReceivers->strengthGraph->SetDataSeriesColor(1, 1, 0, 1, ReceivingDataMode::Near);
									}

							if (strengths)
								delete strengths;							
						}
						
						
						/*if (nearDataLength > 0)
						{
							duration = 10000;
							double *farStrengths = spectrumAnalyzer->GetStrengthForRangeOverTimeFromCurrentBandwidthBuffer(ReceivingDataMode::Far, 0, 0, &duration, &farDataLength);

							uint32_t totalDataLength = nearDataLength + farDataLength;

							deviceReceivers->strengthGraph->SetMaxResolution(totalDataLength);

							if (totalDataLength > nearDataLength)
							{
								nearStrengths = ArrayUtilities::ResizeArray(nearStrengths, nearDataLength, totalDataLength);

								memcpy(&nearStrengths[nearDataLength], farStrengths, farDataLength * sizeof(double));
							}

							//deviceReceivers->strengthGraph->SetData(strengths, resultLength, spectrumAnalyzer->currentFFTBufferIndex, false, 0, 0, false, SignalProcessingUtilities::DataType::DOUBLE);
							//deviceReceivers->strengthGraph->SetData(strengths, resultLength, 0, false, 0, 0, false, SignalProcessingUtilities::DataType::DOUBLE);
							//deviceReceivers->strengthGraph->SetData(nearStrengths, totalDataLength, 0, false, 0, 0, false, SignalProcessingUtilities::DataType::DOUBLE);
							deviceReceivers->strengthGraph->SetData(nearStrengths, nearDataLength, 0, false, 0, 0, false, SignalProcessingUtilities::DataType::DOUBLE);

							if (spectrumAnalyzer->currentFFTBufferIndex == ReceivingDataMode::Near)
							{
								deviceReceivers->strengthGraph->SetText(1, "Range Strength Graph: Near");
								deviceReceivers->strengthGraph->SetDataSeriesColor(1, 0, 0, 1, ReceivingDataMode::Near);
							}
							else
								if (spectrumAnalyzer->currentFFTBufferIndex == ReceivingDataMode::Far)
								{
									deviceReceivers->strengthGraph->SetText(1, "Range Strength Graph: Far");
									deviceReceivers->strengthGraph->SetDataSeriesColor(0, 1, 0, 1, ReceivingDataMode::Near);
								}
								else
									if (spectrumAnalyzer->currentFFTBufferIndex == ReceivingDataMode::Undetermined)
									{
										deviceReceivers->strengthGraph->SetText(1, "Range Strength Graph: Near/Far?");
										deviceReceivers->strengthGraph->SetDataSeriesColor(1, 1, 0, 1, ReceivingDataMode::Near);
									}

							//delete strengths;
							if (nearStrengths)
								delete nearStrengths;

							if (farStrengths)
								delete farStrengths;
						}*/
					//}


					referenceDataBuffer = spectrumAnalyzer->GetFFTSpectrumBuffer(spectrumAnalyzer->currentFFTBufferIndex)->GetSampleDataForDevice(0);
					dataBuffer = spectrumAnalyzer->GetFFTSpectrumBuffer(spectrumAnalyzer->currentFFTBufferIndex)->GetSampleDataForDevice(1);

					deviceReceivers->dataGraph->SetData(referenceDataBuffer, DeviceReceiver::FFT_SEGMENT_BUFF_LENGTH, 0, true, -128, -128, true, SignalProcessingUtilities::DataType::UINT8_T);

					if (dataBuffer)
						deviceReceivers->dataGraph->SetData(dataBuffer, DeviceReceiver::FFT_SEGMENT_BUFF_LENGTH, 1, true, -128, -128, true, SignalProcessingUtilities::DataType::UINT8_T);

					if (deviceReceivers->synchronizing)
						deviceReceivers->dataGraph->SetText(1, "IQ Signal Data Waveform Graph: Synchronizing");

					
					if (deviceReceivers->combinedFFTGraphForBandwidth && deviceReceivers->fftGraphStrengthsForDeviceRange && deviceReceivers->fftAverageGraphForDeviceRange && deviceReceivers->fftAverageGraphStrengthsForDeviceRange)
					{	

						deviceReceivers->combinedFFTGraphForBandwidth->SetGraphFrequencyRangeText("FFT: %.2f-%.2fMHz", frequencyRange, 1);

						/*FrequencyRange selectedFrequencyRange = SignalProcessingUtilities::GetSelectedFrequencyRangeFromDataRange(deviceReceivers->combinedFFTGraphForBandwidth->startDataIndex, deviceReceivers->combinedFFTGraphForBandwidth->endDataIndex, 0, deviceReceivers->combinedFFTGraphForBandwidth->GetPointsCount(), frequencyRange->lower, frequencyRange->upper);

						char textBuffer[255];
						sprintf(textBuffer, "FFT: %.2f-%.2fMHz", SignalProcessingUtilities::ConvertToMHz(selectedFrequencyRange.lower), SignalProcessingUtilities::ConvertToMHz(selectedFrequencyRange.upper));
						deviceReceivers->combinedFFTGraphForBandwidth->SetText(1, textBuffer);						
						deviceReceivers->combinedFFTGraphForBandwidth->SetGraphLabelValuesXAxis(SignalProcessingUtilities::ConvertToMHz(selectedFrequencyRange.lower), SignalProcessingUtilities::ConvertToMHz(selectedFrequencyRange.upper));
						*/

						spectrumAnalyzer->GetFFTData(fftBuffer, samplesCount, -1, frequencyRange->lower, frequencyRange->upper, ReceivingDataBufferSpecifier::CurrentBuffer);
						deviceReceivers->combinedFFTGraphForBandwidth->SetData(&fftBuffer[1], samplesCount - 1, 0, true, 0, 0, !spectrumAnalyzer->usePhase);

						double signalStrength, avgSignalStrength, recentAvgSignalStrength;

						FrequencyRange selectedFrequencyRange = SignalProcessingUtilities::GetSelectedFrequencyRangeFromDataRange(deviceReceivers->combinedFFTGraphForBandwidth->startDataIndex, deviceReceivers->combinedFFTGraphForBandwidth->endDataIndex, 0, deviceReceivers->combinedFFTGraphForBandwidth->GetPointsCount(), frequencyRange->lower, frequencyRange->upper);
						signalStrength = spectrumAnalyzer->GetStrengthForRange(selectedFrequencyRange.lower, selectedFrequencyRange.upper, ReceivingDataBufferSpecifier::CurrentBuffer);

						fftBuffer[0][0] = 0;
						fftBuffer[0][1] = signalStrength;

						fftBuffer[1][0] = 0;
						fftBuffer[1][1] = signalStrength;

						deviceReceivers->fftGraphStrengthsForDeviceRange->SetData(fftBuffer, 2, referenceDevice ? 0 : 1);

						deviceReceivers->fftAverageGraphForDeviceRange->SetGraphFrequencyRangeText("Averaged FFT: %.2f-%.2fMHz", frequencyRange, 1);

						/*selectedFrequencyRange = SignalProcessingUtilities::GetSelectedFrequencyRangeFromDataRange(deviceReceivers->fftAverageGraphForDeviceRange->startDataIndex, deviceReceivers->fftAverageGraphForDeviceRange->endDataIndex, 0, deviceReceivers->fftAverageGraphForDeviceRange->GetPointsCount(), frequencyRange->lower, frequencyRange->upper);

						sprintf(textBuffer, "Averaged FFT: %.2f-%.2fMHz", SignalProcessingUtilities::ConvertToMHz(selectedFrequencyRange.lower), SignalProcessingUtilities::ConvertToMHz(selectedFrequencyRange.upper));
						deviceReceivers->fftAverageGraphForDeviceRange->SetText(1, textBuffer);						

						deviceReceivers->fftAverageGraphForDeviceRange->SetGraphLabelValuesXAxis(SignalProcessingUtilities::ConvertToMHz(selectedFrequencyRange.lower), SignalProcessingUtilities::ConvertToMHz(selectedFrequencyRange.upper));
						*/

						FFTSpectrumBuffer* undeterminedAndNearBuffer = spectrumAnalyzer->GetFFTSpectrumBuffer(ReceivingDataMode::NearAndUndetermined);

						undeterminedAndNearBuffer->ClearData();

						spectrumAnalyzer->GetFFTSpectrumBuffer(0)->TransferDataToFFTSpectrumBuffer(undeterminedAndNearBuffer);
						spectrumAnalyzer->GetFFTSpectrumBuffer(2)->TransferDataToFFTSpectrumBuffer(undeterminedAndNearBuffer);

						spectrumAnalyzer->GetFFTData(fftBuffer, samplesCount, ReceivingDataMode::NearAndUndetermined, frequencyRange->lower, frequencyRange->upper, ReceivingDataBufferSpecifier::AveragedBuffer);

						deviceReceivers->fftAverageGraphForDeviceRange->SetData(&fftBuffer[1], samplesCount - 1, ReceivingDataMode::Near, true, 0, 0, !spectrumAnalyzer->usePhase);

						spectrumAnalyzer->GetFFTData(fftBuffer, samplesCount, ReceivingDataMode::Far, frequencyRange->lower, frequencyRange->upper, ReceivingDataBufferSpecifier::AveragedBuffer);

						deviceReceivers->fftAverageGraphForDeviceRange->SetData(&fftBuffer[1], samplesCount - 1, ReceivingDataMode::Far, true, 0, 0, !spectrumAnalyzer->usePhase);

						//deviceReceivers->dataGraph->SetText(0, "Near Frames: %d Far Frames: %d", undeterminedAndNearBuffer->GetFrameCountForRange(), spectrumAnalyzer->GetFFTSpectrumBuffer(1)->GetFrameCountForRange());
						deviceReceivers->spectrumRangeGraph->SetText(0, "Near Frames: %d Far Frames: %d", undeterminedAndNearBuffer->GetFrameCountForRange(), spectrumAnalyzer->GetFFTSpectrumBuffer(1)->GetFrameCountForRange());

						spectrumAnalyzer->GetFFTData(fftBuffer, samplesCount, ReceivingDataMode::Far, frequencyRange->lower, frequencyRange->upper, ReceivingDataBufferSpecifier::AveragedBuffer);

						fftBuffer[0][0] = 0;
						fftBuffer[0][1] = 0;

						fftBuffer[1][0] = 0;
						fftBuffer[1][1] = 0;

						double gradient, avgGradient, gradientOfGradient;

						MinMax strengthRange, gradientRange;

						if (spectrumAnalyzer->currentFFTBufferIndex != ReceivingDataMode::Near && spectrumAnalyzer->currentFFTBufferIndex != ReceivingDataMode::Undetermined)
						{
							deviceReceivers->fftAverageGraphStrengthsForDeviceRange->SetData(fftBuffer, 2, ReceivingDataMode::Near);

							signalStrength = spectrumAnalyzer->GetStrengthForRange(selectedFrequencyRange.lower, selectedFrequencyRange.upper, ReceivingDataBufferSpecifier::AveragedBuffer, ReceivingDataMode::Far);

							fftBuffer[0][0] = 0;
							fftBuffer[0][1] = signalStrength;

							fftBuffer[1][0] = 0;
							fftBuffer[1][1] = signalStrength;

							deviceReceivers->fftAverageGraphStrengthsForDeviceRange->SetData(fftBuffer, 2, ReceivingDataMode::Far);

							avgSignalStrength = deviceReceivers->fftAverageGraphStrengthsForDeviceRange->GetAvgValueForIndex(ReceivingDataMode::Far, 0);
							recentAvgSignalStrength = deviceReceivers->fftAverageGraphStrengthsForDeviceRange->GetAvgValueForIndex(ReceivingDataMode::Far, 0, 50);
							strengthRange = deviceReceivers->fftAverageGraphStrengthsForDeviceRange->dataSeries[ReceivingDataMode::Far]->GetMinMax(0, 0, true, false);

							gradient = deviceReceivers->fftAverageGraphStrengthsForDeviceRange->GetGradientForIndex(ReceivingDataMode::Far, 0);
						}
						else
						{
							deviceReceivers->fftAverageGraphStrengthsForDeviceRange->SetData(fftBuffer, 2, ReceivingDataMode::Far);
							signalStrength = spectrumAnalyzer->GetStrengthForRange(selectedFrequencyRange.lower, selectedFrequencyRange.upper, ReceivingDataBufferSpecifier::AveragedBuffer, 3);


							fftBuffer[0][0] = 0;
							fftBuffer[0][1] = signalStrength;

							fftBuffer[1][0] = 0;
							fftBuffer[1][1] = signalStrength;

							deviceReceivers->fftAverageGraphStrengthsForDeviceRange->SetData(fftBuffer, 2, ReceivingDataMode::Near);
							avgSignalStrength = deviceReceivers->fftAverageGraphStrengthsForDeviceRange->GetAvgValueForIndex(ReceivingDataMode::Near, 0);
							recentAvgSignalStrength = deviceReceivers->fftAverageGraphStrengthsForDeviceRange->GetAvgValueForIndex(ReceivingDataMode::Near, 0, 50);
							strengthRange = deviceReceivers->fftAverageGraphStrengthsForDeviceRange->dataSeries[ReceivingDataMode::Near]->GetMinMax(0, 0, true, false);

							gradient = deviceReceivers->fftAverageGraphStrengthsForDeviceRange->GetGradientForIndex(ReceivingDataMode::Near, 0);
						}

						avgGradient = deviceReceivers->fftAverageGraphStrengthsForDeviceRange->GetAvgValueForIndex(2, 0);
						gradientRange = deviceReceivers->fftAverageGraphStrengthsForDeviceRange->dataSeries[2]->GetMinMax(0, 0, true, false);

						fftBuffer[0][0] = 0;
						fftBuffer[0][1] = gradient;

						fftBuffer[1][0] = 0;
						fftBuffer[1][1] = gradient;

						deviceReceivers->fftAverageGraphStrengthsForDeviceRange->SetData(fftBuffer, 2, 2);

						if (spectrumAnalyzer->usePhase)
							deviceReceivers->fftAverageGraphForDeviceRange->SetText(2, "Phase: %.4f", signalStrength * MathUtilities::RadianToDegrees);
						else
							if (spectrumAnalyzer->useRatios)
								deviceReceivers->fftAverageGraphForDeviceRange->SetText(2, "Ratio Strength: %.4f", signalStrength);
							else
								deviceReceivers->fftAverageGraphForDeviceRange->SetText(2, "Signal Strength: %.4f", signalStrength);

						deviceReceivers->fftAverageGraphForDeviceRange->SetText(3, "Gradient of Signal Strength: %.4f", gradient);

						if (deviceReceivers->spectrumRangeGraph)
						{
							deviceReceivers->spectrumRangeGraph->SetGraphFrequencyRangeText("Spectrum Graph: %.2f-%.2fMHz", &spectrumAnalyzer->maxFrequencyRange, 1);

							/*selectedFrequencyRange = SignalProcessingUtilities::GetSelectedFrequencyRangeFromDataRange(deviceReceivers->spectrumRangeGraph->startDataIndex, deviceReceivers->spectrumRangeGraph->endDataIndex, 0, deviceReceivers->spectrumRangeGraph->GetPointsCount(), spectrumAnalyzer->maxFrequencyRange.lower, spectrumAnalyzer->maxFrequencyRange.upper);

							sprintf(textBuffer, "Spectrum Graph: %.2f-%.2fMHz", SignalProcessingUtilities::ConvertToMHz(selectedFrequencyRange.lower), SignalProcessingUtilities::ConvertToMHz(selectedFrequencyRange.upper));
							deviceReceivers->spectrumRangeGraph->SetText(1, textBuffer);
							*/

							/*if (spectrumAnalyzer->spectrumBuffer == NULL)
							{
								spectrumAnalyzer->spectrumBufferSize = DeviceReceiver::FFT_SEGMENT_SAMPLE_COUNT * (spectrumAnalyzer->maxFrequencyRange.length / (double)DeviceReceiver::SAMPLE_RATE);

								spectrumAnalyzer->spectrumBuffer = new double[spectrumAnalyzer->spectrumBufferSize * 2 * sizeof(double)];
							}

							spectrumAnalyzer->GetFFTData(spectrumAnalyzer->spectrumBuffer, spectrumAnalyzer->spectrumBufferSize, 0, spectrumAnalyzer->maxFrequencyRange.lower, spectrumAnalyzer->maxFrequencyRange.upper, ReceivingDataBufferSpecifier::AveragedBuffer);
							deviceReceivers->spectrumRangeGraph->SetData((void *)spectrumAnalyzer->spectrumBuffer, spectrumAnalyzer->spectrumBufferSize, 0, true, 0, 0, !spectrumAnalyzer->usePhase);

							spectrumAnalyzer->GetFFTData(spectrumAnalyzer->spectrumBuffer, spectrumAnalyzer->spectrumBufferSize, 1, spectrumAnalyzer->maxFrequencyRange.lower, spectrumAnalyzer->maxFrequencyRange.upper, ReceivingDataBufferSpecifier::AveragedBuffer);
							deviceReceivers->spectrumRangeGraph->SetData((void *)spectrumAnalyzer->spectrumBuffer, spectrumAnalyzer->spectrumBufferSize, 1, true, 0, 0, !spectrumAnalyzer->usePhase);
							*/

						}

						if (deviceReceivers->allSessionsSpectrumRangeGraph)
						{
							char textBuffer[255];

							//deviceReceivers->spectrumboardGraph->SetText(1, "Spectrumboard: ");
							//deviceReceivers->spectrumboardGraph->SetGraphFrequencyRangeText("%.2f-%.2fMHz", &spectrumAnalyzer->maxFrequencyRange, 2);

							//deviceReceivers->allSessionsSpectrumRangeGraph->SetText(1, "Averaged Graph for", &spectrumAnalyzer->maxFrequencyRange);
							//deviceReceivers->allSessionsSpectrumRangeGraph->SetText(2, "%i Sessions:", deviceReceivers->nearFarDataAnalyzer->sessionCount);

							//sprintf(textBuffer, "%i Sessions:", deviceReceivers->nearFarDataAnalyzer->sessionCount);

							//deviceReceivers->allSessionsSpectrumRangeGraph->SetText(2, "%i Sessions %.2f-%.2fMHz:", deviceReceivers->nearFarDataAnalyzer->sessionCount, SignalProcessingUtilities::ConvertToMHz(spectrumAnalyzer->maxFrequencyRange.lower), SignalProcessingUtilities::ConvertToMHz(spectrumAnalyzer->maxFrequencyRange.upper));
							//deviceReceivers->allSessionsSpectrumRangeGraph->SetText(3, "%.2f-%.2fMHz", SignalProcessingUtilities::ConvertToMHz(spectrumAnalyzer->maxFrequencyRange.lower), SignalProcessingUtilities::ConvertToMHz(spectrumAnalyzer->maxFrequencyRange.upper));

							//deviceReceivers->allSessionsSpectrumRangeGraph->SetText(1, "Averaged Graph for", &spectrumAnalyzer->maxFrequencyRange);

							
							//deviceReceivers->allSessionsSpectrumRangeGraph->SetGraphFrequencyRangeText("%.2f-%.2fMHz", &spectrumAnalyzer->maxFrequencyRange, 3);

							/*selectedFrequencyRange = SignalProcessingUtilities::GetSelectedFrequencyRangeFromDataRange(deviceReceivers->allSessionsSpectrumRangeGraph->startDataIndex, deviceReceivers->allSessionsSpectrumRangeGraph->endDataIndex, 0, deviceReceivers->allSessionsSpectrumRangeGraph->GetPointsCount(), spectrumAnalyzer->maxFrequencyRange.lower, spectrumAnalyzer->maxFrequencyRange.upper);							

							//sprintf(textBuffer, "Average Graph for %i Sessions:", deviceReceivers->nearFarDataAnalyzer->sessionCount);
							//deviceReceivers->allSessionsSpectrumRangeGraph->SetText(1, textBuffer);

							sprintf(textBuffer, "%.2f-%.2fMHz", SignalProcessingUtilities::ConvertToMHz(selectedFrequencyRange.lower), SignalProcessingUtilities::ConvertToMHz(selectedFrequencyRange.upper));
							deviceReceivers->allSessionsSpectrumRangeGraph->SetText(2, textBuffer);
							*/
						}


						double frequency = 0;

						if (strengthDetectionSound)
							if (recentAvgSignalStrength > avgSignalStrength)
							{
								frequency = 3500 + (recentAvgSignalStrength - avgSignalStrength) / strengthRange.range * 15000;

								if (frequency > 3500 && frequency < 15000)
								{
									double rateCounter = soundRateCounter.Add(frequency);
									
									if (rateCounter > 300000)
									{
										spectrumAnalyzer->PlaySound(frequency, 100);
									}
								}
							}

						if (gradientDetectionSound)
							if (gradient > avgGradient)
							{
								frequency = ((gradient - avgGradient) / gradientRange.range) * 15000;

								if (frequency > 8500 && frequency < 15000)
								{
									if (soundRateCounter.Add() > soundThresholdCount)										
										spectrumAnalyzer->PlaySound(frequency, 100);
								}
							}

						graphRefreshTime = GetTickCount();
					}

					deviceReceivers->ReleaseReceiverGates();
					break;
				case WAIT_TIMEOUT:
					break;
				default:
					break;
				}

				if (DebuggingUtilities::DEBUGGING)
				{
					DebuggingUtilities::DebugPrint("ProcessData(): reference device ProcessData() finished\n");
				}

				CheckScheduledFFTBufferIndex();
			}
			else
			{
				dwWaitResult = WaitForSingleObject(receiverGate, 2000);

				if (DebuggingUtilities::DEBUGGING)
				{
					DebuggingUtilities::DebugPrint("ProcessData(): measurement device ProcessData() finished\n");
				}
			}

			currentSegmentIndex += DeviceReceiver::FFT_SEGMENT_BUFF_LENGTH;

			currentTime = GetTickCount();
			remainingTime = ((int32_t)receivedDuration[receivedCount - 1]) - (currentTime - receivedDatatartTime);
			segmentTime = (double)remainingTime / (segments - i);

			delay = (int32_t)(startSegmentTime + segmentTime) - currentTime;
			if (delay > 0)
				Sleep(delay);
			else
			{
				i++;
				segmentsProcessed--;
			}


			segmentDuration = currentTime - startSegmentTime;
		}
	}

	DWORD totalTime = GetTickCount() - receivedDatatartTime;
}

void DeviceReceiver::ProcessData(fftw_complex *data, uint32_t length)
{				
	DWORD currentTime;	

	uint32_t segments, segmentBufferLength;

	bool processAllData = true;
	
	if (processAllData)
	{
		segments = 1;

		segmentBufferLength = length;
	}
	else
	if (length < DeviceReceiver::FFT_SEGMENT_BUFF_LENGTH)
	{
		segments = 1;

		segmentBufferLength = length;
	}
	else
	{
		segments = length / DeviceReceiver::FFT_SEGMENT_SAMPLE_COUNT;

		segmentBufferLength = DeviceReceiver::FFT_SEGMENT_BUFF_LENGTH;
	}

	uint32_t samplesCount = length;

	if (fftBuffer == NULL)
	{
		if (processAllData)
			fftBuffer = new fftw_complex[samplesCount];
		else
			fftBuffer = new fftw_complex[FFT_SEGMENT_SAMPLE_COUNT];
	}

	uint32_t currentSegmentIndex = 0;

	int i;

	DWORD dwWaitResult;
	
	DeviceReceivers* deviceReceivers = (DeviceReceivers*)parent;
	SpectrumAnalyzer* spectrumAnalyzer = ((SpectrumAnalyzer*)deviceReceivers->parent);

	int32_t remainingTime = (int32_t)receivedDuration[receivedCount - 1] - (GetTickCount() - receivedDatatartTime);

	double segmentTime = (double)remainingTime / segments;
	DWORD startSegmentTime, segmentDuration;
	int32_t delay;

	uint32_t segmentsProcessed = segments;
	uint32_t newSegmentsCount;
	for (i = 0; i < segments; i++)
	{
		if (spectrumAnalyzer->currentFFTBufferIndex != ReceivingDataMode::Paused)
		{
			startSegmentTime = GetTickCount();

			FFT_COMPLEX_ARRAY(&data[currentSegmentIndex], fftBuffer, samplesCount, false, false);

			SignalProcessingUtilities::CalculateMagnitudesAndPhasesForArray(fftBuffer, samplesCount);			

			if (spectrumAnalyzer->deviceReceivers->count > 1)
				deviceReceivers->fftGraphForDevicesBandwidth->SetData(fftBuffer, 50, referenceDevice ? 0 : 1, true, 0, 0, !((SpectrumAnalyzer*)deviceReceivers->parent)->usePhase);

			spectrumAnalyzer->SetFFTInput(fftBuffer, frequencyRange, &data[currentSegmentIndex], samplesCount, deviceID);

			if (ReleaseSemaphore(receiverFinished, 1, NULL) == 0)
			{
				if (DebuggingUtilities::DEBUGGING)
				{
					DebuggingUtilities::DebugPrint("ReleaseSemaphore error: %d\n", GetLastError());
				}
			}

			if (DebuggingUtilities::DEBUGGING)
			{
				DebuggingUtilities::DebugPrint("ProcessData(): open deviceReceivers->receiversFinished[deviceID]\n");
			}

			if (referenceDevice)
			{
				if (DebuggingUtilities::DEBUGGING)
				{
					DebuggingUtilities::DebugPrint("referenceDevice WaitForMultipleObjects\n");
				}

				DWORD dwWaitResult = WaitForMultipleObjects(deviceReceivers->count, deviceReceivers->receiversFinished, true, 2000);

				switch (dwWaitResult)
				{
				case WAIT_OBJECT_0:
					if (DebuggingUtilities::DEBUGGING)
						DebuggingUtilities::DebugPrint("ProcessData(): deviceReceivers->receiversFinished open :-> ProcessReceiverData()\n");

					spectrumAnalyzer->ProcessReceiverData(frequencyRange, spectrumAnalyzer->useRatios);

					if (deviceReceivers->strengthGraph)
					{
						DWORD duration = 1000;
						uint32_t resultLength = 0;

						SignalProcessingUtilities::Strengths_ID_Time* strengths = spectrumAnalyzer->GetStrengthForRangeOverTimeFromCurrentBandwidthBuffer(spectrumAnalyzer->currentFFTBufferIndex, 0, 0, &duration, &resultLength);

						deviceReceivers->strengthGraph->SetData(strengths, resultLength, 0, false, 0, 0, false, SignalProcessingUtilities::DataType::DOUBLE);
					}

					if (deviceReceivers->combinedFFTGraphForBandwidth && deviceReceivers->fftGraphStrengthsForDeviceRange && deviceReceivers->fftAverageGraphForDeviceRange && deviceReceivers->fftAverageGraphStrengthsForDeviceRange)
					{
						uint8_t* referenceDataBuffer = spectrumAnalyzer->GetFFTSpectrumBuffer(spectrumAnalyzer->currentFFTBufferIndex)->GetSampleDataForDevice(0);

						uint8_t* dataBuffer = spectrumAnalyzer->GetFFTSpectrumBuffer(spectrumAnalyzer->currentFFTBufferIndex)->GetSampleDataForDevice(1);

						if (dataBuffer)
							deviceReceivers->dataGraph->SetData(dataBuffer, DeviceReceiver::FFT_SEGMENT_BUFF_LENGTH, 1, true, -128, -128, false, SignalProcessingUtilities::DataType::UINT8_T);

						if (!deviceReceivers->synchronizing)
							deviceReceivers->dataGraph->SetText(1, "IQ Signal Data Waveform Graph");
						else
							deviceReceivers->dataGraph->SetText(1, "IQ Signal Data Waveform Graph: Synchronizing");



						deviceReceivers->combinedFFTGraphForBandwidth->SetGraphFrequencyRangeText("FFT: %.2f-%.2fMHz", frequencyRange, 1);
						/*FrequencyRange selectedFrequencyRange = SignalProcessingUtilities::GetSelectedFrequencyRangeFromDataRange(deviceReceivers->combinedFFTGraphForBandwidth->startDataIndex, deviceReceivers->combinedFFTGraphForBandwidth->endDataIndex, 0, deviceReceivers->combinedFFTGraphForBandwidth->GetPointsCount(), frequencyRange->lower, frequencyRange->upper);

						char textBuffer[255];
						sprintf(textBuffer, "FFT Graph: %.2f-%.2fMHz", SignalProcessingUtilities::ConvertToMHz(selectedFrequencyRange.lower), SignalProcessingUtilities::ConvertToMHz(selectedFrequencyRange.upper));
						deviceReceivers->combinedFFTGraphForBandwidth->SetText(1, textBuffer);
						deviceReceivers->combinedFFTGraphForBandwidth->SetGraphLabelValuesXAxis(SignalProcessingUtilities::ConvertToMHz(selectedFrequencyRange.lower), SignalProcessingUtilities::ConvertToMHz(selectedFrequencyRange.upper));
						*/


						spectrumAnalyzer->GetFFTData(fftBuffer, samplesCount, -1, frequencyRange->lower, frequencyRange->upper, ReceivingDataBufferSpecifier::CurrentBuffer);
						deviceReceivers->combinedFFTGraphForBandwidth->SetData(&fftBuffer[1], 50, 0, true, 0, 0, !spectrumAnalyzer->usePhase);

						double signalStrength, avgSignalStrength, recentAvgSignalStrength;
						FrequencyRange selectedFrequencyRange = SignalProcessingUtilities::GetSelectedFrequencyRangeFromDataRange(deviceReceivers->combinedFFTGraphForBandwidth->startDataIndex, deviceReceivers->combinedFFTGraphForBandwidth->endDataIndex, 0, deviceReceivers->combinedFFTGraphForBandwidth->GetPointsCount(), frequencyRange->lower, frequencyRange->upper);
						signalStrength = spectrumAnalyzer->GetStrengthForRange(selectedFrequencyRange.lower, selectedFrequencyRange.upper, ReceivingDataBufferSpecifier::CurrentBuffer);

						fftBuffer[0][0] = 0;
						fftBuffer[0][1] = signalStrength;

						fftBuffer[1][0] = 0;
						fftBuffer[1][1] = signalStrength;

						deviceReceivers->fftGraphStrengthsForDeviceRange->SetData(fftBuffer, 2, referenceDevice ? 0 : 1);

						deviceReceivers->fftAverageGraphForDeviceRange->SetGraphFrequencyRangeText("Averaged FFT: %.2f-%.2fMHz", frequencyRange, 1);

						/*selectedFrequencyRange = SignalProcessingUtilities::GetSelectedFrequencyRangeFromDataRange(deviceReceivers->fftAverageGraphForDeviceRange->startDataIndex, deviceReceivers->fftAverageGraphForDeviceRange->endDataIndex, 0, deviceReceivers->fftAverageGraphForDeviceRange->GetPointsCount(), frequencyRange->lower, frequencyRange->upper);
						textBuffer[255];
						sprintf(textBuffer, "Averaged FFT: %.2f-%.2fMHz", SignalProcessingUtilities::ConvertToMHz(selectedFrequencyRange.lower), SignalProcessingUtilities::ConvertToMHz(selectedFrequencyRange.upper));
						deviceReceivers->fftAverageGraphForDeviceRange->SetText(1, textBuffer);

						deviceReceivers->fftAverageGraphForDeviceRange->SetGraphLabelValuesXAxis(SignalProcessingUtilities::ConvertToMHz(selectedFrequencyRange.lower), SignalProcessingUtilities::ConvertToMHz(selectedFrequencyRange.upper));
						*/

						FFTSpectrumBuffer* undeterminedAndNearBuffer = spectrumAnalyzer->GetFFTSpectrumBuffer(ReceivingDataMode::NearAndUndetermined);

						undeterminedAndNearBuffer->ClearData();

						spectrumAnalyzer->GetFFTSpectrumBuffer(0)->TransferDataToFFTSpectrumBuffer(undeterminedAndNearBuffer);
						spectrumAnalyzer->GetFFTSpectrumBuffer(2)->TransferDataToFFTSpectrumBuffer(undeterminedAndNearBuffer);

						spectrumAnalyzer->GetFFTData(fftBuffer, samplesCount, 3, frequencyRange->lower, frequencyRange->upper, ReceivingDataBufferSpecifier::AveragedBuffer);

						deviceReceivers->fftAverageGraphForDeviceRange->SetData(&fftBuffer[1], samplesCount - 1, 0, true, 0, 0, !spectrumAnalyzer->usePhase);

						deviceReceivers->dataGraph->SetText(0, "Near Frames: %d Far Frames: %d", undeterminedAndNearBuffer->GetFrameCountForRange(), spectrumAnalyzer->GetFFTSpectrumBuffer(1)->GetFrameCountForRange());

						spectrumAnalyzer->GetFFTData(fftBuffer, samplesCount, ReceivingDataMode::Far, frequencyRange->lower, frequencyRange->upper, ReceivingDataBufferSpecifier::AveragedBuffer);

						deviceReceivers->fftAverageGraphForDeviceRange->SetData(&fftBuffer[1], samplesCount - 1, 1, true, 0, 0, !spectrumAnalyzer->usePhase);

						fftBuffer[0][0] = 0;
						fftBuffer[0][1] = 0;

						fftBuffer[1][0] = 0;
						fftBuffer[1][1] = 0;

						double gradient, avgGradient, gradientOfGradient;

						MinMax strengthRange, gradientRange;

						if (spectrumAnalyzer->currentFFTBufferIndex != ReceivingDataMode::Near && spectrumAnalyzer->currentFFTBufferIndex != ReceivingDataMode::Undetermined)
						{
							deviceReceivers->fftAverageGraphStrengthsForDeviceRange->SetData(fftBuffer, 2, ReceivingDataMode::Near);

							signalStrength = spectrumAnalyzer->GetStrengthForRange(selectedFrequencyRange.lower, selectedFrequencyRange.upper, ReceivingDataBufferSpecifier::AveragedBuffer, ReceivingDataMode::Far);

							fftBuffer[0][0] = 0;
							fftBuffer[0][1] = signalStrength;

							fftBuffer[1][0] = 0;
							fftBuffer[1][1] = signalStrength;

							deviceReceivers->fftAverageGraphStrengthsForDeviceRange->SetData(fftBuffer, 2, ReceivingDataMode::Far);

							avgSignalStrength = deviceReceivers->fftAverageGraphStrengthsForDeviceRange->GetAvgValueForIndex(ReceivingDataMode::Far, 0);
							recentAvgSignalStrength = deviceReceivers->fftAverageGraphStrengthsForDeviceRange->GetAvgValueForIndex(ReceivingDataMode::Far, 0, 50);
							strengthRange = deviceReceivers->fftAverageGraphStrengthsForDeviceRange->dataSeries[ReceivingDataMode::Far]->GetMinMax(0, 0, true, false);

							gradient = deviceReceivers->fftAverageGraphStrengthsForDeviceRange->GetGradientForIndex(ReceivingDataMode::Far, 0);
						}
						else
						{
							deviceReceivers->fftAverageGraphStrengthsForDeviceRange->SetData(fftBuffer, 2, ReceivingDataMode::Far);
							signalStrength = spectrumAnalyzer->GetStrengthForRange(selectedFrequencyRange.lower, selectedFrequencyRange.upper, ReceivingDataBufferSpecifier::AveragedBuffer, 3);

							fftBuffer[0][0] = 0;
							fftBuffer[0][1] = signalStrength;

							fftBuffer[1][0] = 0;
							fftBuffer[1][1] = signalStrength;

							deviceReceivers->fftAverageGraphStrengthsForDeviceRange->SetData(fftBuffer, 2, ReceivingDataMode::Near);
							avgSignalStrength = deviceReceivers->fftAverageGraphStrengthsForDeviceRange->GetAvgValueForIndex(ReceivingDataMode::Near, 0);
							recentAvgSignalStrength = deviceReceivers->fftAverageGraphStrengthsForDeviceRange->GetAvgValueForIndex(ReceivingDataMode::Near, 0, 50);
							strengthRange = deviceReceivers->fftAverageGraphStrengthsForDeviceRange->dataSeries[ReceivingDataMode::Near]->GetMinMax(0, 0, true, false);

							gradient = deviceReceivers->fftAverageGraphStrengthsForDeviceRange->GetGradientForIndex(ReceivingDataMode::Near, 0);
						}

						avgGradient = deviceReceivers->fftAverageGraphStrengthsForDeviceRange->GetAvgValueForIndex(2, 0);
						gradientRange = deviceReceivers->fftAverageGraphStrengthsForDeviceRange->dataSeries[2]->GetMinMax(0, 0, true, false);

						fftBuffer[0][0] = 0;
						fftBuffer[0][1] = gradient;

						fftBuffer[1][0] = 0;
						fftBuffer[1][1] = gradient;

						deviceReceivers->fftAverageGraphStrengthsForDeviceRange->SetData(fftBuffer, 2, 2);

						if (spectrumAnalyzer->usePhase)
							deviceReceivers->fftAverageGraphForDeviceRange->SetText(4, "Phase: %.4f", signalStrength * MathUtilities::RadianToDegrees);
						else
							if (spectrumAnalyzer->useRatios)
								deviceReceivers->fftAverageGraphForDeviceRange->SetText(4, "Ratio Strength: %.4f", signalStrength);
							else
								deviceReceivers->fftAverageGraphForDeviceRange->SetText(4, "Signal Strength: %.4f", signalStrength);

						deviceReceivers->fftAverageGraphForDeviceRange->SetText(5, "Gradient: %.4f", gradient);

						
						deviceReceivers->spectrumRangeGraph->SetGraphFrequencyRangeText("Spectrum Graph: %.2f-%.2fMHz", &spectrumAnalyzer->maxFrequencyRange, 1);

						/*textBuffer[255];
						selectedFrequencyRange = SignalProcessingUtilities::GetSelectedFrequencyRangeFromDataRange(deviceReceivers->spectrumRangeGraph->startDataIndex, deviceReceivers->spectrumRangeGraph->endDataIndex, 0, deviceReceivers->spectrumRangeGraph->GetPointsCount(), spectrumAnalyzer->maxFrequencyRange.lower, spectrumAnalyzer->maxFrequencyRange.upper);
						sprintf(textBuffer, "Spectrum Graph: %.2f-%.2fMHz", SignalProcessingUtilities::ConvertToMHz(selectedFrequencyRange.lower), SignalProcessingUtilities::ConvertToMHz(selectedFrequencyRange.upper));
						deviceReceivers->spectrumRangeGraph->SetText(1, textBuffer);
						*/

						if (deviceReceivers->allSessionsSpectrumRangeGraph)
						{
							deviceReceivers->allSessionsSpectrumRangeGraph->SetGraphFrequencyRangeText("%.2f-%.2fMHz", &spectrumAnalyzer->maxFrequencyRange, 1);

							/*selectedFrequencyRange = SignalProcessingUtilities::GetSelectedFrequencyRangeFromDataRange(deviceReceivers->allSessionsSpectrumRangeGraph->startDataIndex, deviceReceivers->allSessionsSpectrumRangeGraph->endDataIndex, 0, deviceReceivers->allSessionsSpectrumRangeGraph->GetPointsCount(), spectrumAnalyzer->maxFrequencyRange.lower, spectrumAnalyzer->maxFrequencyRange.upper);

							//sprintf(textBuffer, "Average Graph for %i Sessions:", deviceReceivers->nearFarDataAnalyzer->sessionCount);
							//deviceReceivers->allSessionsSpectrumRangeGraph->SetText(1, textBuffer);

							sprintf(textBuffer, "%.2f-%.2fMHz", SignalProcessingUtilities::ConvertToMHz(selectedFrequencyRange.lower), SignalProcessingUtilities::ConvertToMHz(selectedFrequencyRange.upper));
							deviceReceivers->allSessionsSpectrumRangeGraph->SetText(2, textBuffer);
							*/
						}

						double frequency = 0;

						if (strengthDetectionSound)
							if (recentAvgSignalStrength > avgSignalStrength)
							{
								frequency = 3500 + (recentAvgSignalStrength - avgSignalStrength) / strengthRange.range * 15000;

								if (frequency > 3500 && frequency < 15000)
								{									
									double rateCounter = soundRateCounter.Add(frequency);
									
									if (rateCounter > 400000)
									{
										spectrumAnalyzer->PlaySound(frequency, 100);
									}
								}
							}

						if (gradientDetectionSound)
							if (gradient > avgGradient)								
							{
								frequency = ((gradient - avgGradient) / gradientRange.range) * 15000;							

								if (frequency > 8500 && frequency < 15000)
								{
									if (soundRateCounter.Add() > soundThresholdCount)										
										spectrumAnalyzer->PlaySound(frequency, 100);
								}
							}


						graphRefreshTime = GetTickCount();
					}					
					
					deviceReceivers->ReleaseReceiverGates();
				case WAIT_TIMEOUT:
					break;
				default:
					break;
				}

				if (DebuggingUtilities::DEBUGGING)
				{
					DebuggingUtilities::DebugPrint("ProcessData(): reference device ProcessData() finished\n");
				}

				CheckScheduledFFTBufferIndex();
			}
			else
			{
				dwWaitResult = WaitForSingleObject(receiverGate, 2000);

				if (DebuggingUtilities::DEBUGGING)
				{
					DebuggingUtilities::DebugPrint("ProcessData(): measurement device ProcessData() finished\n");
				}
			}

			currentSegmentIndex += DeviceReceiver::FFT_SEGMENT_BUFF_LENGTH;

			currentTime = GetTickCount();
			remainingTime = ((int32_t)receivedDuration[receivedCount - 1]) - (currentTime - receivedDatatartTime);
			segmentTime = (double)remainingTime / (segments - i);

			delay = (int32_t)(startSegmentTime + segmentTime) - currentTime;			
			if (delay > 0)
				Sleep(delay);
			else
			{
				i++;
				segmentsProcessed--;
			}

			segmentDuration = currentTime - startSegmentTime;
		}
	}

	DWORD totalTime = GetTickCount() - receivedDatatartTime;
	DebuggingUtilities::DebugPrint("Segments processed: %d\n", segmentsProcessed);
}


void ProcessReceivedDataThread(void *param)
{
	DeviceReceiver* deviceReceiver = (DeviceReceiver*)param;
	DeviceReceivers* deviceReceivers = (DeviceReceivers*)deviceReceiver->parent;

	uint32_t requiredBytesForSamples;
	uint32_t segments;

	bool sameData;

	while (deviceReceiver->receivingData)
	{
		if (((SpectrumAnalyzer*)deviceReceivers->parent)->currentFFTBufferIndex != -1)
		{
			if (deviceReceiver->rtlDataAvailableGate == NULL)
			{
				deviceReceiver->rtlDataAvailableGate = CreateSemaphore(
					NULL,           // default security attributes
					0,  // initial count
					10,  // maximum count
					NULL);          // unnamed semaphore

				if (deviceReceiver->rtlDataAvailableGate == NULL)
				{
					if (DebuggingUtilities::DEBUGGING)
					{
						DebuggingUtilities::DebugPrint("CreateSemaphore error: %d\n", GetLastError());
					}
				}
			}

			if (deviceReceiver->referenceDevice)
			{
				if (ReleaseSemaphore(deviceReceivers->receiveDataGate1, 1, NULL))
				{
					if (DebuggingUtilities::DEBUGGING)
						DebuggingUtilities::DebugPrint("ReleaseSemaphore error: %d\n", GetLastError());
				}

				if (DebuggingUtilities::DEBUGGING)
					DebuggingUtilities::DebugPrint("ProcessReceivedDataThread(): open receiveDataGate1 :-> wait for rtlDataAvailableGate\n");
			}
			else
			{
				if (ReleaseSemaphore(deviceReceivers->receiveDataGate2, 1, NULL))
				{
					if (DebuggingUtilities::DEBUGGING)
						DebuggingUtilities::DebugPrint("ReleaseSemaphore error: %d\n", GetLastError());
				}

				if (DebuggingUtilities::DEBUGGING)
					DebuggingUtilities::DebugPrint("ProcessReceivedDataThread(): open receiveDataGate2:-> wait for rtlDataAvailableGate\n");
			}


			DWORD dwWaitResult = WaitForSingleObject(deviceReceiver->rtlDataAvailableGate, 2000);

			fftw_complex* receivedBufferComplex, *receivedBufferComplexAveraged, *receivedBufferComplexAveragedFFT;
			uint32_t receivedBufferComplexSampleLength;
			uint32_t receivedBufferComplexAveragedSegments = 256;

			double strength = 0.0001;

			switch (dwWaitResult)
			{
			case WAIT_OBJECT_0:
				/*if (deviceReceiver->receivedBuffer == NULL || deviceReceiver->receivedBufferLength != deviceReceiver->receivedLength)
				{
					deviceReceiver->receivedBuffer = new uint8_t[deviceReceiver->receivedLength * 2];

					deviceReceiver->receivedBufferLength = deviceReceiver->receivedLength;
				}

				////memcpy(deviceReceiver->receivedBuffer, deviceReceiver->receivedBufferPtr, deviceReceiver->receivedLength);
				////memcpy(deviceReceiver->receivedBuffer, deviceReceiver->gnuReceivedBuffer, deviceReceiver->receivedLength);

				deviceReceiver->receivedBuffer = deviceReceiver->gnuReceivedBuffer;

				for (int i = 0; i < deviceReceiver->receivedLength / 10; i++)
					((uint8_t*)deviceReceiver->receivedBuffer)[i] = ((float)rand() / RAND_MAX) * 255;

				//((DeviceReceivers*)deviceReceiver->parent)->dataGraph->SetData(deviceReceiver->receivedBuffer, DeviceReceiver::FFT_SEGMENT_BUFF_LENGTH, 0, true, -128, -128, true, SignalProcessingUtilities::DataType::UINT8_T);
				//continue;

				if (DebuggingUtilities::DEBUGGING)
					DebuggingUtilities::DebugPrint("ProcessReceivedDataThread(): rtlDataAvailableGate open:-> ProcessData()\n");

				deviceReceiver->WriteReceivedDataToBuffer(deviceReceiver->receivedBuffer, deviceReceiver->receivedLength);
				*/

				requiredBytesForSamples = deviceReceiver->receivedLength / 8 * 2;
				if (deviceReceiver->receivedBuffer == NULL || deviceReceiver->receivedBufferLength < requiredBytesForSamples)
				{
					deviceReceiver->receivedBufferLength = requiredBytesForSamples;

					deviceReceiver->receivedBuffer = new uint8_t[deviceReceiver->receivedBufferLength];
				}

				if (DeviceReceiver::RECEIVING_GNU_DATA)
					ArrayUtilities::ConvertByteArrayOfFloatsToBytes(deviceReceiver->receivedBufferPtr, deviceReceiver->receivedLength, deviceReceiver->receivedBuffer);

				/*
				if (deviceReceiver->prevData == NULL)
					deviceReceiver->prevData = new uint8_t[deviceReceiver->receivedLength];

				//memcpy(prevData, gnuReceivedBuffer, 100);

				sameData = true;
				for (int i = 0; i < deviceReceiver->receivedLength; i++)
					if (deviceReceiver->prevData[i] != deviceReceiver->receivedBufferPtr[i])
					{
						sameData = false;
						break;
					}

				memcpy(deviceReceiver->prevData, deviceReceiver->receivedBufferPtr, deviceReceiver->receivedLength);
				

				if (deviceReceiver->prevProcessingSegment == deviceReceiver->processingSegment)
					sameData = true;

				deviceReceiver->prevProcessingSegment = deviceReceiver->processingSegment;
				*/
					
				
				//memcpy(deviceReceiver->receivedBuffer, deviceReceiver->receivedBufferPtr, deviceReceiver->receivedLength);

				deviceReceiver->WriteReceivedDataToBuffer(deviceReceiver->receivedBuffer, deviceReceiver->receivedBufferLength);

				if (!((SpectrumAnalyzer*)deviceReceivers->parent)->eegStrength)
				{
					//deviceReceiver->GetDelayAndPhaseShiftedData(deviceReceiver->receivedBuffer, deviceReceiver->receivedLength);
					deviceReceiver->GetDelayAndPhaseShiftedData(deviceReceiver->receivedBuffer, deviceReceiver->receivedBufferLength);

					//segments = deviceReceiver->receivedLength / deviceReceiver->RECEIVE_BUFF_LENGTH;
					segments = deviceReceiver->receivedBufferLength / deviceReceiver->RECEIVE_BUFF_LENGTH;

					for (int i = 0; i < segments; i++)
					{
					deviceReceiver->ProcessData(&deviceReceiver->receivedBuffer[i*DeviceReceiver::FFT_SEGMENT_BUFF_LENGTH], DeviceReceiver::FFT_SEGMENT_BUFF_LENGTH);
						//((DeviceReceivers*)deviceReceiver->parent)->dataGraph->SetData(&deviceReceiver->receivedBuffer[i*DeviceReceiver::FFT_SEGMENT_BUFF_LENGTH], DeviceReceiver::FFT_SEGMENT_BUFF_LENGTH, 0, true, -128, -128, true, SignalProcessingUtilities::DataType::UINT8_T);
					}
				}
				else
				{
					uint8_t ffts = 20;

					uint32_t segmentShifts = deviceReceiver->RECEIVE_BUFF_LENGTH / ffts;

					uint32_t startIndex = 0;

					deviceReceiver->GetDelayAndPhaseShiftedData(deviceReceiver->receivedBuffer, deviceReceiver->receivedLength * 2, 0, deviceReceiver->receivedLength * 2);

					for (uint8_t i = 0; i < ffts; i++)
					{
						receivedBufferComplex = ArrayUtilities::ConvertArrayToComplex(&deviceReceiver->receivedBuffer[startIndex], deviceReceiver->receivedLength);

						receivedBufferComplexSampleLength = deviceReceiver->receivedLength / 2;

						/*for (unsigned int k = 0; k < receivedBufferComplexSampleLength; k++) //add a frequency to the data
						{
							receivedBufferComplex[k][0] *= 1 + (strength*deviceReceiver->GetSampleAtIndex(k, receivedBufferComplexSampleLength, GetTickCount()));
							receivedBufferComplex[k][1] *= 1 + (strength*deviceReceiver->GetSampleAtIndex(k, receivedBufferComplexSampleLength, GetTickCount(), false));
							////dataBuffer[k * 2 + 1] = 127;

							////receivedBufferComplex[k][0] += ((float)rand() / \) - 0.5;
							////receivedBufferComplex[k][1] += ((float)rand() / RAND_MAX) - 0.5;

							receivedBufferComplex[k][0] += ((float)rand() / RAND_MAX) / 2;
							receivedBufferComplex[k][1] += ((float)rand() / RAND_MAX) / 2;

							receivedBufferComplex[k][0] = MathUtilities::Round(receivedBufferComplex[k][0], 0);
							receivedBufferComplex[k][1] = MathUtilities::Round(receivedBufferComplex[k][1], 0);
						}
						*/


						SignalProcessingUtilities::CalculateMagnitudesAndPhasesForArray(receivedBufferComplex, receivedBufferComplexSampleLength);
						receivedBufferComplexAveraged = ArrayUtilities::AverageArray(receivedBufferComplex, receivedBufferComplexSampleLength, receivedBufferComplexAveragedSegments, true);
						SignalProcessingUtilities::SetQ(receivedBufferComplexAveraged, receivedBufferComplexAveragedSegments, 0);

						receivedBufferComplexAveraged = ArrayUtilities::ResizeArrayAndData(receivedBufferComplexAveraged, receivedBufferComplexAveragedSegments, DeviceReceiver::FFT_SEGMENT_SAMPLE_COUNT);

						deviceReceivers->dataGraph->SetData(receivedBufferComplexAveraged, DeviceReceiver::FFT_SEGMENT_SAMPLE_COUNT, 0, true, 0, 0, true, SignalProcessingUtilities::DataType::FFTW_COMPLEX);

						/*//// code for ffting strength of samples *////
						deviceReceiver->ProcessData(receivedBufferComplexAveraged, DeviceReceiver::FFT_SEGMENT_SAMPLE_COUNT);

						delete receivedBufferComplex;
						delete receivedBufferComplexAveraged;

						startIndex += segmentShifts;
					}
				}
				

			case WAIT_TIMEOUT:
				break;
			default:
				break;
			}
		}
		else
		{
			deviceReceiver->CheckScheduledFFTBufferIndex();
			Sleep(100);
		}
	}
}

void ProcessReceivedDataThread2(void *param)
{
	DeviceReceiver* deviceReceiver = (DeviceReceiver*)param;
	DeviceReceivers* deviceReceivers = (DeviceReceivers*)deviceReceiver->parent;

	while (deviceReceiver->receivingData)
	{
		if (((SpectrumAnalyzer*)deviceReceivers->parent)->currentFFTBufferIndex != -1)
		{
			if (deviceReceiver->rtlDataAvailableGate == NULL)
			{
				deviceReceiver->rtlDataAvailableGate = CreateSemaphore(
					NULL,           // default security attributes
					0,  // initial count
					10,  // maximum count
					NULL);          // unnamed semaphore

				if (deviceReceiver->rtlDataAvailableGate == NULL)
				{
					if (DebuggingUtilities::DEBUGGING)
					{
						DebuggingUtilities::DebugPrint("CreateSemaphore error: %d\n", GetLastError());
					}
				}
			}


			if (deviceReceiver->referenceDevice)
			{
				if (ReleaseSemaphore(deviceReceivers->receiveDataGate1, 1, NULL))
				{
					if (DebuggingUtilities::DEBUGGING)
						DebuggingUtilities::DebugPrint("ReleaseSemaphore error: %d\n", GetLastError());
				}

				if (DebuggingUtilities::DEBUGGING)
					DebuggingUtilities::DebugPrint("ProcessReceivedDataThread(): open receiveDataGate1 :-> wait for rtlDataAvailableGate\n");
			}
			else
			{
				if (ReleaseSemaphore(deviceReceivers->receiveDataGate2, 1, NULL))
				{
					if (DebuggingUtilities::DEBUGGING)
						DebuggingUtilities::DebugPrint("ReleaseSemaphore error: %d\n", GetLastError());
				}

				if (DebuggingUtilities::DEBUGGING)
					DebuggingUtilities::DebugPrint("ProcessReceivedDataThread(): open receiveDataGate2:-> wait for rtlDataAvailableGate\n");
			}


			DWORD dwWaitResult = WaitForSingleObject(deviceReceiver->rtlDataAvailableGate, 2000);

			fftw_complex* receivedBufferComplex, *receivedBufferComplexAveraged, *receivedBufferComplexAveragedFFT;
			uint32_t receivedBufferComplexSampleLength;
			uint32_t receivedBufferComplexAveragedSegments = 256;

			double strength = 0.0001;

			switch (dwWaitResult)
			{
			case WAIT_OBJECT_0:
				if (deviceReceiver->receivedBuffer == NULL || deviceReceiver->receivedBufferLength != deviceReceiver->receivedLength)
				{
					deviceReceiver->receivedBuffer = new uint8_t[deviceReceiver->receivedLength * 2];

					deviceReceiver->receivedBufferLength = deviceReceiver->receivedLength;
				}

				////memcpy(deviceReceiver->receivedBuffer, deviceReceiver->receivedBufferPtr, deviceReceiver->receivedLength);
				////memcpy(deviceReceiver->receivedBuffer, deviceReceiver->gnuReceivedBuffer, deviceReceiver->receivedLength);

				deviceReceiver->receivedBuffer = deviceReceiver->gnuReceivedBuffer;

				for (int i = 0; i < deviceReceiver->receivedLength / 10; i++)
					((uint8_t*)deviceReceiver->receivedBuffer)[i] = ((float)rand() / RAND_MAX) * 255;

				//((DeviceReceivers*)deviceReceiver->parent)->dataGraph->SetData(deviceReceiver->receivedBuffer, DeviceReceiver::FFT_SEGMENT_BUFF_LENGTH, 0, true, -128, -128, true, SignalProcessingUtilities::DataType::UINT8_T);
				//continue;

				if (DebuggingUtilities::DEBUGGING)
					DebuggingUtilities::DebugPrint("ProcessReceivedDataThread(): rtlDataAvailableGate open:-> ProcessData()\n");

				deviceReceiver->WriteReceivedDataToBuffer(deviceReceiver->receivedBuffer, deviceReceiver->receivedLength);

				/*//// code for ffting strength of samples *////
				if (((SpectrumAnalyzer*)deviceReceivers->parent)->eegStrength)
				{
					uint8_t ffts = 20;

					uint32_t segmentShifts = deviceReceiver->RECEIVE_BUFF_LENGTH / ffts;

					uint32_t startIndex = 0;

					deviceReceiver->GetDelayAndPhaseShiftedData(deviceReceiver->receivedBuffer, deviceReceiver->receivedLength * 2, 0, deviceReceiver->receivedLength * 2);

					for (uint8_t i = 0; i < ffts; i++)
					{
						receivedBufferComplex = ArrayUtilities::ConvertArrayToComplex(&deviceReceiver->receivedBuffer[startIndex], deviceReceiver->receivedLength);

						receivedBufferComplexSampleLength = deviceReceiver->receivedLength / 2;

						/*for (unsigned int k = 0; k < receivedBufferComplexSampleLength; k++) //add a frequency to the data
						{
							receivedBufferComplex[k][0] *= 1 + (strength*deviceReceiver->GetSampleAtIndex(k, receivedBufferComplexSampleLength, GetTickCount()));
							receivedBufferComplex[k][1] *= 1 + (strength*deviceReceiver->GetSampleAtIndex(k, receivedBufferComplexSampleLength, GetTickCount(), false));
							////dataBuffer[k * 2 + 1] = 127;

							////receivedBufferComplex[k][0] += ((float)rand() / \) - 0.5;
							////receivedBufferComplex[k][1] += ((float)rand() / RAND_MAX) - 0.5;

							receivedBufferComplex[k][0] += ((float)rand() / RAND_MAX) / 2;
							receivedBufferComplex[k][1] += ((float)rand() / RAND_MAX) / 2;

							receivedBufferComplex[k][0] = MathUtilities::Round(receivedBufferComplex[k][0], 0);
							receivedBufferComplex[k][1] = MathUtilities::Round(receivedBufferComplex[k][1], 0);
						}
						*/


						SignalProcessingUtilities::CalculateMagnitudesAndPhasesForArray(receivedBufferComplex, receivedBufferComplexSampleLength);
						receivedBufferComplexAveraged = ArrayUtilities::AverageArray(receivedBufferComplex, receivedBufferComplexSampleLength, receivedBufferComplexAveragedSegments, true);
						SignalProcessingUtilities::SetQ(receivedBufferComplexAveraged, receivedBufferComplexAveragedSegments, 0);

						receivedBufferComplexAveraged = ArrayUtilities::ResizeArrayAndData(receivedBufferComplexAveraged, receivedBufferComplexAveragedSegments, DeviceReceiver::FFT_SEGMENT_SAMPLE_COUNT);

						deviceReceivers->dataGraph->SetData(receivedBufferComplexAveraged, DeviceReceiver::FFT_SEGMENT_SAMPLE_COUNT, 0, true, 0, 0, true, SignalProcessingUtilities::DataType::FFTW_COMPLEX);

						/*//// code for ffting strength of samples *////
						deviceReceiver->ProcessData(receivedBufferComplexAveraged, DeviceReceiver::FFT_SEGMENT_SAMPLE_COUNT);

						delete receivedBufferComplex;
						delete receivedBufferComplexAveraged;

						startIndex += segmentShifts;
					}
				}
				else
				{
					deviceReceiver->GetDelayAndPhaseShiftedData(deviceReceiver->receivedBuffer, deviceReceiver->receivedLength);

					deviceReceiver->ProcessData(deviceReceiver->receivedBuffer, deviceReceiver->receivedLength);
				}

			case WAIT_TIMEOUT:
				break;
			default:
				break;
			}
		}
		else
		{
			deviceReceiver->CheckScheduledFFTBufferIndex();
			Sleep(100);
		}
	}
}

void ProcessReceivedDataThread3(void *param)
{
	DeviceReceiver* deviceReceiver = (DeviceReceiver*)param;
	DeviceReceivers* deviceReceivers = (DeviceReceivers*)deviceReceiver->parent;

	uint32_t segments;

	if (deviceReceiver->rtlDataAvailableGate == NULL)
	{
		deviceReceiver->rtlDataAvailableGate = CreateSemaphore(
			NULL,           // default security attributes
			0,  // initial count
			10,  // maximum count
			NULL);          // unnamed semaphore

		if (deviceReceiver->rtlDataAvailableGate == NULL)
		{
			if (DebuggingUtilities::DEBUGGING)
			{
				DebuggingUtilities::DebugPrint("CreateSemaphore error: %d\n", GetLastError());
			}
		}
	}

	//uint8_t *byteArray = new uint8_t[deviceReceiver->RECEIVE_BUFF_LENGTH];
	//uint32_t byteArrayLength = deviceReceiver->RECEIVE_BUFF_LENGTH;

	uint32_t requiredBytesForSamples;

	fftw_complex* receivedBufferComplex, *receivedBufferComplexAveraged, *receivedBufferComplexAveragedFFT;
	uint32_t receivedBufferComplexSampleLength;
	uint32_t receivedBufferComplexAveragedSegments = 256;

	while (deviceReceiver->receivingData)
	{
		DWORD dwWaitResult = WaitForSingleObject(deviceReceiver->rtlDataAvailableGate, 2000);

		switch (dwWaitResult)
		{
		case WAIT_OBJECT_0:

		if (((SpectrumAnalyzer*)deviceReceivers->parent)->currentFFTBufferIndex != -1)
		{			
				if (deviceReceiver->receivedLength > 0 && deviceReceiver->prevProcessingSegment != deviceReceiver->processingSegment)
				{
					/*
					if (deviceReceiver->receivedLength > byteArrayLength)
					{
						byteArray = new uint8_t[deviceReceiver->receivedLength];

						byteArrayLength = deviceReceiver->receivedLength;
					}*/

					/*if (deviceReceiver->prevData == NULL)
						deviceReceiver->prevData = new uint8_t[deviceReceiver->receivedLength];

					//memcpy(prevData, gnuReceivedBuffer, 100);

					bool sameData = true;
					for (int i = 0; i < deviceReceiver->receivedLength; i++)
						if (deviceReceiver->prevData[i] != deviceReceiver->receivedBufferPtr[i])
						{
							sameData = false;
							break;
						}

					memcpy(deviceReceiver->prevData, deviceReceiver->receivedBufferPtr, deviceReceiver->receivedLength);					

					if (deviceReceiver->prevProcessingSegment == deviceReceiver->processingSegment)
						sameData = true;
					*/

					deviceReceiver->prevProcessingSegment = deviceReceiver->processingSegment;
					



					if (DeviceReceiver::RECEIVING_GNU_DATA)
					{
						requiredBytesForSamples = deviceReceiver->receivedLength / 8 * 2;
						if (deviceReceiver->receivedBuffer == NULL || deviceReceiver->receivedBufferLength < requiredBytesForSamples)
						{
							deviceReceiver->receivedBufferLength = requiredBytesForSamples;

							deviceReceiver->receivedBuffer = new uint8_t[deviceReceiver->receivedBufferLength];
						}

						ArrayUtilities::ConvertByteArrayOfFloatsToBytes(deviceReceiver->receivedBufferPtr, deviceReceiver->receivedLength, deviceReceiver->receivedBuffer);

						//deviceReceiver->WriteReceivedDataToBuffer(deviceReceiver->receivedBuffer, deviceReceiver->receivedBufferLength);
					}
					else
					{
						deviceReceiver->receivedBuffer = deviceReceiver->receivedBufferPtr;
						deviceReceiver->receivedBufferLength = deviceReceiver->receivedLength;
						//deviceReceiver->WriteReceivedDataToBuffer(deviceReceiver->receivedBufferPtr, deviceReceiver->receivedLength);

						
						deviceReceiver->processingSegment++;
					}


					deviceReceiver->WriteReceivedDataToBuffer(deviceReceiver->receivedBuffer, deviceReceiver->receivedBufferLength);


					//memcpy(deviceReceiver->receivedBuffer, deviceReceiver->receivedBufferPtr, deviceReceiver->receivedLength);

					

					//deviceReceiver->GetDelayAndPhaseShiftedData(deviceReceiver->receivedBuffer, deviceReceiver->receivedLength);

					if (!((SpectrumAnalyzer*)deviceReceivers->parent)->eegStrength)
					{
						deviceReceiver->GetDelayAndPhaseShiftedData(deviceReceiver->receivedBuffer, deviceReceiver->receivedBufferLength);

						//segments = deviceReceiver->receivedLength / deviceReceiver->RECEIVE_BUFF_LENGTH;
						//original segments = deviceReceiver->receivedBufferLength / deviceReceiver->RECEIVE_BUFF_LENGTH;

						segments = deviceReceiver->receivedBufferLength / DeviceReceiver::FFT_SEGMENT_BUFF_LENGTH;

						


						for (int i = 0; i < segments; i++)
						{
							if (!((SpectrumAnalyzer*)deviceReceivers->parent)->eegStrength)
							{
								deviceReceiver->ProcessData(&deviceReceiver->receivedBuffer[i*DeviceReceiver::FFT_SEGMENT_BUFF_LENGTH], DeviceReceiver::FFT_SEGMENT_BUFF_LENGTH);
							}
						}
					}
					else
					{
						if (deviceReceiver->eegReceivedBuffer == NULL)
						{
							deviceReceiver->eegReceivedBufferLength = DeviceReceiver::SAMPLE_RATE * 2;

							deviceReceiver->eegReceivedBuffer = new uint8_t[deviceReceiver->eegReceivedBufferLength];
						}

						deviceReceiver->GetDelayAndPhaseShiftedData(deviceReceiver->eegReceivedBuffer, deviceReceiver->eegReceivedBufferLength, 1000, DeviceReceiver::SAMPLE_RATE);
						receivedBufferComplex = ArrayUtilities::ConvertArrayToComplex(deviceReceiver->eegReceivedBuffer, deviceReceiver->eegReceivedBufferLength);

						receivedBufferComplexSampleLength = deviceReceiver->receivedLength / 2;

						/*for (unsigned int k = 0; k < receivedBufferComplexSampleLength; k++) //add a frequency to the data
						{
							receivedBufferComplex[k][0] *= 1 + (strength*deviceReceiver->GetSampleAtIndex(k, receivedBufferComplexSampleLength, GetTickCount()));
							receivedBufferComplex[k][1] *= 1 + (strength*deviceReceiver->GetSampleAtIndex(k, receivedBufferComplexSampleLength, GetTickCount(), false));
							////dataBuffer[k * 2 + 1] = 127;

							////receivedBufferComplex[k][0] += ((float)rand() / \) - 0.5;
							////receivedBufferComplex[k][1] += ((float)rand() / RAND_MAX) - 0.5;

							receivedBufferComplex[k][0] += ((float)rand() / RAND_MAX) / 2;
							receivedBufferComplex[k][1] += ((float)rand() / RAND_MAX) / 2;

							receivedBufferComplex[k][0] = MathUtilities::Round(receivedBufferComplex[k][0], 0);
							receivedBufferComplex[k][1] = MathUtilities::Round(receivedBufferComplex[k][1], 0);
						}
						*/


						SignalProcessingUtilities::CalculateMagnitudesAndPhasesForArray(receivedBufferComplex, receivedBufferComplexSampleLength);
						receivedBufferComplexAveraged = ArrayUtilities::AverageArray(receivedBufferComplex, receivedBufferComplexSampleLength, receivedBufferComplexAveragedSegments, true);
						SignalProcessingUtilities::SetQ(receivedBufferComplexAveraged, receivedBufferComplexAveragedSegments, 0);

						receivedBufferComplexAveraged = ArrayUtilities::ResizeArrayAndData(receivedBufferComplexAveraged, receivedBufferComplexAveragedSegments, DeviceReceiver::FFT_SEGMENT_SAMPLE_COUNT);

						deviceReceivers->dataGraph->SetData(receivedBufferComplexAveraged, DeviceReceiver::FFT_SEGMENT_SAMPLE_COUNT, 0, true, 0, 0, true, SignalProcessingUtilities::DataType::FFTW_COMPLEX);

						/*//// code for ffting strength of samples *////
						deviceReceiver->ProcessData(receivedBufferComplexAveraged, DeviceReceiver::FFT_SEGMENT_SAMPLE_COUNT);

						delete receivedBufferComplex;
						delete receivedBufferComplexAveraged;
					}
					
				}


				//deviceReceiver->receivedBuffer = deviceReceiver->gnuReceivedBuffer;

				//SharedBuffer *sB = deviceReceiver->sharedBuffer.get();

				//memcpy(deviceReceiver->receivedBuffer, sB->gnuReceivedBuffer, deviceReceiver->receivedLength);
				////memcpy(deviceReceiver->receivedBuffer, deviceReceiver->gnuReceivedBuffer, deviceReceiver->receivedLength);


				//((DeviceReceivers*)deviceReceiver->parent)->dataGraph->SetData(sB->gnuReceivedBuffer, deviceReceiver->receivedLength, 0, true, -128, -128, true, SignalProcessingUtilities::DataType::UINT8_T);

				//for (int i = 0; i < DeviceReceiver::FFT_SEGMENT_BUFF_LENGTH / 10; i++)
					//sB->gnuReceivedBuffer[i] = ((float)rand() / RAND_MAX) * 255;


				//((DeviceReceivers*)deviceReceiver->parent)->dataGraph->SetData(sB->gnuReceivedBuffer, deviceReceiver->receivedLength, 0, true, -128, -128, true, SignalProcessingUtilities::DataType::UINT8_T);

				//((DeviceReceivers*)deviceReceiver->parent)->dataGraph->SetData(deviceReceiver->receivedBuffer, DeviceReceiver::FFT_SEGMENT_BUFF_LENGTH, 0, true, -128, -128, true, SignalProcessingUtilities::DataType::UINT8_T);

				//((DeviceReceivers*)deviceReceiver->parent)->dataGraph->SetData(deviceReceiver->sharedBuffer.get()->data(), DeviceReceiver::FFT_SEGMENT_BUFF_LENGTH, 0, true, -128, -128, true, SignalProcessingUtilities::DataType::UINT8_T);

				//uint8_t* ptr = deviceReceiver->sharedBuffer.get()->data();

				//memcpy(deviceReceiver->gnuReceivedBuffer, ptr, deviceReceiver->RECEIVE_BUFF_LENGTH);
				//deviceReceiver->sharedBuffer.get()->data();

				//((DeviceReceivers*)deviceReceiver->parent)->dataGraph->SetData(deviceReceiver->sharedBuffer.get()->data(), DeviceReceiver::FFT_SEGMENT_BUFF_LENGTH, 0, true, -128, -128, true, SignalProcessingUtilities::DataType::UINT8_T);

				//Sleep(100);
			}
		else
		{
			deviceReceiver->CheckScheduledFFTBufferIndex();
			Sleep(100);
		}

		break;
		case WAIT_TIMEOUT:
			break;
		default:
			break;

		}
	}
}

void ReceiveDataThread(void *param)
{
	ReceiveData* receiveData = (ReceiveData*)param;

	((DeviceReceiver *)receiveData->deviceReceiver)->ReceiveDataFunction(receiveData);
}

//void DeviceReceiver::ReceiveData(uint8_t *buffer, long length)
void DeviceReceiver::ReceiveDataFunction(ReceiveData* receiveData)
{	
	DeviceReceivers* deviceReceivers = (DeviceReceivers*)parent;

	if (receivedCount >= MAXRECEIVELOG)
		receivedCount = 0;

	if (deviceReceivers->receivedCount1 >= MAXRECEIVELOG)
		deviceReceivers->receivedCount1 = 0;

	if (deviceReceivers->receivedCount2 >= MAXRECEIVELOG)
		deviceReceivers->receivedCount2 = 0;


	if (referenceDevice)
		deviceReceivers->receivedTime1[deviceReceivers->receivedCount1++] = GetTickCount();
	else
		deviceReceivers->receivedTime2[deviceReceivers->receivedCount2++] = GetTickCount();

	DWORD currentTime = GetTickCount();
	if (prevReceivedTime != 0)
		receivedDuration[receivedCount++] = currentTime - prevReceivedTime;
	else
		receivedDuration[receivedCount++] = 1000;


	prevReceivedTime = GetTickCount();	


	DWORD dwWaitResult1 = 0;
	DWORD dwWaitResult2 = 0;
		
	//dwWaitResult1 = WaitForSingleObject(deviceReceivers->receiveDataGate1, 10000);
	//dwWaitResult2 = WaitForSingleObject(deviceReceivers->receiveDataGate2, 10000);
				
		
	if (dwWaitResult1 == WAIT_OBJECT_0 && dwWaitResult2 == WAIT_OBJECT_0)
	{		
		if (receiveData->buffer != NULL)
			receivedBufferPtr = receiveData->buffer;

		if (receiveData->length != 0)
			receivedLength = receiveData->length;

		//processingSegment = receiveData->processingSegment;

		if (rtlDataAvailableGate != NULL)
		{
			if (ReleaseSemaphore(rtlDataAvailableGate, 1, NULL) == 0)
			{
				if (DebuggingUtilities::DEBUGGING)
					DebuggingUtilities::DebugPrint("ReleaseSemaphore error: %d\n", GetLastError());
			}

			if (DebuggingUtilities::DEBUGGING)
			{
				if (referenceDevice)
					DebuggingUtilities::DebugPrint("Reference Device: ");
				else
					DebuggingUtilities::DebugPrint("Measurement Device: ");

				DebuggingUtilities::DebugPrint("ReceiveData(): open rtlDataAvailableGate :-> ReceiveData() finished\n");												
			}
		}
	}
	else
	{
		if (DebuggingUtilities::DEBUGGING)
			DebuggingUtilities::DebugPrint("receiveDataGate closed\n");
	}
}
	
void DataReceiver(unsigned char *buf, uint32_t len, void *ctx)
{
	struct DeviceReceiver* deviceReceiver = (DeviceReceiver*) ctx;

	ReceiveData receiveData;
	receiveData.buffer = buf;
	receiveData.length = len;

	deviceReceiver->processingSegment++;

	deviceReceiver->ReceiveDataFunction(&receiveData);
}

void ReceivingDataThread(void *param)
{
	int result;
	DeviceReceiver* deviceReceiver = (DeviceReceiver*)param;

	if (DeviceReceiver::RECEIVING_GNU_DATA)
	{
		DWORD dwWaitResult = WaitForSingleObject(((DeviceReceivers*)deviceReceiver->parent)->startReceivingDataGate, 2000);

		int bytes_received = 0;

		DWORD startDataTime = GetTickCount();

		int total_bytes_received = 0, bytesPerSecond;

		DWORD elapsed;

		//receives segments of 16384 bytes of floats = 16384/8 = 2048 IQ samples = 4096 bytes. FFT_SEGMENT_SAMPLE_COUNT samples are processed so:
		uint32_t requiredSegmentsOfByteData = 4;
		//uint32_t requiredSegmentsOfFloatData = 8192 / 2048 * requiredSegmentsOfByteData; //gets requiredSegmentsOfByteData(4) x (8192 IQ samples = 16384 bytes = FFT_SEGMENT_BUFF_LENGTH) segments

		uint32_t requiredSegmentsOfFloatData = DeviceReceiver::FFT_SEGMENT_SAMPLE_COUNT / 2048 * requiredSegmentsOfByteData; //gets requiredSegmentsOfByteData(4) x (8192 IQ samples = 16384 bytes = FFT_SEGMENT_BUFF_LENGTH) segments
		

		char *gnuReceivedBuffer = new char[deviceReceiver->RECEIVE_BUFF_LENGTH*requiredSegmentsOfFloatData];
		char *gnuReceivedBuffer2 = new char[deviceReceiver->RECEIVE_BUFF_LENGTH];
		int32_t value = 0;
		uint8_t* prevData = NULL;

		uint32_t segment = 0;
		uint32_t newProcessingSegment = 0;

		ReceiveData receiveData;


		deviceReceiver->prevReceivedTime = GetTickCount();
		while (DeviceReceiver::RECEIVING_GNU_DATA)
		{
			//bytes_received = recvfrom(deviceReceiver->sd, gnuReceivedBuffer, deviceReceiver->RECEIVE_BUFF_LENGTH, 0, (struct sockaddr *)&(deviceReceiver->server), &(deviceReceiver->server_length));
			//bytes_received = recvfrom(deviceReceiver->sd, (char *)deviceReceiver->gnuReceivedBuffer, deviceReceiver->RECEIVE_BUFF_LENGTH, 0, (struct sockaddr *)&(deviceReceiver->server), &(deviceReceiver->server_length));

			//std::shared_ptr<SharedBuffer> p;
			//p = std::make_shared<SharedBuffer>();

			//SharedBuffer *sB = deviceReceiver->sharedBuffer.get();

			//receives 16384 bytes of floats = 16384/8 = 2048 IQ samples = 4096 bytes
			bytes_received = recvfrom(deviceReceiver->sd, &gnuReceivedBuffer[segment *  deviceReceiver->RECEIVE_BUFF_LENGTH], deviceReceiver->RECEIVE_BUFF_LENGTH, 0, (struct sockaddr *)&(deviceReceiver->server), &(deviceReceiver->server_length));

			/* original for (int i = 16376;i<16384;i++)
				gnuReceivedBuffer[segment *  deviceReceiver->RECEIVE_BUFF_LENGTH + i] = 0;
				*/
			

			/*gnuReceivedBuffer[segment *  deviceReceiver->RECEIVE_BUFF_LENGTH + bytes_received - 1] = 0;
			gnuReceivedBuffer[segment *  deviceReceiver->RECEIVE_BUFF_LENGTH + bytes_received - 2] = 0;
			gnuReceivedBuffer[segment *  deviceReceiver->RECEIVE_BUFF_LENGTH + bytes_received - 3] = 0;
			gnuReceivedBuffer[segment *  deviceReceiver->RECEIVE_BUFF_LENGTH + bytes_received - 4] = 0;
			*/
			
			//bytes_received = recvfrom(deviceReceiver->sd, (char *) sB->gnuReceivedBuffer, deviceReceiver->RECEIVE_BUFF_LENGTH, 0, (struct sockaddr *)&(deviceReceiver->server), &(deviceReceiver->server_length));

			//deviceReceiver->receivedLength = bytes_received;

			total_bytes_received += bytes_received;

			elapsed = GetTickCount() - startDataTime;

			segment++;
			
			if (segment == requiredSegmentsOfFloatData) 
			{

				//deviceReceiver->receivedBufferPtr = (uint8_t *)gnuReceivedBuffer;
				//deviceReceiver->receivedLength = deviceReceiver->RECEIVE_BUFF_LENGTH*segment;

				//deviceReceiver->processingSegment++;
				//processingSegment++;

				/*
				if (deviceReceiver->prevData == NULL)
					deviceReceiver->prevData = new uint8_t[deviceReceiver->RECEIVE_BUFF_LENGTH*segment];

				//memcpy(prevData, gnuReceivedBuffer, 100);

				bool sameData = true;
				for (int i = 0; i < deviceReceiver->RECEIVE_BUFF_LENGTH*segment; i++)
					if (deviceReceiver->prevData[i] != gnuReceivedBuffer[i])
					{
						sameData = false;
						break;
					}

				memcpy(deviceReceiver->prevData, gnuReceivedBuffer, deviceReceiver->RECEIVE_BUFF_LENGTH*segment);
				
					*/

				receiveData.deviceReceiver = deviceReceiver;
				receiveData.length = deviceReceiver->RECEIVE_BUFF_LENGTH * segment;

				if (deviceReceiver->gnuReceivedBufferForProcessing == NULL)
					deviceReceiver->gnuReceivedBufferForProcessing = new uint8_t[receiveData.length];
					
				memcpy(deviceReceiver->gnuReceivedBufferForProcessing, gnuReceivedBuffer, receiveData.length);
				receiveData.buffer = (uint8_t *)deviceReceiver->gnuReceivedBufferForProcessing;
				////original receiveData.processingSegment = ++newProcessingSegment;

				deviceReceiver->processingSegment = ++newProcessingSegment;

				
				////HANDLE gnuReceiveDataThreadHandle = (HANDLE)_beginthread(ReceiveDataThread, 0, &receiveData);
				////bool result = SetThreadPriority(gnuReceiveDataThreadHandle, THREAD_PRIORITY_HIGHEST);

				deviceReceiver->ReceiveDataFunction(&receiveData);

				////deviceReceiver->ReceiveData((uint8_t *) gnuReceivedBuffer, deviceReceiver->RECEIVE_BUFF_LENGTH*segment);

				segment = 0;
			}

			value++;
			if (elapsed > 10000)
			{
				bytesPerSecond = total_bytes_received / ((elapsed) / 1000);
				value /= ((elapsed) / 1000);
				//DebuggingUtilities::DebugPrint("bytesPerSecond: %i\n", bytesPerSecond);
				//DebuggingUtilities::DebugPrint("reads: %i\n", value);
				total_bytes_received = 0;
				value = 0;
				startDataTime = GetTickCount();
			}

			
			//uint8_t* ptr = deviceReceiver->sharedBuffer.get();
			//std::memcpy(deviceReceiver->sharedBuffer.get()->data(), gnuReceivedBuffer, deviceReceiver->RECEIVE_BUFF_LENGTH);
			//memcpy((char *)deviceReceiver->gnuReceivedBuffer, gnuReceivedBuffer, bytes_received);
		}
	}
	else
	if (!DebuggingUtilities::RECEIVE_TEST_DATA)
	{
		DWORD dwWaitResult = WaitForSingleObject(((DeviceReceivers*)deviceReceiver->parent)->startReceivingDataGate, 2000);

		switch (dwWaitResult)
		{

		case WAIT_OBJECT_0:
			if (!DebuggingUtilities::TEST_CORRELATION || (DebuggingUtilities::TEST_CORRELATION && deviceReceiver->referenceDevice))
				deviceReceiver->prevReceivedTime = GetTickCount();			
			result = rtlsdr_read_async(deviceReceiver->device, DataReceiver, (void *)deviceReceiver, deviceReceiver->ASYNC_BUF_NUMBER, deviceReceiver->RECEIVE_BUFF_LENGTH);
			break;
		case WAIT_TIMEOUT:
			break;
		default:
			break;
		}
	}
	else
	{
		DWORD dwWaitResult = WaitForSingleObject(((DeviceReceivers*)deviceReceiver->parent)->startReceivingDataGate, 2000);

		while (DebuggingUtilities::RECEIVE_TEST_DATA)
		{
			deviceReceiver->ReceiveTestData(deviceReceiver->RECEIVE_BUFF_LENGTH);
		}
	}

	_endthread();
}


void ReceivingDataThread2(void *param)
{
	int result;
	DeviceReceiver* deviceReceiver = (DeviceReceiver*)param;

	if (DeviceReceiver::RECEIVING_GNU_DATA)
	{
		DWORD dwWaitResult = WaitForSingleObject(((DeviceReceivers*)deviceReceiver->parent)->startReceivingDataGate, 2000);

		int bytes_received = 0;

		DWORD startDataTime = GetTickCount();

		int total_bytes_received = 0, bytesPerSecond;

		DWORD elapsed;

		char *gnuReceivedBuffer = new char[deviceReceiver->RECEIVE_BUFF_LENGTH];
		char *gnuReceivedBuffer2 = new char[deviceReceiver->RECEIVE_BUFF_LENGTH];
		int32_t value = 0;
		uint8_t* prevData = NULL;

		while (DeviceReceiver::RECEIVING_GNU_DATA)
		{
			bytes_received = recvfrom(deviceReceiver->sd, gnuReceivedBuffer, deviceReceiver->RECEIVE_BUFF_LENGTH, 0, (struct sockaddr *)&(deviceReceiver->server), &(deviceReceiver->server_length));
			//bytes_received = recvfrom(deviceReceiver->sd, (char *)deviceReceiver->gnuReceivedBuffer, deviceReceiver->RECEIVE_BUFF_LENGTH, 0, (struct sockaddr *)&(deviceReceiver->server), &(deviceReceiver->server_length));

			memcpy((char *)deviceReceiver->gnuReceivedBuffer, gnuReceivedBuffer, bytes_received);

			/*for (int i = 0; i < bytes_received / 5; i++)
				((uint8_t*)deviceReceiver->gnuReceivedBuffer)[i] = ((float)rand() / RAND_MAX) * 255;
				*/

			////Sleep(10);

			//value++;
			/*if (prevData == NULL)
				prevData = new uint8_t[100];

			//memcpy(prevData, gnuReceivedBuffer, 100);

			bool sameData = true;
			for (int i = 0; i < 100; i++)
				if (prevData[i] != ((uint8_t*)gnuReceivedBuffer)[i])
				{
					sameData = false;
					break;
				}

			memcpy(prevData, gnuReceivedBuffer, 100);
			
				*/

			value+=1;

			/*if (bytes_received < 0)
			{				
				fprintf(stderr, "Error receiving data.\n");
				/*closesocket(deviceReceiver->sd);
				WSACleanup();
				exit(0);
				*/
			/*}
			*/

			total_bytes_received += bytes_received;

			elapsed = GetTickCount() - startDataTime;

			if (elapsed > 10000)
			{
				bytesPerSecond = total_bytes_received / ((elapsed) / 1000);				
				value /= ((elapsed) / 1000);
				DebuggingUtilities::DebugPrint("bytesPerSecond: %i\n", bytesPerSecond);
				DebuggingUtilities::DebugPrint("reads: %i\n", value);
				total_bytes_received = 0;				
				value = 0;
				startDataTime = GetTickCount();
			}

			/*for (int i = 0; i < deviceReceiver->RECEIVE_BUFF_LENGTH; i++)
			{
				//value = gnuReceivedBuffer[i] + 127;
				

				deviceReceiver->gnuReceivedBuffer[i] = gnuReceivedBuffer[i] + 127;
				////deviceReceiver->gnuReceivedBuffer[i] = ((float)rand() / RAND_MAX) * 30;
				//deviceReceiver->gnuReceivedBuffer[i] = (value + i) % 255;
			}
			
			for (int i = 0; i < deviceReceiver->RECEIVE_BUFF_LENGTH/4; i+=2)
			{
				//value = gnuReceivedBuffer[i] + 127				

				//deviceReceiver->gnuReceivedBuffer[i] = gnuReceivedBuffer[i] + 127;
				deviceReceiver->gnuReceivedBuffer[i] = ((float)rand() / RAND_MAX) * 255;
				//deviceReceiver->gnuReceivedBuffer[i] = (value + i) % 255;
			}

			/*
			for (int i = deviceReceiver->RECEIVE_BUFF_LENGTH / 4; i < deviceReceiver->RECEIVE_BUFF_LENGTH; i++)
			{
				//value = gnuReceivedBuffer[i] + 127;				

				deviceReceiver->gnuReceivedBuffer[i] = gnuReceivedBuffer[i] + 127;
				////deviceReceiver->gnuReceivedBuffer[i] = ((float)rand() / RAND_MAX) * 30;
				//deviceReceiver->gnuReceivedBuffer[i] = value % 255;
			}
			*/
			////((DeviceReceivers*)deviceReceiver->parent)->dataGraph->SetData(deviceReceiver->gnuReceivedBuffer, DeviceReceiver::FFT_SEGMENT_BUFF_LENGTH, 0, true, -128, -128, true, SignalProcessingUtilities::DataType::UINT8_T);
			ReceiveData receiveData;
			receiveData.buffer = deviceReceiver->gnuReceivedBuffer;
			receiveData.length = deviceReceiver->RECEIVE_BUFF_LENGTH;
			
			deviceReceiver->ReceiveDataFunction(&receiveData);

			//deviceReceiver->ReceiveData((uint8_t *)gnuReceivedBuffer, deviceReceiver->RECEIVE_BUFF_LENGTH);
			
		}

		closesocket(deviceReceiver->sd);
		WSACleanup();
	}
	else
		if (!DebuggingUtilities::RECEIVE_TEST_DATA)
		{
			DWORD dwWaitResult = WaitForSingleObject(((DeviceReceivers*)deviceReceiver->parent)->startReceivingDataGate, 2000);

			switch (dwWaitResult)
			{
				
			case WAIT_OBJECT_0:
				if (!DebuggingUtilities::TEST_CORRELATION || (DebuggingUtilities::TEST_CORRELATION && deviceReceiver->referenceDevice))
					deviceReceiver->prevReceivedTime = GetTickCount();
					result = rtlsdr_read_async(deviceReceiver->device, DataReceiver, (void *)deviceReceiver, deviceReceiver->ASYNC_BUF_NUMBER, deviceReceiver->RECEIVE_BUFF_LENGTH);
				break;
			case WAIT_TIMEOUT:
				break;
			default:
				break;
			}
		}
		else
		{
			DWORD dwWaitResult = WaitForSingleObject(((DeviceReceivers*)deviceReceiver->parent)->startReceivingDataGate, 2000);

			while (DebuggingUtilities::RECEIVE_TEST_DATA)
			{
				deviceReceiver->ReceiveTestData(deviceReceiver->RECEIVE_BUFF_LENGTH);
			}
		}

	_endthread();
}

void DeviceReceiver::StartReceivingData()
{	
	receivingData = true;

	if (referenceDevice || !DebuggingUtilities::TEST_CORRELATION)
	{
		receivingDataThreadHandle = (HANDLE)_beginthread(ReceivingDataThread, 0, this);
		bool result = SetThreadPriority(receivingDataThreadHandle, THREAD_PRIORITY_HIGHEST);		
	}
}


void DeviceReceiver::StartProcessingData()
{	
	processingDataThreadHandle = (HANDLE)_beginthread(ProcessReceivedDataThread3, 0, this);
	bool result = SetThreadPriority(ProcessReceivedDataThread3, THREAD_PRIORITY_HIGHEST);
}

void DeviceReceiver::StopReceivingData()
{
	receivingData = false;

	if (ReleaseSemaphore(rtlDataAvailableGate, 1, NULL) == 0)
	{
		if (DebuggingUtilities::DEBUGGING)
			DebuggingUtilities::DebugPrint("ReleaseSemaphore error: %d\n", GetLastError());
	}

	if (receivingDataThreadHandle != NULL)
	{
		rtlsdr_cancel_async(device);

		WaitForSingleObject(receivingDataThreadHandle, INFINITE);
		receivingDataThreadHandle = NULL;

		WaitForSingleObject(processingDataThreadHandle, INFINITE);
		processingDataThreadHandle = NULL;
	}
}

int DeviceReceiver::GetDelayAndPhaseShiftedData(uint8_t* dataBuffer, long dataBufferLength, float durationMilliSeconds, int durationBytes, bool async)
{		
		long readStartIndex;
		long length;
		int iValue, qValue, count = 0;

		char buffer[10];

		long seconds;

		if (!circularDataBuffer)
			return 0;
		
		DWORD dwWaitResult = 0;
		//dwWaitResult = WaitForSingleObject(receiverBufferDataAvailableGate, 2000);

		switch (dwWaitResult)
		{
		case WAIT_OBJECT_0:
			if (DebuggingUtilities::DEBUGGING)
				DebuggingUtilities::DebugPrint("GetDelayAndPhaseShiftedData(): receiverBufferDataAvailableGate open:-> circularDataBuffer->ReadData()\n");

			if (durationMilliSeconds > 0)
			{
				length = (float)durationMilliSeconds / 1000 * SAMPLE_RATE * 2;
			}
			else
				if (durationBytes > 0)
				{
					length = durationBytes;
				}
				else
					length = FFT_SEGMENT_BUFF_LENGTH;

			if (async)
				length = circularDataBuffer->ReadData(dataBuffer, length, delayShift, phaseShift);
			else
			{
				int n_read;

				rtlsdr_read_sync(device, dataBuffer, length, &n_read);

				return n_read;
			}

			return length;
			break;
		case WAIT_TIMEOUT:
			return 0;
			break;
		default:
			return 0;
			break;
		}
}

float DeviceReceiver::GetSampleAtIndex(long index, long length, long currentTime, bool realValue)
{
	unsigned int sinFrequency = 10;
	float sinMultiplier = sinFrequency * 2 * MathUtilities::PI;

	float value;

	double phase = 0;

	if (referenceDevice)
		phase = -MathUtilities::PI;
		
	if (realValue)
		value = (float)sin(sinMultiplier * (float)index / length + phase);
	else
		value = (float)cos(sinMultiplier * (float)index / length + phase);

	if (value>1.0)
		value = 1.0;

	if (value < -1.0)
		value = -1;

	if (value>1.0)
		value = 1.0;

	if (value < -1.0)
		value = -1;

	return value;
}

float DeviceReceiver::GetSampleByteAtIndex(long index, long length, long currentTime, bool realValue)
{
	return GetSampleAtIndex(index, length, currentTime, realValue) * 127 + 127;
}

void DeviceReceiver::GetTestData(uint8_t* buffer, uint32_t length)
{
	for (unsigned int k = 0; k < length; k += 2)
	{
		buffer[k] = GetSampleByteAtIndex(k, length, GetTickCount());
		buffer[k + 1] = GetSampleByteAtIndex(k, length, GetTickCount(), false);
	}
}

void DeviceReceiver::ReceiveTestData(uint32_t length)
{
	DWORD startSegment = GetTickCount();

	DeviceReceivers* deviceReceivers = (DeviceReceivers*)parent;

	if (dataBuffer == NULL)
		dataBuffer = new uint8_t [length];

	GetTestData(dataBuffer, length);

	DWORD dwWaitResult1 = 0;
	DWORD dwWaitResult2 = 0;

	if (dwWaitResult1 == WAIT_OBJECT_0 && dwWaitResult2 == WAIT_OBJECT_0)
	{
		if (DebuggingUtilities::DEBUGGING)
		{
			DebuggingUtilities::DebugPrint("ReceiveData(): receiveDataGate1 && receiveDataGate2 open:-> receivedBuffer = buffer\n");			
		}

		receivedBufferPtr = dataBuffer;
		receivedLength = length;

		if (rtlDataAvailableGate != NULL)
		{
			if (ReleaseSemaphore(rtlDataAvailableGate, 1, NULL) == 0)
			{
				if (DebuggingUtilities::DEBUGGING)
					DebuggingUtilities::DebugPrint("ReleaseSemaphore error: %d\n", GetLastError());
			}

			if (DebuggingUtilities::DEBUGGING)
			{
				DebuggingUtilities::DebugPrint("ReceiveDataTestData(): open rtlDataAvailableGate :-> ReceiveDataTestData() finished\n");
			}
		}
	}
	else
	{
		if (DebuggingUtilities::DEBUGGING)
		{
			DebuggingUtilities::DebugPrint("receiveDataGate closed\n");
			
		}
	}
}

void DeviceReceiver::SetDelayShift(double value)
{
	delayShift += value;	

	if (delayShift < -DeviceReceiver::FFT_SEGMENT_BUFF_LENGTH*2/3 || delayShift > DeviceReceiver::FFT_SEGMENT_BUFF_LENGTH*2/3)
		delayShift = 0;
}

void DeviceReceiver::SetPhaseShift(double value)
{
	phaseShift += value;
}

void DeviceReceiver::AddDeviceToSendClonedDataTo(DeviceReceiver *deviceReceiver)
{
	devicesToSendClonedDataTo[devicesToSendClonedDataToCount++] = deviceReceiver;	
}

void DeviceReceiver::GetDeviceCurrentFrequencyRange(uint32_t* startFrequency, uint32_t* endFrequency)
{
	*startFrequency = frequencyRange->lower;
	*endFrequency = frequencyRange->upper;	
}

DeviceReceiver::~DeviceReceiver()
{	
	if (complexArray != NULL)
		fftw_free(complexArray);

	if (complexArray2 != NULL)
		fftw_free(complexArray2);

	if (complexArrayFFTPlan != NULL)
	{
		fftw_destroy_plan(complexArrayFFTPlan);
		fftw_destroy_plan(complexArrayFFTPlan2);
		fftw_destroy_plan(complexArrayCorrelationFFTPlan);

		complexArrayFFTPlan = NULL;
		complexArrayFFTPlan2 = NULL;
		complexArrayCorrelationFFTPlan = NULL;
	}

	delete circularDataBuffer;	

	if (dataBuffer != NULL)
		delete[] dataBuffer;
}


bool DeviceReceiver::RECEIVING_GNU_DATA = true;
//uint32_t DeviceReceiver::SEGMENT_BANDWIDTH = 1024000;
uint32_t DeviceReceiver::SEGMENT_BANDWIDTH = 1000000;
uint32_t DeviceReceiver::SAMPLE_RATE = SEGMENT_BANDWIDTH;

//original long DeviceReceiver::FFT_SEGMENT_BUFF_LENGTH_FOR_SEGMENT_BANDWIDTH = 16384;
//long DeviceReceiver::FFT_SEGMENT_BUFF_LENGTH_FOR_SEGMENT_BANDWIDTH = 1024;
long DeviceReceiver::FFT_SEGMENT_BUFF_LENGTH_FOR_SEGMENT_BANDWIDTH = 4096 * 2;
//long DeviceReceiver::FFT_SEGMENT_BUFF_LENGTH_FOR_SEGMENT_BANDWIDTH = 4096 * 4;
long DeviceReceiver::FFT_SEGMENT_BUFF_LENGTH = SAMPLE_RATE / SEGMENT_BANDWIDTH * FFT_SEGMENT_BUFF_LENGTH_FOR_SEGMENT_BANDWIDTH;
long DeviceReceiver::FFT_SEGMENT_SAMPLE_COUNT = FFT_SEGMENT_BUFF_LENGTH / 2;

long DeviceReceiver::RECEIVE_BUFF_LENGTH = 16384;
long DeviceReceiver::CORRELATION_BUFF_LENGTH = FFT_SEGMENT_BUFF_LENGTH * 2;

uint32_t DeviceReceiver::MAXRECEIVELOG = 100;
