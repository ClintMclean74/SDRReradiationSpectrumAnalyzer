#define _ITERATOR_DEBUG_LEVEL 0
#include <cmath>
#include "DeviceReceivers.h"
#include "convenience.h"
#include "FFTSpectrumBuffers.h"
#include "ArrayUtilities.h"
#include "SignalProcessingUtilities.h"
#include "fftw3.h"
#include "NearFarDataAnalyzer.h"
#include "DebuggingUtilities.h"

DeviceReceivers::DeviceReceivers(void* parent, long bufferSizeInMilliSeconds, uint32_t sampleRate)
{	
	this->parent = parent;
	char vendor[256], product[256], serial[256];

	count = rtlsdr_get_device_count();
	
	fprintf(stderr, "Found %d device(s):\n", count);
	
	/*////for (int i = 0; i < count; i++)
	{
		rtlsdr_get_device_usb_strings(i, vendor, product, serial);

		fprintf(stderr, "  %d:  %s, %s, SN: %s\n", i, vendor, product, serial);
	}*/
	
	deviceReceivers = new DeviceReceiversPtr[count];
	fftBuffers = new fftw_complex_ptr[count];

	////receiverGates = new HANDLE[count];
	////receiversFinished = new HANDLE[count];

	uint8_t id;
	char strBuffer[10];
	for (int i = 0; i < count; i++)
	{		
		rtlsdr_get_device_usb_strings(i, vendor, product, serial);
		fprintf(stderr, "  %d:  %s, %s, SN: %s\n", i, vendor, product, serial);

		id = atoi(serial);

		deviceReceivers[i] = new DeviceReceiver(this, bufferSizeInMilliSeconds, sampleRate, id);

		////receiverGates[i] = CreateSemaphore(NULL, 0, 10, NULL);
		////receiversFinished[i] = CreateSemaphore(NULL, 0, 10, NULL);
	}
		

	startReceivingDataGate = CreateSemaphore(
		NULL,           // default security attributes
		0,  // initial count
		100,  // maximum count
		NULL);          // unnamed semaphore

	if (startReceivingDataGate == NULL)
	{
		DebuggingUtilities::DebugPrint("CreateSemaphore error: %d\n", GetLastError());
	}

	receiveDataGate1 = CreateSemaphore(
		NULL,           // default security attributes
		0,  // initial count
		100,  // maximum count
		NULL);          // unnamed semaphore

	if (receiveDataGate1 == NULL)
	{
		DebuggingUtilities::DebugPrint("CreateSemaphore error: %d\n", GetLastError());
	}

	receiveDataGate2 = CreateSemaphore(
		NULL,           // default security attributes
		0,  // initial count
		100,  // maximum count
		NULL);          // unnamed semaphore

	if (receiveDataGate2 == NULL)
	{
		DebuggingUtilities::DebugPrint("CreateSemaphore error: %d\n", GetLastError());
	}	


	fftBytesGate = CreateSemaphore(
		NULL,           // default security attributes
		0,  // initial count
		100,  // maximum count
		NULL);          // unnamed semaphore

	if (fftBytesGate == NULL)
	{
		DebuggingUtilities::DebugPrint("CreateSemaphore error: %d\n", GetLastError());
	}


	fftComplexGate = CreateSemaphore(
		NULL,           // default security attributes
		0,  // initial count
		100,  // maximum count
		NULL);          // unnamed semaphore

	if (fftComplexGate == NULL)
	{
		DebuggingUtilities::DebugPrint("CreateSemaphore error: %d\n", GetLastError());
	}	
}

