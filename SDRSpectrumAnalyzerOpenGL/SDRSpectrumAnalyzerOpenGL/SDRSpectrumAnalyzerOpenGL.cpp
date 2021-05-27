#include "glew-2.1.0\include\GL/glew.h"
#include "glew-2.1.0\include\GL/wglew.h"
#include <GL/glut.h>

#include <time.h>
#include <conio.h>
#include <process.h>
#include "NearFarDataAnalyzer.h"
#include "Graphs.h"
#include "SignalProcessingUtilities.h"
#include "DebuggingUtilities.h"
#include "GraphicsUtilities.h"

NearFarDataAnalyzer* nearFarDataAnalyzer;

bool paused = false;

Graphs* graphs;
Graph* dataGraph, *fftGraphForDevicesBandwidth, *combinedFFTGraphForBandwidth, *correlationGraph, *fftGraphStrengthsForDeviceRange, *fftAverageGraphForDeviceRange, *fftAverageGraphStrengthsForDeviceRange;
Graph* spectrumRangeGraph;
Graph* spectrumRangeDifGraph;
Graph* allSessionsSpectrumRangeGraph;
Graph* spectrumboardGraph;
Graph* leaderboardGraph;
Graph* strengthGraph;
Graph* transitionGraph;
Graph* averageTransitionsGraph;
Graph* strengthFFTGraph;
Graph* strengthFFTAveragedGraph;
Graph* transitionboardGraph;
Graph* resultsGraph;
Graph* previousGraphView;


SelectedGraph *selectedGraph;
SelectedGraph* selectedGraphStart;
SelectedGraph* selectedGraphEnd;

uint32_t defaultSelectedStartIndex = 0;
uint32_t defaultSelectedEndIndex = 50;

bool drawDataGraph = true;
bool drawCorrelationGraph = true;
bool drawFFTGraphForDevicesRanges = true;
bool drawfftGraphForDevicesBandwidths = true;
bool drawFFTGraphStrengthsForDeviceRange = true;
bool drawFFTAverageGraphForDeviceRange = true;
bool drawFFTAverageGraphStrengthsForDeviceRange = true;

bool correlating = false;

Vector pos;

double xRot = 0;
double yRot = 0;

double rotScale = 0.2;
double translateScale = 2;
double zoomScale = 2;

double fov = 34;

int prevX = -1;
int prevY = -1;

int currentButton = 0;

bool viewChanged = true;

bool ctrl = false;
bool shift = false;
bool alt = false;

bool autoScale = true;

bool eegStrength = false;

DWORD currentTime, prevTime;

double frameRate, frameRateTotal = 0, frameRateCount = 0;;

char textBuffer[255];
char textBufferFrameRate[255];

GLfloat mat_ambient[] = { 1.0, 1.0, 1.0, 1.0 };
GLfloat mat_diffuse[] = { 1.0, 1.0, 1.0, 1.0 };
GLfloat mat_specular[] = { 1.0, 1.0, 1.0, 1.0 };
GLfloat mat_shininess[] = { 50.0 };
GLfloat light_position[] = { -1.0, -1.0, -1.0, 0.0 };

Graph* resultsGraphs[Graphs::MAX_GRAPHS];
uint8_t resultsGraphsCount = 0;

uint8_t nextResultsGraphToViewIndex = 0;

Color device1DataColor(1, 0, 0, 1);
Color device2DataColor(0, 1, 0, 1);
Color correlationColor(249 / 255, 191 / 255, 72 / 255, 1);

Transition* graphTransition;

Vector Get3DPoint(int screenX, int screenY)
{
	Vector vector;

	GLint viewport[4];
	GLdouble modelview[16];
	GLdouble projection[16];

	glGetIntegerv(GL_VIEWPORT, viewport);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glGetDoublev(GL_MODELVIEW_MATRIX, modelview);
	glGetDoublev(GL_PROJECTION_MATRIX, projection);

	GLfloat z_cursor;
	
	screenY = viewport[3] - screenY;

	// Get the z value for the screen coordinate
	glReadPixels(screenX, screenY, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &z_cursor);

	// get the 3D coordinate
	gluUnProject(screenX, screenY, z_cursor, modelview, projection, viewport, &vector.x, &vector.y, &vector.z);

	return vector;
}

void DrawFrequenciesRangeBoard(FrequencyRanges* frequencyRanges, float x, float y, float z, char* string, char* string2)
{
	float textStartHeight = y;
	
	glColor4f(1.0, 1.0, 1.0, 1.0);
	GraphicsUtilities::DrawText(string, x, textStartHeight, z, GraphicsUtilities::fontScale, 0);

	int labelHeight = glutStrokeWidth(GLUT_STROKE_ROMAN, (int)'A') * GraphicsUtilities::fontScale;
	textStartHeight -= labelHeight * 3;

	GraphicsUtilities::DrawText(string2, x, textStartHeight, z, GraphicsUtilities::fontScale, 0);

	x += glutStrokeWidth(GLUT_STROKE_ROMAN, (int)textBuffer[0]) * GraphicsUtilities::fontScale;
	textStartHeight -= labelHeight * 3;

	FrequencyRange *frequencyRange;

	for (uint8_t i = 0; i < frequencyRanges->count; i++)
	{		
		frequencyRange = frequencyRanges->GetFrequencyRangeFromIndex(i);
		if (frequencyRange)
		{			
			if (frequencyRange->flags[0] >= NearFarDataAnalyzer::REQUIRED_TRANSITION_WRITES_FOR_RERADIATED_FREQUENCIES && frequencyRange->strength >= NearFarDataAnalyzer::REQUIRED_TRANSITION_STRENGTH_FOR_RERADIATED_FREQUENCIES)
				glColor4f(Graphs::reradiatedColor.r, Graphs::reradiatedColor.g, Graphs::reradiatedColor.b, 1.0);
			else
				glColor4f(1.0, 1.0, 1.0, 1.0);

			textStartHeight -= labelHeight * 3;

			snprintf(textBuffer, sizeof(textBuffer), "%.3f MHz - %.3f MHz", SignalProcessingUtilities::ConvertToMHz(frequencyRange->lower, 3), SignalProcessingUtilities::ConvertToMHz(frequencyRange->upper, 3));
			GraphicsUtilities::DrawText(textBuffer, x, textStartHeight, z, GraphicsUtilities::fontScale, 0);						
		}
	}

	glColor4f(1.0, 1.0, 1.0, 1.0);
}

