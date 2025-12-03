#include<GL/glut.h>

GLfloat MyVertices[8][3]={{-0.25,-0.25,0.25},{-0.25,0.25,0.25},{0.25,0.25,0.25},{0.25,-0.25,0.25},{-0.25,-0.25,-0.25},{-0.25,0.25,-0.25},{0.25,0.25,-0.25},{0.25,-0.25,-0.25}};
GLfloat MyColors[8][3]={{0.2,0.2,0.2},{1.0,0.0,0.0},{1.0,1.0,0.0},{0.0,1.0,0.0},{0.0,0.0,1.0},{1.0,0.0,1.0},{1.0,1.0,1.0},{0.0,1.0,1.0}};
GLubyte MyVertexList[24]={0,3,2,1,2,3,7,6,0,4,7,3,1,2,6,5,4,5,6,7,0,1,5,4};

int MyListID; 

void MyDisplay() 
{ 
	glClear(GL_COLOR_BUFFER_BIT); 
	glFrontFace(GL_CCW); 
	glEnable(GL_CULL_FACE); 
	glMatrixMode(GL_MODELVIEW); 
	glLoadIdentity(); 
	glPushMatrix(); 
	glRotatef(30.0, 1.0, 1.0, 1.0); 
	glScalef(1.2, 2, 1.4); 
	glTranslatef(-0.3, 0.0, 0.0); 
	glShadeModel(GL_FLAT); 
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); 
	glCallList(MyListID); 
	glPopMatrix(); 
	glPushMatrix(); 
	glTranslatef(0.5, 0, 0); 
	glRotatef(30.0, 1.0, 1.0, 0.0); 
	glShadeModel(GL_SMOOTH); 
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); 
	glCallList(MyListID); 
	glPopMatrix(); 
	glFlush();
}


void MakeCubePlayList() 
{ 
	MyListID = glGenLists(1); 
	glNewList(MyListID, GL_COMPILE); 

	glEnableClientState(GL_COLOR_ARRAY);
	glEnableClientState(GL_VERTEX_ARRAY); 
	glColorPointer(3, GL_FLOAT, 0, MyColors); 

	glVertexPointer(3, GL_FLOAT, 0, MyVertices); 
	for (GLint i = 0; i < 6; i++) glDrawElements(GL_POLYGON, 4, GL_UNSIGNED_BYTE, &MyVertexList[4 * i]); 
	glEndList(); 
}


int main(int argc, char** argv) {
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGB);
	glutInitWindowSize(300, 300);
	glutInitWindowPosition(0, 0);
	glutCreateWindow("20220584_¹Ú¼±¿ì");
	glClearColor(1.0, 1.0, 1.0, 1.0);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(-1.0, 1.0, -1.0, 1.0, -1.0, 1.0);
	glutDisplayFunc(MyDisplay);
	MakeCubePlayList();
	glutMainLoop();
	return 0;
}