void DeviceReceivers::InitializeDevices(int* deviceIDs)
{
	DeviceReceiversPtr* deviceReceiversTemp = new DeviceReceiversPtr[count];

	uint8_t usingDeviceCount = 0;

	int8_t referenceIndex = -1;


	if (!DeviceReceiver::RECEIVING_GNU_DATA)
	{
		for (int i = 0; i < count; i++)
		{
			if (deviceReceivers[i] != NULL)
			{
				if (deviceReceivers[i]->InitializeDeviceReceiver(i) < 0)
				{
					deviceReceivers[i] = NULL;

					continue;
				}


				{
					if (i == 0)
						noiseDevice = deviceReceivers[i];

					if (ArrayUtilities::InArray(deviceReceivers[i]->deviceID, deviceIDs, 3))
					{
						deviceReceiversTemp[usingDeviceCount++] = deviceReceivers[i];
					}
				}
				/*////else
				{
					if (referenceIndex == -1)
					{
						referenceIndex = i;

						deviceReceivers[i]->referenceDevice = true;

						////deviceReceivers[i]->GenerateNoise(1);
					}

					fftBuffers[i] = new fftw_complex[DeviceReceiver::FFT_SEGMENT_BUFF_LENGTH];

					initializedDevices++;
				}*/
			}
		}

		/*////DeviceReceiversPtr* deviceReceiversTemp = new DeviceReceiversPtr[count];

		uint8_t usingDeviceCount = 0;

		for (uint8_t i = 0; i < count; i++)
		{
			if (deviceReceivers[i] != NULL)
			{
				if (i == 0)
					noiseDevice = deviceReceivers[i];

				if (ArrayUtilities::InArray(deviceReceivers[i]->deviceID, deviceIDs, 3))
				{
					deviceReceiversTemp[usingDeviceCount++] = deviceReceivers[i];
				}
			}
		}
		*/

		delete[] deviceReceivers;

		deviceReceivers = deviceReceiversTemp;

		count = usingDeviceCount;
	}

	deviceReceivers[0]->referenceDevice = true;	

	receiverGates = new HANDLE[count];
	receiversFinished = new HANDLE[count];

	for (int i = 0; i < count; i++)
	{	
		deviceReceivers[i]->deviceID = i;

		fftBuffers[i] = new fftw_complex[DeviceReceiver::FFT_SEGMENT_BUFF_LENGTH];

		receiversFinished[i] = deviceReceivers[i]->receiverFinished;
		receiverGates[i] = deviceReceivers[i]->receiverGate;

		initializedDevices++;
	}

	if (!DeviceReceiver::RECEIVING_GNU_DATA)
		noiseDevice->GenerateNoise(1);
}

void DeviceReceivers::GenerateNoise(uint8_t state)
{
	generatingNoise = state;

	noiseDevice->GenerateNoise(state);

	/*////for (int i = 0; i < count; i++)
	{
		if (deviceReceivers[i] != NULL)
		{
			deviceReceivers[i]->GenerateNoise(state);
		}
	}*/
}

void DeviceReceivers::SetCurrentCenterFrequency(uint32_t centerFrequency)
{		
	uint32_t halfBandWidth = DeviceReceiver::SAMPLE_RATE / 2;

	frequencyRange.Set(centerFrequency - halfBandWidth, centerFrequency + halfBandWidth);

	for (int i = 0; i < count; i++)
	{
		if (deviceReceivers[i] != NULL)
		{
			deviceReceivers[i]->SetFrequencyRange(&frequencyRange);
		}
	}
}

void DeviceReceivers::StartReceivingData()
{
	////DWORD startTime = GetTickCount();

	int deviceIndexToCloneReceivedData = -1;
	for (int i = 0; i < count; i++)
	{
		if (deviceReceivers[i] != NULL)
		{			
			deviceReceivers[i]->StartReceivingData();

			if (deviceIndexToCloneReceivedData != -1)
				deviceReceivers[deviceIndexToCloneReceivedData]->AddDeviceToSendClonedDataTo(deviceReceivers[i]);
			
			if (deviceIndexToCloneReceivedData == -1 && DebuggingUtilities::TEST_CORRELATION)
			{
				deviceIndexToCloneReceivedData = i;				
			}
		}
	}

	Sleep(1000);

	if (ReleaseSemaphore(startReceivingDataGate, 2, NULL)==0)
	{
		if (DebuggingUtilities::DEBUGGING)
		{
			DebuggingUtilities::DebugPrint("ReleaseSemaphore error: %d\n", GetLastError());			
		}
	}

	if (DebuggingUtilities::DEBUGGING)
	{
		DebuggingUtilities::DebugPrint("StartReceivingData(): open startReceivingDataGate:-> StartReceivingData() initialized\n");		
	}


	for (int i = 0; i < count; i++)
	{
		if (deviceReceivers[i] != NULL)
		{
			deviceReceivers[i]->StartProcessingData();			
		}
	}
}

void DeviceReceivers::Correlate(bool correlate)
{
	if (correlate)
		correlationCount = 0;
	else
		correlationCount = maxCorrelationCount;
}

void DeviceReceivers::Synchronize()
{
	correlationCount = 0;

	synchronizing = true;
}

bool DeviceReceivers::Correlated()
{
	if (correlationCount < maxCorrelationCount)
		return false;

	synchronizing = false;

	return true;
}