void display(void)
{
	currentTime = GetTickCount();	

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	if (currentTime != prevTime)
	{
		frameRate = (double)1000 / (currentTime - prevTime);

		frameRateTotal += frameRate;

		frameRateCount++;

		if (frameRateCount > 1000)
		{			
			frameRate = frameRateTotal / frameRateCount;

			snprintf(textBufferFrameRate, sizeof(textBufferFrameRate), "Frame Rate: %.4f", frameRate);

			frameRateTotal = 0;
			frameRateCount = 0;
		}

		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();

		if (graphs->graphsGridVertCount == 4)
			gluPerspective(fov, (GLdouble)glutGet(GLUT_WINDOW_HEIGHT) / glutGet(GLUT_WINDOW_WIDTH) * 1.3, 1.0, 12000.0);
		else
			gluPerspective(fov, (GLdouble)glutGet(GLUT_WINDOW_HEIGHT) / glutGet(GLUT_WINDOW_WIDTH) * 2.7, 1.0, 12000.0);

		viewChanged = false;

		uint32_t dataLength;

		glMatrixMode(GL_MODELVIEW);

		gluLookAt(0.0, 0.0, 0.0,
			0.0, 0.0, 0.0,
			0.0, 1.0, 0.);

		glLoadIdentity();

		light_position[0] = 1.0;
		light_position[1] = 1.0;
		light_position[2] = 1.0;
		light_position[3] = 0.0;

		glTranslatef(pos.x, pos.y, pos.z);

		glRotatef(xRot, 1, 0, 0);
		glRotatef(yRot, 0, 1, 0);

		glLightfv(GL_LIGHT0, GL_POSITION, light_position);

		if (nearFarDataAnalyzer)
		{
			if (correlating)
				nearFarDataAnalyzer->spectrumAnalyzer.deviceReceivers->Correlate();

			if (Graphs::DRAWING_GRAPHS && graphs)
			{
				graphs->Draw();
				graphs->DrawTransparencies();	

				if (transitionGraph)
					transitionGraph->Draw();

				averageTransitionsGraph->Draw();

				//if (strengthGraph)
					//strengthGraph->Draw();

				//if (spectrumRangeGraph)
					//spectrumRangeGraph->Draw();

				/*if (spectrumRangeDifGraph)
					spectrumRangeDifGraph->Draw();
					*/

				/*fftAverageGraphStrengthsForDeviceRange->Draw();

				if (transitionboardGraph)
					transitionboardGraph->Draw();

				if (strengthGraph)
					strengthGraph->Draw();

				/*if (transitionGraph)
					transitionGraph->Draw();

				if (transitionboardGraph)
					transitionboardGraph->Draw();
					*/

				/*dataGraph->Draw();

				if (correlationGraph)
					correlationGraph->Draw();

				if (fftGraphForDevicesBandwidth)
					fftGraphForDevicesBandwidth->Draw();

				if (combinedFFTGraphForBandwidth)
					combinedFFTGraphForBandwidth->Draw();

				fftGraphStrengthsForDeviceRange->Draw();

				fftAverageGraphForDeviceRange->Draw();

				fftAverageGraphStrengthsForDeviceRange->Draw();

				if (spectrumRangeGraph)
					spectrumRangeGraph->Draw();

				if (allSessionsSpectrumRangeGraph)
					allSessionsSpectrumRangeGraph->Draw();

				if (leaderboardGraph)
					leaderboardGraph->Draw();

				if (leaderboardGraph)
					leaderboardGraph->Draw();

				if (strengthGraph)
					strengthGraph->Draw();

				if (transitionGraph)
					transitionGraph->Draw();
					*/

					/*
					dataGraph->DrawTransparencies();

					if (correlationGraph)
						correlationGraph->DrawTransparencies();

					if (fftGraphForDevicesBandwidth)
						fftGraphForDevicesBandwidth->DrawTransparencies();

					if (combinedFFTGraphForBandwidth)
						combinedFFTGraphForBandwidth->DrawTransparencies();

					fftAverageGraphForDeviceRange->DrawTransparencies();

					if (spectrumRangeGraph)
						spectrumRangeGraph->DrawTransparencies();

					if (allSessionsSpectrumRangeGraph)
						allSessionsSpectrumRangeGraph->DrawTransparencies();

					if (leaderboardGraph)
						leaderboardGraph->DrawTransparencies();
						*/

				if (graphs->showLabels)
				{
					if (nearFarDataAnalyzer->detectionMode == DetectionMode::Sessions)
					{
						nearFarDataAnalyzer->spectrumFrequencyRangesBoard->QuickSort();
						sprintf(textBuffer, "Spectrum Graph in Session %i:", nearFarDataAnalyzer->sessionCount);
						DrawFrequenciesRangeBoard(nearFarDataAnalyzer->spectrumFrequencyRangesBoard, graphs->x - dataGraph->width * 0.6, 0, graphs->z, "Strongest Reradiated Frequencies For", textBuffer);

						nearFarDataAnalyzer->leaderboardFrequencyRanges->QuickSort();
						sprintf(textBuffer, "Frequencies after %i Sessions: ", nearFarDataAnalyzer->sessionCount - 1);
						DrawFrequenciesRangeBoard(nearFarDataAnalyzer->leaderboardFrequencyRanges, graphs->x + graphs->GetWidth() - Graphs::GRAPH_WIDTH / 10 + Graphs::GRAPH_WIDTH / 2, 0, graphs->z, "LeaderBoard For Strongest Reradiated", textBuffer);
					}
					else
					{
						nearFarDataAnalyzer->spectrumFrequencyRangesBoard->QuickSort();

						sprintf(textBuffer, "Strongest Reradiated");
						DrawFrequenciesRangeBoard(nearFarDataAnalyzer->spectrumFrequencyRangesBoard, graphs->x - dataGraph->width * 0.6, 0, graphs->z, textBuffer, "Frequencies For Spectrum Graph:");
						
						nearFarDataAnalyzer->leaderboardFrequencyRanges->QuickSort();
						DrawFrequenciesRangeBoard(nearFarDataAnalyzer->leaderboardFrequencyRanges, graphs->x + graphs->GetWidth() - Graphs::GRAPH_WIDTH / 10 + Graphs::GRAPH_WIDTH / 2, 0, graphs->z, "LeaderBoard For Strongest", "Reradiated Frequencies:");
						//DrawFrequenciesRangeBoard(nearFarDataAnalyzer->spectrumFrequencyRangesBoard, graphs->x + graphs->GetWidth() + dataGraph->width / 8 + graphs->GetWidth() / 7, 0, graphs->z, "LeaderBoard For Strongest", "Reradiated Frequencies:");						
					}

					//nearFarDataAnalyzer->transitionFrequencyRangesBoard->QuickSort(QuickSortMode::Frequency);
					nearFarDataAnalyzer->transitionFrequencyRangesBoard->QuickSort();
					DrawFrequenciesRangeBoard(nearFarDataAnalyzer->transitionFrequencyRangesBoard, graphs->x + graphs->GetWidth() - Graphs::GRAPH_WIDTH / 10, 0, graphs->z, "TransitionsBoard For Strongest", "Reradiated Frequencies:");
				}
				
			}

			prevTime = currentTime;
		}
		else
		{
			graphs->Draw();			
			graphs->DrawTransparencies();
		}			
	}

	glFinish();

	glutSwapBuffers();

	Sleep(12);

	glutPostRedisplay();
}

void DeleteSelectedGraphData()
{
	if (selectedGraphStart != NULL)
	{
		delete selectedGraphStart;
		selectedGraphStart = NULL;
	}

	if (selectedGraphEnd != NULL)
	{
		delete selectedGraphEnd;
		selectedGraphEnd = NULL;
	}
}

void SetModeNear()
{
	if (nearFarDataAnalyzer->automatedDetection)
	{
		if (nearFarDataAnalyzer->GetMode() == ReceivingDataMode::Far)
			nearFarDataAnalyzer->addTransition = true;

		nearFarDataAnalyzer->SetMode(ReceivingDataMode::Near);
	}
}

void MouseButtons(int button, int state, int x, int y)
{
	currentButton = button;

	SetModeNear();

	if (button == 1) // It's a wheel event
	{
		switch (state)
		{
			case(GLUT_UP):
			break;
			case(GLUT_DOWN):
			break;			
		}
	}
	else
	// Wheel reports as button 3(scroll up) and button 4(scroll down)
	if (button == 3 || button == 4) // It's a wheel event
	{
		// Each wheel event reports like a button click, GLUT_DOWN then GLUT_UP
		if (state == GLUT_UP) return; // Disregard redundant GLUT_UP events

		if (button == 3)
			fov-=zoomScale;
		else
			fov+= zoomScale;
	}
	
	if (button == GLUT_LEFT_BUTTON)
	{
		prevX = x;
		prevY = y;

		if (state == GLUT_DOWN)
		{	
			Vector vector = Get3DPoint(x, y);

			selectedGraphStart = graphs->GetSelectedGraph(vector.x, vector.y, vector.z);
		}
		else
			if (state == GLUT_UP)
			{
				Vector vector = Get3DPoint(x, y);

				if (selectedGraphStart != NULL && selectedGraphEnd != NULL)
				{
					if (selectedGraphEnd->graph == selectedGraphStart->graph)
					{
						if (selectedGraphEnd->point.x < 0)
							selectedGraphEnd->point.x = 0;

						float startX, endX;

						if (selectedGraphEnd->point.x > selectedGraphStart->point.x)
						{
							startX = selectedGraphStart->point.x;
							endX = selectedGraphEnd->point.x;														
						}
						else
						{
							startX = selectedGraphEnd->point.x;
							endX = selectedGraphStart->point.x;							
						}

						selectedGraphEnd->graph->SetGraphViewRangeXAxis(startX, endX);

						if (nearFarDataAnalyzer)
						{
							FrequencyRange centerFrequencyrange(nearFarDataAnalyzer->spectrumAnalyzer.maxFrequencyRange);

							centerFrequencyrange.lower += 500000;
							centerFrequencyrange.upper -= 500000;

							if (selectedGraphEnd->graph == spectrumboardGraph)
							{								
								selectedGraphEnd->graph->SetText(1, "Strongest Near vs Far Ranges:");
								selectedGraphEnd->graph->SetGraphFrequencyRangeText("%.2f-%.2fMHz", &centerFrequencyrange, 2);
							}
							else
								if (selectedGraphEnd->graph == leaderboardGraph)
								{
									selectedGraphEnd->graph->SetGraphFrequencyRangeText("Leaderboard: %.2f-%.2fMHz", &centerFrequencyrange, 1);
								}
								else
									if (selectedGraphEnd->graph == transitionboardGraph)
									{
										selectedGraphEnd->graph->SetGraphFrequencyRangeText("Transitionboard: %.2f-%.2fMHz", &centerFrequencyrange, 1);
									}
									else
									if (selectedGraphEnd->graph == allSessionsSpectrumRangeGraph)
										selectedGraphEnd->graph->SetGraphFrequencyRangeText("%.2f-%.2fMHz", &nearFarDataAnalyzer->spectrumAnalyzer.maxFrequencyRange, 3);
						}
						
					}

					selectedGraphEnd->graph->SetSelectedGraphRange(0, 0);					
				}

				if (selectedGraphStart != NULL)
				{
					selectedGraphStart->graph->SetSelectedGraphRange(0, 0);					
				}								

				DeleteSelectedGraphData();
			}
	}
	else		
		if (button == GLUT_RIGHT_BUTTON)
		{
			if (state == GLUT_UP)
			{
				Vector vector = Get3DPoint(x, y);

				selectedGraphStart = graphs->GetSelectedGraph(vector.x, vector.y, vector.z);

				if (selectedGraphStart != NULL)
				{
					selectedGraphStart->graph->ZoomOut();

					if (nearFarDataAnalyzer)
					{
						FrequencyRange centerFrequencyrange(nearFarDataAnalyzer->spectrumAnalyzer.maxFrequencyRange);

						centerFrequencyrange.lower += 500000;
						centerFrequencyrange.upper -= 500000;

						if (selectedGraphStart->graph == spectrumboardGraph)
						{							
							selectedGraphStart->graph->SetText(1, "Strongest Near vs Far Ranges:");
							selectedGraphStart->graph->SetGraphFrequencyRangeText("%.2f-%.2fMHz", &centerFrequencyrange, 2);
						}
						else
							if (selectedGraphStart->graph == leaderboardGraph)
							{
								selectedGraphStart->graph->SetGraphFrequencyRangeText("Leaderboard: %.2f-%.2fMHz", &centerFrequencyrange, 1);
							}
							else
								if (selectedGraphStart->graph == transitionboardGraph)
								{
									selectedGraphStart->graph->SetGraphFrequencyRangeText("Transitionboard: %.2f-%.2fMHz", &centerFrequencyrange, 1);
								}
								else
									if (selectedGraphStart->graph == allSessionsSpectrumRangeGraph)
										selectedGraphStart->graph->SetGraphFrequencyRangeText("%.2f-%.2fMHz", &nearFarDataAnalyzer->spectrumAnalyzer.maxFrequencyRange, 3);
					}
				}

				DeleteSelectedGraphData();
			}
		}
}

