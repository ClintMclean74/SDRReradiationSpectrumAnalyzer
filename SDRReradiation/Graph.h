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
#ifdef _DEBUG
 #define _ITERATOR_DEBUG_LEVEL 2
#endif
#include <stack>
#include "glm/glm/glm.hpp"
#include "glm/glm/gtc/matrix_inverse.hpp"
#include "glm/glm/gtc/type_ptr.hpp"
#include "fftw/fftw3.h"
#include "Vector.h"
#include "MinMax.h"
#include "pstdint.h"
#include "WindowsToLinuxUtilities.h"
#include "GraphDataSeries.h"
#include "freeglut/include/GL/glut.h"

using namespace std;

typedef enum { FrontView, SideView, AboveView } GraphView;


class Graph
{
	uint8_t dataSeriesCount;

	Color axisColor;
	Color selectedAreaColor;

	uint32_t points;

	double dataScale;

	GLfloat axisLineWidth;
	GLfloat selectedStart;
    GLfloat selectedEnd;
	bool paused;

	glm::mat4* matrix;

	double xRot;
	double yRot;
	double zRot;

	uint32_t dataWidth;

	double viewYMin;;
	double viewYMax;;

	double startX;
	double endX;

	double startXAxisLabel;
	double endXAxisLabel;

	uint32_t labelsCount;

	char YaxisLabel[10];

	stack<MinMax> zoomStack;

	DWORD currentTime, prevTime, frameRateCount;
	double frameRate, frameRateTotal;
	char textBufferFrameRate[255];

	uint8_t* prevData;


	public:
		Vector pos;
		double width;
		double height;
		double length;
		#define TEXTCOUNT 10
        static const uint8_t textCount = 10;
		char text[textCount][255];
		char textBuffer[255];

		bool graph_3D;
		bool wireFrame;

		uint8_t showXAxis;;
		uint8_t showYAxis;;
		uint8_t showZAxis;;

		bool showLabels;
		bool showXLabels;

		bool selectable;
		bool visible;

		uint32_t maxDepth;
		uint32_t drawDepth;
		uint32_t userSetDepth;

		uint32_t verticesCount;

		uint32_t startDataIndex;
		uint32_t endDataIndex;

		bool dataSeriesSameScale;

		GraphDataSeriesPtr* dataSeries;

		bool autoScale;

		GraphView view;

		bool automaticPlacement;

		IValueUseForColors iValueUseForColors;

		Graph(uint32_t maxDepth = 1, uint32_t verticesCount = 200, uint8_t dataSeriesCount = 2);
		uint32_t GetPointsCount();
		void CalculateScale();
		void SetSize(double width, double height);
		void SetPos(double x, double y, double z);
		void SetRotation(double xRot, double yRot, double zRot);
		void SetMaxResolution(uint32_t maxResolution);
		void DataSeriesAccruesAndAverages(uint8_t seriesIndex, bool averageData);
		uint32_t SetData(void* data, uint32_t length, uint8_t seriesIndex, bool complex = true, double iOffset = 0, double qOffset = 0, bool swapIQ = false, SignalProcessingUtilities::DataType dataType = SignalProcessingUtilities::FFTW_COMPLEX, bool insertAtEnd = false);
		void SetDepth(uint32_t depth, bool userSet = true);
		double GetGradientForIndex(uint8_t seriesIndex, uint8_t index);
		double GetAvgValueForIndex(uint8_t seriesIndex, uint8_t index, uint32_t count = 0);
		void SetDataSeriesLineWidth(GLfloat width, int8_t seriesIndex = -1);
		void SetDataSeriesStyle(GraphStyle style, int8_t seriesIndex = -1);
		void SetDataSeriesColor(Color color, int8_t seriesIndex = -1, uint8_t colorIndex = 255);
		void SetDataSeriesColor(float red, float green, float blue, float alpha, int8_t seriesIndex = -1, uint8_t colorIndex = 255);
		void SetDataWidth(uint32_t width, uint8_t seriesIndex);
		void ZoomOut();
		glm::vec4 PointOnGraph(float x, float y, float z);
		void SetGraphXRange(uint32_t start, uint32_t end);
		void SetSelectedGraphRange(uint32_t start, uint32_t end);
		void SetGraphViewRangeXAxis(uint32_t start, uint32_t end);
		void SetGraphViewRangeYAxis(uint32_t start, uint32_t end);
		void SetGraphLabelValuesXAxis(double startX, double endX);
		char* SetGraphFrequencyRangeText(const char* rangeText, FrequencyRange* frequencyRange, uint8_t textIndex = 0, bool adjustForSelectedRegion = true);
		void SetText(uint8_t index, const char* format, ...);
		void Draw();
		void DrawTransparencies();
		void TogglePause();

		~Graph();
};

typedef Graph* GraphPtr;