uint32_t DeviceReceivers::SynchronizeData(uint8_t* data1, uint8_t* data2)
{
	int8_t referenceIndex = 0;	
	double avgDelay = 0;
	double phaseAngleShift = 0;
	int32_t index;
	
	char textBuffer[255];
		
	correlationBufferLengthZeroPadded = DeviceReceiver::FFT_SEGMENT_BUFF_LENGTH * 2;
	correlationBufferSamples = correlationBufferLengthZeroPadded / 2;

	if (referenceDataBuffer == NULL)
	{
		referenceDataBuffer = new uint8_t[correlationBufferLengthZeroPadded];
		dataBuffer = new uint8_t[correlationBufferLengthZeroPadded];

		convolutionFFT = new fftw_complex[correlationBufferSamples*2];
		convolution = new fftw_complex[correlationBufferSamples];
	}

	
	////deviceReceivers[referenceIndex]->GetDelayAndPhaseShiftedData(referenceDataBuffer, correlationBufferLengthZeroPadded, -1, DeviceReceiver::FFT_SEGMENT_BUFF_LENGTH);
	////deviceReceivers[1]->GetDelayAndPhaseShiftedData(dataBuffer, correlationBufferLengthZeroPadded, -1, DeviceReceiver::FFT_SEGMENT_BUFF_LENGTH);

	if (dataGraph)
	{				
		dataGraph->SetData(data1, DeviceReceiver::FFT_SEGMENT_BUFF_LENGTH, 0, true,  -128, -128, false, SignalProcessingUtilities::DataType::UINT8_T);
		dataGraph->SetData(data2, DeviceReceiver::FFT_SEGMENT_BUFF_LENGTH, 1, true, -128, -128, false, SignalProcessingUtilities::DataType::UINT8_T);
	}

	memcpy(&dataBuffer[DeviceReceiver::FFT_SEGMENT_BUFF_LENGTH], data2, DeviceReceiver::FFT_SEGMENT_BUFF_LENGTH);
	memset(dataBuffer, 128, DeviceReceiver::FFT_SEGMENT_BUFF_LENGTH);

	////deviceReceivers[1]->FFT_BYTES(dataBuffer, fftBuffers[1], correlationBufferSamples, false, true, false);
	////deviceReceivers[1]->FFT_BYTES(dataBuffer, fftBuffers[1], correlationBufferSamples, false, true, false);
	deviceReceivers[referenceIndex]->FFT_BYTES(dataBuffer, fftBuffers[1], correlationBufferSamples, false, true, false);



	memcpy(referenceDataBuffer, data1, DeviceReceiver::FFT_SEGMENT_BUFF_LENGTH);
	memset(&referenceDataBuffer[DeviceReceiver::FFT_SEGMENT_BUFF_LENGTH], 128, DeviceReceiver::FFT_SEGMENT_BUFF_LENGTH);

	deviceReceivers[referenceIndex]->FFT_BYTES(referenceDataBuffer, fftBuffers[referenceIndex], correlationBufferSamples, false, true, false);
	
	SignalProcessingUtilities::ConjugateArray(fftBuffers[referenceIndex], correlationBufferSamples);



	SignalProcessingUtilities::ComplexMultiplyArrays(fftBuffers[referenceIndex], fftBuffers[1], convolutionFFT, correlationBufferSamples);

	deviceReceivers[referenceIndex]->FFT_COMPLEX_ARRAY(convolutionFFT, convolution, correlationBufferSamples, true, false);
	
	
	
	double* maxIndexValue = ArrayUtilities::GetMaxValueAndIndex(convolution, correlationBufferSamples, true, false, true);
	index = maxIndexValue[0];

	avgDelay = (index - correlationBufferSamples/2);
	avgDelay *= 2;

	phaseAngleShift = (atan2(convolution[index][1], convolution[index][0]));
	

	if (correlationGraph)
	{
		////snprintf(textBuffer, 255, "Delay: %f", avgDelay);
		////correlationGraph->SetText(textBuffer, 255, 1);

		correlationGraph->SetText(1, "Delay: %f", avgDelay);

		////snprintf(textBuffer, 255, "Phase: %f", phaseAngleShift);
		////correlationGraph->SetText(textBuffer, 255, 2);
		correlationGraph->SetText(2, "Phase: %f", phaseAngleShift);

		correlationGraph->SetData(convolution, correlationBufferSamples, 0, true, 0, 0, true);
		
			////SignalProcessingUtilities::CalculateMagnitudesAndPhasesForArray(referenceDataBuffer, correlationBufferSamples);
			////SignalProcessingUtilities::CalculateMagnitudesAndPhasesForArray(dataBuffer, correlationBufferSamples);

			////ArrayUtilities::SubArrays(referenceDataBuffer, correlationBufferSamples, dataBuffer);

			////dataGraph->SetData(dataBuffer, correlationBufferSamples, 0, false, true);
	}	
	
	/*////if (avgDelay != 0)
	{
		avgDelay *= -1;

		if (avgDelay > 0)
			deviceReceivers[referenceIndex]->SetDelayShift(avgDelay);
		else
			deviceReceivers[1]->SetDelayShift(-avgDelay);
	}*/


	if (synchronizing)
	{
		deviceReceivers[referenceIndex]->SetDelayShift(-avgDelay);
		deviceReceivers[referenceIndex]->SetPhaseShift(-phaseAngleShift);
	}
		
	/*////if (avgDelaysCount < 1000)
		avgDelays[avgDelaysCount++] = avgDelay;
	else
		int grc = 1;
		*/

	correlationCount++;

	return avgDelay;
}

