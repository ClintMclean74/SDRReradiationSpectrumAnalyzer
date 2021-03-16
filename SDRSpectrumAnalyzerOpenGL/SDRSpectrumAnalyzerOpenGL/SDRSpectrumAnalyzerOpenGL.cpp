#include "glew-2.1.0\include\GL/glew.h"
#include <GL/glut.h>

#include <time.h>
#include<conio.h>
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

SelectedGraph *selectedGraph;
SelectedGraph* selectedGraphStart;
SelectedGraph* selectedGraphEnd;

////uint32_t defaultSelectedStartIndex = 710;
////uint32_t defaultSelectedEndIndex = 760;

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

////GLfloat mat_ambient[] = { 0.5, 0.5, 0.5, 1.0 };
GLfloat mat_ambient[] = { 1.0, 1.0, 1.0, 1.0 };
GLfloat mat_diffuse[] = { 1.0, 1.0, 1.0, 1.0 };
GLfloat mat_specular[] = { 1.0, 1.0, 1.0, 1.0 };
GLfloat mat_shininess[] = { 50.0 };
////GLfloat light_position[] = { -1.0, -1.0, -1.0, 1.0 };
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

/*void DrawText(char *string, float x, float y, float z, float scale)
{
	glPushMatrix();

	char *c;
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glTranslatef(x, y, z);

	glScalef(scale, scale, scale);
	
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	
	glDisable(GL_DEPTH_TEST);

	for (c = string; *c != '\0'; c++)
	{
		glutStrokeCharacter(GLUT_STROKE_ROMAN, *c);
	}

	glEnable(GL_DEPTH_TEST);

	glPopMatrix();
}*/

void DrawLeaderboardFrequencies(float x, float y, float z)
{
	float textStartHeight = y;

	snprintf(textBuffer, sizeof(textBuffer), "LeaderBoard Frequencies: ");	

	GraphicsUtilities::DrawText(textBuffer, x, textStartHeight, z, GraphicsUtilities::fontScale, 0);

	//x+= (strlen(textBuffer) * glutStrokeWidth(GLUT_STROKE_ROMAN, (int)textBuffer[0]) * GraphicsUtilities::fontScale);

	x += glutStrokeWidth(GLUT_STROKE_ROMAN, (int)textBuffer[0]) * GraphicsUtilities::fontScale;

	FrequencyRange *frequencyRange;
	int labelHeight;

	for (uint8_t i = 0; i < nearFarDataAnalyzer->leaderboardFrequencyRanges->count; i++)
	{		
		frequencyRange = nearFarDataAnalyzer->leaderboardFrequencyRanges->GetFrequencyRangeFromIndex(i);
		if (frequencyRange)
		{
			labelHeight = glutStrokeWidth(GLUT_STROKE_ROMAN, (int)textBuffer[0]) * GraphicsUtilities::fontScale;
			textStartHeight -= labelHeight * 3;

			snprintf(textBuffer, sizeof(textBuffer), "%.4d %.d", frequencyRange->lower, frequencyRange->frames);
			GraphicsUtilities::DrawText(textBuffer, x, textStartHeight, z, GraphicsUtilities::fontScale, 0);			

			//nearFarDataAnalyzer->leaderboardFrequencyRanges[i].GetFrequencyRangeFromIndex(i)->strength			
		}
	}
}

