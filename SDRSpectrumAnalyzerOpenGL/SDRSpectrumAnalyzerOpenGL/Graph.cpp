#ifdef _DEBUG 
 #define _ITERATOR_DEBUG_LEVEL 2 
#endif
#include <algorithm>
#include "Graph.h"
#include "ArrayUtilities.h"
#include "MathUtilities.h"
#include "GraphicsUtilities.h"
#include "DebuggingUtilities.h"

Graph::Graph(uint32_t maxDepth, uint32_t verticesCount, uint8_t dataSeriesCount)
{
	this->maxDepth = maxDepth;
	drawDepth = maxDepth;
	userSetDepth = 1;

	this->verticesCount = verticesCount;

	this->dataSeriesCount = dataSeriesCount;
	
	sprintf(YaxisLabel, "");
	
	matrix = new glm::mat4(1.0f);

	dataSeries = new GraphDataSeriesPtr[dataSeriesCount];

	for (int i = 0; i < dataSeriesCount; i++)
	{
		dataSeries[i] = new GraphDataSeries(this);
		dataSeries[i]->maxResolution = verticesCount;
	}

	width = 100;
	height = 100;

	axisLineWidth = 2;

	selectedAreaColor.r = 1.0;
	selectedAreaColor.g = 1.0;
	selectedAreaColor.b = 1.0;
	selectedAreaColor.a = 0.2;

	for (int i = 0; i < textCount; i++)
	{
		memset(&text[i][0], 0, 255);
	}

	prevTime = GetTickCount();
}
	
uint32_t Graph::GetPointsCount()
{
	verticesCount = 0;

	for (int i = 0; i < dataSeriesCount; i++)
	{
		if (dataSeries[i])
		{
			if (dataSeries[i]->verticesCount > verticesCount)
				verticesCount = dataSeries[i]->verticesCount;
		}
	}

	return verticesCount;
}

void Graph::SetSize(double width, double height)
{
	this->width = width;
	this->height = height;
	this->length = height;
}

void Graph::SetPos(double x, double y, double z)
{
	pos.x = x;
	pos.y = y;
	pos.z = z;
}

void Graph::SetRotation(double xRot, double yRot, double zRot)
{
	this->xRot = xRot;
	this->yRot = yRot;
	this->zRot = zRot;
}

void Graph::CalculateScale()
{	
	double minScale=999999999, max;

	MinMax minMax;

	for (int i = 0; i < dataSeriesCount; i++)
	{
		if (dataSeries[i] && dataSeries[i]->verticesCount > 0)
		{			
			minMax = dataSeries[i]->GetMinMaxForMagnitudes();

			max = std::max(abs(minMax.min), abs(minMax.max));

			dataScale = height / max;

			if (dataScale < minScale)
				minScale = dataScale;
		}
	}

	dataScale = minScale;	
}

void Graph::SetMaxResolution(uint32_t maxResolution)
{
	for (int i = 0; i < dataSeriesCount; i++)
	{
		if (dataSeries[i])
		{
			dataSeries[i]->maxResolution = maxResolution;

			dataSeries[i]->verticesCount = maxResolution;
		}
	}
}

