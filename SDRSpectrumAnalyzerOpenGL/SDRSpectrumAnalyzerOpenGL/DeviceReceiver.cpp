#define _ITERATOR_DEBUG_LEVEL 0
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


DeviceReceiver::DeviceReceiver(void* parent, long bufferSizeInMilliSeconds, uint32_t sampleRate, uint8_t ID)
{
	this->parent = parent;
	this->deviceID = ID;	

	SAMPLE_RATE = 1000000; //1 second of samples
	//RECEIVE_BUFF_LENGTH = SignalProcessingUtilities::ClosestIntegerMultiple(SAMPLE_RATE * 2, 16384);
	

	////RECEIVE_BUFF_LENGTH = FFT_SEGMENT_BUFF_LENGTH * 10;
	//RECEIVE_BUFF_LENGTH = FFT_SEGMENT_BUFF_LENGTH * 2;
	RECEIVE_BUFF_LENGTH = FFT_SEGMENT_BUFF_LENGTH;

	if (DebuggingUtilities::RECEIVE_TEST_DATA)
		RECEIVE_BUFF_LENGTH = 200;

	if (RECEIVE_BUFF_LENGTH < FFT_SEGMENT_BUFF_LENGTH)
		FFT_SEGMENT_BUFF_LENGTH = RECEIVE_BUFF_LENGTH;

	////FFT_SEGMENT_BUFF_LENGTH = RECEIVE_BUFF_LENGTH;

	CORRELATION_BUFF_LENGTH = FFT_SEGMENT_BUFF_LENGTH * 2;
	FFT_SEGMENT_SAMPLE_COUNT = FFT_SEGMENT_BUFF_LENGTH / 2;

	/*////
	receiveDataGate = CreateSemaphore(
		NULL,           // default security attributes
		0,  // initial count
		100,  // maximum count
		NULL);          // unnamed semaphore

	if (receiveDataGate == NULL)
	{
		DebuggingUtilities::DebugPrint("CreateSemaphore error: %d\n", GetLastError());
	}*/

	/*////rtlDataAvailableGate = CreateSemaphore(
			NULL,           // default security attributes
			0,  // initial count
			10,  // maximum count
			NULL);          // unnamed semaphore

	if (rtlDataAvailableGate == NULL)
	{
		if (DebuggingUtilities::DEBUGGING)
		{
			DebuggingUtilities::DebugPrint("CreateSemaphore error: %d\n", GetLastError());			
		}
	}*/

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
	//gnuReceivedBuffer = new uint8_t[4096];

	/*////
	complexArrayFFTPlans2[0] = NULL;
	complexArrayFFTPlans2[1] = NULL;
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
		client.sin_addr.s_addr = inet_addr("10.0.0.3");
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
	if (dev_index == 1)
	{
		if (rtlsdr_set_tuner_gain(device, gain) != 0)
			//if (rtlsdr_set_tuner_gain(device, 0) != 0)
		{
			fprintf(stderr, "[ ERROR ] Failed to set gain value: %s\n", strerror(errno));
		}
	}
	else
	{
		if (rtlsdr_set_tuner_gain(device, gain) != 0)
			//if (rtlsdr_set_tuner_gain(device, 0) != 0)
		{
			fprintf(stderr, "[ ERROR ] Failed to set gain value: %s\n", strerror(errno));
		}
	}	
	

	////rtlsdr_set_center_freq(device, 96000000);
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
	////unsigned int measure = FFTW_ESTIMATE;

	int j = 0;

	////inverse = true;

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
		////if (samples == DeviceReceiver::FFT_SEGMENT_SAMPLE_COUNT)
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
		////if (complexArrayFFTPlans2[(referenceDevice ? 0 : 1)] == NULL && samples == DeviceReceiver::FFT_SEGMENT_SAMPLE_COUNT * 2)
		if (complexArrayFFTPlan2 == NULL && samples == DeviceReceiver::FFT_SEGMENT_SAMPLE_COUNT * 2)
		////if (samples == DeviceReceiver::FFT_SEGMENT_SAMPLE_COUNT * 2)
		////if (true)
		{					
			if (synchronizing && !referenceDevice)
				WaitForSingleObject(deviceReceivers->fftBytesGate, 2000);
			
			////complexArrayFFTPlans2[(referenceDevice ? 0 : 1)] = fftw_plan_dft_1d(samples,
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

		/*////if (samples == DeviceReceiver::FFT_SEGMENT_SAMPLE_COUNT)
			fftw_execute_dft(deviceReceivers->complexArrayFFTPlan, array, fftData);
		else
			if (samples == DeviceReceiver::FFT_SEGMENT_SAMPLE_COUNT * 2)
				////fftw_execute_dft(deviceReceivers->complexArrayFFTPlan2, array, fftData);
				fftw_execute_dft(deviceReceivers->complexArrayFFTPlans2[(referenceDevice ? 0 : 1)], array, fftData);
			else
				fftw_execute_dft(deviceReceivers->complexArrayFFTPlan, array, fftData);
				*/
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
				////fftw_execute_dft(complexArrayFFTPlans2[(referenceDevice ? 0 : 1)], array, fftData);
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
	////unsigned int measure = FFTW_ESTIMATE;

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

	/*////deviceReceivers->complexArrayFFTPlan2 = fftw_plan_dft_1d(samples,
		data,
		fftData,
		inverse ? FFTW_BACKWARD : FFTW_FORWARD,
		measure);
		*/


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
		/*////for (int i = 0; i < receivedLength; i++)
		receivedBuffer[i] = ((float)rand() / RAND_MAX) * 255;
		*/

		if (receivedLength == RECEIVE_BUFF_LENGTH)
		{			
			////memcpy(transferDataBuffer, &receivedBuffer[4], receivedLength - 4);
			memcpy(&transferDataBuffer[4], receivedBuffer, receivedLength - 4);
			////memcpy(transferDataBuffer, receivedBuffer, receivedLength);		
		}
		else
		{
			int grc = 1;
		}
	}

	circularDataBuffer->WriteData(receivedBuffer, receivedLength);

	for (int i = 0; i < devicesToSendClonedDataToCount; i++)
		devicesToSendClonedDataTo[i]->ReceiveData(transferDataBuffer, receivedLength);

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

	////if (spectrumAnalyzer->scheduledFFTBufferIndex > -1)
	{
		spectrumAnalyzer->currentFFTBufferIndex = spectrumAnalyzer->scheduledFFTBufferIndex;

		////spectrumAnalyzer->scheduledFFTBufferIndex = -1;
	}
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

	/////uint32_t currentSegmentIndex = delayShift;

	////SignalProcessingUtilities::Rotate(data, length, -phaseShift);

	////fftw_complex* fftBuffer = new fftw_complex[DeviceReceiver::FFT_SEGMENT_SAMPLE_COUNT];

	int i;

	DWORD dwWaitResult;

	DeviceReceivers* deviceReceivers = (DeviceReceivers*)parent;
	SpectrumAnalyzer* spectrumAnalyzer = ((SpectrumAnalyzer*)deviceReceivers->parent);

	int32_t remainingTime = (int32_t)receivedDuration[receivedCount - 1] - (GetTickCount() - receivedDatatartTime);

	double segmentTime = (double)remainingTime / segments;
	DWORD startSegmentTime, segmentDuration;
	int32_t delay;
	
	uint32_t newSegmentsCount;
	////for (i = 0; i < 1; i++)

 	//segments = 30;
	uint32_t segmentsProcessed = segments;
	for (i = 0; i < segments; i++)
	{
		if (spectrumAnalyzer->currentFFTBufferIndex != ReceivingDataMode::Paused)
		{
			startSegmentTime = GetTickCount();

			/*////if (i > 0 && !referenceDevice)
			////if (!referenceDevice)
			dwWaitResult = WaitForSingleObject(deviceReceivers->receiverGates[deviceID], 10000000);
			*/

			FFT_BYTES(&data[currentSegmentIndex], fftBuffer, samplesCount, true, true, false, true);
			SignalProcessingUtilities::CalculateMagnitudesAndPhasesForArray(fftBuffer, samplesCount);


			/*//if (spectrumAnalyzer->currentFFTBufferIndex == 0 || spectrumAnalyzer->currentFFTBufferIndex == 2)
			{
				for (int i = 0; i < samplesCount; i++)
					fftBuffer[i][0] = deviceReceivers->frequencyRange.lower / 1000000;
			}
			else
				for (int i = 0; i < samplesCount; i++)
					fftBuffer[i][0] = 1;
					*/

			if (deviceReceivers->fftGraphForDeviceRange)
				deviceReceivers->fftGraphForDeviceRange->SetData(fftBuffer, samplesCount - 1, referenceDevice ? 0 : 1, true, 0, 0, !((SpectrumAnalyzer*)deviceReceivers->parent)->usePhase);

			spectrumAnalyzer->SetFFTInput(fftBuffer, frequencyRange, &data[currentSegmentIndex], samplesCount, deviceID);


			////if (ReleaseSemaphore(deviceReceivers->receiversFinished[deviceID], 1, NULL)==0)
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

					////spectrumAnalyzer->useRatios = false;
					////spectrumAnalyzer->usePhase = true;

					spectrumAnalyzer->ProcessReceiverData(frequencyRange, spectrumAnalyzer->useRatios);


					referenceDataBuffer = spectrumAnalyzer->GetFFTSpectrumBuffer(spectrumAnalyzer->currentFFTBufferIndex)->GetSampleDataForDevice(0);
					dataBuffer = spectrumAnalyzer->GetFFTSpectrumBuffer(spectrumAnalyzer->currentFFTBufferIndex)->GetSampleDataForDevice(1);

					deviceReceivers->dataGraph->SetData(referenceDataBuffer, DeviceReceiver::FFT_SEGMENT_BUFF_LENGTH, 0, true, -128, -128, true, SignalProcessingUtilities::DataType::UINT8_T);

					if (dataBuffer)
						deviceReceivers->dataGraph->SetData(dataBuffer, DeviceReceiver::FFT_SEGMENT_BUFF_LENGTH, 1, true, -128, -128, true, SignalProcessingUtilities::DataType::UINT8_T);

					if (deviceReceivers->synchronizing)
						deviceReceivers->dataGraph->SetText(1, "IQ Signal Data Waveform Graph: Synchronizing");

					
					if (deviceReceivers->fftGraphForDevicesRange && deviceReceivers->fftGraphStrengthsForDeviceRange && deviceReceivers->fftAverageGraphForDeviceRange && deviceReceivers->fftAverageGraphStrengthsForDeviceRange)
					{											
						FrequencyRange selectedFrequencyRange = SignalProcessingUtilities::GetSelectedFrequencyRangeFromDataRange(deviceReceivers->fftGraphForDevicesRange->startDataIndex, deviceReceivers->fftGraphForDevicesRange->endDataIndex, 0, deviceReceivers->fftGraphForDevicesRange->GetPointsCount(), frequencyRange->lower, frequencyRange->upper);

						char textBuffer[255];
						sprintf(textBuffer, "FFT: %.2f-%.2fMHz", SignalProcessingUtilities::ConvertToMHz(selectedFrequencyRange.lower), SignalProcessingUtilities::ConvertToMHz(selectedFrequencyRange.upper));
						deviceReceivers->fftGraphForDevicesRange->SetText(1, textBuffer);

						deviceReceivers->fftGraphForDevicesRange->SetGraphLabelValuesXAxis(SignalProcessingUtilities::ConvertToMHz(selectedFrequencyRange.lower), SignalProcessingUtilities::ConvertToMHz(selectedFrequencyRange.upper));
						spectrumAnalyzer->GetFFTData(fftBuffer, samplesCount, -1, frequencyRange->lower, frequencyRange->upper, ReceivingDataBufferSpecifier::CurrentBuffer);
						deviceReceivers->fftGraphForDevicesRange->SetData(&fftBuffer[1], samplesCount - 1, 0, true, 0, 0, !spectrumAnalyzer->usePhase);

						double signalStrength, avgSignalStrength, recentAvgSignalStrength;
						signalStrength = spectrumAnalyzer->GetStrengthForRange(selectedFrequencyRange.lower, selectedFrequencyRange.upper, ReceivingDataBufferSpecifier::CurrentBuffer);

						fftBuffer[0][0] = 0;
						fftBuffer[0][1] = signalStrength;

						fftBuffer[1][0] = 0;
						fftBuffer[1][1] = signalStrength;

						deviceReceivers->fftGraphStrengthsForDeviceRange->SetData(fftBuffer, 2, referenceDevice ? 0 : 1);




						selectedFrequencyRange = SignalProcessingUtilities::GetSelectedFrequencyRangeFromDataRange(deviceReceivers->fftAverageGraphForDeviceRange->startDataIndex, deviceReceivers->fftAverageGraphForDeviceRange->endDataIndex, 0, deviceReceivers->fftAverageGraphForDeviceRange->GetPointsCount(), frequencyRange->lower, frequencyRange->upper);

						//textBuffer[255];
						sprintf(textBuffer, "Averaged FFT: %.2f-%.2fMHz", SignalProcessingUtilities::ConvertToMHz(selectedFrequencyRange.lower), SignalProcessingUtilities::ConvertToMHz(selectedFrequencyRange.upper));
						deviceReceivers->fftAverageGraphForDeviceRange->SetText(1, textBuffer);

						//deviceReceivers->fftAverageGraphForDeviceRange->SetText(1, "Averaged FFT %.2f - %.2fMHz", SignalProcessingUtilities::ConvertToMHz(selectedFrequencyRange.lower), SignalProcessingUtilities::ConvertToMHz(selectedFrequencyRange.upper));


						deviceReceivers->fftAverageGraphForDeviceRange->SetGraphLabelValuesXAxis(SignalProcessingUtilities::ConvertToMHz(selectedFrequencyRange.lower), SignalProcessingUtilities::ConvertToMHz(selectedFrequencyRange.upper));

						FFTSpectrumBuffer* undeterminedAndNearBuffer = spectrumAnalyzer->GetFFTSpectrumBuffer(ReceivingDataMode::NearAndUndetermined);

						undeterminedAndNearBuffer->ClearData();

						spectrumAnalyzer->GetFFTSpectrumBuffer(0)->TransferDataToFFTSpectrumBuffer(undeterminedAndNearBuffer);
						spectrumAnalyzer->GetFFTSpectrumBuffer(2)->TransferDataToFFTSpectrumBuffer(undeterminedAndNearBuffer);


						spectrumAnalyzer->GetFFTData(fftBuffer, samplesCount, ReceivingDataMode::NearAndUndetermined, frequencyRange->lower, frequencyRange->upper, ReceivingDataBufferSpecifier::AveragedBuffer);

						deviceReceivers->fftAverageGraphForDeviceRange->SetData(&fftBuffer[1], samplesCount - 1, ReceivingDataMode::Near, true, 0, 0, !spectrumAnalyzer->usePhase);


						spectrumAnalyzer->GetFFTData(fftBuffer, samplesCount, ReceivingDataMode::Far, frequencyRange->lower, frequencyRange->upper, ReceivingDataBufferSpecifier::AveragedBuffer);

						deviceReceivers->fftAverageGraphForDeviceRange->SetData(&fftBuffer[1], samplesCount - 1, ReceivingDataMode::Far, true, 0, 0, !spectrumAnalyzer->usePhase);

						//deviceReceivers->fftAverageGraphForDeviceRange->SetText(2, \Frames: %d", undeterminedAndNearBuffer->GetFrameCountForRange());
						deviceReceivers->dataGraph->SetText(0, "Near Frames: %d Far Frames: %d", undeterminedAndNearBuffer->GetFrameCountForRange(), spectrumAnalyzer->GetFFTSpectrumBuffer(1)->GetFrameCountForRange());



						spectrumAnalyzer->GetFFTData(fftBuffer, samplesCount, ReceivingDataMode::Far, frequencyRange->lower, frequencyRange->upper, ReceivingDataBufferSpecifier::AveragedBuffer);

						//deviceReceivers->fftAverageGraphForDeviceRange->SetData(&fftBuffer[1], samplesCount - 1, 1, true, 0, 0, !spectrumAnalyzer->usePhase);

						//deviceReceivers->fftAverageGraphForDeviceRange->SetText(3, "Far Frames: %d", spectrumAnalyzer->GetFFTSpectrumBuffer(1)->GetFrameCountForRange());



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

						////deviceReceivers->fftAverageGraphStrengthsForDeviceRange->SetData(fftBuffer, 2, 2);



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
							selectedFrequencyRange = SignalProcessingUtilities::GetSelectedFrequencyRangeFromDataRange(deviceReceivers->spectrumRangeGraph->startDataIndex, deviceReceivers->spectrumRangeGraph->endDataIndex, 0, deviceReceivers->spectrumRangeGraph->GetPointsCount(), spectrumAnalyzer->maxFrequencyRange.lower, spectrumAnalyzer->maxFrequencyRange.upper);

							sprintf(textBuffer, "Spectrum Graph: %.2f-%.2fMHz", SignalProcessingUtilities::ConvertToMHz(selectedFrequencyRange.lower), SignalProcessingUtilities::ConvertToMHz(selectedFrequencyRange.upper));
							deviceReceivers->spectrumRangeGraph->SetText(1, textBuffer);
						}


						////deviceReceivers->fftAverageGraphForDeviceRange->SetText(4, "Gradient: %.4f", frequency);


						double frequency = 0;

						if (strengthDetectionSound)
							if (recentAvgSignalStrength > avgSignalStrength)
							{
								////frequency = ((signalStrength - avgSignalStrength) / strengthRange.range) * 15000;						
								////frequency = recentAvgSignalStrength / avgSignalStrength * 6000;
								frequency = 3500 + (recentAvgSignalStrength - avgSignalStrength) / strengthRange.range * 15000;

								if (frequency > 3500 && frequency < 15000)
								{
									////if (soundRateCounter.Add()>soundThresholdCount)
									double rateCounter = soundRateCounter.Add(frequency);
									//deviceReceivers->fftAverageGraphForDeviceRange->SetText(7, "Rate Counter: %.4f", rateCounter);
									////if (rateCounter > soundThresholdCount)
									////if (rateCounter > 680000)
									if (rateCounter > 300000)
									{
										spectrumAnalyzer->PlaySound(frequency, 100);
										//deviceReceivers->fftAverageGraphForDeviceRange->SetText(6, "Frequency: %.4f", frequency);
									}
								}
							}

						if (gradientDetectionSound)
							if (gradient > avgGradient)
								////if (gradientOfGradient>0)
								////if (gradient > 0)
							{
								frequency = ((gradient - avgGradient) / gradientRange.range) * 15000;

								/*////frequency = gradient / avgGradient;

								deviceReceivers->fftAverageGraphForDeviceRange->SetText(4, "GradientToAvg Ratio: %i", frequency);

								frequency *= 10000;
								*////

								////frequency = gradientOfGradient * 10000;

								////deviceReceivers->fftAverageGraphStrengthsForDeviceRange->dataSeries[0]->GetMinMax(0)

								if (frequency > 8500 && frequency < 15000)
								{
									if (soundRateCounter.Add() > soundThresholdCount)
										////Beep(frequency, 100);		
										spectrumAnalyzer->PlaySound(frequency, 100);
								}
							}


						graphRefreshTime = GetTickCount();
					}


					////spectrumAnalyzer->ProcessFFTInput(frequencyRange);

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
			////DebuggingUtilities::DebugPrint("Delay: %d\n", delay);
			if (delay > 0)
				Sleep(delay);
			else
			{
				i++;
				segmentsProcessed--;
			}


			segmentDuration = currentTime - startSegmentTime;

			/*if (referenceDevice)
			segmentDuration = currentTime - startSegmentTime;

			remainingTime = ((int32_t)receivedDuration[receivedCount - 1]) - (GetTickCount() - receivedDatatartTime);

			if (segmentDuration > 0)
			{
			newSegmentsCount = i + (remainingTime / segmentDuration);

			if (newSegmentsCount < segments)
			segments = newSegmentsCount;
			}*/
		}
	}

	/*////if (ReleaseSemaphore(receiverBufferDataAvailableGate, 1, NULL))
	{
	if (DebuggingUtilities::DEBUGGING)
	DebuggingUtilities::DebugPrint("ReleaseSemaphore error: %d\n", GetLastError());
	}

	if (DebuggingUtilities::DEBUGGING)
	DebuggingUtilities::DebugPrint("ProcessData() processed: open receiverBufferDataAvailableGate\n");
	*/

	////if (!referenceDevice)
	////dwWaitResult = WaitForSingleObject(deviceReceivers->receiverGates[deviceID], 10000000);

	DWORD totalTime = GetTickCount() - receivedDatatartTime;
	if (segmentsProcessed>70)
		DebuggingUtilities::DebugPrint("Segments processed: %d\n", segmentsProcessed);

	if (referenceDevice)
		int grc = 1;
	else
		int rtl = 2;
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

	////uint32_t samplesCount = segmentBufferLength / 2;		
	uint32_t samplesCount = length;

	if (fftBuffer == NULL)
	{
		if (processAllData)
			fftBuffer = new fftw_complex[samplesCount];
		else
			fftBuffer = new fftw_complex[FFT_SEGMENT_SAMPLE_COUNT];
	}


	uint32_t currentSegmentIndex = 0;

	/////uint32_t currentSegmentIndex = delayShift;

	////SignalProcessingUtilities::Rotate(data, length, -phaseShift);

	////fftw_complex* fftBuffer = new fftw_complex[DeviceReceiver::FFT_SEGMENT_SAMPLE_COUNT];
	
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
	////for (i = 0; i < 1; i++)
	for (i = 0; i < segments; i++)
	{
		if (spectrumAnalyzer->currentFFTBufferIndex != ReceivingDataMode::Paused)
		{
			startSegmentTime = GetTickCount();

			/*////if (i > 0 && !referenceDevice)
			////if (!referenceDevice)
				dwWaitResult = WaitForSingleObject(deviceReceivers->receiverGates[deviceID], 10000000);
				*/

			////FFT_BYTES(&data[currentSegmentIndex], fftBuffer, samplesCount, true, true, false, true);

			////FFT_COMPLEX_ARRAY(&data[currentSegmentIndex], fftBuffer, samplesCount, true, true, false);
			FFT_COMPLEX_ARRAY(&data[currentSegmentIndex], fftBuffer, samplesCount, false, false);

			SignalProcessingUtilities::CalculateMagnitudesAndPhasesForArray(fftBuffer, samplesCount);			


			//// original deviceReceivers->fftGraphForDeviceRange->SetData(fftBuffer, samplesCount - 1, referenceDevice ? 0 : 1, true, 0, 0, !((SpectrumAnalyzer*)deviceReceivers->parent)->usePhase);

			deviceReceivers->fftGraphForDeviceRange->SetData(fftBuffer, 50, referenceDevice ? 0 : 1, true, 0, 0, !((SpectrumAnalyzer*)deviceReceivers->parent)->usePhase);




			////spectrumAnalyzer->SetFFTInput(fftBuffer, frequencyRange, &data[currentSegmentIndex], samplesCount, deviceID);

			spectrumAnalyzer->SetFFTInput(fftBuffer, frequencyRange, &data[currentSegmentIndex], samplesCount, deviceID);


			////if (ReleaseSemaphore(deviceReceivers->receiversFinished[deviceID], 1, NULL)==0)
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

					////spectrumAnalyzer->useRatios = false;
					////spectrumAnalyzer->usePhase = true;

					spectrumAnalyzer->ProcessReceiverData(frequencyRange, spectrumAnalyzer->useRatios);

					////if (deviceReceivers->fftGraphForDevicesRange && GetTickCount() - graphRefreshTime > 200)
					if (deviceReceivers->fftGraphForDevicesRange && deviceReceivers->fftGraphStrengthsForDeviceRange && deviceReceivers->fftAverageGraphForDeviceRange && deviceReceivers->fftAverageGraphStrengthsForDeviceRange)
					{
						uint8_t* referenceDataBuffer = spectrumAnalyzer->GetFFTSpectrumBuffer(spectrumAnalyzer->currentFFTBufferIndex)->GetSampleDataForDevice(0);

						uint8_t* dataBuffer = spectrumAnalyzer->GetFFTSpectrumBuffer(spectrumAnalyzer->currentFFTBufferIndex)->GetSampleDataForDevice(1);

						//// original deviceReceivers->dataGraph->SetData(referenceDataBuffer, DeviceReceiver::FFT_SEGMENT_BUFF_LENGTH, 0, true, 0, 0, true, SignalProcessingUtilities::DataType::UINT8_T);
						////deviceReceivers->dataGraph->SetData(referenceDataBuffer, DeviceReceiver::FFT_SEGMENT_BUFF_LENGTH, 0, true, -128, -128, false, SignalProcessingUtilities::DataType::UINT8_T);

						if (dataBuffer)
							deviceReceivers->dataGraph->SetData(dataBuffer, DeviceReceiver::FFT_SEGMENT_BUFF_LENGTH, 1, true, -128, -128, false, SignalProcessingUtilities::DataType::UINT8_T);

						if (!deviceReceivers->synchronizing)
							deviceReceivers->dataGraph->SetText(1, "IQ Signal Data Waveform Graph");
						else
							deviceReceivers->dataGraph->SetText(1, "IQ Signal Data Waveform Graph: Synchronizing");
						

						////deviceReceivers->fftGraphForDevicesRange->GetSelectedFrequencyRange(frequencyRange->lower, frequencyRange->upper);


						FrequencyRange selectedFrequencyRange = SignalProcessingUtilities::GetSelectedFrequencyRangeFromDataRange(deviceReceivers->fftGraphForDevicesRange->startDataIndex, deviceReceivers->fftGraphForDevicesRange->endDataIndex, 0, deviceReceivers->fftGraphForDevicesRange->GetPointsCount(), frequencyRange->lower, frequencyRange->upper);

						/*////uint32_t startFrequency = SignalProcessingUtilities::GetFrequencyFromDataIndex(deviceReceivers->fftGraphForDevicesRange->startDataIndex, 0, deviceReceivers->fftGraphForDevicesRange->GetPointsCount(), frequencyRange->lower, frequencyRange->upper);

						uint32_t endFrequency;

						if (deviceReceivers->fftGraphForDevicesRange->endDataIndex == 0)
							endFrequency = frequencyRange->upper;
						else
						{
							endFrequency = SignalProcessingUtilities::GetFrequencyFromDataIndex(deviceReceivers->fftGraphForDevicesRange->endDataIndex, 0, deviceReceivers->fftGraphForDevicesRange->GetPointsCount(), frequencyRange->lower, frequencyRange->upper);
						}

						deviceReceivers->fftGraphForDevicesRange->SetGraphLabelValuesXAxis(SignalProcessingUtilities::ConvertToMHz(startFrequency), SignalProcessingUtilities::ConvertToMHz(endFrequency));
						*/

						char textBuffer[255];
						sprintf(textBuffer, "FFT Graph: %.2f-%.2fMHz", SignalProcessingUtilities::ConvertToMHz(selectedFrequencyRange.lower), SignalProcessingUtilities::ConvertToMHz(selectedFrequencyRange.upper));
						deviceReceivers->fftGraphForDevicesRange->SetText(1, textBuffer);

						deviceReceivers->fftGraphForDevicesRange->SetGraphLabelValuesXAxis(SignalProcessingUtilities::ConvertToMHz(selectedFrequencyRange.lower), SignalProcessingUtilities::ConvertToMHz(selectedFrequencyRange.upper));

						spectrumAnalyzer->GetFFTData(fftBuffer, samplesCount, -1, frequencyRange->lower, frequencyRange->upper, ReceivingDataBufferSpecifier::CurrentBuffer);



						////deviceReceivers->fftGraphForDevicesRange->SetData(&fftBuffer[1], samplesCount-1, 0, true, 0, 0, true);
						////deviceReceivers->fftGraphForDevicesRange->SetData(&fftBuffer[1], samplesCount - 1, 0, true, 0, 0, !spectrumAnalyzer->usePhase);
						deviceReceivers->fftGraphForDevicesRange->SetData(&fftBuffer[1], 50, 0, true, 0, 0, !spectrumAnalyzer->usePhase);


						double signalStrength, avgSignalStrength, recentAvgSignalStrength;
						signalStrength = spectrumAnalyzer->GetStrengthForRange(selectedFrequencyRange.lower, selectedFrequencyRange.upper, ReceivingDataBufferSpecifier::CurrentBuffer);



						fftBuffer[0][0] = 0;
						fftBuffer[0][1] = signalStrength;

						fftBuffer[1][0] = 0;
						fftBuffer[1][1] = signalStrength;

						deviceReceivers->fftGraphStrengthsForDeviceRange->SetData(fftBuffer, 2, referenceDevice ? 0 : 1);




						selectedFrequencyRange = SignalProcessingUtilities::GetSelectedFrequencyRangeFromDataRange(deviceReceivers->fftAverageGraphForDeviceRange->startDataIndex, deviceReceivers->fftAverageGraphForDeviceRange->endDataIndex, 0, deviceReceivers->fftAverageGraphForDeviceRange->GetPointsCount(), frequencyRange->lower, frequencyRange->upper);

						textBuffer[255];
						sprintf(textBuffer, "Averaged FFT: %.2f-%.2fMHz", SignalProcessingUtilities::ConvertToMHz(selectedFrequencyRange.lower), SignalProcessingUtilities::ConvertToMHz(selectedFrequencyRange.upper));
						deviceReceivers->fftAverageGraphForDeviceRange->SetText(1, textBuffer);

						//deviceReceivers->fftAverageGraphForDeviceRange->SetText(1, "Averaged FFT %.2f - %.2fMHz", SignalProcessingUtilities::ConvertToMHz(selectedFrequencyRange.lower), SignalProcessingUtilities::ConvertToMHz(selectedFrequencyRange.upper));
						deviceReceivers->fftAverageGraphForDeviceRange->SetGraphLabelValuesXAxis(SignalProcessingUtilities::ConvertToMHz(selectedFrequencyRange.lower), SignalProcessingUtilities::ConvertToMHz(selectedFrequencyRange.upper));



						////spectrumAnalyzer->GetFFTData(fftBuffer, samplesCount, ReceivingDataMode::Near, frequencyRange->lower, frequencyRange->upper, ReceivingDataBufferSpecifier::AveragedBuffer);

						FFTSpectrumBuffer* undeterminedAndNearBuffer = spectrumAnalyzer->GetFFTSpectrumBuffer(ReceivingDataMode::NearAndUndetermined);

						undeterminedAndNearBuffer->ClearData();

						spectrumAnalyzer->GetFFTSpectrumBuffer(0)->TransferDataToFFTSpectrumBuffer(undeterminedAndNearBuffer);
						spectrumAnalyzer->GetFFTSpectrumBuffer(2)->TransferDataToFFTSpectrumBuffer(undeterminedAndNearBuffer);

						spectrumAnalyzer->GetFFTData(fftBuffer, samplesCount, 3, frequencyRange->lower, frequencyRange->upper, ReceivingDataBufferSpecifier::AveragedBuffer);


						////deviceReceivers->fftAverageGraphForDeviceRange->SetData(&fftBuffer[1], samplesCount - 1, 0, true, 0, 0, true);
						////original deviceReceivers->fftAverageGraphForDeviceRange->SetData(&fftBuffer[1], samplesCount - 1, 0, true, 0, 0, !spectrumAnalyzer->usePhase);
						deviceReceivers->fftAverageGraphForDeviceRange->SetData(&fftBuffer[1], samplesCount - 1, 0, true, 0, 0, !spectrumAnalyzer->usePhase);
						////deviceReceivers->fftAverageGraphForDeviceRange->SetData(&fftBuffer[1], 50, 0, true, 0, 0, !spectrumAnalyzer->usePhase);


						//deviceReceivers->fftAverageGraphForDeviceRange->SetText(2, "Near Frames: %d", undeterminedAndNearBuffer->GetFrameCountForRange());
						deviceReceivers->dataGraph->SetText(0, "Near Frames: %d Far Frames: %d", undeterminedAndNearBuffer->GetFrameCountForRange(), spectrumAnalyzer->GetFFTSpectrumBuffer(1)->GetFrameCountForRange());
						//deviceReceivers->fftAverageGraphForDeviceRange->SetText(3, "Far Frames: %d", spectrumAnalyzer->GetFFTSpectrumBuffer(1)->GetFrameCountForRange());



						spectrumAnalyzer->GetFFTData(fftBuffer, samplesCount, ReceivingDataMode::Far, frequencyRange->lower, frequencyRange->upper, ReceivingDataBufferSpecifier::AveragedBuffer);

						////deviceReceivers->fftAverageGraphForDeviceRange->SetData(&fftBuffer[1], samplesCount - 1, 1, true, 0, 0, true);
						////original deviceReceivers->fftAverageGraphForDeviceRange->SetData(&fftBuffer[1], samplesCount - 1, 1, true, 0, 0, !spectrumAnalyzer->usePhase);
						deviceReceivers->fftAverageGraphForDeviceRange->SetData(&fftBuffer[1], samplesCount - 1, 1, true, 0, 0, !spectrumAnalyzer->usePhase);
						////deviceReceivers->fftAverageGraphForDeviceRange->SetData(&fftBuffer[1], 50, 1, true, 0, 0, !spectrumAnalyzer->usePhase);

						//deviceReceivers->fftAverageGraphForDeviceRange->SetText(3, "Far Frames: %d", spectrumAnalyzer->GetFFTSpectrumBuffer(1)->GetFrameCountForRange());



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
							////avgGradient = deviceReceivers->fftAverageGraphStrengthsForDeviceRange->GetAvgValueForIndex(2, 0);						
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

							////gradientOfGradient = deviceReceivers->fftAverageGraphStrengthsForDeviceRange->GetGradientForIndex(2, 0);
						}

						avgGradient = deviceReceivers->fftAverageGraphStrengthsForDeviceRange->GetAvgValueForIndex(2, 0);
						gradientRange = deviceReceivers->fftAverageGraphStrengthsForDeviceRange->dataSeries[2]->GetMinMax(0, 0, true, false);



						fftBuffer[0][0] = 0;
						fftBuffer[0][1] = gradient;

						fftBuffer[1][0] = 0;
						fftBuffer[1][1] = gradient;

						////deviceReceivers->fftAverageGraphStrengthsForDeviceRange->SetData(fftBuffer, 2, 2);



						if (spectrumAnalyzer->usePhase)
							deviceReceivers->fftAverageGraphForDeviceRange->SetText(4, "Phase: %.4f", signalStrength * MathUtilities::RadianToDegrees);
						else
							if (spectrumAnalyzer->useRatios)
								deviceReceivers->fftAverageGraphForDeviceRange->SetText(4, "Ratio Strength: %.4f", signalStrength);
							else
								deviceReceivers->fftAverageGraphForDeviceRange->SetText(4, "Signal Strength: %.4f", signalStrength);

						deviceReceivers->fftAverageGraphForDeviceRange->SetText(5, "Gradient: %.4f", gradient);

						textBuffer[255];

						selectedFrequencyRange = SignalProcessingUtilities::GetSelectedFrequencyRangeFromDataRange(deviceReceivers->spectrumRangeGraph->startDataIndex, deviceReceivers->spectrumRangeGraph->endDataIndex, 0, deviceReceivers->spectrumRangeGraph->GetPointsCount(), spectrumAnalyzer->maxFrequencyRange.lower, spectrumAnalyzer->maxFrequencyRange.upper);

						sprintf(textBuffer, "Spectrum Graph: %.2f-%.2fMHz", SignalProcessingUtilities::ConvertToMHz(selectedFrequencyRange.lower), SignalProcessingUtilities::ConvertToMHz(selectedFrequencyRange.upper));
						deviceReceivers->spectrumRangeGraph->SetText(1, textBuffer);


						////deviceReceivers->fftAverageGraphForDeviceRange->SetText(4, "Gradient: %.4f", frequency);


						double frequency = 0;

						if (strengthDetectionSound)
							if (recentAvgSignalStrength > avgSignalStrength)
							{
								////frequency = ((signalStrength - avgSignalStrength) / strengthRange.range) * 15000;						
								////frequency = recentAvgSignalStrength / avgSignalStrength * 6000;
								frequency = 3500 + (recentAvgSignalStrength - avgSignalStrength) / strengthRange.range * 15000;

								if (frequency > 3500 && frequency < 15000)
								{									
									double rateCounter = soundRateCounter.Add(frequency);
									
									//deviceReceivers->fftAverageGraphForDeviceRange->SetText(7, "Rate Counter: %.4f", rateCounter);									
									if (rateCounter > 400000)
									{
										spectrumAnalyzer->PlaySound(frequency, 100);
										//deviceReceivers->fftAverageGraphForDeviceRange->SetText(6, "Frequency: %.4f", frequency);
									}
								}
							}

						if (gradientDetectionSound)
							if (gradient > avgGradient)
								////if (gradientOfGradient>0)
								////if (gradient > 0)
							{
								frequency = ((gradient - avgGradient) / gradientRange.range) * 15000;

								/*////frequency = gradient / avgGradient;

								deviceReceivers->fftAverageGraphForDeviceRange->SetText(4, "GradientToAvg Ratio: %i", frequency);

								frequency *= 10000;
								*////

								////frequency = gradientOfGradient * 10000;

								////deviceReceivers->fftAverageGraphStrengthsForDeviceRange->dataSeries[0]->GetMinMax(0)

								if (frequency > 8500 && frequency < 15000)
								{
									if (soundRateCounter.Add() > soundThresholdCount)
										////Beep(frequency, 100);		
										spectrumAnalyzer->PlaySound(frequency, 100);
								}
							}


						graphRefreshTime = GetTickCount();
					}					

					////spectrumAnalyzer->ProcessFFTInput(frequencyRange);

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
			////DebuggingUtilities::DebugPrint("Delay: %d\n", delay);
			if (delay > 0)
				Sleep(delay);
			else
			{
				i++;
				segmentsProcessed--;
			}


			segmentDuration = currentTime - startSegmentTime;

			/*if (referenceDevice)
				segmentDuration = currentTime - startSegmentTime;

			remainingTime = ((int32_t)receivedDuration[receivedCount - 1]) - (GetTickCount() - receivedDatatartTime);

			if (segmentDuration > 0)
			{
			newSegmentsCount = i + (remainingTime / segmentDuration);

			if (newSegmentsCount < segments)
			segments = newSegmentsCount;
			}*/
		}
	}

	/*////if (ReleaseSemaphore(receiverBufferDataAvailableGate, 1, NULL))
	{
		if (DebuggingUtilities::DEBUGGING)
			DebuggingUtilities::DebugPrint("ReleaseSemaphore error: %d\n", GetLastError());
	}

	if (DebuggingUtilities::DEBUGGING)
		DebuggingUtilities::DebugPrint("ProcessData() processed: open receiverBufferDataAvailableGate\n");
		*/

	////if (!referenceDevice)
		////dwWaitResult = WaitForSingleObject(deviceReceivers->receiverGates[deviceID], 10000000);

	DWORD totalTime = GetTickCount() - receivedDatatartTime;
	DebuggingUtilities::DebugPrint("Segments processed: %d\n", segmentsProcessed);

	if (referenceDevice)
		int grc = 1;
	else
		int rtl = 2;
}