void MouseMotion(int x, int y)
{	
	SetModeNear();

	if (currentButton == GLUT_LEFT_BUTTON)
	{
		Vector vector = Get3DPoint(x, y);

		selectedGraph = graphs->GetSelectedGraph(vector.x, vector.y, vector.z);

		if (selectedGraph != NULL && selectedGraphStart != NULL && selectedGraph->graph == selectedGraphStart->graph)
		{
			selectedGraphEnd = selectedGraph;

			if (selectedGraphEnd->point.x < 0)
				selectedGraphEnd->point.x = 0;

			if (selectedGraphEnd->point.x > selectedGraphStart->point.x)
				selectedGraphEnd->graph->SetSelectedGraphRange(selectedGraphStart->point.x, selectedGraphEnd->point.x);
			else
				selectedGraphEnd->graph->SetSelectedGraphRange(selectedGraphEnd->point.x, selectedGraphStart->point.x);
		}		
	}
	
	prevX = x;
	prevY = y;
}

void GetControlKeys()
{
	ctrl = false;
	shift = false;
	alt = false;

	int mod = glutGetModifiers();

	if (mod != 0)
	{
		switch (mod)
		{
		case 1:
			shift = true;
			break;

		case 2:
			ctrl = true;
			break;

		case 4:
			alt = true;
			break;

			mod = 0;
		}
	}
}

bool Movement(int x, int y)
{
	if (abs(x - prevX) > 10 && abs(y - prevY) > 10)
		return true;

	return false;
}

void PassiveMouseMotion(int x, int y)
{
	if (nearFarDataAnalyzer && Movement(x, y))
		SetModeNear();

	GetControlKeys();

	if (ctrl)
	{
		xRot += (y - prevY) * rotScale;
		yRot += (x - prevX) * rotScale;
	}
	if (shift)
	{
		pos.x += (x - prevX)*translateScale;
		pos.y -= (y - prevY)*translateScale;
	}

	prevX = x;
	prevY = y;
}

void SetViewToGraph(Graph *graph, bool togglePos = true)
{
	if (graph)
	{
		graphs->ResetToUserDrawDepths();

		if (graph == previousGraphView && togglePos)
		{
			if (graph->automaticPlacement)
			{
				if (graph->view == GraphView::Front)
					graph->view = GraphView::Above;
				else if (graph->view == GraphView::Above)
					graph->view = GraphView::Front;
			}
			else
			{
				graph->view = GraphView::Side;
			}
		}

		pos.y = -(graph->pos.y + graph->height / 2);
		pos.z = -9000;

		fov = 14;
		graph->drawDepth = graph->userSetDepth;

		xRot = 0;
		yRot = 0;

		pos.x = -(graph->pos.x + graph->width / 2);

		switch (graph->view)
		{
		case (GraphView::Front):
			yRot = 0;
			graphs->SetVisibility(true);
			break;
		case (GraphView::Side):
			pos.x = combinedFFTGraphForBandwidth->width / 2;
			yRot = 90;
			graphs->SetVisibility(true);
			break;
		case (GraphView::Above):
			pos.y = -(graph->height / 2);
			yRot = 0;
			xRot = 90;

			graphs->SetVisibility(false);

			fov = 10;
			graph->visible = true;

			graph->SetDepth(graph->maxDepth, false);
			break;
		}

		previousGraphView = graph;
	}
}

void SetCenterView()
{
	if (resultsGraph)
		SetViewToGraph(resultsGraph, false);
	else
	{
		graphs->ResetToUserDrawDepths();
		graphs->SetVisibility(true);
		
		pos.x = -2247;
		pos.y = 2392;
		pos.z = -9000;

		fov = 42;		

		if (nearFarDataAnalyzer && nearFarDataAnalyzer->spectrumAnalyzer.deviceReceivers->count > 1)
		{
			fov += 10;
		}

		xRot = 0;
		yRot = 0;
	}
}

void SetSpectrumBoardView()
{	
	pos.x = 6;
	pos.y = 826;
	pos.z = -9000;

	fov = 12;	

	xRot = 0;
	yRot = 0;
}

void SetLeaderBoardView()
{
	//pos.x = -3750;
	pos.x = graphs->x + graphs->GetWidth() - Graphs::GRAPH_WIDTH / 10 + Graphs::GRAPH_WIDTH / 2;
	pos.x *= -1;

	pos.y = 826;
	pos.z = -9000;

	fov = 12;

	xRot = 0;
	yRot = 0;
}

void ToggleVisibilityAndDrawDepths(Graph *graph)
{
	if (graph->visible)
	{
		if (graph->drawDepth == graph->maxDepth)
			graph->SetDepth(1);
		else
		{
			graph->visible = false;
		}
	}
	else
	{
		graph->visible = true;
		graph->SetDepth(graph->maxDepth);
	}
}

