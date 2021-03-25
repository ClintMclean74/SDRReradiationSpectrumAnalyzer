#pragma once
#include "Graph.h"

class SelectedGraph
{
	public:
		Graph* graph;
		glm::vec4 point;

		SelectedGraph()
		{
			graph = NULL;
		}
};

class Graphs
{
	GraphPtr* graphs = NULL;
	uint32_t graphCount = 0;

	uint32_t bufferSize = 10;	
	double gap = 100;	

	public:
		double x = 0, y = 0, z = 0;
		static const bool DRAWING_GRAPHS = true;
		bool showLabels = true;
		Graphs();
		void SetVisibility(bool visible);
		void ShowLabels(bool visible);
		void ToggleLabels();
		void ResetToUserDrawDepths();
		void SetGap(double gap);
		void SetPos(double x, double y, double z);
		float GetHeight();
		uint32_t AddGraph(Graph* graph);
		void Draw();		
		SelectedGraph* GetSelectedGraph(float x, float y, float z);
		void SetAutoScale(bool autoScale);
		
		~Graphs();
};


