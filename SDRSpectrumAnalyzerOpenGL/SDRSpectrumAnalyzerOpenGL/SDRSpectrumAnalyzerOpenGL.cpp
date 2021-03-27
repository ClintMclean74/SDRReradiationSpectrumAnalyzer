#include "glew-2.1.0\include\GL/glew.h"
#include <GL/glut.h>

#include <time.h>
#include<conio.h>
#include <process.h>
#include "NearFarDataAnalyzer.h"
#include "Graphs.h"
#include "SignalProcessingUtilities.h"
#include "DebuggingUtilities.h"
#include "GraphicsUtilities.h"

NearFarDataAnalyzer* nearFarDataAnalyzer;

bool paused = false;

Graphs* graphs = new Graphs();
Graph *dataGraph, *fftGraphForDeviceRange, *fftGraphForDevicesRange, *correlationGraph, *fftGraphStrengthsForDeviceRange, *fftAverageGraphForDeviceRange, *fftAverageGraphStrengthsForDeviceRange;
Graph* spectrumRangeGraph;
Graph* allSessionsSpectrumRangeGraph;

SelectedGraph *selectedGraph;
SelectedGraph* selectedGraphStart;
SelectedGraph* selectedGraphEnd;

uint32_t defaultSelectedStartIndex = 0;
uint32_t defaultSelectedEndIndex = 50;

bool drawDataGraph = true;
bool drawCorrelationGraph = true;
bool drawFFTGraphForDevicesRanges = true;
bool drawFFTGraphForDeviceRanges = true;
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

const INT16 GRAPH_HEIGHT = 700;
const INT16 GRAPH_STRENGTHS_X_OFFSET = -100;

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

void DrawFrequenciesRangeBoard(FrequencyRanges* frequencyRanges, float x, float y, float z, const char* string, const char* string2)
{
	float textStartHeight = y;

	snprintf(textBuffer, sizeof(textBuffer), string);	
	GraphicsUtilities::DrawText(textBuffer, x, textStartHeight, z, GraphicsUtilities::fontScale, 0);

	int labelHeight = glutStrokeWidth(GLUT_STROKE_ROMAN, (int)'A') * GraphicsUtilities::fontScale;
	textStartHeight -= labelHeight * 3;

	snprintf(textBuffer, sizeof(textBuffer), string2);
	GraphicsUtilities::DrawText(textBuffer, x, textStartHeight, z, GraphicsUtilities::fontScale, 0);

	x += glutStrokeWidth(GLUT_STROKE_ROMAN, (int)textBuffer[0]) * GraphicsUtilities::fontScale;
	textStartHeight -= labelHeight * 3;

	FrequencyRange *frequencyRange;

	for (uint8_t i = 0; i < frequencyRanges->count; i++)
	{		
		frequencyRange = frequencyRanges->GetFrequencyRangeFromIndex(i);
		if (frequencyRange)
		{
			textStartHeight -= labelHeight * 3;

			//snprintf(textBuffer, sizeof(textBuffer), "%.4d %f", frequencyRange->lower, frequencyRange->strength);
			snprintf(textBuffer, sizeof(textBuffer), "   %.4d", frequencyRange->lower);
			GraphicsUtilities::DrawText(textBuffer, x, textStartHeight, z, GraphicsUtilities::fontScale, 0);			
		}
	}
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
		glOrtho(0, 1000, 0, 1000, 1.0, 10000);


		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();

		gluPerspective(fov, (GLdouble)glutGet(GLUT_WINDOW_HEIGHT) / glutGet(GLUT_WINDOW_WIDTH), 1.0, 12000.0);

		viewChanged = false;

		uint32_t dataLength;

		glMatrixMode(GL_MODELVIEW);

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

			if (Graphs::DRAWING_GRAPHS)
			{
				dataGraph->Draw();

				if (correlationGraph)
					correlationGraph->Draw();

				if (fftGraphForDeviceRange)
					fftGraphForDeviceRange->Draw();

				if (fftGraphForDevicesRange)
					fftGraphForDevicesRange->Draw();

				fftGraphStrengthsForDeviceRange->Draw();

				fftAverageGraphForDeviceRange->Draw();

				fftAverageGraphStrengthsForDeviceRange->Draw();

				if (spectrumRangeGraph)
					spectrumRangeGraph->Draw();

				if (allSessionsSpectrumRangeGraph)
					allSessionsSpectrumRangeGraph->Draw();

				dataGraph->DrawTransparencies();

				if (correlationGraph)
					correlationGraph->DrawTransparencies();

				if (fftGraphForDeviceRange)
					fftGraphForDeviceRange->DrawTransparencies();

				if (fftGraphForDevicesRange)
					fftGraphForDevicesRange->DrawTransparencies();

				fftAverageGraphForDeviceRange->DrawTransparencies();

				if (spectrumRangeGraph)
					spectrumRangeGraph->DrawTransparencies();

				if (allSessionsSpectrumRangeGraph)
					allSessionsSpectrumRangeGraph->DrawTransparencies();				

			
				if (graphs->showLabels)
				{
					if (nearFarDataAnalyzer->detectionMode == DetectionMode::Sessions)
					{
						sprintf(textBuffer, "Session %i: Strongest Reradiated", nearFarDataAnalyzer->sessionCount);
						DrawFrequenciesRangeBoard(nearFarDataAnalyzer->spectrumFrequencyRangesBoard, graphs->x - dataGraph->width / 2, 0, graphs->z, textBuffer, "Frequencies For Spectrum Graph:");

						DrawFrequenciesRangeBoard(nearFarDataAnalyzer->leaderboardFrequencyRanges, graphs->x + dataGraph->width + dataGraph->width / 10, 0, graphs->z, "LeaderBoard For Strongest Reradiated", "Frequencies Of All Sessions:");
					}
					else
					{
						sprintf(textBuffer, "Strongest Reradiated");
						DrawFrequenciesRangeBoard(nearFarDataAnalyzer->spectrumFrequencyRangesBoard, graphs->x - dataGraph->width / 2, 0, graphs->z, textBuffer, "Frequencies For Spectrum Graph:");

						DrawFrequenciesRangeBoard(nearFarDataAnalyzer->spectrumFrequencyRangesBoard, graphs->x + dataGraph->width + dataGraph->width / 10, 0, graphs->z, "LeaderBoard For Strongest", "Reradiated Frequencies:");
					}
				}					
			}

			prevTime = currentTime;
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

	if (nearFarDataAnalyzer->automatedDetection)
		nearFarDataAnalyzer->SetMode(ReceivingDataMode::Near);

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
				}

				DeleteSelectedGraphData();
			}
		}
}

