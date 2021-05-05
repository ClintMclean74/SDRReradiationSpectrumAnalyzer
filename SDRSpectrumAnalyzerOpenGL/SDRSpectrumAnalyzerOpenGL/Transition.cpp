#include <iostream>
#include "Transitions.h"
#include "Transition.h"
#include "ArrayUtilities.h"

Transition::Transition(BandwidthFFTBuffer* transitionBandwidthFFTBuffer, DWORD transitionDuration)
{
	this->bandwidthFFTBuffer = new BandwidthFFTBuffer(&transitionBandwidthFFTBuffer->range, BandwidthFFTBuffer::FFT_ARRAYS_COUNT);

	CopyTransitionFFTDataIntoBuffer(transitionBandwidthFFTBuffer, transitionDuration);
};

Transition::Transition(BandwidthFFTBuffer* nearBandwidthFFTBuffer, BandwidthFFTBuffer* farBandwidthFFTBuffer, DWORD transitionDuration)
{
	this->bandwidthFFTBuffer = new BandwidthFFTBuffer(&nearBandwidthFFTBuffer->range, BandwidthFFTBuffer::FFT_ARRAYS_COUNT);

	CopyNearFarFFTDataIntoBuffer(nearBandwidthFFTBuffer, farBandwidthFFTBuffer, transitionDuration);
};

Transition::Transition(BandwidthFFTBuffer* transitionBandwidthFFTBuffer, DWORD transitionDuration, FrequencyRange* range, uint32_t startIndex, uint32_t endIndex)
{
	this->range.Set(range);
	this->bandwidthFFTBuffer = new BandwidthFFTBuffer(range, BandwidthFFTBuffer::FFT_ARRAYS_COUNT);

	uint32_t resultLength;
	DWORD duration = transitionDuration;

	transitionBandwidthFFTBuffer->CopyDataIntoBuffer(this->bandwidthFFTBuffer, &duration, 0, &resultLength, 0, startIndex, endIndex);	

	
	this->bandwidthAverageFFTBuffer = new BandwidthFFTBuffer(range, BandwidthFFTBuffer::FFT_ARRAYS_COUNT);
	
	duration = transitionDuration;

	transitionBandwidthFFTBuffer->CopyDataIntoBuffer(this->bandwidthAverageFFTBuffer, &duration, 0, &resultLength, 0, startIndex, endIndex);
};


void Transition::CopyTransitionFFTDataIntoBuffer(BandwidthFFTBuffer* transitionBandwidthFFTBuffer, DWORD transitionDuration)
{
	uint32_t resultLength;
	DWORD duration = transitionDuration;

	transitionBandwidthFFTBuffer->CopyDataIntoBuffer(this->bandwidthFFTBuffer, &duration, 0, &resultLength);

	if (duration != transitionDuration)
		std::cout << "Error: Duration: " << duration << " less than: " << transitionDuration;
}


void Transition::CopyNearFarFFTDataIntoBuffer(BandwidthFFTBuffer* nearBandwidthFFTBuffer, BandwidthFFTBuffer* farBandwidthFFTBuffer, DWORD transitionDuration)
{
	uint32_t resultLength;
	DWORD duration = transitionDuration / 2;

	farBandwidthFFTBuffer->CopyDataIntoBuffer(this->bandwidthFFTBuffer, &duration, 0, &resultLength);

	if (duration != transitionDuration / 2)
		std::cout << "Error: Duration: " << duration << " less than: " << transitionDuration / 2;

	duration = transitionDuration / 2;

	nearBandwidthFFTBuffer->CopyDataIntoBuffer(this->bandwidthFFTBuffer, &duration, 0, &resultLength);

	if (duration != transitionDuration / 2)
		std::cout << "Error: Duration: " << duration << " less than: " << transitionDuration / 2;
}

void Transition::SetTransitionData(BandwidthFFTBuffer* transitionBandwidthFFTBuffer, DWORD transitionDuration)
{
	this->bandwidthFFTBuffer->Reset();
	CopyTransitionFFTDataIntoBuffer(transitionBandwidthFFTBuffer, transitionDuration);
}

void Transition::SetTransitionData(BandwidthFFTBuffer* transitionBandwidthFFTBuffer, DWORD transitionDuration, FrequencyRange* range, uint32_t startIndex, uint32_t endIndex)
{
	this->bandwidthFFTBuffer->Reset();

	uint32_t resultLength;
	DWORD duration = transitionDuration;

	transitionBandwidthFFTBuffer->CopyDataIntoBuffer(this->bandwidthFFTBuffer, &duration, 0, &resultLength, 0, startIndex, endIndex);
	
	duration = transitionDuration;

	transitionBandwidthFFTBuffer->CopyDataIntoBuffer(this->bandwidthAverageFFTBuffer, &duration, 0, &resultLength, 0, startIndex, endIndex, true);
}

void Transition::SetTransitionData(BandwidthFFTBuffer* nearBandwidthFFTBuffer, BandwidthFFTBuffer* farBandwidthFFTBuffer, DWORD transitionDuration)
{
	this->bandwidthFFTBuffer->Reset();
	CopyNearFarFFTDataIntoBuffer(nearBandwidthFFTBuffer, farBandwidthFFTBuffer, transitionDuration);
}

SignalProcessingUtilities::Strengths_ID_Time* Transition::GetStrengthForRangeOverTime(uint32_t startIndex, uint32_t endIndex, uint32_t* resultLength)
{	
	DWORD duration = Transitions::DURATION;

	SignalProcessingUtilities::Strengths_ID_Time *strengths = bandwidthFFTBuffer->GetStrengthForRangeOverTime(startIndex, endIndex, &duration, 0, resultLength, 0);

	return strengths;
}

SignalProcessingUtilities::Strengths_ID_Time* Transition::GetAveragedTransitionsStrengthForRangeOverTime(uint32_t startIndex, uint32_t endIndex, uint32_t* resultLength)
{
	uint32_t dataLength = 0;
	DWORD duration = Transitions::DURATION;

	SignalProcessingUtilities::Strengths_ID_Time *strengths = bandwidthAverageFFTBuffer->GetStrengthForRangeOverTime(startIndex, endIndex, &duration, 0, &dataLength, 0);

	return strengths;
}

void Transition::DetermineTransitionStrength()
{	
	uint32_t dataLength = 0;
	DWORD duration = Transitions::DURATION;

	SignalProcessingUtilities::Strengths_ID_Time *strengths = bandwidthFFTBuffer->GetStrengthForRangeOverTime(0, 0, &duration, 0, &dataLength, 0);

	ArrayUtilities::AverageDataArray(strengths, dataLength, 3); //first half and second half average

	strength = strengths[(uint32_t) (dataLength * 0.25)].strength / strengths[(uint32_t)( dataLength * 0.75)].strength;
}