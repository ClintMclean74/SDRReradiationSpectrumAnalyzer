#include "freeglut/include/GL/glut.h"
#include "GraphicsUtilities.h"


namespace GraphicsUtilities
{
    void DrawText(char *string, float x, float y, float z, float scale, float angle)
    {
        glPushMatrix();

        char *c;
        glMatrixMode(GL_MODELVIEW);

        glTranslatef(x, y, z);
        glScalef(scale, scale, scale);
        glRotatef(angle, 0, 0, 1);

        for (c = string; *c != '\0'; c++)
        {
            glutStrokeCharacter(GLUT_STROKE_ROMAN, *c);
        }

        glPopMatrix();
    }
}