uint32_t Graph::SetData(void* data, uint32_t length, uint8_t seriesIndex, bool complex, double iOffset, double qOffset, bool swapIQ, SignalProcessingUtilities::DataType dataType, bool insertAtEnd)
{
	currentTime = GetTickCount();	

	
	uint8_t byte;

	if (prevData == NULL)
		prevData = new uint8_t[length];

	if (currentTime != prevTime)
	{
		frameRate = (double)1000 / (currentTime - prevTime);

		frameRateTotal += frameRate;

		frameRateCount++;

		/*bool sameData = true;
		for (int i = 0; i < length; i++)
			if (prevData[i] != ((uint8_t*)data)[i])
			{
				sameData = false;
				break;
			}		

		memcpy(prevData, data, length / 10);
		*/

		/*for (int i = 0; i < length/10; i++)
			((uint8_t*)data)[i] = ((float)rand() / RAND_MAX) * 255;
			*/

	
		//if (frameRateCount > 1)
		{
			frameRate = frameRateTotal / frameRateCount;

			//original snprintf(textBufferFrameRate, sizeof(textBufferFrameRate), "Write Rate: %.4f", frameRate);
			snprintf(textBufferFrameRate, sizeof(textBufferFrameRate), "", frameRate);
					

			/*if (sameData)
				snprintf(textBufferFrameRate, sizeof(textBufferFrameRate), "Same data: true");
			else
				snprintf(textBufferFrameRate, sizeof(textBufferFrameRate), "Same data: false");
				*/

			
	

			
			//textBufferFrameRate[0] = ((char*) data)[0];

			frameRateTotal = 0;
			frameRateCount = 0;
		}		

		prevTime = currentTime;
	}
	


	if (!paused)
	{
		if (dataSeries[seriesIndex])
		{			
			dataSeries[seriesIndex]->SetData(data, length, complex, iOffset, qOffset, swapIQ, dataType, insertAtEnd);
		}
		else
		{
			dataSeries[seriesIndex] = new GraphDataSeries(this);

			dataSeries[seriesIndex]->maxResolution = verticesCount;

			dataSeries[seriesIndex]->SetColor(0, 0, 1, 1);

			dataSeries[seriesIndex]->SetData(data, length, complex, iOffset, qOffset, swapIQ, dataType);
		}

		return length;
	}
	else
		return 0;
}

void Graph::SetDepth(uint32_t depth, bool userSet)
{
	if (maxDepth > 2) //less than 3 = graph without depth
	{
		drawDepth = depth;

		if (userSet)
			userSetDepth = drawDepth;
	}
}

double Graph::GetGradientForIndex(uint8_t seriesIndex, uint8_t index)
{
	return dataSeries[seriesIndex]->GetGradientForIndex(index);
}

double Graph::GetAvgValueForIndex(uint8_t seriesIndex, uint8_t index, uint32_t count)
{
	return dataSeries[seriesIndex]->GetAvgValueForIndex(index, count);
}

void Graph::SetDataSeriesLineWidth(GLfloat width, int8_t seriesIndex)
{
	if (seriesIndex == -1)
	{
		for (int i = 0; i < dataSeriesCount; i++)
			if (dataSeries[i])
				dataSeries[i]->lineWidth = width;
	}
	else
		dataSeries[seriesIndex]->lineWidth = width;
}

void Graph::SetDataSeriesStyle(GraphStyle style, int8_t seriesIndex)
{
	if (seriesIndex == -1)
	{
		for (int i = 0; i < dataSeriesCount; i++)
			if (dataSeries[i])
				dataSeries[i]->SetStyle(style);
	}
	else
		dataSeries[seriesIndex]->SetStyle(style);
}

void Graph::SetDataSeriesColor(Color color, int8_t seriesIndex, uint8_t colorIndex)
{
	SetDataSeriesColor(color.r, color.g, color.b, color.a, seriesIndex, colorIndex);
}

void Graph::SetDataSeriesColor(float red, float green, float blue, float alpha, int8_t seriesIndex, uint8_t colorIndex)
{
	if (seriesIndex == -1)
	{
		for (int i = 0; i < dataSeriesCount; i++)
			if (dataSeries[i])
				dataSeries[i]->SetColor(red, green, blue, alpha, colorIndex);
	}
	else
		if (dataSeries[seriesIndex])
			dataSeries[seriesIndex]->SetColor(red, green, blue, alpha, colorIndex);
}


void Graph::SetDataWidth(uint32_t width, uint8_t seriesIndex)
{
	dataSeries[seriesIndex]->SetDataWidth(width);	
}

void Graph::ZoomOut()
{		
	if (zoomStack.size() > 0)
	{
		MinMax startEndIndexes = zoomStack.top();
		zoomStack.pop();
		
		startDataIndex = startEndIndexes.min;
		endDataIndex = startEndIndexes.max;
	}
}

glm::vec4 Graph::PointOnGraph(float x, float y, float z)
{	
	glm::mat4 inverse = glm::inverse(*matrix);

	glm::vec4 v(1.0);
	v.x = x;
	v.y = y;
	v.z = z;

	glm::vec4 result = inverse * v;

	double minHeight = (showYAxis == 0 || showYAxis == 2) ? -height : 0;
	double maxHeight = (showYAxis == 0 || showYAxis == 1) ? height : 0;

	if (result.z > -height && result.z < height)
	{
		if (result.x >= 0 && result.x <= width)
			if (result.y >= minHeight && result.y <= maxHeight)
				return result;
	}

	result.x = -1;
	result.y = -1;
	result.z = -1;

	return result;
}

