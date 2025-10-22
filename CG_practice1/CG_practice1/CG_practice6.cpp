#include <GL/glut.h>
#include <stdlib.h>
#include <iostream>

GLint TopLeftX, TopLeftY, BottomRightX, BottomRightY;
GLsizei winWidth = 300, winHeight = 300;




void MyDisplay()
{
	glViewport(0, 0, winWidth, winHeight);
	glClearColor(1.0, 1.0, 1.0, 1.0);
	glClear(GL_COLOR_BUFFER_BIT);
	glColor3f(0.5, 0.5, 0.5);
	glBegin(GL_POLYGON);
	glVertex3f(TopLeftX / 150.0, (300-TopLeftY) / 150.0, 0.0);
	glVertex3f(TopLeftX / 150.0, (300-BottomRightY) / 150.0, 0.0);
	glVertex3f(BottomRightX / 150.0,  (300-BottomRightY) / 150.0, 0.0);
	glVertex3f(BottomRightX / 150.0, (300-TopLeftY) / 150.0, 0.0);
	glEnd();
	


	glFlush();
}

void MyMouseClick(GLint Button, GLint State, GLint X, GLint Y)
{
	if (Button == GLUT_LEFT_BUTTON && State == GLUT_DOWN)
	{
		TopLeftX = X-150;
		TopLeftY = Y+150;
	}
}

void MyMouseMove(GLint X, GLint Y)
{
	BottomRightX = X-150;
	BottomRightY = Y+150;
	glutPostRedisplay();
}

int main(int argc, char** argv)
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);
	glutInitWindowPosition(100, 100);
	glutInitWindowSize(winWidth, winHeight);
	glutCreateWindow("¹Ú¼±¿ì_20220584");

	glutDisplayFunc(MyDisplay);
	glutMouseFunc(MyMouseClick);
	glutMotionFunc(MyMouseMove);
	glutMainLoop();
}