#include "Graphs.h"

class GUI
{
	private:
		Graphs* graphs = new Graphs();
		Graph *dataGraph, *fftGraphForDevicesRange, *correlationGraph, *fftGraphStrengthsForDeviceRange;
		SelectedGraph selectedGraphStart;
		SelectedGraph selectedGraphEnd;

};