void MouseMotion(int x, int y)
{	
	if (nearFarDataAnalyzer->automatedDetection)
		nearFarDataAnalyzer->SetMode(ReceivingDataMode::Near);

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
	if (nearFarDataAnalyzer)
	{
		if (Movement(x, y) && nearFarDataAnalyzer->automatedDetection)
			nearFarDataAnalyzer->SetMode(ReceivingDataMode::Near);

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
}

void SetCenterView()
{
	graphs->ResetToUserDrawDepths();
	graphs->SetVisibility(true);

	pos.x = -dataGraph->width / 2;	
	pos.z = -9000;

	fov = 24;

	if (nearFarDataAnalyzer->detectionMode == DetectionMode::Sessions)
	{
		pos.y = -allSessionsSpectrumRangeGraph->pos.y / 2;

		fov += 4;
	}
	else
		pos.y = -spectrumRangeGraph->pos.y / 2;

	if (nearFarDataAnalyzer->spectrumAnalyzer.deviceReceivers->count > 1)
	{		
		fov += 10;
	}
	
	xRot = 0;
	yRot = 0;
}

void SetSpectrumBoardView()
{	
	pos.x = -204;
	pos.y = 1249;
	pos.z = -9000;

	fov = 18;

	xRot = 0;
	yRot = 0;
}

void SetLeaderBoardView()
{
	pos.x = -936;
	pos.y = 1223;
	pos.z = -9000;

	fov = 18;

	xRot = 0;
	yRot = 0;
}

void SetViewToGraph(Graph *graph)
{
	if (graph)
	{
		graphs->ResetToUserDrawDepths();

		if (graph->view == GraphView::Front)
			graph->view = GraphView::Above;
		else if (graph->view == GraphView::Above)
			graph->view = GraphView::Front;

		pos.y = -(graph->pos.y + graph->height / 2);
		pos.z = -9000;
		
		fov = 14;
		graph->drawDepth = graph->userSetDepth;
		
		xRot = 0;
		yRot = 0;

		switch (graph->view)
		{
			case (GraphView::Front):
				pos.x = -graph->width / 2;
				yRot = 0;
				graphs->SetVisibility(true);
			break;
			case (GraphView::Side):
				pos.x = fftGraphForDevicesRange->width / 2;
				yRot = 90;
				graphs->SetVisibility(true);
			break;
			case (GraphView::Above):
				pos.y = -(graph->height / 2);
				pos.x = -graph->width / 2;				
				yRot = 0;
				xRot = 90;

				graphs->SetVisibility(false);

				fov = 10;
				graph->visible = true;

				graph->SetDepth(graph->maxDepth, false);
			break;
		}		
	}
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
	if (nearFarDataAnalyzer->automatedDetection)
		nearFarDataAnalyzer->SetMode(ReceivingDataMode::Near);

	GetControlKeys();	

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
			if (fftGraphForDeviceRange)
			{
				ToggleVisibilityAndDrawDepths(fftGraphForDeviceRange);
			}
			break;
		case ('#'):
			SetViewToGraph(fftGraphForDeviceRange);			
		break;
		case ('4'):			
			if (fftGraphForDevicesRange)
			{
				ToggleVisibilityAndDrawDepths(fftGraphForDevicesRange);
			}
		break;
		case ('$'):
				SetViewToGraph(fftGraphForDevicesRange);			
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
			SetViewToGraph(fftAverageGraphForDeviceRange);			
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
				SetViewToGraph(spectrumRangeGraph);
			}
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
		break;
		case ('F'):
			nearFarDataAnalyzer->spectrumAnalyzer.GetFFTSpectrumBuffer(1)->ClearData();			
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
}

