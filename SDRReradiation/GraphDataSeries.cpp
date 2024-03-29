#ifdef _DEBUG
 #define _ITERATOR_DEBUG_LEVEL 2
#endif
#include <math.h>
#include <algorithm>
#include "GL/glew.h"
#include "freeglut/include/GL/glut.h"
#include "ArrayUtilities.h"
#include "DeviceReceiver.h"
#include "Graphs.h"
#include "Graph.h"
#include "GraphDataSeries.h"

GraphDataSeries::GraphDataSeries(void* parentGraph)
{

    vertices = NULL;
	bufferSize = BUFFER_START_SIZE;
	startZIndex = 0;
	currentZIndex = 0;
	style = Area3D;

	lineColorCount = 0;
	colorSet = false;

	vboID = 999999999;
	iboID = 999999999;

	verticesBufferSize = 0;
	verticesBufferCount = 0;

	colorsBufferSize = 0;
	colorsBufferCount = 0;

	trianglesBufferSize = 0;
	trianglesBufferCount = 0;

	indicesBufferSize = 0;
	indicesBufferCount = 0;

	maxResolution = 1024;
	verticesCount = 0;
	lineWidth = 1;
	zOffset = 0;
	writes = 0;
	averageData = false;
	visible = true;

	this->parentGraph = parentGraph;

	vertices = new VectorPtr[((Graph *)parentGraph)->maxDepth];

	for (int i = 0; i < ((Graph *)parentGraph)->maxDepth; i++)
		vertices[i] = new Vector[((Graph *)parentGraph)->verticesCount];

	GenerateSignalStrengthColors();


    if (Graphs::DRAWING_GRAPHS_WITH_BUFFERS)
    {
	verticesBufferSize = ((Graph *)parentGraph)->verticesCount * ((Graph *)parentGraph)->maxDepth;
	colorsBufferSize = verticesBufferSize;

	trianglesBufferSize = ((Graph *)parentGraph)->verticesCount * ((Graph *)parentGraph)->maxDepth * 2;
	indicesBufferSize = verticesBufferSize;
	indicesBufferCount = 0;

	verticesBuffer = new Vector[verticesBufferSize];
	bufferColors = new Color[colorsBufferSize];

	glGenBuffers(1, &vboID);

	size_t vSize = sizeof(Vector) * verticesBufferSize;
	size_t cSize = sizeof(Color) * colorsBufferSize;

	// copy vertex attribs data to VBO
	glBindBuffer(GL_ARRAY_BUFFER, vboID);
	glBufferData(GL_ARRAY_BUFFER, sizeof(Vector) * verticesBufferSize + sizeof(Color) * colorsBufferSize, NULL, GL_DYNAMIC_DRAW); // reserve space
	glBufferSubData(GL_ARRAY_BUFFER, 0, vSize, verticesBuffer);            // copy verts at offset 0
	glBufferSubData(GL_ARRAY_BUFFER, vSize, cSize, bufferColors);          // copy cols

	glBindBuffer(GL_ARRAY_BUFFER, 0);

	delete [] verticesBuffer;
	delete [] bufferColors;
    }
}

void GraphDataSeries::GenerateSignalStrengthColors()
{
	uint8_t stage = 0;

	float r = 255, g = 0, b = 0;

	uint32_t colorIndex = 0;

	while (stage != 4)
	{
		colors[colorIndex].r = r / 255;
		colors[colorIndex].g = g / 255;
		colors[colorIndex].b = b / 255;

		colorIndex++;

		switch (stage)
		{
		case (0):
			g++;

			if (g == 255)
				stage++;
			break;


		case (1):
			r--;

			if (r == 0)
				stage++;
			break;


		case (2):
			b++;

			if (b == 255)
				stage++;
			break;

		case (3):
			g--;

			if (g == 0)
				stage++;
			break;
		}
	}
}

void GraphDataSeries::IncCircularBufferIndices()
{
	currentZIndex++;

	if (currentZIndex >= ((Graph *)parentGraph)->maxDepth)
		currentZIndex = 0;

	if (currentZIndex == startZIndex)
	{
		startZIndex++;

		if (startZIndex >= ((Graph *)parentGraph)->maxDepth)
			startZIndex = 0;
	}
}

