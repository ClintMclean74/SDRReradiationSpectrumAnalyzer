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
#include "pstdint.h"
#include "fftw/fftw3.h"
#include "Vector.h"
#include "MinMax.h"
#include "Color.h"
#include "Triangle.h"
#include "SignalProcessingUtilities.h"

#include <iostream>
using namespace std;

typedef enum { NotUsing, Alpha, Index } IValueUseForColors;

typedef enum { Points3D, Line3D, Area3D, Graph3D } GraphStyle;

class GraphDataSeries
{
	VectorPtr *vertices;
	uint32_t bufferSize;
	Color color;
	void* parentGraph;
	uint32_t dataWidth;
	int32_t startZIndex;
	int32_t currentZIndex;
	static const uint32_t colorsCount = 1020;
	Color colors[colorsCount];
	MinMax dataSeriesMinMax;

	GraphStyle style;

	static const uint8_t maxLineColors = 3;
	uint8_t lineColorCount;
	Color lineColors[maxLineColors];

	bool colorSet;

	GLuint vboID;
	GLuint iboID;

	Vector* verticesBuffer;
	uint32_t verticesBufferSize;
	uint32_t verticesBufferCount;

	Color* bufferColors;
	uint32_t colorsBufferSize;
	uint32_t colorsBufferCount;

	Triangle* trianglesBuffer;
	uint32_t trianglesBufferSize;
	uint32_t trianglesBufferCount;

	GLuint* indicesBuffer;
	uint32_t indicesBufferSize;
	uint32_t indicesBufferCount;

	public:
		uint32_t maxResolution;
		static uint32_t BUFFER_START_SIZE;
		uint32_t verticesCount;
		GLfloat lineWidth;
		double zOffset;
		uint32_t writes;
		bool averageData;
		bool visible;

		GraphDataSeries(void* parentGraph);
		void GenerateSignalStrengthColors();
		void IncCircularBufferIndices();
		uint32_t SetData(void* data, uint32_t dataLength, bool complex = true, double iOffset = 0, double qOffset = 0, bool swapIQ = false, SignalProcessingUtilities::DataType dataType = SignalProcessingUtilities::FFTW_COMPLEX, bool insertAtEnd = false);
		double GetAvgValueForIndex(uint8_t index, uint32_t count = 0);
		double GetGradientForIndex(uint8_t index);
		void SetStyle(GraphStyle style);
		void SetColor(float red, float green, float blue, float alpha, uint8_t colorIndex = 255);
		void SetColor(Color color, uint8_t colorIndex = 255);
		void SetDataWidth(uint32_t width);
		uint32_t ShiftPoints();
		uint32_t AddPoint(double x, double y, double z, int32_t index);
		Vector* GetPoint(int32_t index);
		uint32_t AddColorToBuffer(float r, float g, float b);
		uint32_t AddVertexToBuffer(float x, float y, float z);
		void GenerateVertex(double x, double i, double q, double z, double zScale, double yScale, double viewYMin, bool useMagnitudes = true);
		void GenerateVertex2(double x, double i, double q, double z, double zScale, double yScale, double viewYMin, bool useMagnitudes = true, IValueUseForColors iValueUseForColors = Alpha);
		void GenerateTriangleVertex(float x, float i, float q, float z, float zScale, float yScale, float viewYMin, bool useMagnitudes = true);
		void AddTriangleToBuffer(GLuint i1, GLuint i2, GLuint i3);
		void Draw(uint32_t start, uint32_t end, double viewYMin=0, double viewYMax=-999999999, double scale = 1, bool graphMagnitude = false, IValueUseForColors iValueUseForColors = Alpha);
		void Draw2D(uint32_t start, uint32_t end, double viewYMin = 0, double viewYMax = -999999999, double scale = 1, bool graphMagnitude = false);
		void Draw2D_2(uint32_t start, uint32_t end, double viewYMin = 0, double viewYMax = -999999999, double scale = 1, bool graphMagnitude = false);
		void Draw2D_Without_Buffers(uint32_t startDataIndex, uint32_t endIndex, double viewYMin, double viewYMax, double scale, bool graphMagnitude);
		void Draw3D(uint32_t start, uint32_t end, double viewYMin = 0, double viewYMax = -999999999, double scale = 1, bool graphMagnitude = false, IValueUseForColors iValueUseForColors = Alpha);
		void Draw3D_2(uint32_t start, uint32_t end, double viewYMin = 0, double viewYMax = -999999999, double scale = 1, bool graphMagnitude = false);
		void Draw3D_Without_Buffers(uint32_t startDataIndex, uint32_t endIndex, double viewYMin, double viewYMax, double scale, bool graphMagnitude, IValueUseForColors iValueUseForColors);
		MinMax GetMinMax(uint32_t startDataIndex, uint32_t endIndex, bool useY = true, bool useZ = true);
		MinMax GetMinMaxForMagnitudes(uint32_t startDataIndex = 0, uint32_t endDataIndex = 0);
		~GraphDataSeries();
};

typedef GraphDataSeries* GraphDataSeriesPtr;