uint32_t DeviceReceivers::GetDataForDevice(double *dataBuffer, uint8_t deviceIndex)
{
	if (deviceReceivers[deviceIndex])
	{
		deviceReceivers[deviceIndex]->GetDelayAndPhaseShiftedData(this->dataBuffer, DeviceReceiver::FFT_SEGMENT_BUFF_LENGTH);

		/*////if (deviceReceivers[deviceIndex]->delayShift != 0)
		{
			memset(dataBuffer, 128, DeviceReceiver::FFT_SEGMENT_BUFF_LENGTH * sizeof(double));

				if (deviceReceivers[deviceIndex]->delayShift < 0)
					ArrayUtilities::CopyArray(&this->dataBuffer[(uint32_t)deviceReceivers[deviceIndex]->delayShift], DeviceReceiver::FFT_SEGMENT_BUFF_LENGTH - deviceReceivers[deviceIndex]->delayShift, dataBuffer);
				else
					ArrayUtilities::CopyArray(this->dataBuffer, DeviceReceiver::FFT_SEGMENT_BUFF_LENGTH - deviceReceivers[deviceIndex]->delayShift, &dataBuffer[(uint32_t)-deviceReceivers[deviceIndex]->delayShift]);
		}
		else
			ArrayUtilities::CopyArray(this->dataBuffer, DeviceReceiver::FFT_SEGMENT_BUFF_LENGTH, dataBuffer);
			*/

		return DeviceReceiver::FFT_SEGMENT_BUFF_LENGTH;
	}
	else
		return 0;
}


uint32_t DeviceReceivers::GetDataForDevice(uint8_t *dataBuffer, uint8_t deviceIndex)
{
	if (deviceReceivers[deviceIndex])
	{
		////deviceReceivers[deviceIndex]->GetDelayAndPhaseShiftedData(this->dataBuffer, DeviceReceiver::FFT_SEGMENT_BUFF_LENGTH);
		deviceReceivers[deviceIndex]->GetDelayAndPhaseShiftedData(dataBuffer, DeviceReceiver::FFT_SEGMENT_BUFF_LENGTH);

		/*////if (deviceReceivers[deviceIndex]->delayShift != 0)
		{
			memset(dataBuffer, 128, DeviceReceiver::FFT_SEGMENT_BUFF_LENGTH);

			if (deviceReceivers[deviceIndex]->delayShift > 0)
				////ArrayUtilities::CopyArray(&this->dataBuffer[(uint32_t)deviceReceivers[deviceIndex]->delayShift], DeviceReceiver::FFT_SEGMENT_BUFF_LENGTH - deviceReceivers[deviceIndex]->delayShift, dataBuffer);
				memcpy(dataBuffer, &this->dataBuffer[deviceReceivers[deviceIndex]->delayShift], DeviceReceiver::FFT_SEGMENT_BUFF_LENGTH - deviceReceivers[deviceIndex]->delayShift);
			else
				////ArrayUtilities::CopyArray(this->dataBuffer, DeviceReceiver::FFT_SEGMENT_BUFF_LENGTH - deviceReceivers[deviceIndex]->delayShift, &dataBuffer[(uint32_t)-deviceReceivers[deviceIndex]->delayShift]);
				memcpy(&dataBuffer[-deviceReceivers[deviceIndex]->delayShift], this->dataBuffer, DeviceReceiver::FFT_SEGMENT_BUFF_LENGTH + deviceReceivers[deviceIndex]->delayShift);
				
*/

			/*////
			if (deviceReceivers[deviceIndex]->delayShift < 0)
				////ArrayUtilities::CopyArray(&this->dataBuffer[(uint32_t)deviceReceivers[deviceIndex]->delayShift], DeviceReceiver::FFT_SEGMENT_BUFF_LENGTH - deviceReceivers[deviceIndex]->delayShift, dataBuffer);
				memcpy(dataBuffer, &this->dataBuffer[-deviceReceivers[deviceIndex]->delayShift], DeviceReceiver::FFT_SEGMENT_BUFF_LENGTH + deviceReceivers[deviceIndex]->delayShift);
			else
				////ArrayUtilities::CopyArray(this->dataBuffer, DeviceReceiver::FFT_SEGMENT_BUFF_LENGTH - deviceReceivers[deviceIndex]->delayShift, &dataBuffer[(uint32_t)-deviceReceivers[deviceIndex]->delayShift]);
				memcpy(&dataBuffer[deviceReceivers[deviceIndex]->delayShift], this->dataBuffer, DeviceReceiver::FFT_SEGMENT_BUFF_LENGTH - deviceReceivers[deviceIndex]->delayShift);
				*/
/*////		}
		else
			////ArrayUtilities::CopyArray(this->dataBuffer, DeviceReceiver::FFT_SEGMENT_BUFF_LENGTH, dataBuffer);
			memcpy(dataBuffer, this->dataBuffer, DeviceReceiver::FFT_SEGMENT_BUFF_LENGTH);
			*/

		return DeviceReceiver::FFT_SEGMENT_BUFF_LENGTH;
	}
	else
		return 0;
}