uint32_t GraphDataSeries::SetData(void* data, uint32_t dataLength, bool complex, double iOffset, double qOffset, bool swapIQ, SignalProcessingUtilities::DataType dataType, bool insertAtEnd)
{
	double x, y, z;
	double magnitude;

	uint32_t j = 0;

	if (complex && (dataType == SignalProcessingUtilities::UINT8_T || dataType == SignalProcessingUtilities::DOUBLE))
	{
		dataLength = dataLength >> 1;
	}

	double resolution;

	if (insertAtEnd)
		resolution = (double)maxResolution / verticesCount;
	else
		resolution = (double)maxResolution / dataLength;

	double inc = 1 / resolution;

	double max = -999999999;
	double value;

	SignalProcessingUtilities::IQ iq, maxIQ;

	uint32_t endIndex, maxIndex;

	uint32_t newWrites = writes + 1;

	double writes_newWrites_Ratio;
	double newWrite_Ratio;

	if (averageData)
	{
		writes_newWrites_Ratio = (double)writes / newWrites;
		newWrite_Ratio = 1.0 / newWrites;
	}

	double i, dataLengthAdjustedForRoundingErrors = dataLength - 0.0000001;
	for (i = 0; i < dataLengthAdjustedForRoundingErrors; i+=inc)
	{
		maxIQ.I = 0;
		maxIQ.Q = 0;

		endIndex = i + inc;

		for (uint32_t k = i; (k < endIndex && k < dataLength) || (k == (uint32_t) i && inc < 1); k++)
		{
			if (complex)
			{
				switch (dataType)
				{
				case(SignalProcessingUtilities::UINT8_T):
					iq = SignalProcessingUtilities::GetIQFromData((uint8_t *)data, k);
					break;
				case(SignalProcessingUtilities::DOUBLE):
					iq = SignalProcessingUtilities::GetIQFromData((double *)data, k);
					break;
				case(SignalProcessingUtilities::FFTW_COMPLEX):
					iq = SignalProcessingUtilities::GetIQFromData((fftw_complex *)data, k);
					break;
				}
			}
			else
			{
				switch (dataType)
				{
					case(SignalProcessingUtilities::STRENGTHS_ID_TIME):
						iq = SignalProcessingUtilities::GetIQFromData((SignalProcessingUtilities::Strengths_ID_Time *)data, k);
					break;
					default:
						iq.I = ((double *)data)[k];
						iq.Q = ((double *)data)[k];
					break;
				}
			}

			if (swapIQ)
			{
				iq.Swap();
			}

			//for average
			maxIQ.I += iq.I;
			maxIQ.Q += iq.Q;
		}

		if (inc > 1)
		{
			//average
			maxIQ.I /= inc;
			maxIQ.Q /= inc;
		}

		maxIQ.I += iOffset;
		maxIQ.Q += qOffset;

		if (insertAtEnd)
		{
			ShiftPoints();
			AddPoint(j, maxIQ.Q, maxIQ.I, -1);
		}
		else
		{
			if (!averageData)
			{
				vertices[currentZIndex][j].x = j;
				vertices[currentZIndex][j].y = maxIQ.Q;
				vertices[currentZIndex][j].z = maxIQ.I;
			}
			else
			{
				vertices[currentZIndex][j].x = j;
				vertices[currentZIndex][j].y = (vertices[currentZIndex][j].y * writes_newWrites_Ratio) + maxIQ.Q * newWrite_Ratio;
				vertices[currentZIndex][j].z = (vertices[currentZIndex][j].z * writes_newWrites_Ratio) + maxIQ.I * newWrite_Ratio;
			}

			if (j >= verticesCount)
				verticesCount++;
		}

		j++;
	}

	IncCircularBufferIndices();

	writes++;

	return dataLength;
}

double GraphDataSeries::GetAvgValueForIndex(uint8_t index, uint32_t count)
{
	if (count == 0 || count > ((Graph *)parentGraph)->maxDepth)
		count = ((Graph *)parentGraph)->maxDepth;

	int32_t zIndex = currentZIndex;

	double total = 0, value;

	uint32_t zIndexCount = 0;
	do
	{
		zIndex--;

		if (zIndex < 0)
			zIndex = ((Graph *)parentGraph)->maxDepth - 1;

		value = vertices[zIndex][index].y;

		total += value;

		zIndexCount++;
	} while (zIndex != startZIndex && zIndexCount < count);

	value = total / zIndexCount;

	return value;
}

double GraphDataSeries::GetGradientForIndex(uint8_t index)
{
	int32_t zIndex = currentZIndex;
	int32_t zIndex2 = zIndex - 1;

	if (zIndex2 < 0)
		zIndex2 = ((Graph *)parentGraph)->maxDepth - 1;

	double totalGradient = 0, gradient;

	uint16_t gradientCount = 100;

	uint32_t zIndexCount = 0;
	do
	{
		zIndex--;
		zIndex2--;

		if (zIndex < 0)
			zIndex = ((Graph *)parentGraph)->maxDepth - 1;

		if (zIndex2 < 0)
			zIndex2 = ((Graph *)parentGraph)->maxDepth - 1;

		if (zIndexCount > 0)
		{
			gradient = vertices[zIndex][index].y - vertices[zIndex2][index].y;

			totalGradient += gradient;
		}

		zIndexCount++;

		if (zIndexCount >= gradientCount)
			break;

	} while (zIndex2 != startZIndex);

	gradient = totalGradient / zIndexCount;

	return gradient;
}

void GraphDataSeries::SetStyle(GraphStyle style)
{
	this->style = style;
}

void GraphDataSeries::SetColor(Color color, uint8_t colorIndex)
{
	SetColor(color.r, color.g, color.b, color.a, colorIndex);
}

void GraphDataSeries::SetColor(float red, float green, float blue, float alpha, uint8_t colorIndex)
{
	colorSet = true;

	if (colorIndex == 255)
	{
		lineColors[0].r = red;
		lineColors[0].g = green;
		lineColors[0].b = blue;
	}
	else
	{
		lineColors[colorIndex].r = red;
		lineColors[colorIndex].g = green;
		lineColors[colorIndex].b = blue;
	}

	lineColorCount++;
}

void GraphDataSeries::SetDataWidth(uint32_t width)
{
	dataWidth = width;
}

uint32_t GraphDataSeries::ShiftPoints()
{
	memcpy(vertices[currentZIndex], &vertices[currentZIndex][1], verticesCount * sizeof(Vector));

	verticesCount--;

	return verticesCount;
}

uint32_t GraphDataSeries::AddPoint(double x, double y, double z, int32_t index)
{
	if (index == -1)
		index = verticesCount;

	vertices[currentZIndex][index].x = x;
	vertices[currentZIndex][index].y = y;
	vertices[currentZIndex][index].z = z;

	if ((index + 1) >= verticesCount)
	{
		verticesCount = index + 1;
	}

	return verticesCount;
}

Vector* GraphDataSeries::GetPoint(int32_t index)
{
	int32_t zIndexForPoint = currentZIndex - 1;

	if (zIndexForPoint < 0)
		zIndexForPoint = ((Graph *)parentGraph)->maxDepth - 1;

	return &vertices[zIndexForPoint][index];
}

uint32_t GraphDataSeries::AddColorToBuffer(float r, float g, float b)
{
	if (colorsBufferCount >= colorsBufferSize)
		return 0;

	bufferColors[colorsBufferCount].r = r;
	bufferColors[colorsBufferCount].g = g;
	bufferColors[colorsBufferCount].b = b;

	return colorsBufferCount++;
}

