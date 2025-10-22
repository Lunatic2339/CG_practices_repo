#include <GL/glut.h>

// Initial display-window size.
GLsizei winWidth = 400, winHeight = 300;

void init(void)
{
	// Set display-window color to blue.
	glClearColor(0.0, 0.0, 1.0, 1.0);
	glMatrixMode(GL_PROJECTION);
	gluOrtho2D (0.0, 200.0, 0.0, 150.0);
}

void plotPoint (GLint x, GLint y)
{
	glBegin(GL_POINTS); 
	glVertex2i (x, y);
	glEnd();
}
void displayFcn(void)
{
	//  Clear display window.
	glClear(GL_COLOR_BUFFER_BIT);         
	//  Set point color to red.
	glColor3f(1.0, 0.0, 0.0);              
	//  Set point size to 3.0.}
	glPointSize(3.0);
}
/*  Move cursor while pressing c key enables freehand curve drawing.  */
void curveDrawing(GLubyte curvePlotKey, GLint xMouse, GLint yMouse)
 {
	GLint x = xMouse;
	GLint y = winHeight-yMouse;
	switch (curvePlotKey)
	{
	case 'c':
		plotPoint(x, y);
		break;
	default:
		break;
	}
	glFlush( );
}
void winReshapeFcn(int newWidth, int newHeight) 
{ 
	glViewport(0, 0, newWidth, newHeight); /*  Reset viewport and projection parameters  */
	glMatrixMode(GL_PROJECTION); 
	glLoadIdentity(); 
	gluOrtho2D(0.0, GLdouble(newWidth), 0.0, GLdouble(newHeight)); 
	winWidth = newWidth; /*  Reset display-window size parameters.  */
	winHeight = newHeight;
}

int main(int argc, char** argv) 
{
	glutInit(&argc, argv); 
	glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB); 
	glutInitWindowPosition(100, 100); 
	glutInitWindowSize(winWidth, winHeight); 
	glutCreateWindow("¹Ú¼±¿ì_20220584");
	init();



	glutDisplayFunc(displayFcn); 
	glutReshapeFunc(winReshapeFcn); 
	glutKeyboardFunc(curveDrawing); 
	glutMainLoop(); 

	return 0;
}