void ProcessKey(unsigned char key, int x, int y)
{	
	GetControlKeys();

	if (nearFarDataAnalyzer)
	{
		SetModeNear();

		switch (key)
		{
		case ('1'):
			dataGraph->visible = !dataGraph->visible;
			break;
		case ('!'):
			SetViewToGraph(dataGraph);
			break;
		case ('2'):
			if (correlationGraph)
				correlationGraph->visible = !correlationGraph->visible;
			break;
		case ('@'):
			SetViewToGraph(correlationGraph);
			break;
		case ('3'):
			if (fftGraphForDevicesBandwidth)
			{
				ToggleVisibilityAndDrawDepths(fftGraphForDevicesBandwidth);
			}
			break;
		case ('#'):
			SetViewToGraph(fftGraphForDevicesBandwidth);
			break;
		case ('4'):
			if (combinedFFTGraphForBandwidth)
			{
				ToggleVisibilityAndDrawDepths(combinedFFTGraphForBandwidth);
			}
			break;
		case ('$'):
			SetViewToGraph(combinedFFTGraphForBandwidth);
			break;
		case ('5'):
			if (fftGraphStrengthsForDeviceRange)
			{
				ToggleVisibilityAndDrawDepths(fftGraphStrengthsForDeviceRange);
			}
			break;
		case('%'):
			SetViewToGraph(fftGraphStrengthsForDeviceRange);
			break;
		case ('6'):
			if (fftAverageGraphForDeviceRange)
			{
				ToggleVisibilityAndDrawDepths(fftAverageGraphForDeviceRange);
			}
			break;
		case('^'):
			SetViewToGraph(fftAverageGraphForDeviceRange, false);
			break;
		case ('7'):
			if (fftAverageGraphStrengthsForDeviceRange)
			{
				ToggleVisibilityAndDrawDepths(fftAverageGraphStrengthsForDeviceRange);
			}
			break;
		case('&'):
			SetViewToGraph(fftAverageGraphStrengthsForDeviceRange);
			break;
		case ('8'):
			if (spectrumRangeGraph)
			{
				ToggleVisibilityAndDrawDepths(spectrumRangeGraph);
			}
			break;
		case('*'):
			if (spectrumRangeGraph)
			{
				SetViewToGraph(spectrumRangeGraph, false);
			}
			break;
		case('('):
			SetViewToGraph(resultsGraphs[nextResultsGraphToViewIndex], false);

			nextResultsGraphToViewIndex++;
			nextResultsGraphToViewIndex %= resultsGraphsCount;

			break;
		case ('s'):
			nearFarDataAnalyzer->spectrumAnalyzer.deviceReceivers->Synchronize();
			break;
		case ('c'):
			correlating = !correlating;

			if (!correlating)
				nearFarDataAnalyzer->spectrumAnalyzer.deviceReceivers->Correlate(false);
			break;
		case ('r'):
			nearFarDataAnalyzer->ClearAllData();
			nearFarDataAnalyzer->spectrumAnalyzer.useRatios = !nearFarDataAnalyzer->spectrumAnalyzer.useRatios;
			nearFarDataAnalyzer->spectrumAnalyzer.usePhase = false;
			break;
		case ('p'):
			nearFarDataAnalyzer->ClearAllData();
			nearFarDataAnalyzer->spectrumAnalyzer.usePhase = !nearFarDataAnalyzer->spectrumAnalyzer.usePhase;
			nearFarDataAnalyzer->spectrumAnalyzer.useRatios = false;
			break;
		case ('n'):
			nearFarDataAnalyzer->SetMode(ReceivingDataMode::Near);
			break;
		case ('f'):
			nearFarDataAnalyzer->SetMode(ReceivingDataMode::Far);
			break;
		case ('N'):
			nearFarDataAnalyzer->spectrumAnalyzer.GetFFTSpectrumBuffer(0)->ClearData();
			nearFarDataAnalyzer->spectrumAnalyzer.GetFFTSpectrumBuffer(2)->ClearData();

			nearFarDataAnalyzer->strengthFFTBuffers->GetFFTSpectrumBuffer(0)->ClearData();
			nearFarDataAnalyzer->strengthFFTBuffers->GetFFTSpectrumBuffer(2)->ClearData();
			break;
		case ('F'):
			nearFarDataAnalyzer->spectrumAnalyzer.GetFFTSpectrumBuffer(1)->ClearData();
			nearFarDataAnalyzer->strengthFFTBuffers->GetFFTSpectrumBuffer(1)->ClearData();
			break;
		case (' '):
			paused = !paused;

			if (paused)
				nearFarDataAnalyzer->Pause();
			else
				nearFarDataAnalyzer->Resume();
			break;
		case ('a'):
			autoScale = !autoScale;
			graphs->SetAutoScale(autoScale);
			break;
		case ('e'):
			nearFarDataAnalyzer->spectrumAnalyzer.eegStrength = !nearFarDataAnalyzer->spectrumAnalyzer.eegStrength;
			graphs->SetAutoScale(autoScale);
			break;
		case ('C'):
			SetCenterView();
			break;
		case ('S'):
			SetSpectrumBoardView();
			break;
		case ('L'):
			SetLeaderBoardView();
			break;
		case ('l'):
			graphs->ToggleLabels();
			break;
		case (','):
			if (graphTransition == NULL)
				graphTransition = nearFarDataAnalyzer->transitions.last;
			else
			{
				if (graphTransition->previous)
					graphTransition = graphTransition->previous;
				else
					graphTransition = nearFarDataAnalyzer->transitions.last;
			}

			nearFarDataAnalyzer->ProcessTransitionsAndSetTransitionGraphsData(graphTransition);
			break;

		case ('.'):
			if (graphTransition == NULL)
				graphTransition = nearFarDataAnalyzer->transitions.first;
			else
			{
				if (graphTransition->next)
					graphTransition = graphTransition->next;
				else
					graphTransition = nearFarDataAnalyzer->transitions.first;
			}
				
			nearFarDataAnalyzer->ProcessTransitionsAndSetTransitionGraphsData(graphTransition);
		break;
		case ('R'):
			if (nearFarDataAnalyzer->reradiatedFrequencyRanges->count > 0)
			{
				nearFarDataAnalyzer->checkingReradiatedFrequencyRanges = !nearFarDataAnalyzer->checkingReradiatedFrequencyRanges;

				nearFarDataAnalyzer->checkingReradiatedFrequencyRangeIndex = 0;

				if (nearFarDataAnalyzer->checkingReradiatedFrequencyRanges)
					graphs->SetText(1, "Checking Reradiated Frequency After Current Scan......");
				else
					graphs->SetText(1, "Returning To Spectrum Scan......");
			}
		break;		
		case ('>'):
			nearFarDataAnalyzer->checkingReradiatedFrequencyRangeIndex++;

			if (nearFarDataAnalyzer->checkingReradiatedFrequencyRangeIndex >= nearFarDataAnalyzer->reradiatedFrequencyRanges->count)
				nearFarDataAnalyzer->checkingReradiatedFrequencyRangeIndex = 0;

			graphs->SetText(1, "Checking Reradiated Frequency After Current Scan......");
		break;
		case ('<'):
			if (nearFarDataAnalyzer->checkingReradiatedFrequencyRangeIndex > 0)
				nearFarDataAnalyzer->checkingReradiatedFrequencyRangeIndex--;
			else
				nearFarDataAnalyzer->checkingReradiatedFrequencyRangeIndex = nearFarDataAnalyzer->reradiatedFrequencyRanges->count-1;

			graphs->SetText(1, "Checking Reradiated Frequency After Current Scan......");
		break;
		case ('q'):
			exit(0);
			break;
		case (27):
			exit(0);
			break;
		default:
			break;
		}
	}
	else
	{
		switch (key)
		{
			case ('C'):
				SetCenterView();
			break;
		}
	}
}

bool WGLExtensionSupported(const char *extension_name)
{
	// this is pointer to function which returns pointer to string with list of all wgl extensions
	PFNWGLGETEXTENSIONSSTRINGEXTPROC _wglGetExtensionsStringEXT = NULL;

	// determine pointer to wglGetExtensionsStringEXT function
	_wglGetExtensionsStringEXT = (PFNWGLGETEXTENSIONSSTRINGEXTPROC)wglGetProcAddress("wglGetExtensionsStringEXT");

	if (strstr(_wglGetExtensionsStringEXT(), extension_name) == NULL)
	{
		// string was not found
		return false;
	}

	// extension is supported
	return true;
}

void InitializeGL(void)
{		
	glutInitDisplayMode(GLUT_DEPTH |
		GLUT_DOUBLE |
		GLUT_RGBA |
		GLUT_MULTISAMPLE);
	
	glutInitWindowSize(glutGet(GLUT_SCREEN_WIDTH), glutGet(GLUT_SCREEN_HEIGHT));

	glutCreateWindow("SDR Spectrum Analyzer");	

	GLenum err = glewInit();
	glutDisplayFunc(display);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glClearColor(0.0, 0.0, 0.0, 0.0);	

	glPointSize(5);
	
	glShadeModel(GL_SMOOTH);

	glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_diffuse);
	glMaterialfv(GL_FRONT, GL_AMBIENT, mat_ambient);
	glLightfv(GL_LIGHT0, GL_POSITION, light_position);
	
	glEnable(GL_LIGHT0);

	glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
	glEnable(GL_COLOR_MATERIAL);

	glEnable(GL_DEPTH_TEST);
	
	glMatrixMode(GL_MODELVIEW);
	gluLookAt(0.0, 0.0, 0.0,  
		0.0, 0.0, 0.0,      
		0.0, 1.0, 0.);

	glutMouseFunc(MouseButtons);
	glutMotionFunc(MouseMotion);
	glutPassiveMotionFunc(PassiveMouseMotion);
	glutKeyboardFunc(ProcessKey);
	
	glEnable(GL_MULTISAMPLE);

	PFNWGLSWAPINTERVALEXTPROC       wglSwapIntervalEXT = NULL;
	PFNWGLGETSWAPINTERVALEXTPROC    wglGetSwapIntervalEXT = NULL;

	if (WGLExtensionSupported("WGL_EXT_swap_control"))
	{
		// Extension is supported, init pointers.
		wglSwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC)wglGetProcAddress("wglSwapIntervalEXT");

		// this is another function from WGL_EXT_swap_control extension
		wglGetSwapIntervalEXT = (PFNWGLGETSWAPINTERVALEXTPROC)wglGetProcAddress("wglGetSwapIntervalEXT");
	}

	wglSwapIntervalEXT(1);
}

void ProcessSequenceFinished()
{
	nearFarDataAnalyzer->ProcessSequenceFinished();
}

void OnReceiverDataProcessed()
{
	nearFarDataAnalyzer->OnReceiverDataProcessed();
}