uint32_t GraphDataSeries::AddVertexToBuffer(float x, float y, float z)
{
	if (verticesBufferCount >= verticesBufferSize)
		return 0;

	verticesBuffer[verticesBufferCount].x = x;
	verticesBuffer[verticesBufferCount].y = y;
	verticesBuffer[verticesBufferCount].z = z;

	return verticesBufferCount++;
}

void GraphDataSeries::AddTriangleToBuffer(GLuint i1, GLuint i2, GLuint i3)
{
	if (trianglesBufferCount >= indicesBufferSize)
		return;

	trianglesBuffer[trianglesBufferCount].i1 = i1;
	trianglesBuffer[trianglesBufferCount].i2 = i2;
	trianglesBuffer[trianglesBufferCount].i3 = i3;

	trianglesBufferCount++;
}

void GraphDataSeries::GenerateVertex(double x, double i, double q, double z, double zScale, double yScale, double viewYMin, bool useMagnitudes)
{
	double value;

	if (useMagnitudes)
		value = sqrt(q*q + i*i);
	else
		value = q;

	if (colorSet)
	{
		float normalizedRange = (value - dataSeriesMinMax.min) / dataSeriesMinMax.range;

		if (normalizedRange > 1)
			normalizedRange = 1;

		if (normalizedRange < 0)
			normalizedRange = 0;

		float  value = normalizedRange+0.5;

		float red, green, blue;

		red = green = blue = 0;

		uint8_t colorIndex;

		if (x < verticesCount / 2 || lineColorCount < 2)
			colorIndex = 0;
		else
			colorIndex = 1;

		Color color = lineColors[colorIndex];

		if (color.r == 1)
			red = value;

		if (color.g == 1)
			green = value;

		if (color.b == 1)
			blue = value;

		AddColorToBuffer(red, green, blue);
	}
	else
	{
		float normalizedRange = (value - dataSeriesMinMax.min) / dataSeriesMinMax.range;

		if (normalizedRange > 1)
			normalizedRange = 1;

		if (normalizedRange < 0)
			normalizedRange = 0;

		int colorIndex = (int)((1 - normalizedRange) * (colorsCount - 1));

		AddColorToBuffer(colors[colorIndex].r, colors[colorIndex].g, colors[colorIndex].b);
	}

	AddVertexToBuffer(x, (value - viewYMin) * yScale, z * zScale);
}

void GraphDataSeries::GenerateVertex2(double x, double i, double q, double z, double zScale, double yScale, double viewYMin, bool useMagnitudes, IValueUseForColors iValueUseForColors)
{
	double value;

	if (useMagnitudes)
		value = sqrt(q*q + i*i);
	else
		value = q;

	if (value >= dataSeriesMinMax.min && value <= dataSeriesMinMax.max)
	{
		if (colorSet)
		{
			uint8_t colorIndex = 0;

			if (iValueUseForColors == Index)
			{
				colorIndex = i;

				if (colorIndex > 2)
					colorIndex = 0;
			}

			glColor4f(lineColors[colorIndex].r, lineColors[colorIndex].g, lineColors[colorIndex].b, (iValueUseForColors == Alpha ? i : 1.0));
		}
		else
		{
			float normalizedRange = (value - dataSeriesMinMax.min) / dataSeriesMinMax.range;

			if (normalizedRange > 1)
				normalizedRange = 1;

			if (normalizedRange < 0)
				normalizedRange = 0;

			int colorIndex = (int)((1 - normalizedRange) * (colorsCount - 1));

			glColor4f(colors[colorIndex].r, colors[colorIndex].g, colors[colorIndex].b, (iValueUseForColors == Alpha ? i : 1.0));
		}

		glVertex3f(x, (value - viewYMin) * yScale, z * zScale);
	}
}

void GraphDataSeries::GenerateTriangleVertex(float x, float i, float q, float z, float zScale, float yScale, float viewYMin, bool useMagnitudes)
{
    double value;

	if (useMagnitudes)
		value = sqrt(q*q + i*i);
	else
		value = q;

	float normalizedRange = (value - dataSeriesMinMax.min) / dataSeriesMinMax.range;

	if (normalizedRange > 1)
		normalizedRange = 1;

	if (normalizedRange < 0)
		normalizedRange = 0;

	int colorIndex = (int)((1 - normalizedRange) * (colorsCount - 1));

	glColor3f(colors[colorIndex].r, colors[colorIndex].g, colors[colorIndex].b);

	glVertex3f(x, (value - viewYMin) * yScale, z * zScale);
}