void Graph::SetGraphXRange(uint32_t start, uint32_t end)
{
	this->startX = start;
	this->endX = end;
}

void Graph::SetSelectedGraphRange(uint32_t start, uint32_t end)
{
	selectedStart = start;
	selectedEnd = end;
}

void Graph::SetGraphViewRangeXAxis(uint32_t start, uint32_t end)
{	
	MinMax startEndIndexes;		

	if (endDataIndex == 0)
		endDataIndex = dataSeries[0]->verticesCount - 1;

	uint32_t currentDataRange = endDataIndex - startDataIndex;

	if (currentDataRange > 0)
	{
		startEndIndexes.min = startDataIndex;
		startEndIndexes.max = endDataIndex;

		//double newStartIndex = startDataIndex + start / width * currentDataRange;	
		//endDataIndex = (startDataIndex + end / width * currentDataRange) + 1;
		double newStartIndex = MathUtilities::Round(startDataIndex + start / width * (currentDataRange), 0);
		endDataIndex = MathUtilities::Round((startDataIndex + end / width * (currentDataRange)), 0);

		startDataIndex = newStartIndex;

		uint32_t newDataRange = endDataIndex - startDataIndex;

		if (newDataRange > 0)
		{
			zoomStack.push(startEndIndexes);
		}
		else
		{
			startDataIndex = startEndIndexes.min;
			endDataIndex = startEndIndexes.max;
		}
		

		//DebuggingUtilities::DebugPrint("startDataIndex: %i\n", startDataIndex);
		//DebuggingUtilities::DebugPrint("endDataIndex: %i\n", endDataIndex);
	}
}


void Graph::SetGraphViewRangeYAxis(uint32_t start, uint32_t end)
{
	viewYMin = start;
	viewYMax = end;
}

void Graph::SetGraphLabelValuesXAxis(double startX, double endX)
{
	startXAxisLabel = startX;
	endXAxisLabel = endX;
}

void DebugPrint(const char * format, ...);
void DebugPrint(const char * format, ...)
{
	va_list list;

	va_start(list, format);

	printf(format, va_arg(list, int));
	fflush(stdout);

	va_end(list);
}

void Graph::SetGraphFrequencyRangeText(char *rangeText, FrequencyRange* frequencyRange, uint8_t textIndex, bool adjustForSelectedRegion)
{	
	FrequencyRange frequencyRangeForText;

	if (adjustForSelectedRegion)
	{
		frequencyRangeForText = SignalProcessingUtilities::GetSelectedFrequencyRangeFromDataRange(startDataIndex, endDataIndex, 0, GetPointsCount(), frequencyRange->lower, frequencyRange->upper);
	}
	else
		frequencyRangeForText.Set(frequencyRange);

	sprintf(textBuffer, rangeText, SignalProcessingUtilities::ConvertToMHz(frequencyRangeForText.lower), SignalProcessingUtilities::ConvertToMHz(frequencyRangeForText.upper));
	SetText(textIndex, textBuffer);

	SetGraphLabelValuesXAxis(SignalProcessingUtilities::ConvertToMHz(frequencyRangeForText.lower), SignalProcessingUtilities::ConvertToMHz(frequencyRangeForText.upper));
}

void Graph::SetText(uint8_t index, const char * format, ...)
{
	va_list list;

	va_start(list, format);	

	snprintf(&text[index][0], 255, format, va_arg(list, double));
	
	va_end(list);
}

