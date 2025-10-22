#include <GL/glut.h>
#include <stdlib.h>

GLsizei winWidth = 300, winHeight = 300;

GLfloat Delta = 0.0;

void MyDisplay()
{
	glClear(GL_COLOR_BUFFER_BIT);
	glColor3f(0.0, 0.5, 0.8);
	glBegin(GL_POLYGON);
		glVertex3f(-1.0 + Delta,-0.5, 0.0);
		glVertex3f(0.0 + Delta,-0.5, 0.0);
		glVertex3f(0.0 + Delta, 0.5, 0.0);
		glVertex3f(-1.0 + Delta, 0.5, 0.0);
	glEnd();
	glutSwapBuffers();
}

void MyIdle()
{
	Delta += 0.001f;
	if (Delta > 1.5)
		Delta = -1.0;
	glutPostRedisplay();
}

int main(int argc, char** argv)
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE);
	glutInitWindowPosition(100, 100);
	glutInitWindowSize(winWidth, winHeight);
	glutCreateWindow("¹Ú¼±¿ì_20220584");
	glClearColor(1.0, 1.0, 1.0, 1.0);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(-2.0, 2.0, -2.0, 2.0, -1.0, 1.0);

	glutDisplayFunc(MyDisplay);
	glutIdleFunc(MyIdle);
	glutMainLoop();
	return 0;
}