void GraphDataSeries::Draw3D_Without_Buffers(uint32_t startDataIndex, uint32_t endIndex, double viewYMin, double viewYMax, double scale, bool graphMagnitude, IValueUseForColors iValueUseForColors)
{
	if (endIndex == 0 || endIndex > verticesCount)
		endIndex = verticesCount - 1;

	if (endIndex < startDataIndex)
		startDataIndex = endIndex - 1;

	if (startDataIndex == endIndex)
		endIndex++;

	GLfloat zIndexCount = 0, zIndexCount2 = 1;

	uint32_t dataLength = (endIndex - startDataIndex) + 1;

	if (dataLength >= 1)
	{
		double yValue, zValue, magnitude;

		double normalizedRange = 0;

		glColor3f(color.r, color.g, color.b);

		uint32_t startPosX = 0;
		uint32_t endPosX = ((Graph *)parentGraph)->width;

		double xInc = ((Graph *)parentGraph)->width / (dataLength - 1);

		float x;
		float x2;

		if (((Graph*)parentGraph)->wireFrame)
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		else
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

		if (scale == 0)
		{
			if (!graphMagnitude)
				dataSeriesMinMax = GetMinMax(0, 0, true, false);
			else
				dataSeriesMinMax = GetMinMaxForMagnitudes(startDataIndex, endIndex);

			if (dataSeriesMinMax.range == 0)
				return;

			if (viewYMin == 0)
				viewYMin = dataSeriesMinMax.min;

			if (viewYMax == 0)
				viewYMax = dataSeriesMinMax.max;

			if (viewYMin == 999999999 || viewYMax == 999999999)
			{
				return;
			}

			if (viewYMax != viewYMin)
				scale = ((Graph *)parentGraph)->height / (viewYMax - viewYMin);
			else
				scale = 1;
		}
		else
		{
			dataSeriesMinMax.min = viewYMin;
			dataSeriesMinMax.max = viewYMax;

			dataSeriesMinMax.CalculateRange();

			if (dataSeriesMinMax.range == 0)
				return;
		}

		double zScale = 10;

		if (currentZIndex == startZIndex && ((Graph *)parentGraph)->maxDepth > 1 && style != Line3D)
			return;

		int32_t zIndex = currentZIndex;
		int32_t zIndex2 = zIndex - 1;

		if (zIndex2 < 0)
			zIndex2 = ((Graph *)parentGraph)->maxDepth - 1;

		if (style == Line3D || ((Graph *)parentGraph)->drawDepth <= 3)
		{
			endIndex--;
			glLineWidth(lineWidth);
		}

		double iOffset = 0;

		do
		{
			zIndex--;
			zIndex2--;

			if (zIndex < 0)
				zIndex = ((Graph *)parentGraph)->maxDepth - 1;

			if (zIndex2 < 0)
				zIndex2 = ((Graph *)parentGraph)->maxDepth - 1;

			if (zIndex == startZIndex && ((Graph *)parentGraph)->maxDepth > 1 && style != Line3D)
				break;

			x = startPosX;
			x2 = x + xInc;

			if (style == Points3D || style == Line3D || ((Graph *)parentGraph)->drawDepth <= 3)
			{
				glPointSize(10);

				if (style == Points3D)
					glBegin(GL_POINTS);
				else
					glBegin(GL_LINES);

				for (int i = startDataIndex; i <= endIndex; i++)
				{
                    GenerateVertex2(x, vertices[zIndex][i].z, vertices[zIndex][i].y, -zIndexCount + zOffset, zScale, scale, viewYMin, graphMagnitude, iValueUseForColors);
					GenerateVertex2(x + xInc, vertices[zIndex][i + 1].z, vertices[zIndex][i + 1].y, -zIndexCount + zOffset, zScale, scale, viewYMin, graphMagnitude, iValueUseForColors);

					x += xInc;
				}

				glEnd();

				glPointSize(1);
			}
			else
			if (style == Graph3D)
			{
				for (int i = startDataIndex; i < endIndex; i++)
				{
					glBegin(GL_TRIANGLES);
						GenerateVertex2(x, vertices[zIndex][i].z, vertices[zIndex][i].y, -zIndexCount, zScale, scale, viewYMin, graphMagnitude, iValueUseForColors);
                        GenerateVertex2(x2, vertices[zIndex][i + 1].z, vertices[zIndex][i + 1].y, -zIndexCount, zScale, scale, viewYMin, graphMagnitude, iValueUseForColors);
                        GenerateVertex2(x2, vertices[zIndex2][i + 1].z, vertices[zIndex2][i + 1].y, -zIndexCount2, zScale, scale, viewYMin, graphMagnitude, iValueUseForColors);

                        GenerateVertex2(x2, vertices[zIndex2][i + 1].z, vertices[zIndex2][i + 1].y, -zIndexCount2, zScale, scale, viewYMin, graphMagnitude, iValueUseForColors);
                        GenerateVertex2(x, vertices[zIndex2][i].z, vertices[zIndex2][i].y, -zIndexCount2, zScale, scale, viewYMin, graphMagnitude, iValueUseForColors);
                        GenerateVertex2(x, vertices[zIndex][i].z, vertices[zIndex][i].y, -zIndexCount, zScale, scale, viewYMin, graphMagnitude, iValueUseForColors);
					glEnd();

					x += xInc;
					x2 = x + xInc;
				}
			}

			zIndexCount++;
			zIndexCount2 = zIndexCount + 1;
		} while (zIndex2 != startZIndex && zIndexCount < ((Graph *)parentGraph)->drawDepth);
	}
}

void GraphDataSeries::Draw(uint32_t startDataIndex, uint32_t endIndex, double viewYMin, double viewYMax, double scale, bool graphMagnitude, IValueUseForColors iValueUseForColors)
{
	if (!visible)
		return;

	if (style == Points3D || style == Graph3D)
	{
	    if (Graphs::DRAWING_GRAPHS_WITH_BUFFERS)
            Draw3D(startDataIndex, endIndex, viewYMin, viewYMax, scale, graphMagnitude, iValueUseForColors);
        else
            Draw3D_Without_Buffers(startDataIndex, endIndex, viewYMin, viewYMax, scale, graphMagnitude, iValueUseForColors);
	}
	else
	{
	    if (Graphs::DRAWING_GRAPHS_WITH_BUFFERS)
            Draw2D(startDataIndex, endIndex, viewYMin, viewYMax, scale, graphMagnitude);
        else
        {
            Draw2D_Without_Buffers(startDataIndex, endIndex, viewYMin, viewYMax, scale, graphMagnitude);
        }
	}
}

void GraphDataSeries::Draw2D_Without_Buffers(uint32_t startDataIndex, uint32_t endIndex, double viewYMin, double viewYMax, double scale, bool graphMagnitude)
{
	if (endIndex == 0 || endIndex > verticesCount)
		endIndex = verticesCount - 1;

	if (startDataIndex == endIndex)
		endIndex++;

	GLfloat zIndexCount = 0;

	if (startDataIndex < endIndex && endIndex - startDataIndex >= 2)
	{
		double yValue, zValue, magnitude;

		if (colorSet)
		{
			glColor3f(lineColors[0].r, lineColors[0].g, lineColors[0].b);
		}

		glLineWidth(2);

		uint32_t endPosX = ((Graph *)parentGraph)->width;
		uint32_t dataLength = (endIndex - startDataIndex) + 1;

		double xInc = ((Graph *)parentGraph)->width / (dataLength - 1);
		float x = 0;

            glBegin(GL_LINE_STRIP);
			for (int i = startDataIndex; i <= endIndex; i++)
			{
				yValue = (vertices[0][i].y - viewYMin) * scale;
				zValue = vertices[0][i].z * scale;

				glVertex3f(x, yValue, zValue);

				x += xInc;
			}
			glEnd();
	}
}