void ProcessSequenceFinished()
{
	nearFarDataAnalyzer->ProcessSequenceFinished();
}

int InitializeNearFarSpectrumAnalyzerAndGraphs(uint32_t startFrequency, uint32_t endFrequency, DetectionMode detectionMode)
{
	nearFarDataAnalyzer = new NearFarDataAnalyzer();

	nearFarDataAnalyzer->detectionMode = detectionMode;
	
	int deviceCount = nearFarDataAnalyzer->InitializeNearFarDataAnalyzer(20000, 1000000, startFrequency, endFrequency);
	
	int graphResolution = 1024;

	graphs = new Graphs();
	graphs->SetPos(0, 0, 0);

	dataGraph = new Graph(1, DeviceReceiver::FFT_SEGMENT_BUFF_LENGTH);
	dataGraph->showXAxis = 1;
	dataGraph->showYAxis = 1;
	dataGraph->showZAxis = 0;

	dataGraph->SetSize(1000, GRAPH_HEIGHT);	

	dataGraph->SetDataSeriesColor(1, 0, 0, 1, 0);
	dataGraph->SetDataSeriesColor(0, 1, 0, 1, 1);

	dataGraph->SetDataSeriesStyle(GraphStyle::Area3D);

	dataGraph->SetText(1, "IQ Signal Data Waveform");

	graphs->AddGraph(dataGraph);

	if (nearFarDataAnalyzer->spectrumAnalyzer.deviceReceivers->count > 1)
	{
		correlationGraph = new Graph(1, DeviceReceiver::FFT_SEGMENT_BUFF_LENGTH);
		correlationGraph->showXAxis = 1;
		correlationGraph->showYAxis = 1;
		correlationGraph->showZAxis = 0;
		correlationGraph->SetSize(1000, GRAPH_HEIGHT);

		correlationGraph->SetDataSeriesColor(1, 0, 0, 1, 0);

		correlationGraph->SetDataSeriesStyle(GraphStyle::Area3D);

		correlationGraph->SetText(1, "Correlation Graph");

		graphs->AddGraph(correlationGraph);
	}

	if (nearFarDataAnalyzer->spectrumAnalyzer.deviceReceivers->count > 1)
	{
		fftGraphForDeviceRange = new Graph(100, graphResolution);
		fftGraphForDeviceRange->drawDepth = 1;
		fftGraphForDeviceRange->showXAxis = 1;
		fftGraphForDeviceRange->showYAxis = 1;
		fftGraphForDeviceRange->showZAxis = 2;

		fftGraphForDeviceRange->SetSize(1000, GRAPH_HEIGHT);

		if (nearFarDataAnalyzer->spectrumAnalyzer.deviceReceivers->count > 1)
		{
			fftGraphForDeviceRange->SetDataSeriesColor(1, 0, 0, 1, 0);			
			fftGraphForDeviceRange->SetDataSeriesColor(0, 1, 0, 1, 1);
		}

		fftGraphForDeviceRange->SetDataSeriesStyle(GraphStyle::Graph3D);		

		fftGraphForDeviceRange->SetText(1, "FFT Graph (Device 1 = Red, Device 2 = Blue)");

		graphs->AddGraph(fftGraphForDeviceRange);
	}
	
	fftGraphForDevicesRange = new Graph(100, graphResolution);
	fftGraphForDevicesRange->drawDepth = 1;
	fftGraphForDevicesRange->showXAxis = 1;
	fftGraphForDevicesRange->showYAxis = 1;
	fftGraphForDevicesRange->showZAxis = 2;

	fftGraphForDevicesRange->SetSize(1000, GRAPH_HEIGHT);

	fftGraphForDevicesRange->SetDataSeriesStyle(GraphStyle::Graph3D);
		
	if (nearFarDataAnalyzer->spectrumAnalyzer.deviceReceivers->count == 1)
	{
		fftGraphForDevicesRange->SetText(1, "FFT Graph");
	}
	else
		fftGraphForDevicesRange->SetText(1, "Combined FFT Graph for Devices");		

	graphs->AddGraph(fftGraphForDevicesRange);

	fftGraphStrengthsForDeviceRange = new Graph(100, 100);
	fftGraphStrengthsForDeviceRange->view = GraphView::Side;	
	fftGraphStrengthsForDeviceRange->showXAxis = 0;
	fftGraphStrengthsForDeviceRange->showYAxis = 0;
	fftGraphStrengthsForDeviceRange->showZAxis = 0;
	fftGraphStrengthsForDeviceRange->showLabels = false;
	fftGraphStrengthsForDeviceRange->SetDataSeriesStyle(GraphStyle::Graph3D);	

	fftGraphStrengthsForDeviceRange->SetDataSeriesLineWidth(1);

	fftGraphStrengthsForDeviceRange->SetSize(100, GRAPH_HEIGHT);

	fftGraphStrengthsForDeviceRange->userSetDepth = 100;
	
	graphs->AddGraph(fftGraphStrengthsForDeviceRange);

	fftGraphStrengthsForDeviceRange->SetPos(fftGraphStrengthsForDeviceRange->pos.x + GRAPH_STRENGTHS_X_OFFSET, fftGraphStrengthsForDeviceRange->pos.y + GRAPH_HEIGHT, 0);
	
	fftAverageGraphForDeviceRange = new Graph(100, graphResolution);
	fftAverageGraphForDeviceRange->drawDepth = 1;
	fftAverageGraphForDeviceRange->showXAxis = 1;
	fftAverageGraphForDeviceRange->showYAxis = 1;
	fftAverageGraphForDeviceRange->showZAxis = 2;

	fftAverageGraphForDeviceRange->SetSize(1000, GRAPH_HEIGHT);

	fftAverageGraphForDeviceRange->SetDataSeriesStyle(GraphStyle::Graph3D);	

	fftAverageGraphForDeviceRange->SetDataSeriesLineWidth(2);
	
	fftAverageGraphForDeviceRange->SetDataSeriesColor(1, 0, 0, 1, ReceivingDataMode::Near);
	fftAverageGraphForDeviceRange->SetDataSeriesColor(0, 1, 0, 1, ReceivingDataMode::Far);

	if (nearFarDataAnalyzer->spectrumAnalyzer.deviceReceivers->count == 1)
		fftAverageGraphForDeviceRange->SetText(1, "Averaged FFT For Current Scanning Range: ");

	graphs->AddGraph(fftAverageGraphForDeviceRange);

	fftAverageGraphForDeviceRange->SetPos(fftAverageGraphForDeviceRange->pos.x, fftAverageGraphForDeviceRange->pos.y + GRAPH_HEIGHT, fftAverageGraphForDeviceRange->pos.z);

	fftAverageGraphStrengthsForDeviceRange = new Graph(100, 100, 3);
	fftAverageGraphStrengthsForDeviceRange->view = GraphView::Side;	

	fftAverageGraphStrengthsForDeviceRange->showXAxis = 0;
	fftAverageGraphStrengthsForDeviceRange->showYAxis = 0;
	fftAverageGraphStrengthsForDeviceRange->showZAxis = 0;
	fftAverageGraphStrengthsForDeviceRange->showLabels = false;
	fftAverageGraphStrengthsForDeviceRange->SetDataSeriesStyle(GraphStyle::Graph3D);
	fftAverageGraphStrengthsForDeviceRange->SetDataSeriesColor(1, 0, 0, 1, ReceivingDataMode::Near);
	fftAverageGraphStrengthsForDeviceRange->SetDataSeriesColor(0, 1, 0, 1, ReceivingDataMode::Far);

	fftAverageGraphStrengthsForDeviceRange->dataSeriesSameScale = false;

	fftAverageGraphStrengthsForDeviceRange->selectable = false;

	fftAverageGraphStrengthsForDeviceRange->SetDataSeriesLineWidth(1);

	fftAverageGraphStrengthsForDeviceRange->SetSize(100, GRAPH_HEIGHT);

	fftAverageGraphStrengthsForDeviceRange->userSetDepth = 100;
	
	graphs->AddGraph(fftAverageGraphStrengthsForDeviceRange);

	fftAverageGraphStrengthsForDeviceRange->SetPos(fftAverageGraphStrengthsForDeviceRange->pos.x + GRAPH_STRENGTHS_X_OFFSET, fftAverageGraphStrengthsForDeviceRange->pos.y + GRAPH_HEIGHT * 2, fftAverageGraphStrengthsForDeviceRange->pos.z);

	spectrumRangeGraph = new Graph(100, nearFarDataAnalyzer->scanningRange.length / DeviceReceiver::SAMPLE_RATE * graphResolution);	
	spectrumRangeGraph->drawDepth = 1;	
	spectrumRangeGraph->showXAxis = 1;
	spectrumRangeGraph->showYAxis = 1;
	spectrumRangeGraph->showZAxis = 2;

	spectrumRangeGraph->SetSize(1000, GRAPH_HEIGHT);

	spectrumRangeGraph->SetDataSeriesStyle(GraphStyle::Graph3D);

	spectrumRangeGraph->SetDataSeriesColor(1, 0, 0, 1, ReceivingDataMode::Near);
	spectrumRangeGraph->SetDataSeriesColor(0, 1, 0, 1, ReceivingDataMode::Far);		
	
	spectrumRangeGraph->SetText(1, "Spectrum Graph");
	spectrumRangeGraph->SetGraphXRange(startFrequency, endFrequency);

	graphs->AddGraph(spectrumRangeGraph);

	spectrumRangeGraph->SetPos(spectrumRangeGraph->pos.x, spectrumRangeGraph->pos.y + GRAPH_HEIGHT * 2, spectrumRangeGraph->pos.z);			

	if (nearFarDataAnalyzer->detectionMode == DetectionMode::Sessions)
	{
		allSessionsSpectrumRangeGraph = new Graph(100, nearFarDataAnalyzer->scanningRange.length / DeviceReceiver::SAMPLE_RATE * graphResolution);
		allSessionsSpectrumRangeGraph->drawDepth = 1;
		allSessionsSpectrumRangeGraph->showXAxis = 1;
		allSessionsSpectrumRangeGraph->showYAxis = 1;
		allSessionsSpectrumRangeGraph->showZAxis = 2;

		allSessionsSpectrumRangeGraph->SetSize(1000, GRAPH_HEIGHT);

		allSessionsSpectrumRangeGraph->SetDataSeriesStyle(GraphStyle::Graph3D);

		allSessionsSpectrumRangeGraph->SetDataSeriesColor(1, 0, 0, 1, ReceivingDataMode::Near);
		allSessionsSpectrumRangeGraph->SetDataSeriesColor(0, 1, 0, 1, ReceivingDataMode::Far);

		allSessionsSpectrumRangeGraph->SetGraphXRange(startFrequency, endFrequency);

		graphs->AddGraph(allSessionsSpectrumRangeGraph);

		allSessionsSpectrumRangeGraph->SetPos(allSessionsSpectrumRangeGraph->pos.x, allSessionsSpectrumRangeGraph->pos.y + GRAPH_HEIGHT * 2, allSessionsSpectrumRangeGraph->pos.z);
	}

	SetCenterView();

	nearFarDataAnalyzer->spectrumAnalyzer.deviceReceivers->dataGraph = dataGraph;
	nearFarDataAnalyzer->spectrumAnalyzer.deviceReceivers->correlationGraph = correlationGraph;
	nearFarDataAnalyzer->spectrumAnalyzer.deviceReceivers->fftGraphForDeviceRange = fftGraphForDeviceRange;
	nearFarDataAnalyzer->spectrumAnalyzer.deviceReceivers->fftGraphForDevicesRange = fftGraphForDevicesRange;
	nearFarDataAnalyzer->spectrumAnalyzer.deviceReceivers->fftAverageGraphForDeviceRange = fftAverageGraphForDeviceRange;
	nearFarDataAnalyzer->spectrumAnalyzer.deviceReceivers->fftGraphStrengthsForDeviceRange = fftGraphStrengthsForDeviceRange;
	nearFarDataAnalyzer->spectrumAnalyzer.deviceReceivers->fftAverageGraphStrengthsForDeviceRange = fftAverageGraphStrengthsForDeviceRange;
	nearFarDataAnalyzer->spectrumAnalyzer.deviceReceivers->spectrumRangeGraph = spectrumRangeGraph;
	nearFarDataAnalyzer->spectrumAnalyzer.deviceReceivers->allSessionsSpectrumRangeGraph = allSessionsSpectrumRangeGraph;

	nearFarDataAnalyzer->StartProcessing();

	nearFarDataAnalyzer->spectrumAnalyzer.SetSequenceFinishedFunction(&ProcessSequenceFinished);

	return 1;
}