void ProcessReceivedDataThread(void *param)
{
	DeviceReceiver* deviceReceiver = (DeviceReceiver*) param;
	DeviceReceivers* deviceReceivers = (DeviceReceivers*) deviceReceiver->parent;

	//Sleep(10000);

	while (deviceReceiver->receivingData)
	{
		if (((SpectrumAnalyzer*)deviceReceivers->parent)->currentFFTBufferIndex != -1)
		{
			/*////
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
			*/

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

			DWORD dwWaitResult = WaitForSingleObject(deviceReceiver->rtlDataAvailableGate, 2000);
			////DWORD dwWaitResult = 0;		

			fftw_complex* receivedBufferComplex, *receivedBufferComplexAveraged, *receivedBufferComplexAveragedFFT;
			uint32_t receivedBufferComplexSampleLength;
			////uint32_t receivedBufferComplexAveragedSegments = 8192;
			////uint32_t receivedBufferComplexAveragedSegments = 1024;
			uint32_t receivedBufferComplexAveragedSegments = 256;

			double strength = 0.0001;
			////double strength = 1;

			switch (dwWaitResult)
			{
			case WAIT_OBJECT_0:
				if (deviceReceiver->receivedBuffer == NULL || deviceReceiver->receivedBufferLength != deviceReceiver->receivedLength)
				{
					//// original deviceReceiver->receivedBuffer = new uint8_t[deviceReceiver->receivedLength];

					deviceReceiver->receivedBuffer = new uint8_t[deviceReceiver->receivedLength*2];

					deviceReceiver->receivedBufferLength = deviceReceiver->receivedLength;
				}

				memcpy(deviceReceiver->receivedBuffer, deviceReceiver->receivedBufferPtr, deviceReceiver->receivedLength);

				//memcpy(deviceReceiver->receivedBuffer, deviceReceiver->receivedBufferPtr, (deviceReceiver->receivedLength < deviceReceiver->receivedBufferLength ? deviceReceiver->receivedLength : deviceReceiver->receivedBufferLength));

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
						/*//// original deviceReceiver->GetDelayAndPhaseShiftedData(deviceReceiver->receivedBuffer, deviceReceiver->receivedLength * 2 + startIndex);

						receivedBufferComplex = ArrayUtilities::ConvertArrayToComplex(deviceReceiver->receivedBuffer, deviceReceiver->receivedLength);
						*/						

						receivedBufferComplex = ArrayUtilities::ConvertArrayToComplex(&deviceReceiver->receivedBuffer[startIndex], deviceReceiver->receivedLength);

						receivedBufferComplexSampleLength = deviceReceiver->receivedLength / 2;


						/* original for (unsigned int k = 0; k < receivedBufferComplexSampleLength; k++)
						{
							receivedBufferComplex[k][0] *= 1 + (strength*deviceReceiver->GetSampleAtIndex(k, receivedBufferComplexSampleLength, GetTickCount()));
							receivedBufferComplex[k][1] *= 1 + (strength*deviceReceiver->GetSampleAtIndex(k, receivedBufferComplexSampleLength, GetTickCount(), false));
							////dataBuffer[k * 2 + 1] = 127;

							////receivedBufferComplex[k][0] += ((float)rand() / RAND_MAX) - 0.5;
							////receivedBufferComplex[k][1] += ((float)rand() / RAND_MAX) - 0.5;

							receivedBufferComplex[k][0] += ((float)rand() / RAND_MAX) / 2;
							receivedBufferComplex[k][1] += ((float)rand() / RAND_MAX) / 2;

							receivedBufferComplex[k][0] = MathUtilities::Round(receivedBufferComplex[k][0], 0);
							receivedBufferComplex[k][1] = MathUtilities::Round(receivedBufferComplex[k][1], 0);
						}
						*/


						SignalProcessingUtilities::CalculateMagnitudesAndPhasesForArray(receivedBufferComplex, receivedBufferComplexSampleLength);
						receivedBufferComplexAveraged = ArrayUtilities::AverageArray(receivedBufferComplex, receivedBufferComplexSampleLength, receivedBufferComplexAveragedSegments, true);
						////receivedBufferComplexAveraged = ArrayUtilities::DecimateArray(receivedBufferComplex, receivedBufferComplexSampleLength, receivedBufferComplexAveragedSegments);
						SignalProcessingUtilities::SetQ(receivedBufferComplexAveraged, receivedBufferComplexAveragedSegments, 0);


						receivedBufferComplexAveraged = ArrayUtilities::ResizeArrayAndData(receivedBufferComplexAveraged, receivedBufferComplexAveragedSegments, DeviceReceiver::FFT_SEGMENT_SAMPLE_COUNT);
						////receivedBufferComplexAveraged = ArrayUtilities::ResizeArrayAndData(receivedBufferComplexAveraged, receivedBufferComplexAveragedSegments, 1024);





						////deviceReceivers->dataGraph->SetData(receivedBufferComplexAveraged, receivedBufferComplexAveragedSegments, 0, true, 0, 0, true, SignalProcessingUtilities::DataType::FFTW_COMPLEX);
						deviceReceivers->dataGraph->SetData(receivedBufferComplexAveraged, DeviceReceiver::FFT_SEGMENT_SAMPLE_COUNT, 0, true, 0, 0, true, SignalProcessingUtilities::DataType::FFTW_COMPLEX);
						////deviceReceivers->dataGraph->SetData(receivedBufferComplexAveraged, 1024, 0, true, 0, 0, true, SignalProcessingUtilities::DataType::FFTW_COMPLEX);

						/*//// code for ffting strength of samples *////
						deviceReceiver->ProcessData(receivedBufferComplexAveraged, DeviceReceiver::FFT_SEGMENT_SAMPLE_COUNT);
						////deviceReceiver->ProcessData(receivedBufferComplexAveraged, 1024);


						delete receivedBufferComplex;
						delete receivedBufferComplexAveraged;

						startIndex += segmentShifts;
					}
				}
				else
				{
					deviceReceiver->GetDelayAndPhaseShiftedData(deviceReceiver->receivedBuffer, deviceReceiver->receivedLength);

					/*
					receivedBufferComplexAveraged = receivedBufferComplex;

					receivedBufferComplexAveragedFFT = new fftw_complex[receivedBufferComplexAveragedSegments];

					deviceReceiver->FFT_COMPLEX_ARRAY(receivedBufferComplexAveraged, receivedBufferComplexAveragedFFT, receivedBufferComplexAveragedSegments, false, false);
					SignalProcessingUtilities::CalculateMagnitudesAndPhasesForArray(receivedBufferComplexAveragedFFT, receivedBufferComplexAveragedSegments);
					deviceReceivers->fftGraphForDeviceRange->SetData(receivedBufferComplexAveragedFFT, receivedBufferComplexAveragedSegments, 0, true, 0, 0, true, SignalProcessingUtilities::DataType::FFTW_COMPLEX);
					*/


					////deviceReceiver->WriteReceivedDataToBuffer(deviceReceiver->receivedBuffer, deviceReceiver->receivedLength);
					////deviceReceiver->GetDelayAndPhaseShiftedData(deviceReceiver->receivedBuffer, deviceReceiver->receivedLength);

					deviceReceiver->ProcessData(deviceReceiver->receivedBuffer, deviceReceiver->receivedLength);
					////deviceReceiver->ProcessData(receivedBufferComplexAveraged, receivedBufferComplexAveragedSegments);
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


void DeviceReceiver::ReceiveData(uint8_t *buffer, long length)
{
	////receivedDuration[receivedCount++] = length;	

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

	
	/*////
	DWORD dwWaitResult1 = WaitForSingleObject(deviceReceivers->receiveDataGate1, 1000);
	DWORD dwWaitResult2 = WaitForSingleObject(deviceReceivers->receiveDataGate2, 1000);
	*/
	
	
	DWORD dwWaitResult1 = 0;
	DWORD dwWaitResult2 = 0;
	
	
	if (dwWaitResult1 == WAIT_OBJECT_0 && dwWaitResult2 == WAIT_OBJECT_0)
	{
		/*////if (DebuggingUtilities::DEBUGGING)
			DebuggingUtilities::DebugPrint("ReceiveData(): receiveDataGate1 && receiveDataGate2 open:-> receivedBuffer = buffer\n");
			*/

		receivedBufferPtr = buffer;
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

/*////void DeviceReceiver::ReceiveData2(uint8_t *buffer, long length)
{	
	receivedDuration[receivedCount++] = GetTickCount() - prevReceivedTime;
	prevReceivedTime = GetTickCount();


	////receivedDuration[receivedCount++] = length;

	////unsigned char *dataBuffer = new unsigned char[length];

	////unsigned char *dataBuffer = new unsigned char[FFT_SEGMENT_BUFF_LENGTH];
	////uint32_t length = this->CORRELATION_BUFF_LENGTH / 2;

	length = FFT_SEGMENT_BUFF_LENGTH;

	if (buffer2 == NULL)
		buffer2 = new uint8_t[length];

	if (fftBuffer == NULL)
		fftBuffer = new fftw_complex[length/2];


		*/

	/*////for (unsigned int k = 0; k < length; k += 2)
	{
		buffer2[k] = GetSampleByteAtIndex(k, length, GetTickCount());
		buffer2[k + 1] = GetSampleByteAtIndex(k, length, GetTickCount(), false);
		////dataBuffer[k * 2 + 1] = 127;
	}*/

		/*////
	

	receivedDuration[receivedCount++] = prevReceivedTime;

	if (DebuggingUtilities::TEST_CORRELATION && referenceDevice)
	{
		for (int i = 0; i < length; i++)
			buffer[i] = ((float)rand() / RAND_MAX) * 255;
	}
		
	////buffer[0] = 255;

	*/

	/*////int divisor = length / 100;
	for (int i = 0; i < length; i++)
	{
		if (i%divisor == 0)		
		{
			buffer[i] = 255;			
		}
	}*/
	

	/*////
	if (!referenceDevice)
	{
		if (ReleaseSemaphore(startReceivingDataGate, 1, NULL))
		{
			////DebuggingUtilities::DebugPrint("ReleaseSemaphore error: %d\n", GetLastError());
		}
	}
	else
	{
		DWORD dwWaitResult = WaitForSingleObject(startReceivingDataGate, 1000000);

		switch (dwWaitResult)
		{
		case WAIT_OBJECT_0:			
			break;
		case WAIT_TIMEOUT:
			break;
		default:
			break;
		}
	}
	



	if (referenceDevice && DebuggingUtilities::TEST_CORRELATION)
	{
		/*for (int i = 0; i < length; i++)
			buffer[i] = ((float)rand() / RAND_MAX) * 255;
			*/
/*////
		if (length == RECEIVE_BUFF_LENGTH)
			memcpy(transferDataBuffer, &buffer[10], length - 10);
			////memcpy(&transferDataBuffer[10], buffer, length - 10);
		else
		{
			int grc = 1;
		}
	}


	int32_t readStart = circularDataBuffer->writeStart;

	circularDataBuffer->WriteData(buffer, length);

	uint32_t segments = length / DeviceReceiver::FFT_SEGMENT_BUFF_LENGTH;

	uint32_t currentSegmentIndex = delayShift;

	////fftw_complex* fftBuffer = new fftw_complex[DeviceReceiver::FFT_SEGMENT_SAMPLE_COUNT];

	int i;

	DWORD dwWaitResult;
	DeviceReceivers* deviceReceivers = (DeviceReceivers*)parent;

	for (i = 0; i < segments; i++)
	{					
			if (i>0 && !referenceDevice)
				////if (!referenceDevice)
				dwWaitResult = WaitForSingleObject(deviceReceivers->receiverGates[deviceID], 10000000);

			////SignalProcessingUtilities::FFT_BYTES(&buffer[currentSegmentIndex], fftBuffer, DeviceReceiver::FFT_SEGMENT_SAMPLE_COUNT, false, true, !DebuggingUtilities::RECEIVE_TEST_DATA);			

			////FFT_BYTES(buffer, fftBuffer, DeviceReceiver::FFT_SEGMENT_SAMPLE_COUNT, false, true, !DebuggingUtilities::RECEIVE_TEST_DATA);
			////FFT_BYTES(buffer2, fftBuffer, DeviceReceiver::FFT_SEGMENT_SAMPLE_COUNT, false, true, !DebuggingUtilities::RECEIVE_TEST_DATA);
			FFT_BYTES(buffer, fftBuffer, DeviceReceiver::FFT_SEGMENT_SAMPLE_COUNT, false, true);

			SignalProcessingUtilities::CalculateMagnitudesAndPhasesForArray(fftBuffer, DeviceReceiver::FFT_SEGMENT_SAMPLE_COUNT);			

			((SpectrumAnalyzer*) deviceReceivers->parent)->SetFFTInput(fftBuffer, frequencyRange, deviceID);
			
			////ReleaseSemaphore(setFFTGate2, deviceReceivers->count, NULL);
			ReleaseSemaphore(deviceReceivers->receiversFinished[deviceID], 1, NULL);

			if (referenceDevice)
			{
				WaitForMultipleObjects(deviceReceivers->count, deviceReceivers->receiversFinished, true, 1000000);
				spectrumAnalyzer->ProcessFFTInput(frequencyRange);
				
				deviceReceivers->ReleaseReceiverGates();				
			}
			
			////fftSpectrumBuffers->SetFFTInput(fftSpectrumBufferIndex, fftBuffer, i, frequencyRange, i == 0);		
	}

	////fftSpectrumBuffers->ProcessFFTInput(fftSpectrumBufferIndex, &frequencyRange, useRatios);
	

	////delete[] fftBuffer;



	for (i = 0; i < devicesToSendClonedDataToCount; i++)	
		devicesToSendClonedDataTo[i]->ReceiveData(transferDataBuffer, length);


	if (ReleaseSemaphore(receiverBufferDataAvailableGate, 1, NULL))
	{
		////DebuggingUtilities::DebugPrint("ReleaseSemaphore error: %d\n", GetLastError());
	}
	
}
*/

void DataReceiver(unsigned char *buf, uint32_t len, void *ctx)
{
	struct DeviceReceiver* deviceReceiver = (DeviceReceiver*) ctx;

	deviceReceiver->ReceiveData(buf, len);
	////Sleep(1000);
}

void ReceivingDataThread(void *param)
{
	//Sleep(10000);

	int result;
	DeviceReceiver* deviceReceiver = (DeviceReceiver*)param;

	if (DeviceReceiver::RECEIVING_GNU_DATA)
	{
		DWORD dwWaitResult = WaitForSingleObject(((DeviceReceivers*)deviceReceiver->parent)->startReceivingDataGate, 2000);

		int bytes_received = 0;
		//char buffer[4096];

		while (DeviceReceiver::RECEIVING_GNU_DATA)
		{
			bytes_received = recvfrom(deviceReceiver->sd, (char *)deviceReceiver->gnuReceivedBuffer, deviceReceiver->RECEIVE_BUFF_LENGTH, 0, (struct sockaddr *)&(deviceReceiver->server), &(deviceReceiver->server_length));
			//bytes_received = recvfrom(deviceReceiver->sd, (char *)deviceReceiver->gnuReceivedBuffer, 4096, 0, (struct sockaddr *)&(deviceReceiver->server), &(deviceReceiver->server_length));

			//bytes_received = recvfrom(deviceReceiver->sd, buffer, 4096, 0, (struct sockaddr *)&(deviceReceiver->server), &(deviceReceiver->server_length));

			if (bytes_received < 0)
			{
				fprintf(stderr, "Error receiving data.\n");
				closesocket(deviceReceiver->sd);
				WSACleanup();
				exit(0);
			}

			deviceReceiver->ReceiveData(deviceReceiver->gnuReceivedBuffer, deviceReceiver->RECEIVE_BUFF_LENGTH);
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
		receivingDataThreadHandle = (HANDLE)_beginthread(ReceivingDataThread, 0, this);
}


void DeviceReceiver::StartProcessingData()
{	
	processingDataThreadHandle = (HANDLE)_beginthread(ProcessReceivedDataThread, 0, this);
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
		

		DWORD dwWaitResult = WaitForSingleObject(receiverBufferDataAvailableGate, 2000);

		////DWORD dwWaitResult = WAIT_OBJECT_0;

		switch (dwWaitResult)
		{
		case WAIT_OBJECT_0:
			if (DebuggingUtilities::DEBUGGING)
				DebuggingUtilities::DebugPrint("GetDelayAndPhaseShiftedData(): receiverBufferDataAvailableGate open:-> circularDataBuffer->ReadData()\n");

			if (durationMilliSeconds > 0)
			{
				length = (float)durationMilliSeconds / 1000 * SAMPLE_RATE;
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


			/*////if (length > 0)
			{
				dataBufferLength = std::min(dataBufferLength, length);

				for (int j = 0; j < dataBufferLength; j++)
				{					
					dataBuffer[j] = dataBuffer[j] - 127;					
				}
			}*/

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
	////unsigned int sinFrequency = 16000;

	unsigned int sinFrequency = 10;
	float sinMultiplier = sinFrequency * 2 * MathUtilities::PI;

	float value;

	double phase = 0;

	if (referenceDevice)
		////phase = -MathUtilities::PI * (float) 2/3;
		////phase = -MathUtilities::PI * (float)1 / 2;		
		////phase = MathUtilities::PI * (float)1 / 2;
		phase = -MathUtilities::PI;
		

	if (realValue)
		value = (float)sin(sinMultiplier * (float)index / length + phase);
	else
		value = (float)cos(sinMultiplier * (float)index / length + phase);

	////value /= 2;

	if (value>1.0)
		value = 1.0;

	if (value < -1.0)
		value = -1;

	////value *= (GetAlphaAtTime(currentTime) + 1);

	////value *= (GetAlphaAtTime(currentTime));


	/*////unsigned int alphaFrequency = 1;
	float alphaMultiplier = alphaFrequency * 2 * MathUtilities::PI;

	value += ((float)sin(alphaMultiplier * (float)index / length)/10);

	////value *= ((float)sin(alphaMultiplier * (float)index / length) / 10);

	////value = (float)sin(alphaMultiplier * (float)index / length);
	*/


	if (value>1.0)
		value = 1.0;

	if (value < -1.0)
		value = -1;

	////value += 1;
	////value /= 2;

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
		////dataBuffer[k * 2 + 1] = 127;
	}
}

void DeviceReceiver::ReceiveTestData(uint32_t length)
{
	DWORD startSegment = GetTickCount();

	DeviceReceivers* deviceReceivers = (DeviceReceivers*)parent;

	if (dataBuffer == NULL)
		dataBuffer = new uint8_t [length];

	GetTestData(dataBuffer, length);

	////unsigned char *dataBuffer = new unsigned char[FFT_SEGMENT_BUFF_LENGTH];
	////uint32_t length = this->CORRELATION_BUFF_LENGTH / 2;

	/*////for (unsigned int k = 0; k < length; k+=2)
	{
		dataBuffer[k] = GetSampleByteAtIndex(k, length, GetTickCount());
		dataBuffer[k + 1] = GetSampleByteAtIndex(k, length, GetTickCount(), false);
		////dataBuffer[k * 2 + 1] = 127;
	}
	*/

	////circularDataBuffer->WriteData(dataBuffer, FFT_SEGMENT_BUFF_LENGTH);

	////circularDataBuffer->WriteData(dataBuffer, length);

	////DWORD dwWaitResult1 = WaitForSingleObject(deviceReceivers->receiveDataGate1, 1000);
	////DWORD dwWaitResult2 = WaitForSingleObject(deviceReceivers->receiveDataGate2, 1000);

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

	////delete[] dataBuffer;
}

void DeviceReceiver::SetDelayShift(double value)
{
	////delayShift = value;
	////delayShift = 0;

	delayShift += value;
	////delayShift = 200;

	if (delayShift < -DeviceReceiver::FFT_SEGMENT_BUFF_LENGTH*2/3 || delayShift > DeviceReceiver::FFT_SEGMENT_BUFF_LENGTH*2/3)
		delayShift = 0;
}

void DeviceReceiver::SetPhaseShift(double value)
{
	phaseShift += value;

	////phaseShift = -3.14;
	//phaseShift = 0;
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


bool DeviceReceiver::RECEIVING_GNU_DATA = false;
uint32_t DeviceReceiver::SAMPLE_RATE = 0;

long DeviceReceiver::RECEIVE_BUFF_LENGTH = 16384;

long DeviceReceiver::FFT_SEGMENT_BUFF_LENGTH = 16384;
long DeviceReceiver::CORRELATION_BUFF_LENGTH = FFT_SEGMENT_BUFF_LENGTH * 2;
long DeviceReceiver::FFT_SEGMENT_SAMPLE_COUNT = FFT_SEGMENT_BUFF_LENGTH / 2;

uint32_t DeviceReceiver::MAXRECEIVELOG = 100;