void GraphDataSeries::Draw2D(uint32_t startDataIndex, uint32_t endIndex, double viewYMin, double viewYMax, double scale, bool graphMagnitude)
{
	if (iboID == 999999999)
	{
		glGenBuffers(1, &iboID);

		// copy index data to VBO
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, iboID);

		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * indicesBufferSize, 0, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	}

	if (endIndex == 0 || endIndex > verticesCount)
		endIndex = verticesCount - 1;

	if (startDataIndex == endIndex)
		endIndex++;

	GLfloat zIndexCount = 0;

	if (startDataIndex < endIndex && endIndex - startDataIndex >= 2)
	{
		double yValue, zValue, magnitude;

		if (colorSet)
		{
			glColor3f(lineColors[0].r, lineColors[0].g, lineColors[0].b);
		}

		glLineWidth(2);

		uint32_t endPosX = ((Graph *)parentGraph)->width;
		uint32_t dataLength = (endIndex - startDataIndex) + 1;

		double xInc = ((Graph *)parentGraph)->width / (dataLength - 1);
		float x = 0;

		verticesBufferCount = 0;
		indicesBufferCount = 0;

			glBindBuffer(GL_ARRAY_BUFFER, vboID);
			verticesBuffer = (Vector *)glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);

			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, iboID);
			indicesBuffer = (GLuint *)glMapBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_READ_WRITE);

			for (int i = startDataIndex; i <= endIndex; i++)
			{
				/*if (((Graph *)parentGraph)->graphData == GraphData::EnergyLevel)
				{

                    yValue = vertices[0][i].y;
                    zValue = vertices[0][i].z;

                    yValue = (vertices[0][i].y - viewYMin) * scale;
                    zValue = vertices[0][i].z * scale;
				}
				else*/
				{
                    yValue = (vertices[0][i].y - viewYMin) * scale;
                    zValue = vertices[0][i].z * scale;

                    //zValue = 0;
				}

				if (AddVertexToBuffer(x, yValue, zValue))
				{
					indicesBuffer[indicesBufferCount] = indicesBufferCount;

					indicesBufferCount++;
				}

				x += xInc;
			}

			glUnmapBuffer(GL_ARRAY_BUFFER);
			glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);


			if (verticesBufferCount > 0)
			{
				glBindBuffer(GL_ARRAY_BUFFER, vboID);
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, iboID);

				// enable vertex arrays
				glEnableClientState(GL_VERTEX_ARRAY);

				// before draw, specify vertex and index arrays with their offsets
				glVertexPointer(3, GL_DOUBLE, 0, 0);

				glDrawElements(GL_LINE_STRIP,            // primitive type
					indicesBufferCount,                  // # of indices
					GL_UNSIGNED_INT,         // data type
					(void*)0);               // ptr to indices

				glDisableClientState(GL_VERTEX_ARRAY);  // disable vertex arrays

				// it is good idea to release VBOs with ID 0 after use.
				// Once bound with 0, all pointers in gl*Pointer() behave as real
				// pointer, so, normal vertex array operations are re-activated
				glBindBuffer(GL_ARRAY_BUFFER, 0);
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
			}
	}
}

void GraphDataSeries::Draw2D_2(uint32_t startDataIndex, uint32_t endIndex, double viewYMin, double viewYMax, double scale, bool graphMagnitude)
{
	if (endIndex == 0 || endIndex > verticesCount)
		endIndex = verticesCount - 1;

	if (startDataIndex == endIndex)
		endIndex++;

	uint32_t length = endIndex - startDataIndex;

	GLfloat zIndexCount = 0;

	if (startDataIndex < endIndex && endIndex - startDataIndex >= 2)
	{
		double yValue, zValue, magnitude;

		if (colorSet)
			glColor3f(color.r, color.g, color.b);

		glLineWidth(2);

		uint32_t endPosX = ((Graph *)parentGraph)->width;
		uint32_t dataLength = endIndex - startDataIndex;

		double xInc = ((Graph *)parentGraph)->width / dataLength;
		float x = 0;

		dataSeriesMinMax = GetMinMax(startDataIndex, endIndex);

		if (viewYMax == 0)
			viewYMax = dataSeriesMinMax.max;

		scale = ((Graph *)parentGraph)->height / (viewYMax - viewYMin);

		glBegin(GL_LINE_STRIP);

		for (int i = startDataIndex; i < endIndex; i++)
		{
			yValue = (vertices[0][i].y - viewYMin) * scale;
			zValue = vertices[0][i].z * scale;

			glVertex3f(x, yValue, zValue);

			x += xInc;
		}

		glEnd();
	}
}


