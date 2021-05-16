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

	uint32_t bufferSize = 20;		
	
	public:
		int graphsGridVertCount = 4;

		double x = 0, y = 0, z = 0;
		double xGap = 200;
		double yGap = 300;
		static const uint8_t MAX_GRAPHS = 10;
		static const bool DRAWING_GRAPHS = true;
		static const INT16 Graphs::GRAPH_WIDTH = 1000;
		static const INT16 Graphs::GRAPH_HEIGHT = 900;
		static const INT16 Graphs::GRAPH_STRENGTHS_X_OFFSET = -100;
		static const INT16 Graphs::GRAPH_SEGMENT_RESOLUTION = 1024;


		static Color Graphs::nearColor;
		static Color farColor;
		static Color undeterminedColor;
		static Color transitionNearFarDifColor;
		static Color reradiatedColor;

		bool showLabels = true;
		Graphs();
		uint8_t GetPlacedGraphsCount();
		void SetVisibility(bool visible);
		void ShowLabels(bool visible);
		void ToggleLabels();
		void ResetToUserDrawDepths();
		void SetGap(double xGap, double yGap);		
		void SetPos(double x, double y, double z);
		float GetWidth();
		float GetHeight();
		float GetNewGraphX();
		float GetNewGraphY();
		uint32_t AddGraph(Graph* graph);
		void Draw();		
		void DrawTransparencies();
		SelectedGraph* GetSelectedGraph(float x, float y, float z);
		void SetAutoScale(bool autoScale);
		
		~Graphs();
};
