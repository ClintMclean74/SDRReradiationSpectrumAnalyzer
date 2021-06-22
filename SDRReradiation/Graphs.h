/*
 * SDRReradiation - Code system to detect biologically reradiated
 * electromagnetic energy from humans
 * Copyright (C) 2021 by Clint Mclean <clint@getfitnowapps.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

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
	GraphPtr* graphs;
	uint32_t graphCount;

	uint32_t bufferSize;// = 20;

	public:
		int graphsGridVertCount;// = 4;

		double x, y , z;
		double xGap;// = 200;
		double yGap;// = 300;
		static const uint8_t MAX_GRAPHS = 10;
		static const bool DRAWING_GRAPHS = true;
		static bool DRAWING_GRAPHS_WITH_BUFFERS;
		static const int16_t  GRAPH_WIDTH = 1000;
		static const int16_t GRAPH_HEIGHT = 900;
		static const int16_t GRAPH_STRENGTHS_X_OFFSET = -100;
		static const int16_t GRAPH_SEGMENT_RESOLUTION = 1024;
		static const float FONT_SCALE = 0.2;


		static Color nearColor;
		static Color farColor;
		static Color undeterminedColor;
		static Color transitionNearFarDifColor;
		static Color reradiatedColor;

		static const uint8_t textCount = 10;
		char text[textCount][255];

		bool showLabels;
		Graphs();
		uint8_t GetPlacedGraphsCount();
		void SetVisibility(bool visible);
		void ShowLabels(bool visible);
		void ToggleLabels();
		void ResetToUserDrawDepths();
		void SetGap(double xGap, double yGap);
		void SetPos(double x, double y, double z);
		void SetText(uint8_t index, const char * format, ...);
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