void GraphDataSeries::Draw3D(uint32_t startDataIndex, uint32_t endIndex, double viewYMin, double viewYMax, double scale, bool graphMagnitude, IValueUseForColors iValueUseForColors)
{
	if (iboID == 999999999)
	{
		glGenBuffers(1, &iboID);

		// copy index data to VBO
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, iboID);

		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(Triangle) * trianglesBufferSize, NULL, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	}

	if (endIndex == 0 || endIndex > verticesCount)
		endIndex = verticesCount - 1;

	if (endIndex < startDataIndex)
		startDataIndex = endIndex - 1;

	if (startDataIndex == endIndex)
		endIndex++;

	GLfloat zIndexCount = 0, zIndexCount2 = 1;

	uint32_t dataLength = (endIndex - startDataIndex) + 1;

	if (dataLength >= 1)
	{
		double yValue, zValue, magnitude;

		double normalizedRange = 0;

		glColor3f(color.r, color.g, color.b);

		uint32_t startPosX = 0;
		uint32_t endPosX = ((Graph *)parentGraph)->width;

		double xInc = ((Graph *)parentGraph)->width / (dataLength - 1);

		float x;
		float x2;

		if (((Graph*)parentGraph)->wireFrame)
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		else
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

		if (scale == 0)
		{
			if (!graphMagnitude)
				dataSeriesMinMax = GetMinMax(0, 0, true, false);
			else
				dataSeriesMinMax = GetMinMaxForMagnitudes(startDataIndex, endIndex);

			if (dataSeriesMinMax.range == 0)
				return;

			if (viewYMin == 0)
				viewYMin = dataSeriesMinMax.min;

			if (viewYMax == 0)
				viewYMax = dataSeriesMinMax.max;

			if (viewYMin == 999999999 || viewYMax == 999999999)
			{
				return;
			}

			if (viewYMax != viewYMin)
				scale = ((Graph *)parentGraph)->height / (viewYMax - viewYMin);
			else
				scale = 1;
		}
		else
		{
			dataSeriesMinMax.min = viewYMin;
			dataSeriesMinMax.max = viewYMax;

			dataSeriesMinMax.CalculateRange();

			if (dataSeriesMinMax.range == 0)
				return;
		}

		double zScale = 10;

		if (currentZIndex == startZIndex && ((Graph *)parentGraph)->maxDepth > 1 && style != Line3D)
			return;

		int32_t zIndex = currentZIndex;
		int32_t zIndex2 = zIndex - 1;

		if (zIndex2 < 0)
			zIndex2 = ((Graph *)parentGraph)->maxDepth - 1;

		if (!(style == Line3D || ((Graph *)parentGraph)->drawDepth <= 3))
		{
			verticesBufferCount = 0;
			colorsBufferCount = 0;

			trianglesBufferCount = 0;

			glBindBuffer(GL_ARRAY_BUFFER, vboID);
			verticesBuffer = (Vector *)glMapBuffer(GL_ARRAY_BUFFER, GL_READ_WRITE);
			bufferColors = (Color *)&verticesBuffer[verticesBufferSize];

			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, iboID);
			trianglesBuffer = (Triangle *)glMapBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_READ_WRITE);
		}
		else
		{
			endIndex--;
			glLineWidth(lineWidth);
		}

		double iOffset = 0;

		do
		{
			zIndex--;
			zIndex2--;

			if (zIndex < 0)
				zIndex = ((Graph *)parentGraph)->maxDepth - 1;

			if (zIndex2 < 0)
				zIndex2 = ((Graph *)parentGraph)->maxDepth - 1;

			if (zIndex == startZIndex && ((Graph *)parentGraph)->maxDepth > 1 && style != Line3D)
				break;

			x = startPosX;

			if (style == Points3D || style == Line3D || ((Graph *)parentGraph)->drawDepth <= 3)
			{
				glPointSize(10);

				if (style == Points3D)
					glBegin(GL_POINTS);
				else
					glBegin(GL_LINES);

				for (int i = startDataIndex; i <= endIndex; i++)
				{
                    GenerateVertex2(x, vertices[zIndex][i].z, vertices[zIndex][i].y, -zIndexCount + zOffset, zScale, scale, viewYMin, graphMagnitude, iValueUseForColors);
                    GenerateVertex2(x + xInc, vertices[zIndex][i + 1].z, vertices[zIndex][i + 1].y, -zIndexCount + zOffset, zScale, scale, viewYMin, graphMagnitude, iValueUseForColors);

					x += xInc;
				}

				glEnd();

				glPointSize(1);
			}
			else
			if (style == Graph3D)
			{
				for (int i = startDataIndex; i <= endIndex; i++)
				{
						GenerateVertex(x, vertices[zIndex][i].z, vertices[zIndex][i].y, -zIndexCount, zScale, scale, viewYMin, graphMagnitude);

						if (i > startDataIndex && zIndexCount > 0)
						{
							iOffset = i - startDataIndex;

							AddTriangleToBuffer((zIndexCount - 1) * dataLength + iOffset, zIndexCount * dataLength + iOffset, (zIndexCount * dataLength) + iOffset - 1);
							AddTriangleToBuffer((zIndexCount * dataLength) + iOffset - 1, (zIndexCount - 1) * dataLength + iOffset - 1, (zIndexCount - 1) * dataLength + iOffset);
						}

					x += xInc;
				}
			}

			zIndexCount++;
		} while (zIndex2 != startZIndex && zIndexCount < ((Graph *)parentGraph)->drawDepth);


		if (!(style == Line3D || ((Graph *)parentGraph)->drawDepth <= 3))
		{
			glUnmapBuffer(GL_ARRAY_BUFFER);
			glBindBuffer(GL_ARRAY_BUFFER, 0);

			glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

			if (trianglesBufferCount > 0)
			{
				size_t vSize = sizeof(Vector) * verticesBufferSize;
				size_t cSize = sizeof(Color) * colorsBufferSize;

				// enable vertex arrays
				glEnableClientState(GL_VERTEX_ARRAY);
				glEnableClientState(GL_COLOR_ARRAY);

				glBindBuffer(GL_ARRAY_BUFFER, vboID);
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, iboID);

				// before draw, specify vertex and index arrays with their offsets
				glVertexPointer(3, GL_DOUBLE, 0, 0);
				glColorPointer(4, GL_FLOAT, 0, (void*)vSize);

				glDrawElements(GL_TRIANGLES,
					trianglesBufferCount * 3,
					GL_UNSIGNED_INT,
					(void*)0);

				glDisableClientState(GL_VERTEX_ARRAY);  // disable vertex arrays
				glDisableClientState(GL_COLOR_ARRAY);

				// it is good idea to release VBOs with ID 0 after use.
				// Once bound with 0, all pointers in gl*Pointer() behave as real
				// pointer, so, normal vertex array operations are re-activated
				glBindBuffer(GL_ARRAY_BUFFER, 0);
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
			}
		}
	}
}

