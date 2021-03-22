#pragma once
#include <winsock.h>
#include "rtl-sdr.h"
#include "CircularDataBuffer.h"
#include "FrequencyRange.h"
#include "fftw3.h"
#include "RateCounter.h"

////extern HANDLE receiverBufferDataAvailableGate = NULL;

class DeviceReceiver
{	
	private:					
		CircularDataBuffer *circularDataBuffer = NULL;		
		HANDLE receivingDataThreadHandle = NULL;
		HANDLE processingDataThreadHandle = NULL;
		uint8_t *transferDataBuffer;				
		DeviceReceiver** devicesToSendClonedDataTo;
		uint8_t devicesToSendClonedDataToCount = 0;		
		DWORD* receivedDuration = new DWORD[MAXRECEIVELOG];
		DWORD graphRefreshTime;
		uint32_t receivedCount = 0;
		uint8_t* buffer2 = NULL;
		fftw_complex* fftBuffer = NULL;
		fftw_complex *complexArray = NULL;
		fftw_complex *complexArray2 = NULL;		

		fftw_plan complexArrayFFTPlan = NULL, complexArrayFFTPlan2 = NULL, complexArrayCorrelationFFTPlan = NULL;

		////fftw_plan complexArrayFFTPlan2;
		////fftw_plan complexArrayFFTPlans2[2];

		uint8_t *dataBuffer = NULL;				
				
		float GetSampleByteAtIndex(long index, long length, long currentTime, bool realValue = true);		

		bool strengthDetectionSound = true;
		bool gradientDetectionSound = false;
		RateCounter soundRateCounter;
		uint32_t soundThresholdCount = 3;
				
	public:		
		DWORD prevReceivedTime;
		static bool RECEIVING_GNU_DATA;
		uint8_t *gnuReceivedBuffer;
		SOCKET sd;								/* The socket descriptor */
		int server_length;						/* Length of server struct */		
		struct sockaddr_in server;				/* Information about the server */
		rtlsdr_dev_t *device = NULL;
		uint8_t deviceID;
		static uint32_t SAMPLE_RATE;
		static long RECEIVE_BUFF_LENGTH;
		static long FFT_SEGMENT_BUFF_LENGTH;
		static long CORRELATION_BUFF_LENGTH;
		static long FFT_SEGMENT_SAMPLE_COUNT;
		short MAX_BUF_LEN_FACTOR = 1000;
		short ASYNC_BUF_NUMBER = 1;
		static uint32_t MAXRECEIVELOG;
		////long RTL_READ_FACTOR = 10;
		////long RTL_READ_FACTOR = 61;
		long RTL_READ_FACTOR = 122;
		FrequencyRange* frequencyRange;
		bool referenceDevice = false;		
		DWORD receivedDatatartTime = 0;
		int32_t delayShift = 0;
		double phaseShift = 0;

		////HANDLE receiveDataGate;
		HANDLE rtlDataAvailableGate = NULL;
		HANDLE receiverBufferDataAvailableGate;
		

		HANDLE setFFTGate1;
		HANDLE setFFTGate2;

		HANDLE receiverFinished = NULL;
		HANDLE receiverGate = NULL;
		
		void* parent;

		uint8_t *receivedBuffer = NULL;
		uint32_t receivedBufferLength = 0;
		uint32_t receivedLength = 0;

		uint8_t *receivedBufferPtr = NULL;

		bool receivingData = false;
				
		DeviceReceiver(void* parent, long bufferSizeInMilliSeconds, uint32_t sampleRate, uint8_t ID);
		int InitializeDeviceReceiver(int dev_index);
		void SetGain(int gain);
		void SetFrequencyRange(FrequencyRange* frequencyRange);
		void CheckScheduledFFTBufferIndex();
		void FFT_BYTES(uint8_t *data, fftw_complex *fftData, int samples, bool inverse, bool inputComplex, bool rotate180 = false, bool synchronizing = false);
		void FFT_COMPLEX_ARRAY(fftw_complex* data, fftw_complex* fftData, int samples, bool inverse, bool rotate180, bool synchronizing = false);
		void GenerateNoise(uint8_t state);
		void WriteReceivedDataToBuffer(uint8_t *data, uint32_t length);
		void ProcessData(uint8_t *data, uint32_t length);
		void ProcessData(fftw_complex *data, uint32_t length);
		void ReceiveData(uint8_t* bufffer, long length);
		void ReceiveData2(uint8_t* buffer, long length);
		void GetTestData(uint8_t* buffer, uint32_t length);
		void ReceiveTestData(uint32_t length);
		int GetDelayAndPhaseShiftedData(uint8_t* dataBuffer, long dataBufferLength, float durationMilliSeconds = -1, int durationBytes = -1, bool async = true);
		void StartReceivingData();
		void StartProcessingData();
		void StopReceivingData();
		void SetDelayShift(double value);
		void SetPhaseShift(double value);
		void DeviceReceiver::AddDeviceToSendClonedDataTo(DeviceReceiver *deviceReceiver);
		void GetDeviceCurrentFrequencyRange(uint32_t* startFrequency, uint32_t* endFrequency);
		float GetSampleAtIndex(long index, long length, long currentTime, bool realValue = true);
	
		~DeviceReceiver();
};

////HANDLE DeviceReceiver::receiverBufferDataAvailableGate = NULL;

////int DeviceReceiver::count = 0;
////int DeviceReceiver::objectCount = 0;

typedef DeviceReceiver* DeviceReceiverPtr;

