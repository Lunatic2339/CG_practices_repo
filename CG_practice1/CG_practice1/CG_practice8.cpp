#include <GL/glut.h>
#include <stdlib.h>
#include <iostream>

GLint TopLeftX, TopLeftY, BottomRightX, BottomRightY;
GLsizei winWidth = 300, winHeight = 300;

GLshort Shape = 1;
GLfloat ShapeSize = 1;

void MyDisplay()
{
	glClear(GL_COLOR_BUFFER_BIT);
	glColor3f(0.5, 0.5, 0.5);
	if (Shape == 1)
	{

		glutWireSphere(0.5 / ShapeSize, 20, 20);
	}
	else if (Shape == 2)
	{

		glutWireTorus(0.1 / ShapeSize, 0.3 / ShapeSize, 40, 20);
	}
	else if (Shape == 3)
	{
		glRotatef(45, 1.0, 1.0, 0.0);
		glutWireCube(0.5 / ShapeSize);
		glRotatef(-45, 1, 1, 0);
	}
	glFlush();
}

void MyMainMenu(GLint Select)
{
	if (Select == 1)
		Shape = 1;
	else if (Select == 2)
		Shape = 2;
	else if (Select == 3)
		Shape = 3;
	else if (Select == 4)
		exit(0);
	glutPostRedisplay();
}
void MySubMenu(GLint Select)
{
	if (Select == 1)
		ShapeSize = 2;
	else if (Select == 2)
		ShapeSize = 1;
	else if (Select == 3)
		ShapeSize = 0.67;
	glutPostRedisplay();
}
int main(int argc, char** argv)
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);
	glutInitWindowPosition(100, 100);
	glutInitWindowSize(winWidth, winHeight);
	glutCreateWindow("¹Ú¼±¿ì_20220584");
	glClearColor(1.0, 1.0, 1.0, 1.0);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(-1, 1, -1, 1, -1, 1);

	int MySubMenuID = glutCreateMenu(MySubMenu);
	glutAddMenuEntry("Small", 1);
	glutAddMenuEntry("Big", 2);
	glutAddMenuEntry("Super Big", 3);
	GLint MainMenu = glutCreateMenu(MyMainMenu);
	glutAddMenuEntry("Sphere", 1);
	glutAddMenuEntry("Torus", 2);
	glutAddMenuEntry("Cube", 3);
	glutAddSubMenu("Chagne Size", MySubMenuID);
	glutAddMenuEntry("Exit", 4);
	glutAttachMenu(GLUT_RIGHT_BUTTON);
	glutDisplayFunc(MyDisplay);

	glutMainLoop();

	return 0;
}