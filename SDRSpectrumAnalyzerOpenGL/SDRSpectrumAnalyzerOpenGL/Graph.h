#pragma once
#define _ITERATOR_DEBUG_LEVEL 0
#include <stdint.h>
#include <GL/glut.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "fftw3.h"
#include "Vector.h"
#include "MinMax.h"
#include "GraphDataSeries.h"

enum GraphView { Front, Side, Above };

class Graph
{			
	uint8_t dataSeriesCount = 2;
	
	Color axisColor;
	Color selectedAreaColor;

	uint32_t points;

	double dataScale = 1;
	
	GLfloat axisLineWidth;
	GLfloat selectedStart = 0, selectedEnd = 0;
	bool paused = false;
	
	glm::mat4* matrix;

	double xRot = 0;	
	double yRot = 0;
	double zRot = 0;

	uint32_t dataWidth;

	float viewYMin=0;
	float viewYMax=0;

	double startX = 0;
	double endX = 0;	

	double startXAxisLabel = 0;
	double endXAxisLabel = 0;

	uint32_t labelsCount = 6;

	char YaxisLabel[10];	

	////GLfloat mat_transparent[] = { 1.0, 1.0, 1.0, 0.0 };

	public:
		Vector pos;
		double width;
		double height;
		double length;
		////char text[255];
		static const uint8_t textCount = 10;
		char text[textCount][255];
		char textBuffer[255];		

		bool graph_3D = false;
		bool wireFrame = false;

		uint8_t showXAxis = 3;
		uint8_t showYAxis = 3;
		uint8_t showZAxis = 3;

		bool showLabels = true;
		bool selectable = true;
		bool visible = true;

		uint32_t maxDepth = 1;
		uint32_t drawDepth = maxDepth;

		uint32_t verticesCount = 1;

		uint32_t startDataIndex = 0;
		uint32_t endDataIndex = 0;

		bool dataSeriesSameScale = true;

		GraphDataSeriesPtr* dataSeries;

		bool autoScale = true;

		GraphView view = GraphView::Above;

		Graph(uint32_t maxDepth = 1, uint32_t verticesCount = 200, uint8_t dataSeriesCount = 2);
		uint32_t GetPointsCount();
		void CalculateScale();
		void SetSize(double width, double height);
		void SetPos(double x, double y, double z);
		void SetRotation(double xRot, double yRot, double zRot);
		/*////uint32_t SetData(uint8_t* data, uint32_t length, uint8_t seriesIndex, bool complex = true, double iOffset = 0, double qOffset = 0, bool swapIQ = false);
		uint32_t SetData(double* data, uint32_t length, uint8_t seriesIndex, bool complex = true, double iOffset = 0, double qOffset = 0, bool swapIQ = false);
		uint32_t SetData(fftw_complex* data, uint32_t length, uint8_t seriesIndex, double iOffset = 0, double qOffset = 0, bool swapIQ = false);		
		*/
		uint32_t SetData(void* data, uint32_t length, uint8_t seriesIndex, bool complex = true, double iOffset = 0, double qOffset = 0, bool swapIQ = false, SignalProcessingUtilities::DataType dataType = SignalProcessingUtilities::DataType::FFTW_COMPLEX);
		double GetGradientForIndex(uint8_t seriesIndex, uint8_t index);
		double GetAvgValueForIndex(uint8_t seriesIndex, uint8_t index, uint32_t count = 0);
		void SetDataSeriesLineWidth(GLfloat width, int8_t seriesIndex = -1);
		void SetDataSeriesStyle(GraphStyle style, int8_t seriesIndex = -1);
		void SetDataSeriesColor(float red, float green, float blue, float alpha, int8_t seriesIndex = -1);
		void SetDataWidth(uint32_t width, uint8_t seriesIndex);
		void ZoomOut();
		glm::vec4 PointOnGraph(float x, float y, float z);		
		void SetGraphXRange(uint32_t start, uint32_t end);
		void SetSelectedGraphRange(uint32_t start, uint32_t end);
		void SetGraphViewRangeXAxis(uint32_t start, uint32_t end);
		void SetGraphViewRangeYAxis(uint32_t start, uint32_t end);		
		void SetGraphLabelValuesXAxis(double startX, double endX);
		////void SetText(char *string, uint32_t length, uint8_t index);
		void SetText(uint8_t index, const char * format, ...);
		//void DrawText(char *string, float x, float y, float z, float scale = 1, float angle = 0);
		void Draw();
		void DrawTransparencies();
		void TogglePause();

		~Graph();
};


typedef Graph* GraphPtr;
