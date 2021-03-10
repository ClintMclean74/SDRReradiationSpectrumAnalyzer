#include <GL/glut.h>
#include "GraphicsUtilities.h"

//namespace GraphicsUtilities
//{			
	float GraphicsUtilities::fontScale = 0.2;

	void GraphicsUtilities::DrawText(char *string, float x, float y, float z, float scale, float angle)
	{
		////if (string[0] != '\0')
		{
			glPushMatrix();

			char *c;
			glMatrixMode(GL_MODELVIEW);
			////glLoadIdentity();
			glTranslatef(x, y, z);
			glScalef(scale, scale, scale);
			glRotatef(angle, 0, 0, 1);

			glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

			////glDisable(GL_DEPTH_TEST);

			for (c = string; *c != '\0'; c++)
			{
				glutStrokeCharacter(GLUT_STROKE_ROMAN, *c);
			}

			////glEnable(GL_DEPTH_TEST);

			glPopMatrix();
		}
	}
//};