int InitializeNearFarSpectrumAnalyzerAndGraphs(uint32_t startFrequency, uint32_t endFrequency, uint32_t sampRate, DetectionMode detectionMode)
{
	if (startFrequency + sampRate > endFrequency)
	{
		endFrequency = startFrequency + sampRate;

		wchar_t textBuffer[255];

		swprintf(textBuffer, L"Frequency range adjusted to %i to %i.\n\nThis is due to the bandwidth (determined according to the set sample rate) exceeding the specified frequency range.\n\nSet the sample rate if necessary with the '-sr' command line option", startFrequency, endFrequency);

		MessageBox(nullptr, textBuffer, TEXT("Reradiation Spectrum Analyzer"), MB_ICONINFORMATION | MB_SETFOREGROUND | MB_TOPMOST | MB_SYSTEMMODAL);
	}

	nearFarDataAnalyzer = new NearFarDataAnalyzer();

	nearFarDataAnalyzer->detectionMode = detectionMode;
	
	int deviceCount = nearFarDataAnalyzer->InitializeNearFarDataAnalyzer(20000, sampRate, startFrequency, endFrequency);
	
	int graphResolution = DeviceReceiver::SAMPLE_RATE / DeviceReceiver::SEGMENT_BANDWIDTH * Graphs::GRAPH_SEGMENT_RESOLUTION;

	graphs = new Graphs();
	graphs->SetPos(0, 0, 0);
	graphs->graphsGridVertCount = 4;

	dataGraph = new Graph(1, DeviceReceiver::FFT_SEGMENT_SAMPLE_COUNT);
	dataGraph->showXAxis = 1;
	dataGraph->showYAxis = 1;
	dataGraph->showZAxis = 0;

	dataGraph->showXLabels = false;

	dataGraph->SetSize(Graphs::GRAPH_WIDTH, Graphs::GRAPH_HEIGHT);	

	dataGraph->SetDataSeriesColor(device1DataColor, 0);
	dataGraph->SetDataSeriesColor(device2DataColor, 1);

	dataGraph->SetDataSeriesStyle(GraphStyle::Area3D);

	dataGraph->SetText(1, "IQ Signal Data Waveform");


	if (nearFarDataAnalyzer->spectrumAnalyzer.deviceReceivers->count > 1)
	{
		correlationGraph = new Graph(1, DeviceReceiver::FFT_BUFF_LENGTH_FOR_DEVICE_BANDWIDTH);
		correlationGraph->showXAxis = 1;
		correlationGraph->showYAxis = 1;
		correlationGraph->showZAxis = 0;
		correlationGraph->SetSize(Graphs::GRAPH_WIDTH, Graphs::GRAPH_HEIGHT);

		correlationGraph->SetDataSeriesColor(correlationColor, 0);

		correlationGraph->SetDataSeriesStyle(GraphStyle::Area3D);

		correlationGraph->SetText(1, "Correlation Graph");
	}

	
	if (nearFarDataAnalyzer->spectrumAnalyzer.deviceReceivers->count > 1)
	{
		fftGraphForDevicesBandwidth = new Graph(100, graphResolution);
		fftGraphForDevicesBandwidth->drawDepth = 1;
		fftGraphForDevicesBandwidth->showXAxis = 1;
		fftGraphForDevicesBandwidth->showYAxis = 1;
		fftGraphForDevicesBandwidth->showZAxis = 2;

		fftGraphForDevicesBandwidth->SetSize(Graphs::GRAPH_WIDTH, Graphs::GRAPH_HEIGHT);

		if (nearFarDataAnalyzer->spectrumAnalyzer.deviceReceivers->count > 1)
		{
			fftGraphForDevicesBandwidth->SetDataSeriesColor(device1DataColor, 0);			
			fftGraphForDevicesBandwidth->SetDataSeriesColor(device2DataColor, 1);
		}

		fftGraphForDevicesBandwidth->SetDataSeriesStyle(GraphStyle::Graph3D);		

		fftGraphForDevicesBandwidth->SetText(1, "FFT Graph (Device 1 = Red, Device 2 = Blue)");
	}
	
	
	combinedFFTGraphForBandwidth = new Graph(200, graphResolution);	
	combinedFFTGraphForBandwidth->drawDepth = 1;
	combinedFFTGraphForBandwidth->showXAxis = 1;
	combinedFFTGraphForBandwidth->showYAxis = 1;
	combinedFFTGraphForBandwidth->showZAxis = 2;

	combinedFFTGraphForBandwidth->SetSize(Graphs::GRAPH_WIDTH, Graphs::GRAPH_HEIGHT);

	combinedFFTGraphForBandwidth->SetDataSeriesStyle(GraphStyle::Graph3D);
		
	if (nearFarDataAnalyzer->spectrumAnalyzer.deviceReceivers->count == 1)
	{
		combinedFFTGraphForBandwidth->SetText(1, "FFT Graph");
	}
	else
		combinedFFTGraphForBandwidth->SetText(1, "Combined FFT Graph for Devices");		

	
	fftGraphStrengthsForDeviceRange = new Graph(200, 10);
	fftGraphStrengthsForDeviceRange->view = GraphView::Side;	
	fftGraphStrengthsForDeviceRange->showXAxis = 0;
	fftGraphStrengthsForDeviceRange->showYAxis = 0;
	fftGraphStrengthsForDeviceRange->showZAxis = 0;
	fftGraphStrengthsForDeviceRange->showLabels = false;
	fftGraphStrengthsForDeviceRange->SetDataSeriesStyle(GraphStyle::Graph3D);	

	fftGraphStrengthsForDeviceRange->SetDataSeriesLineWidth(1);

	fftGraphStrengthsForDeviceRange->SetSize(100, Graphs::GRAPH_HEIGHT);

	fftGraphStrengthsForDeviceRange->userSetDepth = 100;
	
	fftGraphStrengthsForDeviceRange->automaticPlacement = false;


	fftAverageGraphForDeviceRange = new Graph(1, graphResolution, 3);
	fftAverageGraphForDeviceRange->drawDepth = 1;
	fftAverageGraphForDeviceRange->showXAxis = 1;
	fftAverageGraphForDeviceRange->showYAxis = 1;
	fftAverageGraphForDeviceRange->showZAxis = 2;

	fftAverageGraphForDeviceRange->SetSize(Graphs::GRAPH_WIDTH, Graphs::GRAPH_HEIGHT);

	fftAverageGraphForDeviceRange->SetDataSeriesStyle(GraphStyle::Graph3D);	

	fftAverageGraphForDeviceRange->SetDataSeriesLineWidth(2);
	
	fftAverageGraphForDeviceRange->SetDataSeriesColor(Graphs::nearColor, ReceivingDataMode::Near);
	fftAverageGraphForDeviceRange->SetDataSeriesColor(Graphs::Graphs::farColor, ReceivingDataMode::Far);
	fftAverageGraphForDeviceRange->SetDataSeriesColor(Graphs::reradiatedColor, 2);

	if (nearFarDataAnalyzer->spectrumAnalyzer.deviceReceivers->count == 1)
		fftAverageGraphForDeviceRange->SetText(1, "Averaged FFT For Current Scanning Range: ");

	
	fftAverageGraphStrengthsForDeviceRange = new Graph(200, 10, 3);
	fftAverageGraphStrengthsForDeviceRange->view = GraphView::Side;	

	fftAverageGraphStrengthsForDeviceRange->showXAxis = 0;
	fftAverageGraphStrengthsForDeviceRange->showYAxis = 0;
	fftAverageGraphStrengthsForDeviceRange->showZAxis = 0;
	fftAverageGraphStrengthsForDeviceRange->showLabels = false;
	fftAverageGraphStrengthsForDeviceRange->SetDataSeriesStyle(GraphStyle::Graph3D);
	fftAverageGraphStrengthsForDeviceRange->SetDataSeriesColor(Graphs::nearColor, ReceivingDataMode::Near);
	fftAverageGraphStrengthsForDeviceRange->SetDataSeriesColor(Graphs::farColor, ReceivingDataMode::Far);
	fftAverageGraphStrengthsForDeviceRange->SetDataSeriesColor(Graphs::undeterminedColor, ReceivingDataMode::Undetermined);

	fftAverageGraphStrengthsForDeviceRange->dataSeriesSameScale = false;

	fftAverageGraphStrengthsForDeviceRange->selectable = false;

	fftAverageGraphStrengthsForDeviceRange->SetDataSeriesLineWidth(1);

	fftAverageGraphStrengthsForDeviceRange->SetSize(100, Graphs::GRAPH_HEIGHT);

	fftAverageGraphStrengthsForDeviceRange->userSetDepth = 100;
	
	fftAverageGraphStrengthsForDeviceRange->automaticPlacement = false;
	
	
	spectrumRangeGraph = new Graph(1, nearFarDataAnalyzer->scanningRange.length / DeviceReceiver::SAMPLE_RATE * graphResolution, 3);		
	spectrumRangeGraph->drawDepth = 1;	
	spectrumRangeGraph->showXAxis = 1;
	spectrumRangeGraph->showYAxis = 1;
	spectrumRangeGraph->showZAxis = 2;

	spectrumRangeGraph->SetSize(Graphs::GRAPH_WIDTH, Graphs::GRAPH_HEIGHT);

	spectrumRangeGraph->SetDataSeriesStyle(GraphStyle::Graph3D);

	spectrumRangeGraph->SetDataSeriesColor(Graphs::nearColor, ReceivingDataMode::Near);
	spectrumRangeGraph->SetDataSeriesColor(Graphs::farColor, ReceivingDataMode::Far);		
	spectrumRangeGraph->SetDataSeriesColor(Graphs::reradiatedColor, 2);
	
	spectrumRangeGraph->SetText(1, "Spectrum Graph");
	spectrumRangeGraph->SetGraphXRange(startFrequency, endFrequency);


	spectrumRangeDifGraph = new Graph(1, nearFarDataAnalyzer->scanningRange.length / DeviceReceiver::SAMPLE_RATE * graphResolution, 2);
	spectrumRangeDifGraph->drawDepth = 1;
	spectrumRangeDifGraph->showXAxis = 1;
	spectrumRangeDifGraph->showYAxis = 1;
	spectrumRangeDifGraph->showZAxis = 2;

	spectrumRangeDifGraph->SetSize(Graphs::GRAPH_WIDTH, Graphs::GRAPH_HEIGHT);

	spectrumRangeDifGraph->SetDataSeriesStyle(GraphStyle::Graph3D);
	
	spectrumRangeDifGraph->SetDataSeriesColor(Graphs::reradiatedColor, 0);
	spectrumRangeDifGraph->SetDataSeriesColor(Graphs::farColor, 1);

	spectrumRangeDifGraph->SetText(1, "Spectrum Range Difference Graph");
	spectrumRangeDifGraph->SetGraphXRange(startFrequency, endFrequency);

	resultsGraphs[resultsGraphsCount++] = spectrumRangeDifGraph;
	

	spectrumboardGraph = new Graph(1, nearFarDataAnalyzer->scanningRange.length / DeviceReceiver::SEGMENT_BANDWIDTH);
	spectrumboardGraph->drawDepth = 1;
	spectrumboardGraph->showXAxis = 1;
	spectrumboardGraph->showYAxis = 1;
	spectrumboardGraph->showZAxis = 2;

	spectrumboardGraph->SetSize(Graphs::GRAPH_WIDTH, Graphs::GRAPH_HEIGHT);

	spectrumboardGraph->SetDataSeriesStyle(GraphStyle::Graph3D);

	spectrumboardGraph->SetDataSeriesColor(Graphs::nearColor, ReceivingDataMode::Near);
	spectrumboardGraph->SetDataSeriesColor(Graphs::farColor, ReceivingDataMode::Far);

	spectrumboardGraph->SetText(1, "Strongest Near vs Far Ranges Graph");
	spectrumboardGraph->SetGraphXRange(startFrequency + 500000, endFrequency - 500000);


	resultsGraphs[resultsGraphsCount++] = spectrumboardGraph;	

	uint32_t strengthGraphsResolution = BandwidthFFTBuffer::FFT_ARRAYS_COUNT;

	strengthGraph = new Graph(1, strengthGraphsResolution, 2);
	strengthGraph->drawDepth = 1;
	strengthGraph->showXAxis = 1;
	strengthGraph->showYAxis = 1;
	strengthGraph->showZAxis = 2;

	strengthGraph->showXLabels = false;

	strengthGraph->SetSize(Graphs::GRAPH_WIDTH, Graphs::GRAPH_HEIGHT);
	strengthGraph->iValueUseForColors = IValueUseForColors::Index;

	strengthGraph->SetDataSeriesStyle(GraphStyle::Graph3D);

	strengthGraph->SetDataSeriesColor(Graphs::nearColor, 0, ReceivingDataMode::Near);
	strengthGraph->SetDataSeriesColor(Graphs::farColor, 0, ReceivingDataMode::Far);
	strengthGraph->SetDataSeriesColor(Graphs::undeterminedColor, 0, ReceivingDataMode::Undetermined);

	strengthGraph->dataSeriesSameScale = false;

	strengthGraph->SetText(1, "FFT Range Strength Graph");
	strengthGraph->SetGraphXRange(0, 1000);

	resultsGraphs[resultsGraphsCount++] = strengthGraph;


	transitionGraph = new Graph(1, strengthGraphsResolution, 3);
	transitionGraph->drawDepth = 1;
	transitionGraph->showXAxis = 1;
	transitionGraph->showYAxis = 1;
	transitionGraph->showZAxis = 2;

	transitionGraph->showXLabels = false;

	transitionGraph->SetSize(Graphs::GRAPH_WIDTH, Graphs::GRAPH_HEIGHT);
	transitionGraph->iValueUseForColors = IValueUseForColors::Index;

	transitionGraph->SetDataSeriesStyle(GraphStyle::Graph3D);
	
	transitionGraph->SetDataSeriesColor(Graphs::reradiatedColor, 0, ReceivingDataMode::Near);
	transitionGraph->SetDataSeriesColor(Graphs::farColor, 0, ReceivingDataMode::Far);
	transitionGraph->SetDataSeriesColor(Graphs::undeterminedColor, 0, ReceivingDataMode::Undetermined);

	transitionGraph->SetDataSeriesColor(Graphs::transitionNearFarDifColor, 1, 0);
	transitionGraph->SetDataSeriesColor(Graphs::transitionNearFarDifColor, 1, 1);
	transitionGraph->SetDataSeriesColor(Graphs::transitionNearFarDifColor, 1, 2);

	transitionGraph->SetText(1, "Transition Strength Graph");
	transitionGraph->SetGraphXRange(0, 1000);
	
	resultsGraphs[resultsGraphsCount++] = transitionGraph;


	averageTransitionsGraph = new Graph(1, strengthGraphsResolution, 3);
	averageTransitionsGraph->drawDepth = 1;
	averageTransitionsGraph->showXAxis = 1;
	averageTransitionsGraph->showYAxis = 1;
	averageTransitionsGraph->showZAxis = 2;

	averageTransitionsGraph->showXLabels = false;

	averageTransitionsGraph->SetSize(Graphs::GRAPH_WIDTH, Graphs::GRAPH_HEIGHT);
	averageTransitionsGraph->iValueUseForColors = IValueUseForColors::Index;

	averageTransitionsGraph->SetDataSeriesStyle(GraphStyle::Graph3D);

	averageTransitionsGraph->SetDataSeriesColor(Graphs::reradiatedColor, 0, ReceivingDataMode::Near);
	averageTransitionsGraph->SetDataSeriesColor(Graphs::farColor, 0, ReceivingDataMode::Far);
	averageTransitionsGraph->SetDataSeriesColor(Graphs::undeterminedColor, 0, ReceivingDataMode::Undetermined);


	averageTransitionsGraph->SetDataSeriesColor(Graphs::transitionNearFarDifColor, 1, 0);
	averageTransitionsGraph->SetDataSeriesColor(Graphs::transitionNearFarDifColor, 1, 1);
	averageTransitionsGraph->SetDataSeriesColor(Graphs::transitionNearFarDifColor, 1, 2);

	averageTransitionsGraph->SetText(1, "Averaged Transitions Strength Graph");
	averageTransitionsGraph->SetGraphXRange(0, 1000);

	resultsGraphs[resultsGraphsCount++] = averageTransitionsGraph;


	strengthFFTGraph = new Graph(1, strengthGraphsResolution, 1);
	strengthFFTGraph->drawDepth = 1;
	strengthFFTGraph->showXAxis = 1;
	strengthFFTGraph->showYAxis = 1;
	strengthFFTGraph->showZAxis = 2;

	strengthFFTGraph->SetSize(Graphs::GRAPH_WIDTH, Graphs::GRAPH_HEIGHT);

	strengthFFTGraph->SetDataSeriesColor(1, 0, 0, 1, 0);
	
	strengthFFTGraph->SetText(1, "Strength FFT Graph");
	
	resultsGraphs[resultsGraphsCount++] = strengthFFTGraph;


	strengthFFTAveragedGraph = new Graph(1, strengthGraphsResolution, 2);
	strengthFFTAveragedGraph->drawDepth = 1;
	strengthFFTAveragedGraph->showXAxis = 1;
	strengthFFTAveragedGraph->showYAxis = 1;
	strengthFFTAveragedGraph->showZAxis = 2;

	strengthFFTAveragedGraph->SetSize(Graphs::GRAPH_WIDTH, Graphs::GRAPH_HEIGHT);
	
	strengthFFTAveragedGraph->SetDataSeriesColor(Graphs::nearColor, ReceivingDataMode::Near);
	strengthFFTAveragedGraph->SetDataSeriesColor(Graphs::farColor, ReceivingDataMode::Far);

	strengthFFTAveragedGraph->SetText(1, "Strength FFT Averaged Graph");
	
	resultsGraphs[resultsGraphsCount++] = strengthFFTAveragedGraph;


	transitionboardGraph = new Graph(1, nearFarDataAnalyzer->scanningRange.length / DeviceReceiver::SEGMENT_BANDWIDTH, 1);
	transitionboardGraph->drawDepth = 1;
	transitionboardGraph->showXAxis = 1;
	transitionboardGraph->showYAxis = 1;
	transitionboardGraph->showZAxis = 2;

	transitionboardGraph->SetSize(Graphs::GRAPH_WIDTH, Graphs::GRAPH_HEIGHT);

	transitionboardGraph->SetDataSeriesStyle(GraphStyle::Points3D);

	transitionboardGraph->SetDataSeriesColor(1, 0, 0, 1, 0);
	
	transitionboardGraph->SetText(1, "Transitionboard Graph");
	transitionboardGraph->SetGraphXRange(startFrequency + 500000, endFrequency - 500000);

	resultsGraphs[resultsGraphsCount++] = transitionboardGraph;	

	if (nearFarDataAnalyzer->detectionMode == DetectionMode::Sessions)
	{
		allSessionsSpectrumRangeGraph = new Graph(1, nearFarDataAnalyzer->scanningRange.length / DeviceReceiver::SAMPLE_RATE * graphResolution);
		allSessionsSpectrumRangeGraph->drawDepth = 1;
		allSessionsSpectrumRangeGraph->showXAxis = 1;
		allSessionsSpectrumRangeGraph->showYAxis = 1;
		allSessionsSpectrumRangeGraph->showZAxis = 2;

		allSessionsSpectrumRangeGraph->SetSize(Graphs::GRAPH_WIDTH, Graphs::GRAPH_HEIGHT);

		allSessionsSpectrumRangeGraph->SetDataSeriesStyle(GraphStyle::Graph3D);

		allSessionsSpectrumRangeGraph->SetDataSeriesColor(1, 0, 0, 1, ReceivingDataMode::Near);
		allSessionsSpectrumRangeGraph->SetDataSeriesColor(0, 1, 0, 1, ReceivingDataMode::Far);

		allSessionsSpectrumRangeGraph->SetText(1, "Averaged Spectrum for Sessions");
		allSessionsSpectrumRangeGraph->SetGraphXRange(startFrequency, endFrequency);		

		resultsGraphs[resultsGraphsCount++] = allSessionsSpectrumRangeGraph;
	}


	leaderboardGraph = new Graph(1, nearFarDataAnalyzer->scanningRange.length / DeviceReceiver::SEGMENT_BANDWIDTH);
	leaderboardGraph->drawDepth = 1;
	leaderboardGraph->showXAxis = 1;
	leaderboardGraph->showYAxis = 1;
	leaderboardGraph->showZAxis = 2;

	leaderboardGraph->SetSize(Graphs::GRAPH_WIDTH, Graphs::GRAPH_HEIGHT);

	leaderboardGraph->SetDataSeriesStyle(GraphStyle::Graph3D);

	leaderboardGraph->SetDataSeriesColor(1, 0, 0, 1, ReceivingDataMode::Near);
	leaderboardGraph->SetDataSeriesColor(0, 1, 0, 1, ReceivingDataMode::Far);

	leaderboardGraph->SetText(1, "Leaderboard Graph");
	leaderboardGraph->SetGraphXRange(startFrequency + 500000, endFrequency - 500000);

	resultsGraphs[resultsGraphsCount++] = leaderboardGraph;


	graphs->AddGraph(dataGraph);
	if (correlationGraph)
		graphs->AddGraph(correlationGraph);
	if (fftGraphForDevicesBandwidth)
		graphs->AddGraph(fftGraphForDevicesBandwidth);
	graphs->AddGraph(combinedFFTGraphForBandwidth);
	
	graphs->AddGraph(fftGraphStrengthsForDeviceRange);
	fftGraphStrengthsForDeviceRange->SetPos(combinedFFTGraphForBandwidth->pos.x + Graphs::GRAPH_STRENGTHS_X_OFFSET, combinedFFTGraphForBandwidth->pos.y, 0);

	graphs->AddGraph(fftAverageGraphForDeviceRange);
	graphs->AddGraph(fftAverageGraphStrengthsForDeviceRange);
	fftAverageGraphStrengthsForDeviceRange->SetPos(fftAverageGraphForDeviceRange->pos.x + Graphs::GRAPH_STRENGTHS_X_OFFSET, fftAverageGraphForDeviceRange->pos.y, fftAverageGraphStrengthsForDeviceRange->pos.z);

	graphs->AddGraph(spectrumRangeGraph);
	graphs->AddGraph(spectrumRangeDifGraph);	
	graphs->AddGraph(strengthGraph);
	graphs->AddGraph(transitionGraph);
	graphs->AddGraph(averageTransitionsGraph);	
	graphs->AddGraph(spectrumboardGraph);	
	graphs->AddGraph(strengthFFTGraph);
	graphs->AddGraph(strengthFFTAveragedGraph);
	if (allSessionsSpectrumRangeGraph)
		graphs->AddGraph(allSessionsSpectrumRangeGraph);
	graphs->AddGraph(transitionboardGraph);
	graphs->AddGraph(leaderboardGraph);

	SetCenterView();

	nearFarDataAnalyzer->graphs = graphs;
	nearFarDataAnalyzer->spectrumAnalyzer.deviceReceivers->dataGraph = dataGraph;
	nearFarDataAnalyzer->spectrumAnalyzer.deviceReceivers->correlationGraph = correlationGraph;
	nearFarDataAnalyzer->spectrumAnalyzer.deviceReceivers->fftGraphForDevicesBandwidth = fftGraphForDevicesBandwidth;
	nearFarDataAnalyzer->spectrumAnalyzer.deviceReceivers->combinedFFTGraphForBandwidth = combinedFFTGraphForBandwidth;
	nearFarDataAnalyzer->spectrumAnalyzer.deviceReceivers->fftAverageGraphForDeviceRange = fftAverageGraphForDeviceRange;
	nearFarDataAnalyzer->spectrumAnalyzer.deviceReceivers->fftGraphStrengthsForDeviceRange = fftGraphStrengthsForDeviceRange;
	nearFarDataAnalyzer->spectrumAnalyzer.deviceReceivers->fftAverageGraphStrengthsForDeviceRange = fftAverageGraphStrengthsForDeviceRange;
	nearFarDataAnalyzer->spectrumAnalyzer.deviceReceivers->spectrumRangeGraph = spectrumRangeGraph;
	nearFarDataAnalyzer->spectrumAnalyzer.deviceReceivers->spectrumRangeDifGraph = spectrumRangeDifGraph;
	nearFarDataAnalyzer->spectrumAnalyzer.deviceReceivers->allSessionsSpectrumRangeGraph = allSessionsSpectrumRangeGraph;	
	nearFarDataAnalyzer->strengthGraph = strengthGraph;
	nearFarDataAnalyzer->strengthFFTGraph = strengthFFTGraph;
	nearFarDataAnalyzer->strengthFFTAveragedGraph = strengthFFTAveragedGraph;
	nearFarDataAnalyzer->transitionGraph = transitionGraph;
	nearFarDataAnalyzer->averageTransitionsGraph = averageTransitionsGraph;
	nearFarDataAnalyzer->transitionboardGraph = transitionboardGraph;
	nearFarDataAnalyzer->leaderboardGraph = leaderboardGraph;
	nearFarDataAnalyzer->spectrumboardGraph = spectrumboardGraph;

	nearFarDataAnalyzer->StartProcessing();

	nearFarDataAnalyzer->spectrumAnalyzer.SetSequenceFinishedFunction(&ProcessSequenceFinished);
	nearFarDataAnalyzer->spectrumAnalyzer.SetOnReceiverDataProcessed(&OnReceiverDataProcessed);

	return 1;
}

