#include <GL/glut.h>
#include "GraphicsUtilities.h"

float GraphicsUtilities::fontScale = 0.2;

void GraphicsUtilities::DrawText(char *string, float x, float y, float z, float scale, float angle)
{
	glPushMatrix();

	char *c;
	glMatrixMode(GL_MODELVIEW);

	glTranslatef(x, y, z);
	glScalef(scale, scale, scale);
	glRotatef(angle, 0, 0, 1);

	//glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

	for (c = string; *c != '\0'; c++)
	{
		glutStrokeCharacter(GLUT_STROKE_ROMAN, *c);
	}

	glPopMatrix();

}
