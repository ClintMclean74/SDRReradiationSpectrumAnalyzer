#include <GL/glut.h>
#include "Graphs.h"


Graphs::Graphs()
{	
	graphs = new GraphPtr[bufferSize];	
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

float Graphs::GetWidth()
{
	return GetNewGraphX() + Graphs::GRAPH_WIDTH;
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