LRESULT CALLBACK LowLevelMouseProc(int nCode, WPARAM wParam, LPARAM lParam)
{	
	SetModeNear();

	return CallNextHookEx(NULL, nCode, wParam, lParam);
}

LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	SetModeNear();
	
	return CallNextHookEx(NULL, nCode, wParam, lParam);
}

void Initialize(uint32_t startFrequency, uint32_t endFrequency, uint32_t sampRate, DetectionMode detectionMode)
{	
	InitializeNearFarSpectrumAnalyzerAndGraphs(startFrequency, endFrequency, sampRate, detectionMode);
	
	#if !defined(_DEBUG) || defined(RELEASE_SETTINGS)
		//Key and mouse detection
		HHOOK hhkLowLevelKybd = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, 0, 0);
		HHOOK hhkLowLevelMouse = SetWindowsHookEx(WH_MOUSE_LL, LowLevelMouseProc, 0, 0);
	#endif
}

void Close(void)
{
	if (nearFarDataAnalyzer)
		delete nearFarDataAnalyzer;

	delete graphs;
}

void Usage(int argc, char **argv)
{	
	printf("To connect a compatible device using GNU Radio first launch the flowgraph GNURadioDeviceFlowgraph.grc(in the 'GNURadioDeviceFlowgraph\\' folder)\n");
	printf("then the launch.bat files or SDRSpectrumAnalyzerOpenGL.exe\n");
	printf("\n");
	printf("Usage: SDRSpectrumAnalyzerOpenGL [-a] [-m] [-f] [-s] [-e] [-S]\n", argv[0]);		
	printf("-a: Automatically detect reradiated frequency ranges\n");
	printf("-m: Scanning Mode [normal, sessions]\n");
	printf("-f: Required frames for sessions\n");
	printf("-s: Start frequency\n");
	printf("-e: End frequency\n");
	printf("-S: Sound cues for detecting increasing signal strengths\n");	
	printf("-rg: show the averaged previous results graphs using the data automatically saved in the 'results\' folder (more data produces brighter lines)\n");
	printf("\n");
	printf("In Program Keys:\n", argv[0]);
	printf("\n");
	printf("1: Toggle data graph\n");
	printf("2: Toggle correlation graph (only for synchronizable devices)\n");
	printf("3: Toggle devices fft graphs' graphics (only for synchronizable devices)\n");
	printf("4: Toggle fft graph graphics (off/2D/3D)\n");
	printf("5: Toggle strength graph graphics (off/2D/3D)\n");
	printf("6: Toggle average fft graph graphics (off/2D/3D)\n");
	printf("7: Toggle average strength graph graphics (off/2D/3D)\n");
	printf("8: Toggle full spectrum range graph graphics (off/2D/3D)\n");
	printf("\n");
	printf("SHIFT 1: Set view to data graph\n");
	printf("SHIFT 2: Set view to correlation graph (only for synchronizable devices)\n");
	printf("SHIFT 3: Toggle view of devices fft graphs or waterfall of FFT (only for synchronizable devices)\n");
	printf("SHIFT 4: Toggle view of fft graph or waterfall of FFT\n");
	printf("SHIFT 5: Set view to strength graph\n");
	printf("SHIFT 6: Toggle view of average fft graph or waterfall of average FFT\n");
	printf("SHIFT 7: Set view to average strength graph\n");
	printf("SHIFT 8: Toggle view of full spectrum range graph or waterfall of spectrum\n");
	printf("SHIFT 9: Toggle the results graphs(Spectrumboard Graph, All Sessions SpectrumRange Graph and the Leaderboard Graph)\n");
	printf("SHIFT S: Set view to current session's strongest reradiated frequencies\n");
	printf("SHIFT L: Set view to leaderBoard for strongest reradiated frequencies of all sessions\n");
	printf("SHIFT C: Set view to all graphs\n");			
	printf("\n");
	printf("SHIFT and mouse moves graphs\n");
	printf("CTRL and mouse rotates graphs\n");
	printf("\n");
	printf("Left mouse button and drag on graph selects frequency range and zooms in\n");	
	printf("Right mouse button on graph zooms out\n");
	printf("\n");
	printf("Mouse wheel zooms in/out graphics\n");
	printf("\n");
	printf("n: Record near series data (for non-automated detection)\n");
	printf("f: Record far series data (for non-automated detection)\n");
	printf("\n");
	printf("l: Toggle Graph Labels\n");
	printf("\n");
	printf("SHIFT N: Clear near series data\n");
	printf("SHIFT F: Clear far series data\n");
	printf("\n");
	printf(",: Cycle transitions back\n");
	printf(".: Cycle transitions forward\n");
	printf("\n");
	printf("R: Check Reradiated Frequencies. Sets the SDR to the reradiated frequency so that the effect on strength levels of moving nearer and further from the antenna can be determined\n");
	printf("\n");
	printf("<: Cycle reradiated frequencies back\n");
	printf(">: Cycle reradiated frequencies forward\n");
	printf("\n");	
	printf("e.g., SDRSpectrumAnalyzerOpenGL.exe -a -s 430000000 -e 470000000");
	printf("\n");
	printf("press any key to continue\n");

	getch();
}