void GraphDataSeries::Draw3D_2(uint32_t startDataIndex, uint32_t endIndex, double viewYMin, double viewYMax, double scale, bool graphMagnitude)
{
	if (endIndex == 0 || endIndex > verticesCount)
		endIndex = verticesCount - 1;

	if (startDataIndex == endIndex)
		endIndex++;

	uint32_t length = endIndex - startDataIndex;
	GLfloat zIndexCount = 0, zIndexCount2 = 1;

	if (endIndex - startDataIndex >= 2)
	{
		double yValue, zValue, magnitude;

		double normalizedRange = 0;

		glColor3f(color.r, color.g, color.b);
		glLineWidth(2);

		uint32_t startPosX = 0;
		uint32_t endPosX = ((Graph *)parentGraph)->width;
		uint32_t dataLength = endIndex - startDataIndex;

		double xInc = ((Graph *)parentGraph)->width / dataLength;
		float x;
		float x2;

		if (((Graph*)parentGraph)->wireFrame)
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		else
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

		if (scale == 1)
		{
			if (!graphMagnitude)
				dataSeriesMinMax = GetMinMax(0, 0, true, false);
			else
				dataSeriesMinMax = GetMinMaxForMagnitudes(startDataIndex, endIndex);

			if (viewYMin == 0)
				viewYMin = dataSeriesMinMax.min;

			if (viewYMax == 0)
				viewYMax = dataSeriesMinMax.max;

			if (viewYMax != viewYMin)
				scale = ((Graph *)parentGraph)->height / (viewYMax - viewYMin);
			else
				scale = 1;

			double zScale = 10;

			if (currentZIndex == startZIndex)
				return;

			int32_t zIndex = currentZIndex;
			int32_t zIndex2 = zIndex - 1;

			if (zIndex2 < 0)
				zIndex2 = ((Graph *)parentGraph)->maxDepth - 1;

			do
			{
				zIndex--;
				zIndex2--;

				if (zIndex < 0)
					zIndex = ((Graph *)parentGraph)->maxDepth - 1;

				if (zIndex2 < 0)
					zIndex2 = ((Graph *)parentGraph)->maxDepth - 1;

				if (zIndex == startZIndex)
					break;

				x = startPosX;
				x2 = x + xInc;

				if (style == Line3D)
				{
					glLineWidth(lineWidth);

					for (int i = startDataIndex; i < endIndex - 1; i++)
					{
						glBegin(GL_LINES);

						GenerateVertex2(x, vertices[zIndex][i].z, vertices[zIndex][i].y, -zIndexCount, zScale, scale, viewYMin, graphMagnitude);
						GenerateVertex2(x, vertices[zIndex2][i].z, vertices[zIndex2][i].y, -zIndexCount2, zScale, scale, viewYMin, graphMagnitude);

						glEnd();

						x += xInc;
					}
				}
				else
					if (style == Area3D)
					{
						glDisable(GL_CULL_FACE);
						for (int i = startDataIndex; i < endIndex - 1; i++)
						{
							glBegin(GL_TRIANGLES);

							GenerateVertex2(x, vertices[zIndex][i].z, vertices[zIndex][i].y, -zIndexCount, zScale, scale, viewYMin, graphMagnitude);
							GenerateVertex2(x, vertices[zIndex2][i].z, vertices[zIndex2][i].y, -zIndexCount2, zScale, scale, viewYMin, graphMagnitude);
							glVertex3f(x, 0, -zIndexCount2 * zScale);

							glVertex3f(x, 0, -zIndexCount2 * zScale);
							glVertex3f(x, 0, -zIndexCount * zScale);
							GenerateVertex2(x, vertices[zIndex][i].z, vertices[zIndex][i].y, -zIndexCount, zScale, scale, viewYMin, graphMagnitude);

							glEnd();

							x += xInc;
						}
						glEnable(GL_CULL_FACE);
					}
					else if (style == Graph3D)
					{
						glBegin(GL_TRIANGLES);


						if (zIndexCount == 0)
						{
							for (int i = startDataIndex; i < endIndex - 1; i++)
							{
								GenerateVertex2(x2, vertices[zIndex][i + 1].z, vertices[zIndex][i + 1].y, -zIndexCount, zScale, scale, viewYMin, graphMagnitude);
								GenerateVertex2(x, vertices[zIndex][i].z, vertices[zIndex][i].y, -zIndexCount, zScale, scale, viewYMin, graphMagnitude);
								glVertex3f(x, 0, 0);

								glVertex3f(x, 0, 0);
								glVertex3f(x2, 0, 0);
								GenerateVertex2(x2, vertices[zIndex][i + 1].z, vertices[zIndex][i + 1].y, -zIndexCount, zScale, scale, viewYMin, graphMagnitude);

								x += xInc;
								x2 = x + xInc;
							}

							x = startPosX;
							x2 = x + xInc;
						}

						GenerateVertex2(x, vertices[zIndex][0].z, vertices[zIndex][0].y, -zIndexCount, zScale, scale, viewYMin, graphMagnitude);
						GenerateVertex2(x, vertices[zIndex2][0].z, vertices[zIndex2][0].y, -zIndexCount2, zScale, scale, viewYMin, graphMagnitude);
						glVertex3f(x, 0, -zIndexCount2 * zScale);

						glVertex3f(x, 0, -zIndexCount2 * zScale);
						glVertex3f(x, 0, -zIndexCount * zScale);
						GenerateVertex2(x, vertices[zIndex][0].z, vertices[zIndex][0].y, -zIndexCount, zScale, scale, viewYMin, graphMagnitude);

						int i;

						for (i = startDataIndex; i < endIndex - 1; i++)
						{
							GenerateVertex2(x, vertices[zIndex][i].z, vertices[zIndex][i].y, -zIndexCount, zScale, scale, viewYMin, graphMagnitude);
							GenerateVertex2(x2, vertices[zIndex][i + 1].z, vertices[zIndex][i + 1].y, -zIndexCount, zScale, scale, viewYMin, graphMagnitude);
							GenerateVertex2(x2, vertices[zIndex2][i + 1].z, vertices[zIndex2][i + 1].y, -zIndexCount2, zScale, scale, viewYMin, graphMagnitude);

							GenerateVertex2(x2, vertices[zIndex2][i + 1].z, vertices[zIndex2][i + 1].y, -zIndexCount2, zScale, scale, viewYMin, graphMagnitude);
							GenerateVertex2(x, vertices[zIndex2][i].z, vertices[zIndex2][i].y, -zIndexCount2, zScale, scale, viewYMin, graphMagnitude);
							GenerateVertex2(x, vertices[zIndex][i].z, vertices[zIndex][i].y, -zIndexCount, zScale, scale, viewYMin, graphMagnitude);

							x += xInc;
							x2 = x + xInc;
						}

						GenerateVertex2(x, vertices[zIndex][i].z, vertices[zIndex][i].y, -zIndexCount, zScale, scale, viewYMin, graphMagnitude);
						glVertex3f(x, 0, -zIndexCount * zScale);
						glVertex3f(x, 0, -zIndexCount2 * zScale);

						glVertex3f(x, 0, -zIndexCount2 * zScale);
						GenerateVertex2(x, vertices[zIndex2][i].z, vertices[zIndex2][i].y, -zIndexCount2, zScale, scale, viewYMin, graphMagnitude);
						GenerateVertex2(x, vertices[zIndex][i].z, vertices[zIndex][i].y, -zIndexCount, zScale, scale, viewYMin, graphMagnitude);

						glEnd();
					}

				zIndexCount++;
				zIndexCount2 = zIndexCount + 1;

			} while (zIndex2 != startZIndex);
		}
		else
		{
			for (uint32_t zIndex = 0; zIndex < ((Graph *)parentGraph)->maxDepth; zIndex++)
			{
				glBegin(GL_LINE_STRIP);

				for (int i = startDataIndex; i < endIndex; i++)
				{
					yValue = vertices[zIndex][i].y * scale;
					zValue = vertices[zIndex][i].z * scale;

					glVertex3f(x, yValue, zValue);

					x += xInc;
				}

				glEnd();
			}
		}
	}
}

