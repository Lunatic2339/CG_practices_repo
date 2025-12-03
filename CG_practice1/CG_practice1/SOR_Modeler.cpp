#define _CRT_SECURE_NO_WARNINGS 
// 위 코드는 fopen 에러(C4996)를 막아주는 마법의 코드입니다. 꼭 맨 윗줄에 있어야 합니다.

#include <GL/glut.h>
#include <vector>
#include <cmath>
#include <iostream>
#include <cstdio> 

#define M_PI 3.14159265358979323846

// ----------------------------------------------------------
// 데이터 구조 클래스
// ----------------------------------------------------------
class Point3D {
public:
    float x, y, z;
    Point3D() { x = 0; y = 0; z = 0; }
    Point3D(float _x, float _y, float _z) {
        x = _x; y = _y; z = _z;
    }
};

// ----------------------------------------------------------
// 전역 변수
// ----------------------------------------------------------
std::vector<Point3D> inputPoints;
std::vector<std::vector<Point3D>> mesh;
std::vector<Point3D> flattenedVertices;

int winWidth = 800;
int winHeight = 600;
bool is3DMode = false;
float viewAngleY = 0.0f;
float viewAngleX = 0.0f;

int rotationSteps = 36; // 360도를 10도씩 쪼갬

// ----------------------------------------------------------
// 모델 생성 함수
// ----------------------------------------------------------
void generateSOR() {
    if (inputPoints.empty()) return;

    mesh.clear();
    flattenedVertices.clear();

    float angleStep = (2.0f * M_PI) / (float)rotationSteps;

    for (int i = 0; i < inputPoints.size(); i++) {
        std::vector<Point3D> row;
        Point3D p = inputPoints[i];

        float rawX = p.x - (winWidth / 2);
        float rawY = p.y - (winHeight / 2);

        for (int j = 0; j <= rotationSteps; j++) {
            float theta = j * angleStep;

            float x_new = rawX * cos(theta);
            float y_new = rawY;
            float z_new = -rawX * sin(theta);

            Point3D newPt(x_new, y_new, z_new);
            row.push_back(newPt);
            flattenedVertices.push_back(newPt);
        }
        mesh.push_back(row);
    }
    std::cout << "3D Created! (Space: Mode Change, S: Save)" << std::endl;
}

// ----------------------------------------------------------
// 파일 저장 함수
// ----------------------------------------------------------
void saveModel() {
    if (mesh.empty()) {
        std::cout << "No model to save." << std::endl;
        return;
    }

    FILE* fout = fopen("myModel.dat", "w");
    if (!fout) {
        std::cout << "File Open Error!" << std::endl;
        return;
    }

    // VERTEX 저장
    int pnum = flattenedVertices.size();
    fprintf(fout, "VERTEX = %d\n", pnum);
    for (int i = 0; i < pnum; i++) {
        fprintf(fout, "%.1f %.1f %.1f\n", flattenedVertices[i].x, flattenedVertices[i].y, flattenedVertices[i].z);
    }

    // FACE 계산 및 저장
    std::vector<int> faces;
    int cols = rotationSteps + 1;

    for (int i = 0; i < mesh.size() - 1; i++) {
        for (int j = 0; j < rotationSteps; j++) {
            int p1 = i * cols + j;
            int p2 = i * cols + (j + 1);
            int p3 = (i + 1) * cols + (j + 1);
            int p4 = (i + 1) * cols + j;

            // 삼각형 1
            faces.push_back(p1); faces.push_back(p2); faces.push_back(p3);
            // 삼각형 2
            faces.push_back(p1); faces.push_back(p3); faces.push_back(p4);
        }
    }

    int fnum = faces.size() / 3;
    fprintf(fout, "FACE = %d\n", fnum);
    for (int i = 0; i < fnum; i++) {
        fprintf(fout, "%d %d %d\n", faces[i * 3], faces[i * 3 + 1], faces[i * 3 + 2]);
    }

    fclose(fout);
    std::cout << "Save Complete: myModel.dat" << std::endl;
}

// ----------------------------------------------------------
// 화면 그리기 함수
// ----------------------------------------------------------
void display() {
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glLoadIdentity();

    if (!is3DMode) {
        // 2D 모드
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        gluOrtho2D(0, winWidth, 0, winHeight);

        glColor3f(0.5f, 0.5f, 0.5f);
        glBegin(GL_LINES);
        glVertex2f(winWidth / 2, 0); glVertex2f(winWidth / 2, winHeight);
        glEnd();

        glColor3f(1.0f, 1.0f, 1.0f);
        glPointSize(5.0f);
        glBegin(GL_POINTS);
        for (int i = 0; i < inputPoints.size(); i++) glVertex2f(inputPoints[i].x, inputPoints[i].y);
        glEnd();

        glBegin(GL_LINE_STRIP);
        for (int i = 0; i < inputPoints.size(); i++) glVertex2f(inputPoints[i].x, inputPoints[i].y);
        glEnd();
    }
    else {
        // 3D 모드
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        gluPerspective(45.0f, (float)winWidth / winHeight, 0.1f, 2000.0f);

        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        gluLookAt(0, 100, 800, 0, 0, 0, 0, 1, 0);

        glRotatef(viewAngleX, 1.0f, 0.0f, 0.0f);
        glRotatef(viewAngleY, 0.0f, 1.0f, 0.0f);

        glColor3f(0.0f, 1.0f, 0.0f);
        for (int i = 0; i < mesh.size() - 1; i++) {
            for (int j = 0; j < mesh[i].size() - 1; j++) {
                glBegin(GL_LINE_LOOP);
                glVertex3f(mesh[i][j].x, mesh[i][j].y, mesh[i][j].z);
                glVertex3f(mesh[i][j + 1].x, mesh[i][j + 1].y, mesh[i][j + 1].z);
                glVertex3f(mesh[i + 1][j + 1].x, mesh[i + 1][j + 1].y, mesh[i + 1][j + 1].z);
                glVertex3f(mesh[i + 1][j].x, mesh[i + 1][j].y, mesh[i + 1][j].z);
                glEnd();
            }
        }
    }
    glutSwapBuffers();
}

// ----------------------------------------------------------
// 입력 콜백 함수들
// ----------------------------------------------------------
void keyboard(unsigned char key, int x, int y) {
    if (key == ' ') {
        is3DMode = !is3DMode;
        if (is3DMode) generateSOR();
        else inputPoints.clear();
        glutPostRedisplay();
    }
    else if (key == 's' || key == 'S') {
        if (is3DMode) saveModel();
    }
    else if (key == 27) exit(0);
}

void specialKeys(int key, int x, int y) {
    if (is3DMode) {
        if (key == GLUT_KEY_LEFT) viewAngleY -= 5.0f;
        if (key == GLUT_KEY_RIGHT) viewAngleY += 5.0f;
        if (key == GLUT_KEY_UP) viewAngleX -= 5.0f;
        if (key == GLUT_KEY_DOWN) viewAngleX += 5.0f;
        glutPostRedisplay();
    }
}

void mouse(int button, int state, int x, int y) {
    if (!is3DMode && button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
        float inputX = (float)x;
        float inputY = (float)(winHeight - y);
        Point3D pt(inputX, inputY, 0);
        inputPoints.push_back(pt);
        glutPostRedisplay();
    }
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(winWidth, winHeight);
    glutCreateWindow("SOR Modeler - Clean Version");

    glEnable(GL_DEPTH_TEST);

    glutDisplayFunc(display);
    glutMouseFunc(mouse);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(specialKeys);

    glutMainLoop();
    return 0;
}