void display(void)
{	
	//if (false)
	{
		currentTime = GetTickCount();

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		//if (false)
		{
			if (currentTime != prevTime)
			{
				frameRate = (double)1000 / (currentTime - prevTime);

				frameRateTotal += frameRate;

				frameRateCount++;

				if (frameRateCount > 100)
				{
					frameRate = frameRateTotal / frameRateCount;

					snprintf(textBufferFrameRate, sizeof(textBufferFrameRate), "Frame Rate: %.4f", frameRate);

					frameRateTotal = 0;
					frameRateCount = 0;

					/*////
					nearFarDataAnalyzer->spectrumAnalyzer.GetFFTSpectrumBuffer(0)->ClearData();
					nearFarDataAnalyzer->spectrumAnalyzer.GetFFTSpectrumBuffer(1)->ClearData();
					nearFarDataAnalyzer->spectrumAnalyzer.GetFFTSpectrumBuffer(3)->ClearData();
					*/
				}


				glMatrixMode(GL_PROJECTION);
				glLoadIdentity();
				glOrtho(0, 1000, 0, 1000, 1.0, 10000);

				////DrawText(textBufferFrameRate, 0, 950, -100, 0.2);



			////if (viewChanged)
				{
					glMatrixMode(GL_PROJECTION);
					glLoadIdentity();
					////glOrtho(dataGraph->pos.x - 10, dataGraph->pos.x + dataGraph->width + 10, correlationGraph->pos.y - correlationGraph->height, dataGraph->pos.y + dataGraph->height, 1.0, 10000);
					////gluPerspective(90, glutGet(GLUT_WINDOW_HEIGHT)/glutGet(GLUT_WINDOW_WIDTH), 0, 1000);

					gluPerspective(fov, (GLdouble)glutGet(GLUT_WINDOW_HEIGHT) / glutGet(GLUT_WINDOW_WIDTH), 1.0, 12000.0);

					viewChanged = false;
				}



				uint32_t dataLength;

				glMatrixMode(GL_MODELVIEW);


				glLoadIdentity();

				light_position[0] = 1.0;
				light_position[1] = 1.0;
				light_position[2] = 1.0;
				light_position[3] = 0.0;


				glTranslatef(pos.x, pos.y, pos.z);

				////glLightfv(GL_LIGHT0, GL_POSITION, light_position);

				////glTranslatef(-pos.x, -pos.y, -pos.z);	

				////gluLookAt(pos.x, pos.y, pos.z, 1100, 2800, 0, 0, 1, 0);

				glRotatef(xRot, 1, 0, 0);
				glRotatef(yRot, 0, 1, 0);



				glLightfv(GL_LIGHT0, GL_POSITION, light_position);

				//if (false)
				{
					if (nearFarDataAnalyzer)
					{
						if (correlating)
							nearFarDataAnalyzer->spectrumAnalyzer.deviceReceivers->Correlate();


						if (Graphs::DRAWING_GRAPHS)
						{
							////if (drawDataGraph)
							dataGraph->Draw();

							if (correlationGraph)
								correlationGraph->Draw();

							if (fftGraphForDeviceRange)
								fftGraphForDeviceRange->Draw();

							if (fftGraphForDevicesRange)
								fftGraphForDevicesRange->Draw();

							////if (drawFFTGraphStrengthsForDeviceRange)
							fftGraphStrengthsForDeviceRange->Draw();

							////if (drawFFTAverageGraphForDeviceRange)
							fftAverageGraphForDeviceRange->Draw();

							////if (drawFFTAverageGraphStrengthsForDeviceRange)
							fftAverageGraphStrengthsForDeviceRange->Draw();

							if (spectrumRangeGraph)
								spectrumRangeGraph->Draw();


							////if (drawDataGraph)
							dataGraph->DrawTransparencies();

							if (correlationGraph)
								correlationGraph->DrawTransparencies();

							if (fftGraphForDeviceRange)
								fftGraphForDeviceRange->DrawTransparencies();

							if (fftGraphForDevicesRange)
								fftGraphForDevicesRange->DrawTransparencies();

							////fftGraphStrengthsForDeviceRange->DrawTransparencies();

							////if (drawFFTAverageGraphForDeviceRange)
							fftAverageGraphForDeviceRange->DrawTransparencies();

							////fftAverageGraphStrengthsForDeviceRange->DrawTransparencies();	

							if (spectrumRangeGraph)
								spectrumRangeGraph->DrawTransparencies();

							DrawLeaderboardFrequencies(graphs->x + dataGraph->width + dataGraph->width / 10, 0, graphs->z);

							////DrawText(textBuffer, -2400, 3400, -10000);
							////DrawText(textBufferFrameRate, -800, 1200, -10000);

							/*////
							////Vector* point = fftGraphStrengthsForDeviceRange->dataSeries[nearFarDataAnalyzer->spectrumAnalyzer.currentFFTBufferIndex]->GetPoint(0);
							Vector* point = fftAverageGraphStrengthsForDeviceRange->dataSeries[nearFarDataAnalyzer->spectrumAnalyzer.currentFFTBufferIndex]->GetPoint(0);

							snprintf(textBuffer, sizeof(textBuffer), "%.4f", point->y * MathUtilities::RadianToDegrees);

							DrawText(textBuffer, -800, 1000, -10000);
							*/
						}

						prevTime = currentTime;
					}
				}
			}
		}

		glutSwapBuffers();

		glutPostRedisplay();
	}
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

	//DebuggingUtilities::DebugPrint("4 dataIsNearTimeStamp = GetTickCount(); %d\n");
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

		////DebuggingUtilities::DebugPrint("Scroll %s At %d %d\n", (button == 3) ? "Up" : "Down", x, y);
	}
	
	if (button == GLUT_LEFT_BUTTON)
	{
		prevX = x;
		prevY = y;

		if (state == GLUT_DOWN)
		{	
			/*////
			nearFarDataAnalyzer->spectrumAnalyzer.GetFFTSpectrumBuffer(0)->ClearData();		
			nearFarDataAnalyzer->spectrumAnalyzer.GetFFTSpectrumBuffer(1)->ClearData();
			nearFarDataAnalyzer->spectrumAnalyzer.GetFFTSpectrumBuffer(3)->ClearData();
			*/

			Vector vector = Get3DPoint(x, y);

			selectedGraphStart = graphs->GetSelectedGraph(vector.x, vector.y, vector.z);

			////DeleteSelectedGraphData();
		}
		else
			if (state == GLUT_UP)
			{
				Vector vector = Get3DPoint(x, y);

				////selectedGraphEnd = graphs->GetSelectedGraph(vector.x, vector.y, vector.z);

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

						////fftGraphForDevicesRange->SetGraphViewRangeXAxis(462, 719);
						selectedGraphEnd->graph->SetGraphViewRangeXAxis(startX, endX);

						////SignalProcessingUtilities::GetFrequencyFromDataIndex(startX, )

						////selectedGraphEnd->graph->SetGraphLabelValues(startX, endX);
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
			Vector vector = Get3DPoint(x, y);

			selectedGraphStart = graphs->GetSelectedGraph(vector.x, vector.y, vector.z);

			if (selectedGraphStart != NULL)
			{
				selectedGraphStart->graph->ZoomOut();				
			}

			DeleteSelectedGraphData();
		}
}

