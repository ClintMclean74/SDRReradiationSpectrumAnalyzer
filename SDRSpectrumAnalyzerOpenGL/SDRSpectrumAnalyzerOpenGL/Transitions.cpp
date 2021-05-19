#include "Transitions.h"
#include "NearFarDataAnalyzer.h"

Transitions::Transitions()
{


};

Transition* Transitions::GetTransition(FrequencyRange *range)
{
	Transition* transitionPtr = first;

	while (transitionPtr!=NULL)
	{
		if (transitionPtr->bandwidthFFTBuffer->range.lower == range->lower && transitionPtr->bandwidthFFTBuffer->range.upper == range->upper)
			return transitionPtr;

		transitionPtr = transitionPtr->next;
	}

	return transitionPtr;
}

Transition* Transitions::Add(BandwidthFFTBuffer* transitionBandwidthFFTBuffer, DWORD transitionDuration)
{
	Transition* transition = GetTransition(&transitionBandwidthFFTBuffer->range);

	if (transition)
	{
		transition->SetTransitionData(transitionBandwidthFFTBuffer, transitionDuration);
	}
	else
	{

		transition = new Transition(transitionBandwidthFFTBuffer, transitionDuration);

		if (first == NULL)
		{
			first = transition;
			last = first;
		}
		else
		{
			last->next = transition;
			transition->previous = last;
			last = last->next;
		}

		count++;
	}

	return transition;
}

Transition* Transitions::Add(BandwidthFFTBuffer* transitionBandwidthFFTBuffer, DWORD transitionDuration, FrequencyRange* range, uint32_t startIndex, uint32_t endIndex)
{
	Transition* transition = GetTransition(range);

	if (transition)
	{
		transition->SetTransitionData(transitionBandwidthFFTBuffer, transitionDuration, range, startIndex, endIndex, true);
	}
	else
	{
		transition = new Transition(transitionBandwidthFFTBuffer, transitionDuration, range, startIndex, endIndex);

		if (first == NULL)
		{
			first = transition;
			last = first;
		}
		else
		{
			last->next = transition;
			transition->previous = last;
			last = last->next;
		}

		count++;		
	}

	return transition;
}


Transition* Transitions::Add(BandwidthFFTBuffer* nearBandwidthFFTBuffer, BandwidthFFTBuffer* farBandwidthFFTBuffer, DWORD transitionDuration)
{
	Transition* transition = new Transition(nearBandwidthFFTBuffer, farBandwidthFFTBuffer, DURATION);

	if (first == NULL)
	{
		first = transition;
		last = first;
	}
	else
	{
		last->next = transition;
		transition->previous = last;
		last = last->next;
	}

	count++;

	return transition;
}

void Transitions::DetermineTransitionStrengths()
{	
	Transition* transitionPtr = first;

	while (transitionPtr != NULL)
	{
		transitionPtr->DetermineTransitionStrength();

		transitionPtr = transitionPtr->next;
	}
}


void Transitions::GetFrequencyRangesAndTransitionStrengths(FrequencyRanges* frequencyRanges)
{	
	//DetermineTransitionStrengths();

	Transition* transitionPtr = first;
	
	FrequencyRange* range;

	while (transitionPtr != NULL)
	{		
		frequencyRanges->Add(transitionPtr->bandwidthFFTBuffer->range.lower, transitionPtr->bandwidthFFTBuffer->range.upper, transitionPtr->averagedStrengthForAllTransitions, 1, true);		

		//if (transitionPtr->writes >= NearFarDataAnalyzer::REQUIRED_TRANSITION_WRITES_FOR_RERADIATED_FREQUENCIES)
		//if (transitionPtr->writes >= NearFarDataAnalyzer::REQUIRED_TRANSITION_WRITES_FOR_RERADIATED_FREQUENCIES && transitionPtr->averagedStrengthForAllTransitions >= NearFarDataAnalyzer::REQUIRED_TRANSITION_STRENGTH_FOR_RERADIATED_FREQUENCIES)
		{
			range = frequencyRanges->GetFrequencyRange(transitionPtr->bandwidthFFTBuffer->range.lower, transitionPtr->bandwidthFFTBuffer->range.upper);

			range->flags[0] = transitionPtr->writes;
		}
		
		transitionPtr = transitionPtr->next;
	}
}

uint32_t Transitions::DURATION = 6000;
double Transitions::RERADIATED_STRENGTH_INC = 999999999;