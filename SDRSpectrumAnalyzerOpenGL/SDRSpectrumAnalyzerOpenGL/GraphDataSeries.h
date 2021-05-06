#pragma once
#include <stdint.h>
#include "fftw3.h"
#include "Vector.h"
#include "MinMax.h"
#include "Color.h"
#include "Triangle.h"
#include "SignalProcessingUtilities.h"

enum GraphStyle { Points3D, Line3D, Area3D, Graph3D };

enum IValueUseForColors { None, Alpha, Index };

class GraphDataSeries
{
	VectorPtr *vertices = NULL;	
	uint32_t bufferSize = BUFFER_START_SIZE;
	Color color;	
	void* parentGraph;
	uint32_t dataWidth;	
	int32_t startZIndex = 0;
	int32_t currentZIndex = 0;
	static const uint32_t colorsCount = 1020;
	Color colors[colorsCount];
	MinMax dataSeriesMinMax;
	
	GraphStyle style = GraphStyle::Area3D;	

	static const uint8_t maxLineColors = 3;
	uint8_t lineColorCount = 0;
	Color lineColors[maxLineColors];

	bool colorSet = false;
	
	GLuint vboID = 999999999;
	GLuint iboID = 999999999;

	Vector* verticesBuffer;
	uint32_t verticesBufferSize = 0;	
	uint32_t verticesBufferCount = 0;

	Color* bufferColors;
	uint32_t colorsBufferSize = 0;
	uint32_t colorsBufferCount = 0;

	Triangle* trianglesBuffer;
	uint32_t trianglesBufferSize = 0;
	uint32_t trianglesBufferCount = 0;

	GLuint* indicesBuffer;
	uint32_t indicesBufferSize = 0;
	uint32_t indicesBufferCount = 0;
	
	public:
		uint32_t maxResolution = 1024;

		static uint32_t GraphDataSeries::BUFFER_START_SIZE;		
		uint32_t verticesCount = 0;
		GLfloat lineWidth = 1;

		bool visible = true;

		GraphDataSeries(void* parentGraph);
		void GenerateSignalStrengthColors();		
		void IncCircularBufferIndices();
		uint32_t SetData(void* data, uint32_t dataLength, bool complex = true, double iOffset = 0, double qOffset = 0, bool swapIQ = false, SignalProcessingUtilities::DataType dataType = SignalProcessingUtilities::DataType::FFTW_COMPLEX, bool insertAtEnd = false);
		double GetAvgValueForIndex(uint8_t index, uint32_t count = 0);
		double GetGradientForIndex(uint8_t index);
		void SetStyle(GraphStyle style);
		void SetColor(float red, float green, float blue, float alpha, uint8_t colorIndex = 255);
		void SetColor(Color color, uint8_t colorIndex = 255);
		void SetDataWidth(uint32_t width);
		uint32_t ShiftPoints();
		uint32_t AddPoint(double x, double y, double z, int32_t index);		
		Vector* GraphDataSeries::GetPoint(int32_t index);
		uint32_t AddColorToBuffer(float r, float g, float b);
		uint32_t AddVertexToBuffer(float x, float y, float z);
		void GenerateVertex(double x, double i, double q, double z, double zScale, double yScale, double viewYMin, bool useMagnitudes = true);
		void GenerateVertex2(double x, double i, double q, double z, double zScale, double yScale, double viewYMin, bool useMagnitudes = true, IValueUseForColors iValueUseForColors = IValueUseForColors::Alpha);
		void AddTriangleToBuffer(GLuint i1, GLuint i2, GLuint i3);
		void Draw(uint32_t start, uint32_t end, double viewYMin=0, double viewYMax=-999999999, double scale = 1, bool graphMagnitude = false, IValueUseForColors iValueUseForColors = IValueUseForColors::Alpha);
		void Draw2D(uint32_t start, uint32_t end, double viewYMin = 0, double viewYMax = -999999999, double scale = 1, bool graphMagnitude = false);
		void Draw2D_2(uint32_t start, uint32_t end, double viewYMin = 0, double viewYMax = -999999999, double scale = 1, bool graphMagnitude = false);
		void Draw3D(uint32_t start, uint32_t end, double viewYMin = 0, double viewYMax = -999999999, double scale = 1, bool graphMagnitude = false, IValueUseForColors iValueUseForColors = IValueUseForColors::Alpha);
		void Draw3D_2(uint32_t start, uint32_t end, double viewYMin = 0, double viewYMax = -999999999, double scale = 1, bool graphMagnitude = false);
		MinMax GetMinMax(uint32_t startDataIndex, uint32_t endIndex, bool useY = true, bool useZ = true);
		MinMax GetMinMaxForMagnitudes(uint32_t startDataIndex = 0, uint32_t endDataIndex = 0);
		~GraphDataSeries();
};

typedef GraphDataSeries* GraphDataSeriesPtr;