LRESULT CALLBACK LowLevelMouseProc(int nCode, WPARAM wParam, LPARAM lParam)
{	
	if (nearFarDataAnalyzer->automatedDetection)
		nearFarDataAnalyzer->SetMode(ReceivingDataMode::Near);

	return CallNextHookEx(NULL, nCode, wParam, lParam);
}

LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	if (nearFarDataAnalyzer->automatedDetection)
		nearFarDataAnalyzer->SetMode(ReceivingDataMode::Near);	
	
	return CallNextHookEx(NULL, nCode, wParam, lParam);
}

void Initialize(uint32_t startFrequency, uint32_t endFrequency, DetectionMode detectionMode)
{	
	InitializeNearFarSpectrumAnalyzerAndGraphs(startFrequency, endFrequency, detectionMode);

	#if !defined(_DEBUG)
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
	printf("Usage: SDRSpectrumAnalyzerOpenGL [-a] [-m] [-f] [-s] [-e] [-S]\n", argv[0]);	
	printf("-a: Automatically detect reradiated frequency ranges\n");
	printf("-m: Scanning Mode [normal, sessions]\n");
	printf("-f: Required frames for sessions\n");
	printf("-s: Start frequency\n");
	printf("-e: End frequency\n");
	printf("-S: Sound cues for detecting increasing signal strengths\n");	
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
	printf("e.g., SDRSpectrumAnalyzerOpenGL.exe -a -s 430000000 -e 470000000");
	printf("\n");
	printf("press any key to continue\n");

	getch();
}

int main(int argc, char **argv)
{		
	if (argc < 2)
	{
		Usage(argc, argv);
		return 0;
	}

	uint32_t startFrequency = 88000000;
	uint32_t endFrequency = 108000000;
	bool automatedDetection = false;
	DetectionMode detectionMode = DetectionMode::Normal;
	uint32_t requiredFramesForSessions = 1000;
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
	}		

	Initialize(startFrequency, endFrequency, detectionMode);
	
	if (nearFarDataAnalyzer)
	{
		nearFarDataAnalyzer->automatedDetection = automatedDetection;		

		nearFarDataAnalyzer->requiredFramesForSessions = requiredFramesForSessions;

		nearFarDataAnalyzer->spectrumAnalyzer.sound = sound;
	}

	glutMainLoop();

	return 0;
}
