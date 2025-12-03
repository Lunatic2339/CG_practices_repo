#include <GL/glut.h>

GLint winWidth= 600, winHeight= 600;   //  Initial display-window size.
GLfloat x0 = 0.0, y0 = 0.0, z0 = 500;//  Viewing-coordinate origin.
GLfloat xref= 0.0, yref= 0.0, zref= 0.0;  //  Look-at point.
GLfloat Vx= 0.0, Vy= 1.0, Vz= 0.0;          //  View up vector.
GLfloat xwMin= -200.0, ywMin= -200.0, xwMax= 200.0, ywMax= 200.0; /*  Set coordinate limits for the clipping window:  */
GLfloat dnear= 10.0, dfar= 500.0; /*  Set positions for near and far clipping planes:  */
GLfloat theta = 60, aspect = winWidth/(float)winHeight;

void init (void)
{
	glClearColor(1.0, 1.0, 1.0, 0.0);
	glMatrixMode(GL_MODELVIEW);
	gluLookAt(x0, y0, z0, xref, yref, zref, Vx, Vy, Vz);
	glMatrixMode(GL_PROJECTION);
	// glFrustum(xwMin, xwMax, ywMin, ywMax, dnear, dfar);
	//gluPerspective(theta, aspect, dnear, dfar);
	glOrtho(xwMin, xwMax, ywMin, ywMax, dnear, dfar);
}

void displayFcn(void)
{
	glClear(GL_COLOR_BUFFER_BIT);
	glColor3f (1.0, 0.0, 0.0);         //  Set fill color to green.
	glPolygonMode(GL_FRONT, GL_FILL);
	glPolygonMode(GL_BACK, GL_LINE);  //  Wire-frame back face.
	glBegin(GL_QUADS);
	glVertex3f (-100.0, -100.0, 0.0);
	glVertex3f (100.0, -100.0, 0.0);
	glVertex3f (100.0, 100.0, 0.0);
	glVertex3f (-100.0, 100.0, 0.0);
	glEnd( );
	glFlush( );
}

void reshapeFcn(GLint newWidth, GLint newHeight) 
{
	glViewport(0, 0, newWidth, newHeight); 
	winWidth = newWidth; 
	winHeight = newHeight; 
}

void main(int argc, char** argv) 
{ 
	glutInit(&argc, argv); 
	glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB); 
	glutInitWindowPosition(50, 50); 
	glutInitWindowSize(winWidth, winHeight); 
	glutCreateWindow("¹Ú¼±¿ì 20220584"); 
	init(); 
	glutDisplayFunc(displayFcn); 
	glutReshapeFunc(reshapeFcn); 
	glutMainLoop(); 
}