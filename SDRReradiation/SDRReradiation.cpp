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


/*#ifndef CALLBACK
#if defined(_ARM_)
#define CALLBACK
#else
#define CALLBACK __stdcall
#endif
#endif
*/

#ifdef _WIN32
    #include "GL/glew.h"
    #include "freeglut/include/GL/glut.h"
#else
    #include "GL/glxew.h"
    #include <GL/glut.h>
#endif // _WIN32


#include <time.h>
 #include <string>

#include "NearFarDataAnalyzer.h"
#include "Graphs.h"
#include "SignalProcessingUtilities.h"
#include "DebuggingUtilities.h"
#include "GraphicsUtilities.h"
#include "GNU_Radio_Utilities.h"
#include "ThreadUtilities.h"
#include "WindowsToLinuxUtilities.h"

GNU_Radio_Utilities gnuUtilities;
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

float msgBoxWidth = 800;
float msgBoxXStart = -msgBoxWidth/2;
float msgBoxHeight = 100;

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

bool Movement(int x, int y)
{
    if (prevX != -1 && prevY != -1)
        if (abs(x - prevX) > 10 && abs(y - prevY) > 10)
            return true;

	return false;
}

void DrawFrequenciesRangeBoard(FrequencyRanges* frequencyRanges, float x, float y, float z, char* string, char* string2)
{
	float textStartHeight = y;

	glColor4f(1.0, 1.0, 1.0, 1.0);
	GraphicsUtilities::DrawText(string, x, textStartHeight, z, Graphs::FONT_SCALE, 0);

	int labelHeight = glutStrokeWidth(GLUT_STROKE_ROMAN, (int)'A') * Graphs::FONT_SCALE;
	textStartHeight -= labelHeight * 3;

	GraphicsUtilities::DrawText(string2, x, textStartHeight, z, Graphs::FONT_SCALE, 0);

	x += glutStrokeWidth(GLUT_STROKE_ROMAN, (int)textBuffer[0]) * Graphs::FONT_SCALE;
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
			GraphicsUtilities::DrawText(textBuffer, x, textStartHeight, z, Graphs::FONT_SCALE, 0);
		}
	}

	glColor4f(1.0, 1.0, 1.0, 1.0);
}

/*void GetControlKeys()
{
	ctrl = false;
	shift = false;
	alt = false;

	int mod = glutGetModifiers();

	if (mod & GLUT_ACTIVE_CTRL)
        ctrl = true;

	if (mod != 0)
	{
		switch (mod)
		{
		case 1:
			shift = true;
			break;

		case GLUT_ACTIVE_CTRL:
			ctrl = true;
			break;

		case 4:
			alt = true;
			break;

			mod = 0;
		}
	}
}
*/


static void msgWndResize(int width, int height)
{
    glutSetWindow(nearFarDataAnalyzer->glutMsgWindowID);

    const float ar = (float) width / (float) height;

    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    glOrtho(msgBoxXStart, msgBoxXStart + msgBoxWidth, -msgBoxHeight/2, msgBoxHeight/2, 1.0, 10.0);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity() ;
}

void msgWndDisplay(void)
{
    glutSetWindow(nearFarDataAnalyzer->glutMsgWindowID);

    #ifdef _WIN32
        HWND hWnd = FindWindow(NULL, "SDR Reradiation Spectrum Analyzer Messages");
        SetForegroundWindow(hWnd);
        glutPopWindow();
    #endif

 	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

 	glMatrixMode(GL_MODELVIEW);

		gluLookAt(0.0, 0.0, 0.0,
			0.0, 0.0, 0.0,
			0.0, 1.0, 0.);

		glLoadIdentity();

    glColor3f(Graphs::reradiatedColor.r, Graphs::reradiatedColor.g, Graphs::reradiatedColor.b);

    glLineWidth(3);

	float textStartHeight;
	float fontScale = Graphs::FONT_SCALE;

    float labelHeight = glutStrokeWidth(GLUT_STROKE_ROMAN, 'H');
	textStartHeight = fontScale * 0.5;//(labelHeight * fontScale * 1.5);

    char textBuffer[255];

        uint32_t inactiveSecondsMSGTime = NearFarDataAnalyzer::INACTIVE_NOTIFICATION_MSG_TIME / 1000;
        sprintf(textBuffer, "Processing data as far series in %i seconds", inactiveSecondsMSGTime);
        GraphicsUtilities::DrawText(textBuffer, msgBoxXStart + labelHeight, textStartHeight, -2, fontScale, 0);

        textStartHeight -= (labelHeight * fontScale * 1.5);
        sprintf(textBuffer, "Press a key/button or move the mouse if near");
        GraphicsUtilities::DrawText(textBuffer, msgBoxXStart + labelHeight, textStartHeight, -2, fontScale, 0);

	glutSwapBuffers();

	glutPostRedisplay();
}