void MouseMotion(int x, int y)
{
	//DebuggingUtilities::DebugPrint("5 dataIsNearTimeStamp = GetTickCount(); %d\n");
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

			////DeleteSelectedGraphData();
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
		//DebuggingUtilities::DebugPrint("1 dataIsNearTimeStamp = GetTickCount(); %d\n");
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

			////pos.z -= (y - prevY)*translateScale;
		}

		prevX = x;
		prevY = y;
	}
}

void ClearAllData()
{
	nearFarDataAnalyzer->spectrumAnalyzer.GetFFTSpectrumBuffer(0)->ClearData();
	nearFarDataAnalyzer->spectrumAnalyzer.GetFFTSpectrumBuffer(1)->ClearData();
	nearFarDataAnalyzer->spectrumAnalyzer.GetFFTSpectrumBuffer(2)->ClearData();
}


void SetCenterView()
{
	pos.x = -dataGraph->width / 2;
	pos.y = -spectrumRangeGraph->pos.y / 2;	
	pos.z = -9000;

	fov = 24;

	xRot = 0;
	yRot = 0;
}

void SetLeaderBoardView()
{
	pos.x = -1196.0000000000000;
	pos.y = 912;
	pos.z = -9000;

	fov = 14;

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

		//graph->view = view;

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

void ProcessKey2(int key, int x, int y)
{
	GetControlKeys();

	if (GetKeyState('1') & 0x8000/*Check if high-order bit is set (1 << 15)*/)
	{
		GetControlKeys();
	}
}

void ToggleVisibilityAndDrawDepths(Graph *graph)
{
	if (graph->visible)
	{
		if (graph->drawDepth == graph->maxDepth)
			graph->SetDepth(1);
			//graph->drawDepth = 1;
		else
		{
			graph->visible = false;
		}
	}
	else
	{
		graph->visible = true;
		//graph->drawDepth = graph->maxDepth;
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
			ClearAllData();
			nearFarDataAnalyzer->spectrumAnalyzer.useRatios = !nearFarDataAnalyzer->spectrumAnalyzer.useRatios;
			nearFarDataAnalyzer->spectrumAnalyzer.usePhase = false;
			break;
		case ('p'):
			ClearAllData();
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

			/*////dataGraph->TogglePause();
			fftGraphForDevicesRange->TogglePause();
			fftGraphStrengthsForDeviceRange->TogglePause();
			correlationGraph->TogglePause();
			*/
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
		case ('L'):
			SetLeaderBoardView();
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
	////glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
	////glMaterialfv(GL_FRONT, GL_SHININESS, mat_shininess);
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
	//glutSpecialFunc(ProcessKey2);

	glEnable(GL_MULTISAMPLE);
}

void ProcessSequenceFinished()
{
	nearFarDataAnalyzer->ProcessSequenceFinished();
}

int InitializeNearFarSpectrumAnalyzerAndGraphs(uint32_t startFrequency, uint32_t endFrequency)
{
	nearFarDataAnalyzer = new NearFarDataAnalyzer();

	////int deviceCount = nearFarDataAnalyzer->InitializeNearFarDataAnalyzer(20000, 1000000, 96000000, 97000000);
	////int deviceCount = nearFarDataAnalyzer->InitializeNearFarDataAnalyzer(20000, 1000000, 444000000, 445000000);	
	////int deviceCount = nearFarDataAnalyzer->InitializeNearFarDataAnalyzer(20000, 1000000, 445000000, 446000000);

	////int deviceCount = nearFarDataAnalyzer->InitializeNearFarDataAnalyzer(20000, 1000000, 433000000, 434000000);
	////int deviceCount = nearFarDataAnalyzer->InitializeNearFarDataAnalyzer(20000, 1000000, 444000000, 445000000);
	////int deviceCount = nearFarDataAnalyzer->InitializeNearFarDataAnalyzer(20000, 1000000, 94000000, 97000000);
	int deviceCount = nearFarDataAnalyzer->InitializeNearFarDataAnalyzer(20000, 1000000, startFrequency, endFrequency);
	
	////pos.x = 322;
	////pos.y = 702;

	/*////pos.x = 1600;
	pos.y = 1400;
	pos.z = -9000;
	*/

	/*////xRot = 10.8;
	yRot = 95;
	*/	

	int graphResolution = 1024;

	graphs = new Graphs();
	////graphs->SetPos(1100, 2800, -9000);

	graphs->SetPos(0, 0, 0);

	dataGraph = new Graph(1, DeviceReceiver::FFT_SEGMENT_BUFF_LENGTH);
	dataGraph->showXAxis = 1;
	dataGraph->showYAxis = 1;
	dataGraph->showZAxis = 0;

	dataGraph->SetSize(1000, GRAPH_HEIGHT);
	////dataGraph->SetPos(1100, 2800, 0);

	dataGraph->SetDataSeriesColor(1, 0, 0, 1, 0);
	dataGraph->SetDataSeriesColor(0, 0, 1, 1, 1);

	dataGraph->SetDataSeriesStyle(GraphStyle::Area3D);

	


	////snprintf(textBuffer, 255, "Data Graph:");
	dataGraph->SetText(1, "IQ Signal Data");

	graphs->AddGraph(dataGraph);


	if (nearFarDataAnalyzer->spectrumAnalyzer.deviceReceivers->count > 1)
	{
		correlationGraph = new Graph(1, DeviceReceiver::FFT_SEGMENT_BUFF_LENGTH);
		correlationGraph->showXAxis = 1;
		correlationGraph->showYAxis = 1;
		correlationGraph->showZAxis = 0;
		////correlationGraph->graph_3D = true;
		correlationGraph->SetSize(1000, GRAPH_HEIGHT);
		////correlationGraph->SetPos(1100, 1400, 0);
		////correlationGraph->SetRotation(-90, 0, 0);

		correlationGraph->SetDataSeriesColor(1, 0, 0, 1, 0);

		correlationGraph->SetDataSeriesStyle(GraphStyle::Area3D);


		////snprintf(textBuffer, 255, "Correlation Graph:");
		////correlationGraph->SetText(textBuffer, 255, 0);
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
		////fftGraphForDeviceRange->SetPos(1100, 2100, 0);

		if (nearFarDataAnalyzer->spectrumAnalyzer.deviceReceivers->count > 1)
		{
			fftGraphForDeviceRange->SetDataSeriesColor(1, 0, 0, 1, 0);
			fftGraphForDeviceRange->SetDataSeriesColor(0, 0, 1, 1, 1);
		}

		fftGraphForDeviceRange->SetDataSeriesStyle(GraphStyle::Graph3D);



		////fftGraphForDeviceRange->graph_3D = true;
		////fftGraphForDeviceRange->wireFrame = true;



		////fftGraphForDeviceRange->SetGraphViewRangeXAxis(462, 719);

		fftGraphForDeviceRange->startDataIndex = defaultSelectedStartIndex;
		fftGraphForDeviceRange->endDataIndex = defaultSelectedEndIndex;

		////snprintf(textBuffer, 255, "Devices FFT Graph:");
		////fftGraphForDeviceRange->SetText(textBuffer, 255, 0);

		fftGraphForDeviceRange->SetText(1, "FFT Graph (Device 1 = Red, Device 2 = Blue)");

		graphs->AddGraph(fftGraphForDeviceRange);
	}

	//if (nearFarDataAnalyzer->spectrumAnalyzer.deviceReceivers->count > 1)
	{
		fftGraphForDevicesRange = new Graph(100, graphResolution);
		fftGraphForDevicesRange->drawDepth = 1;
		fftGraphForDevicesRange->showXAxis = 1;
		fftGraphForDevicesRange->showYAxis = 1;
		fftGraphForDevicesRange->showZAxis = 2;

		fftGraphForDevicesRange->SetSize(1000, GRAPH_HEIGHT);
		////fftGraphForDevicesRange->SetPos(1100, GRAPH_HEIGHT, 0);

		fftGraphForDevicesRange->SetDataSeriesStyle(GraphStyle::Graph3D);

		////fftGraphForDevicesRange->graph_3D = true;
		////fftGraphForDevicesRange->wireFrame = true;



		////fftGraphForDevicesRange->SetGraphViewRangeXAxis(462, 719);

		fftGraphForDevicesRange->startDataIndex = defaultSelectedStartIndex;
		fftGraphForDevicesRange->endDataIndex = defaultSelectedEndIndex;


		////snprintf(textBuffer, 255, "Devices");
		////fftGraphForDevicesRange->SetText(textBuffer, 255, 0);
		
		if (nearFarDataAnalyzer->spectrumAnalyzer.deviceReceivers->count == 1)
		{
			fftGraphForDevicesRange->SetText(1, "FFT Graph");
		}
		else
			fftGraphForDevicesRange->SetText(1, "Combined FFT Graph for Devices");

		////snprintf(textBuffer, 255, "Combined Graph:");
		////fftGraphForDevicesRange->SetText(textBuffer, 255, 1);
		//fftGraphForDevicesRange->SetText(1, "Combined Graph:");

		graphs->AddGraph(fftGraphForDevicesRange);
	}	

	fftGraphStrengthsForDeviceRange = new Graph(300, 100);
	fftGraphStrengthsForDeviceRange->view = GraphView::Side;
	////fftGraphStrengthsForDeviceRange->graph_3D = true;
	fftGraphStrengthsForDeviceRange->showXAxis = 0;
	fftGraphStrengthsForDeviceRange->showYAxis = 0;
	fftGraphStrengthsForDeviceRange->showZAxis = 0;
	fftGraphStrengthsForDeviceRange->showLabels = false;
	fftGraphStrengthsForDeviceRange->SetDataSeriesStyle(GraphStyle::Graph3D);
	////fftGraphStrengthsForDeviceRange->SetDataSeriesColor(0, 0, 1, 1, ReceivingDataMode::Far);

	fftGraphStrengthsForDeviceRange->SetDataSeriesLineWidth(1);

	fftGraphStrengthsForDeviceRange->SetSize(100, GRAPH_HEIGHT);
	////fftGraphStrengthsForDeviceRange->SetPos(1000, GRAPH_HEIGHT, 0);	
	
	graphs->AddGraph(fftGraphStrengthsForDeviceRange);

	fftGraphStrengthsForDeviceRange->SetPos(fftGraphStrengthsForDeviceRange->pos.x + GRAPH_STRENGTHS_X_OFFSET, fftGraphStrengthsForDeviceRange->pos.y + GRAPH_HEIGHT, 0);


	////fftAverageGraphForDeviceRange = new Graph(100, graphResolution);
	fftAverageGraphForDeviceRange = new Graph(100, graphResolution);
	fftAverageGraphForDeviceRange->drawDepth = 1;
	fftAverageGraphForDeviceRange->showXAxis = 1;
	fftAverageGraphForDeviceRange->showYAxis = 1;
	fftAverageGraphForDeviceRange->showZAxis = 2;

	fftAverageGraphForDeviceRange->SetSize(1000, GRAPH_HEIGHT);
	////fftAverageGraphForDeviceRange->SetPos(1100, 0, 0);

	fftAverageGraphForDeviceRange->SetDataSeriesStyle(GraphStyle::Graph3D);	

	fftAverageGraphForDeviceRange->SetDataSeriesLineWidth(2);
	
	fftAverageGraphForDeviceRange->SetDataSeriesColor(1, 0, 0, 1, ReceivingDataMode::Near);
	fftAverageGraphForDeviceRange->SetDataSeriesColor(0, 0, 1, 1, ReceivingDataMode::Far);
	


	fftAverageGraphForDeviceRange->startDataIndex = defaultSelectedStartIndex;
	fftAverageGraphForDeviceRange->endDataIndex = defaultSelectedEndIndex;

	////snprintf(textBuffer, 255, "Devices Averaged");

	if (nearFarDataAnalyzer->spectrumAnalyzer.deviceReceivers->count == 1)
		fftAverageGraphForDeviceRange->SetText(1, "Averaged FFT For Current Scanning Range: ");

	////snprintf(textBuffer, 255, "Combined Graph:");
	//fftAverageGraphForDeviceRange->SetText(1, "Combined Graph:");

	graphs->AddGraph(fftAverageGraphForDeviceRange);

	//return 0;

	fftAverageGraphForDeviceRange->SetPos(fftAverageGraphForDeviceRange->pos.x, fftAverageGraphForDeviceRange->pos.y + GRAPH_HEIGHT, fftAverageGraphForDeviceRange->pos.z);


	fftAverageGraphStrengthsForDeviceRange = new Graph(100, 100, 3);
	fftAverageGraphStrengthsForDeviceRange->view = GraphView::Side;

	////fftAverageGraphStrengthsForDeviceRange = new Graph(100, 100);	
	//return 0;

	fftAverageGraphStrengthsForDeviceRange->showXAxis = 0;
	fftAverageGraphStrengthsForDeviceRange->showYAxis = 0;
	fftAverageGraphStrengthsForDeviceRange->showZAxis = 0;
	fftAverageGraphStrengthsForDeviceRange->showLabels = false;
	fftAverageGraphStrengthsForDeviceRange->SetDataSeriesStyle(GraphStyle::Graph3D);
	fftAverageGraphStrengthsForDeviceRange->SetDataSeriesColor(1, 0, 0, 1, ReceivingDataMode::Near);
	fftAverageGraphStrengthsForDeviceRange->SetDataSeriesColor(0, 0, 1, 1, ReceivingDataMode::Far);
	//fftAverageGraphStrengthsForDeviceRange->SetDataSeriesColor(1, 1, 0, 1, 2);



	fftAverageGraphStrengthsForDeviceRange->dataSeriesSameScale = false;

	fftAverageGraphStrengthsForDeviceRange->selectable = false;

	fftAverageGraphStrengthsForDeviceRange->SetDataSeriesLineWidth(1);

	fftAverageGraphStrengthsForDeviceRange->SetSize(100, GRAPH_HEIGHT);
	////fftAverageGraphStrengthsForDeviceRange->SetPos(1000, 0, 0);	
	

	graphs->AddGraph(fftAverageGraphStrengthsForDeviceRange);

	//return 0;

	fftAverageGraphStrengthsForDeviceRange->SetPos(fftAverageGraphStrengthsForDeviceRange->pos.x + GRAPH_STRENGTHS_X_OFFSET, fftAverageGraphStrengthsForDeviceRange->pos.y + GRAPH_HEIGHT * 2, fftAverageGraphStrengthsForDeviceRange->pos.z);

	//return 0;

	spectrumRangeGraph = new Graph(100, nearFarDataAnalyzer->scanningRange.length / DeviceReceiver::SAMPLE_RATE * graphResolution);
	////spectrumRangeGraph = new Graph(100, graphResolution);
	spectrumRangeGraph->drawDepth = 1;	
	spectrumRangeGraph->showXAxis = 1;
	spectrumRangeGraph->showYAxis = 1;
	spectrumRangeGraph->showZAxis = 2;

	spectrumRangeGraph->SetSize(1000, GRAPH_HEIGHT);
	////spectrumRangeGraph->SetPos(1100, -GRAPH_HEIGHT, 0);

	spectrumRangeGraph->SetDataSeriesStyle(GraphStyle::Graph3D);

	spectrumRangeGraph->SetDataSeriesColor(1, 0, 0, 1, ReceivingDataMode::Near);
	spectrumRangeGraph->SetDataSeriesColor(0, 0, 1, 1, ReceivingDataMode::Far);	

	////spectrumRangeGraph->SetGraphLabelValuesXAxis(SignalProcessingUtilities::ConvertToMHz(nearFarDataAnalyzer->scanningRange.lower), SignalProcessingUtilities::ConvertToMHz(nearFarDataAnalyzer->scanningRange.upper));
	
	spectrumRangeGraph->SetText(1, "Spectrum Graph");

	graphs->AddGraph(spectrumRangeGraph);

	spectrumRangeGraph->SetPos(spectrumRangeGraph->pos.x, spectrumRangeGraph->pos.y + GRAPH_HEIGHT * 2, spectrumRangeGraph->pos.z);	
	

	SetCenterView();

	//spectrumRangeGraph = NULL;

	nearFarDataAnalyzer->spectrumAnalyzer.deviceReceivers->dataGraph = dataGraph;
	nearFarDataAnalyzer->spectrumAnalyzer.deviceReceivers->correlationGraph = correlationGraph;
	nearFarDataAnalyzer->spectrumAnalyzer.deviceReceivers->fftGraphForDeviceRange = fftGraphForDeviceRange;
	nearFarDataAnalyzer->spectrumAnalyzer.deviceReceivers->fftGraphForDevicesRange = fftGraphForDevicesRange;
	nearFarDataAnalyzer->spectrumAnalyzer.deviceReceivers->fftAverageGraphForDeviceRange = fftAverageGraphForDeviceRange;
	nearFarDataAnalyzer->spectrumAnalyzer.deviceReceivers->fftGraphStrengthsForDeviceRange = fftGraphStrengthsForDeviceRange;
	nearFarDataAnalyzer->spectrumAnalyzer.deviceReceivers->fftAverageGraphStrengthsForDeviceRange = fftAverageGraphStrengthsForDeviceRange;
	nearFarDataAnalyzer->spectrumAnalyzer.deviceReceivers->spectrumRangeGraph = spectrumRangeGraph;

	////int deviceCount = nearFarDataAnalyzer->InitializeNearFarDataAnalyzer(20000, 1000000, 88000000, 108000000);
	////int deviceCount = nearFarDataAnalyzer->InitializeNearFarDataAnalyzer(20000, 1000000, 94000000, 97000000);

	nearFarDataAnalyzer->StartProcessing();

	nearFarDataAnalyzer->spectrumAnalyzer.SetSequenceFinishedFunction(&ProcessSequenceFinished);

	////Sleep(1000);	

	return 1;
}

LRESULT CALLBACK LowLevelMouseProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	//DebuggingUtilities::DebugPrint("2 dataIsNearTimeStamp = GetTickCount(); %d\n");
	if (nearFarDataAnalyzer->automatedDetection)
		nearFarDataAnalyzer->SetMode(ReceivingDataMode::Near);

	return CallNextHookEx(NULL, nCode, wParam, lParam);
}

LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	//DebuggingUtilities::DebugPrint("3 dataIsNearTimeStamp = GetTickCount(); %d\n");
	if (nearFarDataAnalyzer->automatedDetection)
		nearFarDataAnalyzer->SetMode(ReceivingDataMode::Near);	
	
	return CallNextHookEx(NULL, nCode, wParam, lParam);
}