string makeMeString(GLint versionRaw)
{
	char str[255];

	itoa(versionRaw, str, 10);

	return str;
}


void formatMe(string *text)
{
	string dot = ".";

	text->insert(1, dot); // transforms 30000 into 3.0000
	text->insert(4, dot); // transforms 3.0000 into 3.00.00
}

/**
* Message
*/
void consoleMessage()
{
	const char *versionGL = "\0";
	GLint versionFreeGlutInt = 0;

	versionGL = (char *)(glGetString(GL_VERSION));
	
	DebuggingUtilities::DebugPrint("OpenGL version: %s\n", versionGL);	
}

void ProcessFrequencyRangeData(char *rangeData, FrequencyRanges *frequencyRanges)
{
	char *endChar;
	uint32_t start, end;
	double strength;
	uint32_t frames;

	start = strtol(rangeData, &rangeData, 10);
	end = strtol(rangeData, &rangeData, 10);	
	strength = strtod(rangeData, &rangeData);	
	frames = 1;

	frequencyRanges->Add(start, end, strength, frames);
}

void LoadFile(const char *fileName, FrequencyRanges *frequencyRanges)
{
	FILE * pFile;
	const uint32_t lineLength = 255;

	char textBuffer[lineLength];	

	pFile = fopen(fileName, "r+");
		
	while (fgets(textBuffer, lineLength, pFile))
	{
		if (strlen(textBuffer)>10)
			ProcessFrequencyRangeData(textBuffer, frequencyRanges);
	}
	
	fclose(pFile);
}