void display(void)
{
    glutSetWindow(nearFarDataAnalyzer->glutWindowID);

	currentTime = GetTime();

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

				if (graphs->showLabels)
				{
					if (nearFarDataAnalyzer->detectionMode == Sessions)
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
					}

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

	glutSwapBuffers();

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

void MouseButtons(int button, int state, int x, int y)
{
	currentButton = button;

    nearFarDataAnalyzer->SetModeNear();

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

void PassiveMouseMotion(int x, int y)
{
    prevX = x;
	prevY = y;
}

void MouseMotion(int x, int y)
{
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
	else
        if (currentButton == GLUT_RIGHT_BUTTON)
        {
            xRot += (y - prevY) * rotScale;
            yRot += (x - prevX) * rotScale;
        }
        else if (currentButton == GLUT_MIDDLE_BUTTON)
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
				if (graph->view == FrontView)
					graph->view = AboveView;
				else if (graph->view == AboveView)
					graph->view = FrontView;
			}
			else
			{
				graph->view = SideView;
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
		case (FrontView):
			yRot = 0;
			graphs->SetVisibility(true);
			break;
		case (SideView):
			pos.x = combinedFFTGraphForBandwidth->width / 2;
			yRot = 90;
			graphs->SetVisibility(true);
			break;
		case (AboveView):
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
		if (graph->drawDepth == 1)
            graph->SetDepth(graph->maxDepth);
		else
		{
			graph->visible = false;
		}
	}
	else
	{
		graph->visible = true;
		graph->SetDepth(1);
	}
}

void ProcessKey(unsigned char key, int x, int y)
{
	if (nearFarDataAnalyzer)
	{
		nearFarDataAnalyzer->SetModeNear();

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
			nearFarDataAnalyzer->SetMode(Near);
			break;
		case ('f'):
			nearFarDataAnalyzer->SetMode(Far);
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
					graphs->SetText(4, "Checking Reradiated Frequency After Current Scan......");
				else
					graphs->SetText(4, "Returning To Spectrum Scan......");
			}
		break;
		case ('>'):
			nearFarDataAnalyzer->checkingReradiatedFrequencyRangeIndex++;

			if (nearFarDataAnalyzer->checkingReradiatedFrequencyRangeIndex >= nearFarDataAnalyzer->reradiatedFrequencyRanges->count)
				nearFarDataAnalyzer->checkingReradiatedFrequencyRangeIndex = 0;

			graphs->SetText(4, "Checking Reradiated Frequency After Current Scan......");
		break;
		case ('<'):
			if (nearFarDataAnalyzer->checkingReradiatedFrequencyRangeIndex > 0)
				nearFarDataAnalyzer->checkingReradiatedFrequencyRangeIndex--;
			else
				nearFarDataAnalyzer->checkingReradiatedFrequencyRangeIndex = nearFarDataAnalyzer->reradiatedFrequencyRanges->count-1;

			graphs->SetText(4, "Checking Reradiated Frequency After Current Scan......");
		break;
		/*case ('q'):
			exit(0);
			break;
		case (27):
			exit(0);
			break;
			*/
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

/*bool WGLExtensionSupported(const char *extension_name)
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
}*/

void windowStatusFunction(int state)
{
    if (!nearFarDataAnalyzer->ignoreNextWindowStateChange)
        nearFarDataAnalyzer->glutWindowState = state;
    else
        nearFarDataAnalyzer->ignoreNextWindowStateChange = false;
}

static void idle(void)
{
    /*char textBuffer[255];
        uint32_t inactiveSecondsMSGTime = NearFarDataAnalyzer::INACTIVE_NOTIFICATION_MSG_TIME / 1000;

        sprintf(textBuffer, "Processing far series data in %i seconds. Press a key or move the mouse if near", inactiveSecondsMSGTime);
        graphs->SetText(3, textBuffer);
*/
    if (nearFarDataAnalyzer->processingFarDataMessageBoxEventState == LAUNCH)
    {
        char textBuffer[255];
        uint32_t inactiveSecondsMSGTime = NearFarDataAnalyzer::INACTIVE_NOTIFICATION_MSG_TIME / 1000;

        sprintf(textBuffer, "Processing far series data in %i seconds. Press a key or move the mouse if near", inactiveSecondsMSGTime);
        graphs->SetText(3, textBuffer);

        glutSetWindow(nearFarDataAnalyzer->glutMsgWindowID);
        glutShowWindow();
        glutPopWindow();
        glutPopWindow();
        glutPopWindow();

        nearFarDataAnalyzer->processingFarDataMessageBoxEventState = SHOW;
    }
    else if (nearFarDataAnalyzer->processingFarDataMessageBoxEventState == CLOSE)
    {
        graphs->SetText(3, "\0");

        nearFarDataAnalyzer->CloseProcessingFarDataMessageBox();
    }
}

const GLfloat light_ambient[]  = { 0.0f, 0.0f, 0.0f, 1.0f };
const GLfloat light_diffuse[]  = { 1.0f, 1.0f, 1.0f, 1.0f };
const GLfloat light_specular[] = { 1.0f, 1.0f, 1.0f, 1.0f };
const GLfloat light_position2[] = { 2.0f, 5.0f, 5.0f, 0.0f };

const GLfloat mat_ambient2[]    = { 0.7f, 0.7f, 0.7f, 1.0f };
const GLfloat mat_diffuse2[]    = { 0.8f, 0.8f, 0.8f, 1.0f };
const GLfloat mat_specular2[]   = { 1.0f, 1.0f, 1.0f, 1.0f };
const GLfloat high_shininess[] = { 100.0f };

void CreateMsgBoxWnd()
{
    //First create a message box window
    glutInitWindowSize(glutGet(GLUT_SCREEN_WIDTH)/2, glutGet(GLUT_SCREEN_HEIGHT)/4);
    nearFarDataAnalyzer->glutMsgWindowID = glutCreateWindow("SDR Reradiation Spectrum Analyzer Messages");
    //nearFarDataAnalyzer->glutMsgWindowID = glutCreateWindow("SDRMessages");
    glutSetWindow(nearFarDataAnalyzer->glutMsgWindowID);

    glutReshapeFunc(msgWndResize);
    glutDisplayFunc(msgWndDisplay);
    glClearColor(0,0,0,1);

    #ifdef _WIN32
        HWND hWnd = FindWindow(NULL, "SDR Reradiation Spectrum Analyzer Messages");

        RECT rc;
        GetWindowRect ( hWnd, &rc ) ;

        int xPos = GetSystemMetrics(SM_CXSCREEN)/2;
        xPos -= (rc.right - rc.left)/2;

        int yPos = GetSystemMetrics(SM_CYSCREEN)/2;
        yPos -= (rc.bottom - rc.top)/2;

        SetWindowPos( hWnd, HWND_TOP, xPos, yPos, 0, 0, SWP_NOZORDER | SWP_NOSIZE );
    #endif

    glutHideWindow();
}

void InitializeGL(void)
{
	glutInitDisplayMode(GLUT_DEPTH |
		GLUT_DOUBLE |
		GLUT_RGBA |
		GLUT_MULTISAMPLE);

    CreateMsgBoxWnd();

    //Now create the graphics window
    glutInitWindowSize(glutGet(GLUT_SCREEN_WIDTH), glutGet(GLUT_SCREEN_HEIGHT));
    nearFarDataAnalyzer->glutWindowID = glutCreateWindow("SDR Reradiation Spectrum Analyzer");
    glutSetWindow(nearFarDataAnalyzer->glutWindowID);
    nearFarDataAnalyzer->glutWindowState = GLUT_VISIBLE;
    glutWindowStatusFunc(windowStatusFunction);

	GLenum err = glewInit();

	float glVersion = atof((const char *)glGetString(GL_VERSION));

    Graphs::DRAWING_GRAPHS_WITH_BUFFERS = glVersion >= 1.5;

	glutDisplayFunc(display);
	glutIdleFunc(idle);

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
	glutKeyboardFunc(ProcessKey);
	glutPassiveMotionFunc(PassiveMouseMotion);

	glEnable(GL_MULTISAMPLE);

	/*////PFNWGLSWAPINTERVALEXTPROC       wglSwapIntervalEXT = NULL;
	PFNWGLGETSWAPINTERVALEXTPROC    wglGetSwapIntervalEXT = NULL;

	if (WGLExtensionSupported("WGL_EXT_swap_control"))
	{
		// Extension is supported, init pointers.
		wglSwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC)wglGetProcAddress("wglSwapIntervalEXT");

		// this is another function from WGL_EXT_swap_control extension
		wglGetSwapIntervalEXT = (PFNWGLGETSWAPINTERVALEXTPROC)wglGetProcAddress("wglGetSwapIntervalEXT");
	}

	wglSwapIntervalEXT(1);*/
}

void ProcessSequenceFinished()
{
	nearFarDataAnalyzer->ProcessSequenceFinished();
}

void OnReceiverDataProcessed()
{
	nearFarDataAnalyzer->OnReceiverDataProcessed();
}

int InitializeNearFarSpectrumAnalyzer(uint32_t startFrequency, uint32_t endFrequency, uint32_t sampRate, DetectionMode detectionMode)
{
    ////printf("InitializeNearFarSpectrumAnalyzer()\n");
	if (startFrequency + sampRate > endFrequency)
	{
		endFrequency = startFrequency + sampRate;

		char textBuffer[255];

		sprintf(textBuffer, "Frequency range adjusted to %i to %i.\n\nThis is due to the bandwidth (determined according to the set sample rate) exceeding the specified frequency range.\n\nSet the sample rate if necessary with the '-sr' command line option", startFrequency, endFrequency);
	}

	nearFarDataAnalyzer = new NearFarDataAnalyzer();

	nearFarDataAnalyzer->detectionMode = detectionMode;

	////printf("deviceCount = ");
	////int deviceCount = 0;
	int deviceCount = nearFarDataAnalyzer->InitializeNearFarDataAnalyzer(20000, sampRate, startFrequency, endFrequency);

	return deviceCount;
}

int InitializeGraphs(uint32_t startFrequency, uint32_t endFrequency)
{
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

	dataGraph->SetDataSeriesStyle(Area3D);

	dataGraph->SetText(1, "IQ Signal Data Waveform");

	if (nearFarDataAnalyzer->spectrumAnalyzer.deviceReceivers->count > 1)
	{
		correlationGraph = new Graph(1, DeviceReceiver::FFT_BUFF_LENGTH_FOR_DEVICE_BANDWIDTH);
		correlationGraph->showXAxis = 1;
		correlationGraph->showYAxis = 1;
		correlationGraph->showZAxis = 0;
		correlationGraph->SetSize(Graphs::GRAPH_WIDTH, Graphs::GRAPH_HEIGHT);

		correlationGraph->SetDataSeriesColor(correlationColor, 0);

		correlationGraph->SetDataSeriesStyle(Area3D);

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

		fftGraphForDevicesBandwidth->SetDataSeriesStyle(Graph3D);

		fftGraphForDevicesBandwidth->SetText(1, "FFT Graph (Device 1 = Red, Device 2 = Blue)");
	}


	combinedFFTGraphForBandwidth = new Graph(200, graphResolution);
	combinedFFTGraphForBandwidth->drawDepth = 1;
	combinedFFTGraphForBandwidth->showXAxis = 1;
	combinedFFTGraphForBandwidth->showYAxis = 1;
	combinedFFTGraphForBandwidth->showZAxis = 2;

	combinedFFTGraphForBandwidth->SetSize(Graphs::GRAPH_WIDTH, Graphs::GRAPH_HEIGHT);

	combinedFFTGraphForBandwidth->SetDataSeriesStyle(Graph3D);

	if (nearFarDataAnalyzer->spectrumAnalyzer.deviceReceivers->count == 1)
	{
		combinedFFTGraphForBandwidth->SetText(1, "FFT Graph");
	}
	else
		combinedFFTGraphForBandwidth->SetText(1, "Combined FFT Graph for Devices");


	fftGraphStrengthsForDeviceRange = new Graph(1000, 10);
	fftGraphStrengthsForDeviceRange->view = SideView;
	fftGraphStrengthsForDeviceRange->showXAxis = 0;
	fftGraphStrengthsForDeviceRange->showYAxis = 0;
	fftGraphStrengthsForDeviceRange->showZAxis = 0;
	fftGraphStrengthsForDeviceRange->showLabels = false;
	fftGraphStrengthsForDeviceRange->SetDataSeriesStyle(Graph3D);

	fftGraphStrengthsForDeviceRange->SetDataSeriesLineWidth(1);

	fftGraphStrengthsForDeviceRange->SetSize(100, Graphs::GRAPH_HEIGHT);

	fftGraphStrengthsForDeviceRange->userSetDepth = 1000;

	fftGraphStrengthsForDeviceRange->automaticPlacement = false;


	fftAverageGraphForDeviceRange = new Graph(1, graphResolution, 3);
	fftAverageGraphForDeviceRange->drawDepth = 1;
	fftAverageGraphForDeviceRange->showXAxis = 1;
	fftAverageGraphForDeviceRange->showYAxis = 1;
	fftAverageGraphForDeviceRange->showZAxis = 2;

	fftAverageGraphForDeviceRange->SetSize(Graphs::GRAPH_WIDTH, Graphs::GRAPH_HEIGHT);

	fftAverageGraphForDeviceRange->SetDataSeriesStyle(Graph3D);

	fftAverageGraphForDeviceRange->SetDataSeriesLineWidth(2);

	fftAverageGraphForDeviceRange->SetDataSeriesColor(Graphs::nearColor, Near);
	fftAverageGraphForDeviceRange->SetDataSeriesColor(Graphs::Graphs::farColor, Far);
	fftAverageGraphForDeviceRange->SetDataSeriesColor(Graphs::reradiatedColor, 2);

	if (nearFarDataAnalyzer->spectrumAnalyzer.deviceReceivers->count == 1)
		fftAverageGraphForDeviceRange->SetText(1, "Averaged FFT For Current Scanning Range: ");


	fftAverageGraphStrengthsForDeviceRange = new Graph(200, 10, 3);
	fftAverageGraphStrengthsForDeviceRange->view = SideView;

	fftAverageGraphStrengthsForDeviceRange->showXAxis = 0;
	fftAverageGraphStrengthsForDeviceRange->showYAxis = 0;
	fftAverageGraphStrengthsForDeviceRange->showZAxis = 0;
	fftAverageGraphStrengthsForDeviceRange->showLabels = false;
	fftAverageGraphStrengthsForDeviceRange->SetDataSeriesStyle(Graph3D);
	fftAverageGraphStrengthsForDeviceRange->SetDataSeriesColor(Graphs::nearColor, Near);
	fftAverageGraphStrengthsForDeviceRange->SetDataSeriesColor(Graphs::farColor, Far);
	fftAverageGraphStrengthsForDeviceRange->SetDataSeriesColor(Graphs::undeterminedColor, Undetermined);

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

	spectrumRangeGraph->SetDataSeriesStyle(Graph3D);

	spectrumRangeGraph->SetDataSeriesColor(Graphs::nearColor, Near);
	spectrumRangeGraph->SetDataSeriesColor(Graphs::farColor, Far);
	spectrumRangeGraph->SetDataSeriesColor(Graphs::reradiatedColor, 2);

	spectrumRangeGraph->SetText(1, "Spectrum Graph");
	spectrumRangeGraph->SetGraphXRange(startFrequency, endFrequency);


	spectrumRangeDifGraph = new Graph(1, nearFarDataAnalyzer->scanningRange.length / DeviceReceiver::SAMPLE_RATE * graphResolution, 2);
	spectrumRangeDifGraph->drawDepth = 1;
	spectrumRangeDifGraph->showXAxis = 1;
	spectrumRangeDifGraph->showYAxis = 1;
	spectrumRangeDifGraph->showZAxis = 2;

	spectrumRangeDifGraph->SetSize(Graphs::GRAPH_WIDTH, Graphs::GRAPH_HEIGHT);

	spectrumRangeDifGraph->SetDataSeriesStyle(Graph3D);

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

	spectrumboardGraph->SetDataSeriesStyle(Graph3D);

	spectrumboardGraph->SetDataSeriesColor(Graphs::nearColor, Near);
	spectrumboardGraph->SetDataSeriesColor(Graphs::farColor, Far);

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
	strengthGraph->iValueUseForColors = Index;

	strengthGraph->SetDataSeriesStyle(Graph3D);

	strengthGraph->SetDataSeriesColor(Graphs::nearColor, 0, Near);
	strengthGraph->SetDataSeriesColor(Graphs::farColor, 0, Far);
	strengthGraph->SetDataSeriesColor(Graphs::undeterminedColor, 0, Undetermined);

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
	transitionGraph->iValueUseForColors = Index;

	transitionGraph->SetDataSeriesStyle(Graph3D);

	transitionGraph->SetDataSeriesColor(Graphs::reradiatedColor, 0, Near);
	transitionGraph->SetDataSeriesColor(Graphs::farColor, 0, Far);
	transitionGraph->SetDataSeriesColor(Graphs::undeterminedColor, 0, Undetermined);

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
	averageTransitionsGraph->iValueUseForColors = Index;

	averageTransitionsGraph->SetDataSeriesStyle(Graph3D);

	averageTransitionsGraph->SetDataSeriesColor(Graphs::reradiatedColor, 0, Near);
	averageTransitionsGraph->SetDataSeriesColor(Graphs::farColor, 0, Far);
	averageTransitionsGraph->SetDataSeriesColor(Graphs::undeterminedColor, 0, Undetermined);


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

	strengthFFTAveragedGraph->SetDataSeriesColor(Graphs::nearColor, Near);
	strengthFFTAveragedGraph->SetDataSeriesColor(Graphs::farColor, Far);

	strengthFFTAveragedGraph->SetText(1, "Strength FFT Averaged Graph");

	resultsGraphs[resultsGraphsCount++] = strengthFFTAveragedGraph;


	transitionboardGraph = new Graph(1, nearFarDataAnalyzer->scanningRange.length / DeviceReceiver::SEGMENT_BANDWIDTH, 1);
	transitionboardGraph->drawDepth = 1;
	transitionboardGraph->showXAxis = 1;
	transitionboardGraph->showYAxis = 1;
	transitionboardGraph->showZAxis = 2;

	transitionboardGraph->SetSize(Graphs::GRAPH_WIDTH, Graphs::GRAPH_HEIGHT);

	transitionboardGraph->SetDataSeriesStyle(Points3D);

	transitionboardGraph->SetDataSeriesColor(1, 0, 0, 1, 0);

	transitionboardGraph->SetText(1, "Transitionboard Graph");
	transitionboardGraph->SetGraphXRange(startFrequency + 500000, endFrequency - 500000);

	resultsGraphs[resultsGraphsCount++] = transitionboardGraph;

	if (nearFarDataAnalyzer->detectionMode == Sessions)
	{
		allSessionsSpectrumRangeGraph = new Graph(1, nearFarDataAnalyzer->scanningRange.length / DeviceReceiver::SAMPLE_RATE * graphResolution);
		allSessionsSpectrumRangeGraph->drawDepth = 1;
		allSessionsSpectrumRangeGraph->showXAxis = 1;
		allSessionsSpectrumRangeGraph->showYAxis = 1;
		allSessionsSpectrumRangeGraph->showZAxis = 2;

		allSessionsSpectrumRangeGraph->SetSize(Graphs::GRAPH_WIDTH, Graphs::GRAPH_HEIGHT);

		allSessionsSpectrumRangeGraph->SetDataSeriesStyle(Graph3D);

		allSessionsSpectrumRangeGraph->SetDataSeriesColor(1, 0, 0, 1, Near);
		allSessionsSpectrumRangeGraph->SetDataSeriesColor(0, 1, 0, 1, Far);

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

	leaderboardGraph->SetDataSeriesStyle(Graph3D);

	leaderboardGraph->SetDataSeriesColor(1, 0, 0, 1, Near);
	leaderboardGraph->SetDataSeriesColor(0, 1, 0, 1, Far);

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

	nearFarDataAnalyzer->spectrumAnalyzer.SetSequenceFinishedFunction(&ProcessSequenceFinished);
	nearFarDataAnalyzer->spectrumAnalyzer.SetOnReceiverDataProcessed(&OnReceiverDataProcessed);

	return 1;
}

void Initialize(uint32_t startFrequency, uint32_t endFrequency, uint32_t sampRate, DetectionMode detectionMode)
{
    ////printf("Initialize()\n");
	InitializeNearFarSpectrumAnalyzer(startFrequency, endFrequency, sampRate, detectionMode);
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
	printf("Usage: SDRSpectrumAnalyzerOpenGL [-a] [-m] [-f] [-s] [-e] [-c] [-sr] [-S] [-rg]\n", argv[0]);
	printf("-a: Automatically detect reradiated frequency ranges\n");
	printf("-m: Scanning Mode [normal, sessions]\n");
	printf("-f: Required frames for sessions\n");
	printf("-s: Start frequency\n");
	printf("-e: End frequency\n");
	printf("-c: Center frequency, sets a specific frequency range equal to specified frequency +- bandwidth(sr)/2\n");
	printf("-sr: Sample Rate\n");
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
	printf("Center mouse button and mouse moves graphs\n");
	printf("Right mouse button and mouse rotates graphs\n");
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

	getchar();
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

#ifdef _WIN32
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
#endif

void ShowResultsGraph()
{
	FrequencyRanges frequencyRanges(10000);

	#ifdef _WIN32
        LoadResults(&frequencyRanges);
    #endif

	frequencyRanges.QuickSort(Frequency);

	uint32_t resultsLength = frequencyRanges.count * 2;
	double *resultsPoints = new double[resultsLength];

	frequencyRanges.GetStrengthValuesAndFrames(resultsPoints);

	for (int i = 0; i < frequencyRanges.count; i++) //average the strength values for the frames
		resultsPoints[i * 2] /= resultsPoints[i * 2 + 1];

	//convert frames to an alpha value, so that the data with more frames is more visible
	uint32_t maxFrames = 0;
	for (uint32_t i = 0; i < frequencyRanges.count; i++)
	{
		if (resultsPoints[i * 2 + 1] > maxFrames)
			maxFrames = resultsPoints[i * 2 + 1];
	}

	for (uint32_t i = 0; i < frequencyRanges.count; i++)
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

	resultsGraph->SetDataSeriesStyle(Graph3D);
	resultsGraph->iValueUseForColors = Alpha;

	resultsGraph->SetDataSeriesColor(1, 0, 0, 1, Near);
	resultsGraph->SetDataSeriesColor(0, 1, 0, 1, Far);

	resultsGraph->SetText(0, "Results Graph");
	resultsGraph->SetGraphXRange(frequencyRanges.GetFrequencyRangeFromIndex(0)->centerFrequency, frequencyRanges.GetFrequencyRangeFromIndex(frequencyRanges.count - 1)->centerFrequency);

	graphs->AddGraph(resultsGraph);

	resultsGraph->SetData((void *)resultsPoints, resultsLength, 0, true, 0, 0, true, SignalProcessingUtilities::DOUBLE);
	resultsGraph->SetData((void *)resultsPoints, resultsLength, 0, true, 0, 0, true, SignalProcessingUtilities::DOUBLE);

	SetViewToGraph(resultsGraph);
}


int main(int argc, char **argv)
{
    #ifdef _WIN32
        WSADATA w;
        /* Open windows connection */
        if (WSAStartup(0x0101, &w) != 0)
        {
            fprintf(stderr, "Could not open Windows connection.\n");

            exit(0);
        }

        /*////
        SOCKET sd;
	struct sockaddr_in serv_addr;

	struct sockaddr_in serv_addr2;

	closesocket(sd);
	memset((char *) &serv_addr, '\0', sizeof(serv_addr));

    serv_addr.sin_addr.s_addr = inet_addr(GNU_Radio_Utilities::GNU_RADIO_XMLRPC_SERVER_ADDRESS);
    serv_addr.sin_family = AF_INET;

    serv_addr.sin_port = htons(GNU_Radio_Utilities::GNU_RADIO_XMLRPC_SERVER_ADDRESS_PORT);

	sd = socket(AF_INET, SOCK_STREAM, 0);

	if (sd == -1)
	{
		UnitializeSockets();

		////printf(callingFunction);
		printf("main()1: ");

        printf("CallXMLRPC(): Could not create socket");// to GNU RADIO server\nlaunch GNURadioDeviceFlowgraph.grc for connecting to SDRs other than rtl sdrs\n");

		////return "Socket Error";
    }

        GNU_Radio_Utilities gnuUtilities2;
        gnuUtilities2.CreateSocket("main()2");


        gnuUtilities.CreateSocket("main()3");
        */

    #endif // _WIN32

	uint32_t startFrequency = 420000000;
	uint32_t endFrequency = 460000000;

    uint32_t centerFrequency = 0;

	if (argc < 2)
	{
		Usage(argc, argv);
		return 0;
	}

	bool automatedDetection = false;
	DetectionMode detectionMode = Normal;
	uint32_t requiredFramesForSessions = 1000;
	bool resultsGraph = false;
	bool sound = false;

	int argc2 = 1;
	char *argv2[1] = { (char*)"" };

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
				detectionMode = Sessions;
		}

		if (strcmp(argv[i], "-f") == 0)
			requiredFramesForSessions = atoi(argv[++i]);

		if (strcmp(argv[i], "-s") == 0)
			startFrequency = atoi(argv[++i]);

		if (strcmp(argv[i], "-e") == 0)
			endFrequency = atoi(argv[++i]);

        if (strcmp(argv[i], "-c") == 0)
			centerFrequency = atoi(argv[++i]);

		if (strcmp(argv[i], "-S") == 0)
			sound = true;

		if (strcmp(argv[i], "-sr") == 0)
		{
			DeviceReceiver::SAMPLE_RATE = atoi(argv[++i]);
		}

		if (strcmp(argv[i], "-rg") == 0)
			resultsGraph = true;
	}

	uint32_t segment_bandwidth = DeviceReceiver::SEGMENT_BANDWIDTH = 128000;

        do
        {
            segment_bandwidth += 128000;

            while (DeviceReceiver::SAMPLE_RATE % segment_bandwidth !=0 && segment_bandwidth < DeviceReceiver::SAMPLE_RATE)
                segment_bandwidth += 128000;

            if (segment_bandwidth > DeviceReceiver::SAMPLE_RATE)
            {
                segment_bandwidth -= 128000;

                DeviceReceiver::SAMPLE_RATE = segment_bandwidth;
            }

            if (segment_bandwidth > DeviceReceiver::SEGMENT_BANDWIDTH)
                DeviceReceiver::SEGMENT_BANDWIDTH = segment_bandwidth;

        }while (DeviceReceiver::SEGMENT_BANDWIDTH < 1024000);


	if (centerFrequency != 0)
    {
        startFrequency = centerFrequency - DeviceReceiver::SAMPLE_RATE / 2;
        endFrequency = centerFrequency + DeviceReceiver::SAMPLE_RATE / 2;
    }

	if (resultsGraph)
	{
		ShowResultsGraph();
	}
	else
	{
        uint32_t frequencyRange = endFrequency - startFrequency;

        double numberOfSegments = (double) (frequencyRange) / DeviceReceiver::SAMPLE_RATE;

        numberOfSegments = ceil(numberOfSegments);

        double newFrequencyRange =  numberOfSegments * DeviceReceiver::SAMPLE_RATE;

        endFrequency = startFrequency + ceil(newFrequencyRange);

        ////gnuUtilities.CreateSocket("main() 2");
        ////printf("Initialize()");
		Initialize(startFrequency, endFrequency, DeviceReceiver::SAMPLE_RATE, detectionMode);


		////gnuUtilities.CreateSocket("main() 2");

		if (nearFarDataAnalyzer)
		{
			nearFarDataAnalyzer->automatedDetection = automatedDetection;

			nearFarDataAnalyzer->requiredFramesForSessions = requiredFramesForSessions;

			////original nearFarDataAnalyzer->spectrumAnalyzer.SetSound(sound);
		}
	}

	glutInit(&argc2, argv2);
	InitializeGL();

	glClearColor(0.0, 0.0, 0.0, 1.0);
    glLineWidth(3);

    InitializeGraphs(startFrequency, endFrequency);
    nearFarDataAnalyzer->StartProcessing();

    glutSetWindow(nearFarDataAnalyzer->glutWindowID);

	glutMainLoop();


	return 0;
}