MinMax GraphDataSeries::GetMinMax(uint32_t startDataIndex, uint32_t endIndex, bool useY, bool useZ)
{
	double value, yValue, zValue;

	MinMax dataSeriesMinMax;

	if (endIndex != 0 && startDataIndex == endIndex)
		return dataSeriesMinMax;

	int32_t zIndex = currentZIndex;

	if (endIndex == 0 && verticesCount > 0)
		endIndex = verticesCount - 1;

	if ((endIndex == 0 || endIndex > verticesCount) && verticesCount > 0)
		endIndex = verticesCount - 1;

	uint32_t zIndexCount = 0;
	do
	{
		zIndex--;

		if (zIndex < 0)
			zIndex = ((Graph *)parentGraph)->maxDepth - 1;

		for (int i = startDataIndex; i <= endIndex; i++)
		{
			if (useY && useZ)
				value = std::min(vertices[zIndex][i].y, vertices[zIndex][i].z);
			else
				if (useY)
					value = vertices[zIndex][i].y;
				else
					if (useZ)
						value = vertices[zIndex][i].z;
					else
					{
						yValue = vertices[zIndex][i].y;
						zValue = vertices[zIndex][i].z;

						value = sqrt(yValue*yValue + zValue*zValue);
					}

			if (value != 0 && (value < dataSeriesMinMax.min || dataSeriesMinMax.min == 999999999))
				dataSeriesMinMax.min = value;

			if (useY && useZ)
				value = std::max(vertices[zIndex][i].y, vertices[zIndex][i].z);

			if (value > dataSeriesMinMax.max || dataSeriesMinMax.max == -999999999)
				dataSeriesMinMax.max = value;
		}

		zIndexCount++;

	} while (zIndex != startZIndex && zIndexCount <= ((Graph *)parentGraph)->drawDepth);

	dataSeriesMinMax.range = dataSeriesMinMax.max - dataSeriesMinMax.min;

	return dataSeriesMinMax;
}

MinMax GraphDataSeries::GetMinMaxForMagnitudes(uint32_t startDataIndex, uint32_t endIndex)
{
	double yValue, zValue, magnitude;

	MinMax dataSeriesMinMax;

	if (startDataIndex > endIndex)
		return dataSeriesMinMax;

	int32_t zIndex = currentZIndex;

	if (endIndex == 0)
		endIndex = verticesCount - 1;

	do
	{
		zIndex--;

		if (zIndex < 0)
			zIndex = ((Graph *)parentGraph)->maxDepth - 1;

		for (int i = startDataIndex; i < endIndex; i++)
		{
			yValue = vertices[zIndex][i].y;
			zValue = vertices[zIndex][i].z;

			magnitude = sqrt(yValue*yValue + zValue*zValue);

			if (magnitude < dataSeriesMinMax.min || dataSeriesMinMax.min == 999999999)
				dataSeriesMinMax.min = magnitude;

			if (magnitude > dataSeriesMinMax.max || dataSeriesMinMax.max == -999999999)
				dataSeriesMinMax.max = magnitude;
		}
	} while (zIndex != startZIndex);

	dataSeriesMinMax.range = dataSeriesMinMax.max - dataSeriesMinMax.min;

	return dataSeriesMinMax;
}

GraphDataSeries::~GraphDataSeries()
{
	delete vertices;
}

uint32_t GraphDataSeries::BUFFER_START_SIZE = 20000;