void LoadResults(FrequencyRanges* frequencyRanges)
{
	char dataFolderAndFilename[255];	

	HANDLE hFind;
	strcpy(dataFolderAndFilename, "results/*");
	uint8_t dataFolderLength = strlen(dataFolderAndFilename);

	WIN32_FIND_DATAA ffd;

	int index;
	int counter = 0;
	hFind = ::FindFirstFileA(dataFolderAndFilename, &ffd);
	if (hFind != INVALID_HANDLE_VALUE)
	{
		do
		{
			counter++;

			if (counter > 2)
			{
				strcpy(&dataFolderAndFilename[dataFolderLength - 1], ffd.cFileName);
				if (strcmp(&dataFolderAndFilename[strlen(dataFolderAndFilename)-4], ".txt")==0)								
					LoadFile(dataFolderAndFilename, frequencyRanges);
			}

		} while (::FindNextFileA(hFind, &ffd) == TRUE);

		::FindClose(hFind);
	}

	if (counter > 2)
		counter -= 2;

	counter++;
}

void ShowResultsGraph()
{
	FrequencyRanges frequencyRanges(10000);	
	LoadResults(&frequencyRanges);

	frequencyRanges.QuickSort(QuickSortMode::Frequency);
	
	uint32_t resultsLength = frequencyRanges.count * 2;
	double *resultsPoints = new double[resultsLength];

	frequencyRanges.GetStrengthValuesAndFrames(resultsPoints);	

	for (int i = 0; i < frequencyRanges.count; i++) //average the strength values for the frames
		resultsPoints[i * 2] /= resultsPoints[i * 2 + 1];

	//convert frames to an alpha value, so that the data with more frames is more visible 
	uint32_t maxFrames = 0;
	for (int i = 0; i < frequencyRanges.count; i++)
	{
		if (resultsPoints[i * 2 + 1] > maxFrames)
			maxFrames = resultsPoints[i * 2 + 1];		
	}

	for (int i = 0; i < frequencyRanges.count; i++)
	{
		resultsPoints[i * 2 + 1] = (float) resultsPoints[i * 2 + 1] / maxFrames;			
	}

	graphs = new Graphs();
	graphs->SetPos(0, 0, 0);

	resultsGraph = new Graph(1, frequencyRanges.count);
	resultsGraph->drawDepth = 1;
	resultsGraph->showXAxis = 1;
	resultsGraph->showYAxis = 1;
	resultsGraph->showZAxis = 2;

	resultsGraph->SetSize(Graphs::GRAPH_WIDTH, Graphs::GRAPH_HEIGHT);

	resultsGraph->SetDataSeriesStyle(GraphStyle::Graph3D);
	resultsGraph->iValueUseForColors = IValueUseForColors::Alpha;

	resultsGraph->SetDataSeriesColor(1, 0, 0, 1, ReceivingDataMode::Near);
	resultsGraph->SetDataSeriesColor(0, 1, 0, 1, ReceivingDataMode::Far);

	resultsGraph->SetText(0, "Results Graph");
	//resultsGraph->SetGraphXRange(frequencyRanges.GetFrequencyRangeFromIndex(0)->centerFrequency, frequencyRanges.GetFrequencyRangeFromIndex(frequencyRanges.count-1)->centerFrequency);
	resultsGraph->SetGraphXRange(frequencyRanges.GetFrequencyRangeFromIndex(0)->centerFrequency, frequencyRanges.GetFrequencyRangeFromIndex(frequencyRanges.count - 1)->centerFrequency);

	graphs->AddGraph(resultsGraph);

	resultsGraph->SetData((void *)resultsPoints, resultsLength, 0, true, 0, 0, true, SignalProcessingUtilities::DataType::DOUBLE);
	resultsGraph->SetData((void *)resultsPoints, resultsLength, 0, true, 0, 0, true, SignalProcessingUtilities::DataType::DOUBLE);
	
	SetViewToGraph(resultsGraph);
}

int main(int argc, char **argv)
{	
	uint32_t startFrequency = 420000000;
	uint32_t endFrequency = 460000000;

	/*FrequencyRange currentBandwidthRange;

	currentBandwidthRange.Set(startFrequency, startFrequency + DeviceReceiver::SEGMENT_BANDWIDTH);

	Transitions transitions;
	Transition* transition;
	Transition* maxStrengthTransition = NULL;

	do
	{
		transition = transitions.Add(NULL, Transitions::DURATION, &currentBandwidthRange, SignalProcessingUtilities::GetDataIndexFromFrequency(currentBandwidthRange.lower, startFrequency, endFrequency, 0, DeviceReceiver::FFT_SEGMENT_SAMPLE_COUNT), SignalProcessingUtilities::GetDataIndexFromFrequency(currentBandwidthRange.upper, startFrequency, endFrequency, 0, DeviceReceiver::FFT_SEGMENT_SAMPLE_COUNT));

		currentBandwidthRange.Set(currentBandwidthRange.lower + DeviceReceiver::SEGMENT_BANDWIDTH, currentBandwidthRange.upper + DeviceReceiver::SEGMENT_BANDWIDTH);
	} while (currentBandwidthRange.lower < endFrequency);
	*/


	if (argc < 2)
	{
		Usage(argc, argv);
		return 0;
	}

	

	bool automatedDetection = false;
	DetectionMode detectionMode = DetectionMode::Normal;
	uint32_t requiredFramesForSessions = 1000;
	bool resultsGraph = false;
	bool sound = false;

	int argc2 = 1;
	char *argv2[1] = { (char*)"" };
	glutInit(&argc2, argv2);	
	InitializeGL();	

	for (uint8_t i = 0; i < argc; i++)
	{
		if (strcmp(argv[i], "-?") == 0)
		{
			Usage(argc, argv);
			return 0;
		}

		if (strcmp(argv[i], "-a") == 0)
			automatedDetection = true;

		if (strcmp(argv[i], "-m") == 0)
		{
			const char* scanningModeString = argv[++i];

			if (strcmp(scanningModeString, "sessions") == 0)
				detectionMode = DetectionMode::Sessions;			
		}

		if (strcmp(argv[i], "-f") == 0)
			requiredFramesForSessions = atoi(argv[++i]);

		if (strcmp(argv[i], "-s") == 0)
			startFrequency = atoi(argv[++i]);

		if (strcmp(argv[i], "-e") == 0)
			endFrequency = atoi(argv[++i]);

		if (strcmp(argv[i], "-S") == 0)
			sound = true;

		if (strcmp(argv[i], "-sr") == 0)
		{
			DeviceReceiver::SAMPLE_RATE = atoi(argv[++i]);
		}

		if (strcmp(argv[i], "-rg") == 0)
			resultsGraph = true;
	}

	if (resultsGraph)
	{
		ShowResultsGraph();
	}
	else
	{		
		Initialize(startFrequency, endFrequency, DeviceReceiver::SAMPLE_RATE, detectionMode);

		if (nearFarDataAnalyzer)
		{
			nearFarDataAnalyzer->automatedDetection = automatedDetection;

			nearFarDataAnalyzer->requiredFramesForSessions = requiredFramesForSessions;

			nearFarDataAnalyzer->spectrumAnalyzer.sound = sound;
		}
	}

	glutMainLoop();

	return 0;
}
