#define _CRT_SECURE_NO_WARNINGS 
#include <GL/glut.h>
#include <vector>
#include <cstdio>
#include <iostream>

// ----------------------------------------------------------
// 데이터 구조
// ----------------------------------------------------------
struct Point3D {
    float x, y, z;
};

struct Face {
    int v1, v2, v3; // 삼각형을 이루는 점의 인덱스 3개
};

// ----------------------------------------------------------
// 전역 변수
// ----------------------------------------------------------
std::vector<Point3D> vertices; // 불러온 점들
std::vector<Face> faces;       // 불러온 면들

float angleY = 0.0f; // 모델 회전용

// ----------------------------------------------------------
// [핵심] 파일 불러오기 함수 (Load)
// 저장된 myModel.dat를 읽어서 메모리에 적재합니다.
// ----------------------------------------------------------
void loadModel(const char* filename) {
    FILE* fp = fopen(filename, "r");
    if (!fp) {
        std::cout << "파일을 찾을 수 없습니다: " << filename << std::endl;
        std::cout << "프로젝트 폴더에 myModel.dat 파일이 있는지 확인하세요!" << std::endl;
        return;
    }

    char buffer[128];
    int numVertices = 0;
    int numFaces = 0;

    // 1. VERTEX 개수 읽기
    // 포맷: VERTEX = 100
    fscanf(fp, "%s %s %d", buffer, buffer, &numVertices);

    // 점 좌표 읽기
    for (int i = 0; i < numVertices; i++) {
        Point3D p;
        fscanf(fp, "%f %f %f", &p.x, &p.y, &p.z);
        vertices.push_back(p);
    }

    // 2. FACE 개수 읽기
    // 포맷: FACE = 200
    fscanf(fp, "%s %s %d", buffer, buffer, &numFaces);

    // 면 인덱스 읽기
    for (int i = 0; i < numFaces; i++) {
        Face f;
        fscanf(fp, "%d %d %d", &f.v1, &f.v2, &f.v3);
        faces.push_back(f);
    }

    fclose(fp);
    std::cout << "모델 로딩 성공! (점: " << numVertices << ", 면: " << numFaces << ")" << std::endl;
}

// ----------------------------------------------------------
// 화면 그리기 (렌즈 설정 추가 + 축 그리기)
// ----------------------------------------------------------
void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // 1. [중요] 렌즈 설정 (Projection Matrix)
    // 이 부분이 빠져서 그동안 검은 화면만 보였습니다.
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    // 시야각 60도, 종횡비 800/600, 가까운곳 1.0, 먼곳 2000.0까지 보겠다
    gluPerspective(60.0f, 800.0f / 600.0f, 1.0f, 2000.0f);

    // 2. 카메라 위치 설정 (ModelView Matrix)
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // 카메라를 (300, 300, 800) 위치에 두고, (0,0,0)을 바라봄
    gluLookAt(300, 300, 800, 0, 0, 0, 0, 1, 0);

    // 3. 디버깅용 기준 축 그리기 (빨/초/파)
    glDisable(GL_LIGHTING);
    glLineWidth(3.0f);
    glBegin(GL_LINES);
    glColor3f(1.0f, 0.0f, 0.0f); // X축 (Red)
    glVertex3f(0, 0, 0); glVertex3f(500, 0, 0);

    glColor3f(0.0f, 1.0f, 0.0f); // Y축 (Green)
    glVertex3f(0, 0, 0); glVertex3f(0, 500, 0);

    glColor3f(0.0f, 0.0f, 1.0f); // Z축 (Blue)
    glVertex3f(0, 0, 0); glVertex3f(0, 0, 500);
    glEnd();

    // 4. 모델 그리기
    glPushMatrix();
    // 모델이 너무 크거나 작을 수 있으니 스케일 조정 (필요하면 숫자 변경)
    // glScalef(0.5f, 0.5f, 0.5f); 

    glRotatef(angleY, 0.0f, 1.0f, 0.0f); // 회전

    glColor3f(1.0f, 1.0f, 0.0f); // 노란색

    // 점이 제대로 로드되었는지 확인하기 위해 점(POINT)으로 먼저 찍어봄
    glPointSize(2.0f);
    glBegin(GL_POINTS);
    // 만약 면(Face) 정보가 꼬였어도 점은 나와야 함
    for (int i = 0; i < vertices.size(); i++) {
        glVertex3f(vertices[i].x, vertices[i].y, vertices[i].z);
    }
    glEnd();

    // 와이어프레임(선)으로 그리기
    glColor3f(1.0f, 1.0f, 1.0f); // 흰색
    for (int i = 0; i < faces.size(); i++) {
        Point3D p1 = vertices[faces[i].v1];
        Point3D p2 = vertices[faces[i].v2];
        Point3D p3 = vertices[faces[i].v3];

        glBegin(GL_LINE_LOOP);
        glVertex3f(p1.x, p1.y, p1.z);
        glVertex3f(p2.x, p2.y, p2.z);
        glVertex3f(p3.x, p3.y, p3.z);
        glEnd();
    }
    glPopMatrix();

    glutSwapBuffers();
}

// 애니메이션 (자동 회전)
void timer(int value) {
    angleY += 1.0f;
    if (angleY > 360) angleY -= 360;
    glutPostRedisplay();
    glutTimerFunc(16, timer, 0); // 60 FPS
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(800, 600);
    glutCreateWindow("Game Step 1: Model Viewer");

    glEnable(GL_DEPTH_TEST);

    // 모델 불러오기 (파일 이름 확인!)
    loadModel("myModel.dat");

    glutDisplayFunc(display);
    glutTimerFunc(0, timer, 0);

    glutMainLoop();
    return 0;
}