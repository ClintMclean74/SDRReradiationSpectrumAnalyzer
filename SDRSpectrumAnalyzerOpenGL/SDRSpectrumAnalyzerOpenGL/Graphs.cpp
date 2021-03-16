#include <GL/glut.h>
#include "Graphs.h"


Graphs::Graphs()
{
	////bufferSize = 3;
	graphs = new GraphPtr[bufferSize];	
}

void Graphs::SetVisibility(bool visible)
{
	for (int i = 0; i < graphCount; i++)
		graphs[i]->visible = visible;	
}

void Graphs::ResetToUserDrawDepths()
{
	for (int i = 0; i < graphCount; i++)
		graphs[i]->drawDepth = graphs[i]->userSetDepth;
}

void Graphs::SetGap(double gap)
{
	this->gap = gap;	
}

void Graphs::SetPos(double x, double y, double z)
{
	this->x = x;
	this->y = y;
	this->z = z;
}

float Graphs::GetHeight()
{
	float height = 0;

	for (int i = 0; i < graphCount; i++)
	{
		height += graphs[i]->height;
	}

	height += gap * (graphCount - 1);

	return height;
}

uint32_t Graphs::AddGraph(Graph* graph)
{
	graphs[graphCount++] = graph;

	float graphsHeight = GetHeight();

	graph->SetPos(x, y - graphsHeight - gap, z);

	return graphCount;
}

void Graphs::Draw()
{
	for (int i = 0; i < graphCount; i++)
		graphs[i]->Draw();
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