void Graph::Draw()
{
	if (visible)
	{
		glPushMatrix();

		glTranslatef(pos.x, pos.y, pos.z);

		glRotatef(xRot, 1, 0, 0);

 		glGetFloatv(GL_MODELVIEW_MATRIX, glm::value_ptr(*matrix));

		glColor3f(axisColor.r, axisColor.g, axisColor.b);

		glLineWidth(axisLineWidth);

		double axisHeight = height;

		glBegin(GL_LINES);

		if (showYAxis & 1)
		{
			glVertex2f(0, 0);
			glVertex2f(0, axisHeight);
		}

		if (showYAxis & 2)
		{
			glVertex2f(0, -axisHeight);
			glVertex2f(0, 0);
		}

		if (showXAxis & 1)
		{
			glVertex2f(0, 0);
			glVertex2f(width, 0);
		}

		if (showXAxis & 2)
		{
			glVertex2f(-width, 0);
			glVertex2f(0, 0);
		}

		if (showZAxis & 1)
		{
			glVertex3f(0, 0, 0);
			glVertex3f(0, 0, axisHeight);
		}

		if (showZAxis & 2)
		{
			glVertex3f(0, 0, -axisHeight);
			glVertex3f(0, 0, 0);
		}

		glEnd();

		if (showLabels)
		{
			float labelHeight;

			float textStartHeight = height;
			
			//GraphicsUtilities::DrawText(textBufferFrameRate, width / 20, textStartHeight + GraphicsUtilities::fontScale * 2 + 100, 0, GraphicsUtilities::fontScale * 2);
			
			for (int i = 0; i < textCount; i++)
			{
				if (text[i][0] != 0)
				{
					GraphicsUtilities::DrawText(&text[i][0], width/20, textStartHeight, 0, GraphicsUtilities::fontScale * 2);				
				}

				labelHeight = glutStrokeWidth(GLUT_STROKE_ROMAN, 'H');
				textStartHeight -= (labelHeight * GraphicsUtilities::fontScale) * 5;
			}

			if (showXLabels)
			{
				float labelWidth = 0;

				uint32_t increments = labelsCount - 1;

				uint32_t startFrequency = SignalProcessingUtilities::GetFrequencyFromDataIndex(startDataIndex, 0, GetPointsCount(), startX, endX);
				uint32_t endFrequency;

				if (endDataIndex == 0)
					endFrequency = endX;
				else
				{
					endFrequency = SignalProcessingUtilities::GetFrequencyFromDataIndex(endDataIndex, 0, GetPointsCount(), startX, endX);
				}

				startXAxisLabel = SignalProcessingUtilities::ConvertToMHz(startFrequency);
				endXAxisLabel = SignalProcessingUtilities::ConvertToMHz(endFrequency);

				double label = startXAxisLabel;
				float xPos = 0;

				double labelInc = (endXAxisLabel - startXAxisLabel) / increments;
				double xInc = width / increments;

				for (int i = 0; i < labelsCount; i++)
				{
					snprintf(textBuffer, sizeof(textBuffer), "%.4f", MathUtilities::Round(label, 4));

					labelWidth = glutStrokeLength(GLUT_STROKE_ROMAN, (const unsigned char *)textBuffer);

					labelWidth *= GraphicsUtilities::fontScale;

					GraphicsUtilities::DrawText(textBuffer, (i == 0 ? 0 : xPos - labelWidth / 2), -100, 0, GraphicsUtilities::fontScale);

					label += labelInc;
					xPos += xInc;
				}
			}
		}

		glPushMatrix();

		double scale = 0;

		if (dataSeriesSameScale)
		{
			if (autoScale)
			{
				MinMax minMax, dataSeriesMinMax;

				bool set = false;

				for (int i = 0; i < dataSeriesCount; i++)
				{
					if (dataSeries[i]->verticesCount > 0)
					{
						dataSeriesMinMax = dataSeries[i]->GetMinMax(startDataIndex, endDataIndex, true, false);

						//if (dataSeriesMinMax.range == 0)
						if (dataSeries[i]->verticesCount == 0 || (dataSeriesMinMax.min == 0 && dataSeriesMinMax.max == 0))
						{
							dataSeries[i]->visible = false;

							continue;
						}

						if ((!set || dataSeriesMinMax.min < minMax.min) && dataSeriesMinMax.min != 0)
							minMax.min = dataSeriesMinMax.min;

						if (!set || dataSeriesMinMax.max > minMax.max)
							minMax.max = dataSeriesMinMax.max;

						set = true;

						dataSeries[i]->visible = true;
					}
				}

				if (minMax.min == 999999999 || minMax.max == 999999999)
				{
					minMax.min = 0;
					minMax.max = 0;
				}

				if (minMax.min == minMax.max)
				{
					minMax.min = minMax.max - 0.5;
					minMax.max = minMax.max + 0.5;
				}
				
				minMax.CalculateRange();

				scale = height / minMax.range;

				viewYMin = minMax.min;
				viewYMax = minMax.max;
			}

			if (showLabels)
			{
				uint32_t increments = labelsCount - 1;

				double label = viewYMin;
				float yPos = 0;

				double labelInc = (viewYMax - viewYMin) / increments;
				double xInc = width / increments;
				double yInc = height / increments;

				float labelHeight, labelWidth, maxLabelWidth=-1;
				for (int i = 0; i < labelsCount; i++)
				{
					snprintf(textBuffer, sizeof(textBuffer), "%.4f", MathUtilities::Round(label, 4));

					labelWidth = glutStrokeLength(GLUT_STROKE_ROMAN, (const unsigned char *)textBuffer);

					labelWidth *= GraphicsUtilities::fontScale;

					if (labelWidth > maxLabelWidth)
						maxLabelWidth = labelWidth;

					labelHeight = glutStrokeLength(GLUT_STROKE_ROMAN, (const unsigned char *)textBuffer);

					labelHeight *= GraphicsUtilities::fontScale;

					GraphicsUtilities::DrawText(textBuffer, -labelWidth, yPos, 0, GraphicsUtilities::fontScale);

					label += labelInc;
					yPos += yInc;
				}

				float xPos = maxLabelWidth * 1.1;

				snprintf(textBuffer, sizeof(textBuffer), "%s", YaxisLabel);

				labelWidth = glutStrokeLength(GLUT_STROKE_ROMAN, (const unsigned char *)textBuffer);

				labelWidth *= GraphicsUtilities::fontScale;
				
				GraphicsUtilities::DrawText(textBuffer, -xPos, height/2 - labelWidth/2, 0, GraphicsUtilities::fontScale, 90);
			}
		}


		for (int i = 0; i < dataSeriesCount; i++)
		{
			if (dataSeries[i] && dataSeries[i]->verticesCount > 0)
			{
				dataSeries[i]->Draw(startDataIndex, endDataIndex, viewYMin, viewYMax, scale, false, iValueUseForColors);				
			}
		}

		glPopMatrix();
		glPopMatrix();
	}
}

