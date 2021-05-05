#include "Graphs.h"

class GUI
{
	private:
		Graphs* graphs = new Graphs();
		Graph *dataGraph, *combinedFFTGraphForBandwidth, *correlationGraph, *fftGraphStrengthsForDeviceRange;
		SelectedGraph selectedGraphStart;
		SelectedGraph selectedGraphEnd;
};