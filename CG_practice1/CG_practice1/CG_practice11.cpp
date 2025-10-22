#include <GL/glut.h>
#include <time.h>

GLsizei winWidth = 500, winHeight = 500;

unsigned char PALETTE[16][3] =
{
	{255, 255, 255},
	{0, 255, 255},
	{255, 0, 255},
	{0, 0, 255},

	{192, 192, 192},
	{128,128, 128},
	{0, 128, 128},
	{128, 0, 128},

	{0, 0, 128},
	{255, 255, 0},
	{0, 255, 0},
	{128, 128, 0},

	{0, 128, 0},
	{255, 0, 0}, 
	{128, 0, 0},
	{0, 0, 0}
};

GLfloat Delta = 0.0;
GLint Index = 0;
GLfloat Red = 0.0, Green = 0.0, Blue = 0.0;

void winReshapeFcn(GLint newWidth, GLint newHeight)
{
	/*  Reset viewport and projection parameters  */
	glViewport(0, 0, newWidth, newHeight);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	GLfloat WidthFactor = (GLfloat)newWidth / (GLfloat)winWidth;
	GLfloat HeightFactor = (GLfloat)newHeight / (GLfloat)winHeight;
	glOrtho(-1.0 * WidthFactor, 1.0 * WidthFactor, -1.0 * HeightFactor, 1.0 * HeightFactor, -1.0, 1.0);

	/*  Reset display-window size parameters.  */
	winWidth = newWidth;
	winHeight = newHeight;
}
void MyDisplay() 
{ 
	Red = PALETTE[Index][0] / 255.0f;
	Green = PALETTE[Index][1] / 255.0f; 
	Blue = PALETTE[Index][2] / 255.0f; 
	glColor3f(Red, Green, Blue); 
	glBegin(GL_LINES); 
	glVertex3f(-1.0 + Delta, 1.0, 0.0); 
	glVertex3f(1.0 - Delta, -1.0, 0.0); 
	glVertex3f(-1.0, -1.0 + Delta, 0.0); 
	glVertex3f(1.0, 1.0 - Delta, 0.0); 
	glEnd(); 
	glutSwapBuffers(); 
}
void MyTimer(int Value)
{
	if (Delta < 2.0f)
		Delta = Delta + 0.01;
	else
	{
		Delta = 0.0;
		if (++Index >= 15)
		{
			Index = 0;
			glClear(GL_COLOR_BUFFER_BIT);
		}
	}

	glutPostRedisplay();
	glutTimerFunc(5, MyTimer, 1);
}

int main(int argc, char** argv)
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE);
	glutInitWindowSize(500, 500);
	glutInitWindowPosition(0, 0);
	glutCreateWindow("¹Ú¼±¿ì_20220584");
	glutTimerFunc(5, MyTimer, 1);
	glutDisplayFunc(MyDisplay);
	glutReshapeFunc(winReshapeFcn);
	glutMainLoop();
	return 0;
}