void Graph::DrawTransparencies()
{
	if (visible)
	{
		glPushMatrix();

		glTranslatef(pos.x, pos.y, pos.z);

		glRotatef(xRot, 1, 0, 0);

		double axisHeight = height;

		glDisable(GL_DEPTH_TEST);

		double selectedLength = (selectedEnd - selectedStart);
		if (selectedLength > 0)
		{
			float selectedCubeWidth = selectedEnd - selectedStart;
			float selectedCubeLength = axisHeight;

			float selectedCubeHeight = 0;
			float yOffset = 0;

			if ((showYAxis & 1 || showYAxis & 2) && !((showYAxis & 3) == 3))
				selectedCubeHeight = axisHeight;
			else
				if ((showYAxis & 3) == 3)
					selectedCubeHeight = axisHeight * 2;

			if (showYAxis & 1)
				yOffset = selectedCubeHeight / 2;
			else
				if (showYAxis & 2)
					yOffset = -selectedCubeHeight / 2;

			glColor4f(selectedAreaColor.r, selectedAreaColor.g, selectedAreaColor.b, 0.4f);			

			glPushMatrix();
			glTranslatef(selectedLength / 2 + selectedStart, yOffset, -selectedCubeLength / 2);
			glScalef(selectedCubeWidth, selectedCubeHeight, selectedCubeLength);
			glutSolidCube(1);
			glPopMatrix();
		}

		glEnable(GL_DEPTH_TEST);

		glColor4f(1, 1, 1, 0.4);

		glBegin(GL_TRIANGLES);

		glVertex3f(0, 0, -0.1);
		glVertex3f(width, 0, -0.1);
		glVertex3f(width, axisHeight, -0.1);

		glVertex3f(width, axisHeight, -0.1);
		glVertex3f(0, axisHeight, -0.1);
		glVertex3f(0, 0, -0.1);


		glVertex3f(0, 0, -length);
		glVertex3f(width, 0, -length);
		glVertex3f(width, 0, 0);

		glVertex3f(width, 0, 0);
		glVertex3f(0, 0, 0);
		glVertex3f(0, 0, -length);

		glEnd();

		glPopMatrix();
	}
}

void Graph::TogglePause()
{
	paused = !paused;
}

Graph::~Graph()
{
	delete dataSeries;
}
