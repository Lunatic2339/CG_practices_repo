#include<GL/glut.h>
#include<GL/gl.h>
#include<GL/glu.h>

//디스플레이콜백함수
static void MyDisplay()
{

	//GL상태변수설정
	glClear(GL_COLOR_BUFFER_BIT);
	glViewport(0, 0, 300, 300);


	//흰색사각형
	glColor3f(1.0, 1.0, 1.0);
	glBegin(GL_POLYGON);
	glVertex3f(-0.5, -0.5, 0.0);
	glVertex3f(0.5, -0.5, 0.0);
	glVertex3f(0.5, 0.5, 0.0);
	glVertex3f(-0.5, 0.5, 0.0);
	glEnd();

	/*
	//빨간색삼각형
	glColor3f(1, 0, 0);
	glBegin(GL_POLYGON);
	glVertex3f(-0.25, -0.25, -0.5);
	glVertex3f(0.25, -0.25, -0.5);
	glVertex3f(0, 0.25, -0.5);
	glEnd();
	*/

	//출력명령
	glFlush();
}

int main(int argc, char** argv)
{
	//GLUT윈도우함수
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGB);
	glutInitWindowSize(300, 300);
	glutInitWindowPosition(0, 0);
	glutCreateWindow("박선우_20220584");

	//GL상태변수설정
	glClearColor(0.0, 0.0, 0.0, 1.0);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(-1, 1, -1, 1, -1, 1);


	//GLUT콜백함수등록
	glutDisplayFunc(MyDisplay);

	//이벤트루프진입
	glutMainLoop();

	return 0;
}