void Initialize(uint32_t startFrequency, uint32_t endFrequency)
{	
	//InitializeGL();
	InitializeNearFarSpectrumAnalyzerAndGraphs(startFrequency, endFrequency);

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
	printf("Usage: SDRSpectrumAnalyzerOpenGL [-a] [-s] [-e] [-S]\n", argv[0]);	
	printf("-a: Automatically detect reradiated frequency ranges\n");
	printf("-s: Start frequency\n");
	printf("-e: End frequency\n");
	printf("-S: Sound cues for detecting increasing signal strengths\n");	
	printf("\n");
	printf("In Program Keys:\n", argv[0]);
	printf("\n");
	printf("1: Toggle data graph\n");
	printf("2: Toggle correlation graph (only for synchronized devices)\n");
	printf("3: Toggle devices fft graphs' graphics (only for synchronized devices)\n");
	printf("4: Toggle fft graph graphics\n");
	printf("5: Toggle strength graph graphics\n");
	printf("6: Toggle average fft graph graphics\n");
	printf("7: Toggle average strength graph graphics\n");
	printf("8: Toggle full spectrum range graph graphics\n");
	printf("\n");
	printf("SHIFT 1: Set View to data graph\n");
	printf("SHIFT 2: Set View to correlation graph\n");
	printf("SHIFT 3: Set View to devices fft graphs\n");
	printf("SHIFT 4: Set View to fft graph\n");
	printf("SHIFT 5: Set View to strength graph\n");
	printf("SHIFT 6: Set View to average fft graph\n");
	printf("SHIFT 7: Set View to average strength graph\n");
	printf("SHIFT 8: Set View to full spectrum range graph\n");
	printf("SHIFT C: Set View to all graphs\n");
	printf("\n");
	printf("n: Record near series data (for non-automated detection)\n");
	printf("f: Record far series data (for non-automated detection)\n");
	printf("\n");
	printf("SHIFT N: Clear near series data\n");
	printf("SHIFT F: Clear far series data\n");
	printf("\n");
	printf("press any key to continue\n");

	getch();
}

int main(int argc, char **argv)
{		
	uint32_t startFrequency = 88000000;
	uint32_t endFrequency = 108000000;
	bool automatedDetection = false;
	bool sound = false;

	int argc2 = 1;
	char *argv2[1] = { (char*)"Something" };
	glutInit(&argc2, argv2);	
	InitializeGL();

	if (argc < 2)
	{
		Usage(argc, argv);
		return 0;
	}

	for (uint8_t i = 0; i < argc; i++)
	{
		if (strcmp(argv[i], "-?") == 0)
		{
			Usage(argc, argv);
			return 0;
		}

		if (strcmp(argv[i], "-a") == 0)
			automatedDetection = true;

		if (strcmp(argv[i], "-s") == 0)
			startFrequency = atoi(argv[++i]);

		if (strcmp(argv[i], "-e") == 0)
			endFrequency = atoi(argv[++i]);

		if (strcmp(argv[i], "-S") == 0)
			sound = true;
	}		

	Initialize(startFrequency, endFrequency);
	
	if (nearFarDataAnalyzer)
	{
		nearFarDataAnalyzer->automatedDetection = automatedDetection;

		nearFarDataAnalyzer->spectrumAnalyzer.sound = sound;
	}

	glutMainLoop();

	return 0;
}