void DeviceReceivers::ReleaseReceiverGates()
{
	for (int i = 0; i < count; i++)
	{
		if (ReleaseSemaphore(receiverGates[i], 1, NULL) == 0)
		{
			if (DebuggingUtilities::DEBUGGING)
			{
				DebuggingUtilities::DebugPrint("ReleaseSemaphore error: %d\n", GetLastError());
			}
		}
	}
}

void DeviceReceivers::TransferDataToFFTSpectrumBuffer(FFTSpectrumBuffers* fftSpectrumBuffers, uint8_t fftSpectrumBufferIndex, bool useRatios)
{
	for (int i = 0; i < count; i++)
	{
		if (deviceReceivers[i] != NULL)
		{
			deviceReceivers[i]->GetDelayAndPhaseShiftedData(dataBuffer, DeviceReceiver::FFT_SEGMENT_BUFF_LENGTH);

			SignalProcessingUtilities::FFT_BYTES(dataBuffer, fftBuffers[i], DeviceReceiver::FFT_SEGMENT_SAMPLE_COUNT, false, true, !DebuggingUtilities::RECEIVE_TEST_DATA);

			SignalProcessingUtilities::CalculateMagnitudesAndPhasesForArray(fftBuffers[i], DeviceReceiver::FFT_SEGMENT_SAMPLE_COUNT);
		
			fftSpectrumBuffers->SetFFTInput(fftSpectrumBufferIndex, fftBuffers[i], dataBuffer, DeviceReceiver::FFT_SEGMENT_SAMPLE_COUNT, i, deviceReceivers[i]->frequencyRange, i==0);
		}
	}

	fftSpectrumBuffers->ProcessFFTInput(fftSpectrumBufferIndex, &frequencyRange, useRatios);	
}

void DeviceReceivers::GetDeviceCurrentFrequencyRange(uint32_t deviceIndex, uint32_t* startFrequency, uint32_t* endFrequency)
{
	if (deviceReceivers[deviceIndex])
	{
		deviceReceivers[deviceIndex]->GetDeviceCurrentFrequencyRange(startFrequency, endFrequency);
	}
}


DeviceReceivers::~DeviceReceivers()
{
	/*////if (complexArrayFFTPlan != NULL)
	{
		fftw_destroy_plan(complexArrayFFTPlan);
		fftw_destroy_plan(complexArrayFFTPlan2);
		fftw_destroy_plan(complexArrayCorrelationFFTPlan);

		complexArrayFFTPlan = NULL;
		complexArrayFFTPlan2 = NULL;
		complexArrayCorrelationFFTPlan = NULL;
	}*/

	
	for (int i = 0; i < count; i++)
	{
		deviceReceivers[i]->StopReceivingData();
		delete deviceReceivers[i];
	}

	delete[] deviceReceivers;

	delete[] dataBuffer;
}


HANDLE DeviceReceivers::startReceivingDataGate = NULL;
uint32_t DeviceReceivers::maxCorrelationCount = 300;