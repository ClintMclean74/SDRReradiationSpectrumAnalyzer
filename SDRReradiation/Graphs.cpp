#include <stdarg.h>
#include "Graphs.h"
#include "GraphicsUtilities.h"


Graphs::Graphs()
{
    graphCount = 0;
	bufferSize = 20;

	graphsGridVertCount = 4;

	x = y = z = 0;
    xGap = 200;
	yGap = 300;
	//MAX_GRAPHS = 10;
	//DRAWING_GRAPHS = true;
	//GRAPH_WIDTH = 1000;
	//GRAPH_HEIGHT = 900;
	//GRAPH_STRENGTHS_X_OFFSET = -100;
	//GRAPH_SEGMENT_RESOLUTION = 1024;
    //textCount = TEXTCOUNT;

	showLabels = true;

	graphs = new GraphPtr[bufferSize];

	for (int i = 0; i < textCount; i++)
	{
		memset(&text[i][0], 0, 255);
	}
}

void Graphs::SetVisibility(bool visible)
{
	for (int i = 0; i < graphCount; i++)
		graphs[i]->visible = visible;
}

void Graphs::ShowLabels(bool visible)
{
	for (int i = 0; i < graphCount; i++)
		graphs[i]->showLabels = visible;
}

void Graphs::ToggleLabels()
{
	showLabels = !showLabels;

	for (int i = 0; i < graphCount; i++)
		graphs[i]->showLabels = showLabels;
}

void Graphs::ResetToUserDrawDepths()
{
	for (int i = 0; i < graphCount; i++)
		graphs[i]->drawDepth = graphs[i]->userSetDepth;
}

void Graphs::SetGap(double xGap, double yGap)
{
	this->xGap = xGap;
	this->yGap = yGap;
}

void Graphs::SetPos(double x, double y, double z)
{
	this->x = x;
	this->y = y;
	this->z = z;
}

void Graphs::SetText(uint8_t index, const char * format, ...)
{
	va_list list;

	va_start(list, format);

	snprintf(&text[index][0], 255, format, va_arg(list, double));

	va_end(list);
}

float Graphs::GetWidth()
{
	uint8_t placedGraphsCount = GetPlacedGraphsCount();

	int columnCount = ceil((double)placedGraphsCount / graphsGridVertCount);
	//int columnCount = MathUtilities::Round((double)placedGraphsCount / graphsGridVertCount, 0);

	return columnCount * (Graphs::GRAPH_WIDTH + xGap);
}

float Graphs::GetHeight()
{
	float height = 0;

	for (int i = 0; i < graphCount; i++)
	{
		height += graphs[i]->height;
	}

	height += yGap * (graphCount - 1);

	y - height - yGap;

	return height;
}

uint8_t Graphs::GetPlacedGraphsCount()
{
	uint8_t count = 0;

	for (int i = 0; i < graphCount; i++)
		if (graphs[i]->automaticPlacement)
			count++;

	return count;
}

float Graphs::GetNewGraphX()
{
	float newGraphX = x;

	uint8_t placedGraphsCount = GetPlacedGraphsCount();

	int columnCount = placedGraphsCount / graphsGridVertCount;

	newGraphX += (columnCount * (Graphs::GRAPH_WIDTH + xGap));

	return newGraphX;
}

float Graphs::GetNewGraphY()
{
	float newGraphY = y;

	uint8_t placedGraphsCount = GetPlacedGraphsCount();

	int columnCount = placedGraphsCount / graphsGridVertCount;

	placedGraphsCount -= (columnCount * graphsGridVertCount);

	int rowCount = placedGraphsCount % graphsGridVertCount;

	for (int i = 0; i < rowCount; i++)
	{
		newGraphY  -= (graphs[i]->height + yGap);
	}

	//newGraphY -= gap * placedGraphsCount;

	return newGraphY;
}

uint32_t Graphs::AddGraph(Graph* graph)
{
	if (graph->automaticPlacement)
	{
		float newGraphX = GetNewGraphX();
		float newGraphY = GetNewGraphY();

		graph->SetPos(newGraphX, newGraphY - graph->height, z);
	}

	graphs[graphCount++] = graph;

	return graphCount;
}

void Graphs::Draw()
{
	float labelHeight;

	float textStartHeight = GRAPH_HEIGHT*0.8;

	float fontScale;

	for (int i = 0; i < textCount; i++)
	{
		//if (text[i][0] != 0)
		{
		    if (i<3)
            {
                glLineWidth(3);
                fontScale = Graphs::FONT_SCALE * 5;

                if (GetTime() % 1000 > 200)
                    glColor3f(Graphs::reradiatedColor.r, Graphs::reradiatedColor.g, Graphs::reradiatedColor.b);
                else
                    glColor3f(0, 0, 0);
            }
            else if (i<5)
            {
                glLineWidth(2);
                fontScale = Graphs::FONT_SCALE * 4;

                if (GetTime() % 1000 > 200)
                    glColor3f(Graphs::reradiatedColor.r, Graphs::reradiatedColor.g, Graphs::reradiatedColor.b);
                else
                    glColor3f(0, 0, 0);
            }
            else
            {
                glLineWidth(2);
                fontScale = Graphs::FONT_SCALE * 4;

                glColor3f(1.0, 1.0, 1.0);
            }

            if (text[i][0] != 0)
                GraphicsUtilities::DrawText(&text[i][0], GRAPH_WIDTH / 20, textStartHeight, 0, fontScale);
		}

		labelHeight = glutStrokeWidth(GLUT_STROKE_ROMAN, 'H');
		textStartHeight -= (labelHeight * fontScale * 1.5);
	}

	for (int i = 0; i < graphCount; i++)
		graphs[i]->Draw();
}

void Graphs::DrawTransparencies()
{
	for (int i = 0; i < graphCount; i++)
		graphs[i]->DrawTransparencies();
}

SelectedGraph* Graphs::GetSelectedGraph(float x, float y, float z)
{
	SelectedGraph* selectedGraph = NULL;

	glm::vec4 vector;

	for (int i = 0; i < graphCount; i++)
	{
		if (graphs[i]->visible)
		{
			vector = graphs[i]->PointOnGraph(x, y, z);

			if (vector.x != -1 && vector.y != -1 && vector.z != -1)
			{
				if (!selectedGraph || vector.z < selectedGraph->point.z)
				{
					selectedGraph = new SelectedGraph();

					selectedGraph->graph = graphs[i];
					selectedGraph->point = vector;
				}
			}
		}
	}

	return selectedGraph;
}

void Graphs::SetAutoScale(bool autoScale)
{
	for (int i = 0; i < graphCount; i++)
	{
		graphs[i]->autoScale = autoScale;
	}
}



Graphs::~Graphs()
{

}

Color Graphs::nearColor(1, 0, 0, 1);
Color Graphs::farColor(0, 1, 0, 1);
Color Graphs::undeterminedColor(1, 1, 0, 1);
Color Graphs::transitionNearFarDifColor(1, 1, 0, 1);
Color Graphs::reradiatedColor((float)254 / 255, (float)217 / 255, (float)67 / 255, 1);

bool Graphs::DRAWING_GRAPHS_WITH_BUFFERS = true;
