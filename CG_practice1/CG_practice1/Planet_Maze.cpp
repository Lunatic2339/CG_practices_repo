#define _CRT_SECURE_NO_WARNINGS
#include <GL/glut.h>
#include <vector>
#include <cmath>
#include <cstdio>
#include <cstdlib> 

// ----------------------------------------------------------
// 데이터 구조
// ----------------------------------------------------------
struct Point3D { float x, y, z; };
struct Face { int v1, v2, v3; };

std::vector<Point3D> vertices;
std::vector<Face> faces;

// ----------------------------------------------------------
// 전역 변수
// ----------------------------------------------------------
float planetRadius = 30.0f; // 행성 크기 적당히 조절
float playerHeight = 2.0f;
float worldRotX = 0.0f;
float worldRotY = 0.0f;

// 윈도우 크기 (화면 분할 비율 계산용)
int winW = 1200; // 가로를 좀 넓게 잡음
int winH = 600;

// 미로 맵 (20x20) - 1:벽, 9:모델
int map[20][20] = { 0 };

// ----------------------------------------------------------
// 유틸리티 함수
// ----------------------------------------------------------
Point3D calculateNormal(Point3D p1, Point3D p2, Point3D p3) {
    Point3D u = { p2.x - p1.x, p2.y - p1.y, p2.z - p1.z };
    Point3D v = { p3.x - p1.x, p3.y - p1.y, p3.z - p1.z };
    Point3D normal = { u.y * v.z - u.z * v.y, u.z * v.x - u.x * v.z, u.x * v.y - u.y * v.x };
    float len = sqrt(normal.x * normal.x + normal.y * normal.y + normal.z * normal.z);
    if (len > 0) { normal.x /= len; normal.y /= len; normal.z /= len; }
    return normal;
}

void loadModel(const char* filename) {
    FILE* fp = fopen(filename, "r");
    if (!fp) return;
    char buf[128];
    int numV, numF;
    fscanf(fp, "%s %s %d", buf, buf, &numV);
    for (int i = 0; i < numV; i++) {
        Point3D p; fscanf(fp, "%f %f %f", &p.x, &p.y, &p.z); vertices.push_back(p);
    }
    fscanf(fp, "%s %s %d", buf, buf, &numF);
    for (int i = 0; i < numF; i++) {
        Face f; fscanf(fp, "%d %d %d", &f.v1, &f.v2, &f.v3); faces.push_back(f);
    }
    fclose(fp);
}

void initMap() {
    // 맵 초기화 (테두리 벽 + 랜덤 장애물 + 모델)
    for (int i = 0; i < 20; i++) {
        map[0][i] = 1; map[19][i] = 1;
        map[i][0] = 1; map[i][19] = 1;
    }
    // 장애물 예시
    map[5][5] = 1; map[5][6] = 1; map[6][5] = 1;
    // 모델 배치 (9번)
    map[10][10] = 9;
    map[15][5] = 9;
}

// ----------------------------------------------------------
// [공통] 장면 그리기 함수
// 1인칭, 3인칭 모두 똑같은 월드를 그려야 하므로 함수로 분리
// ----------------------------------------------------------
void drawScene() {
    // 1. 행성 (파란 공)
    GLfloat planetAmb[] = { 0.1f, 0.1f, 0.4f, 1.0f };
    GLfloat planetDiff[] = { 0.2f, 0.2f, 0.8f, 1.0f };
    glMaterialfv(GL_FRONT, GL_AMBIENT, planetAmb);
    glMaterialfv(GL_FRONT, GL_DIFFUSE, planetDiff);
    glutSolidSphere(planetRadius, 50, 50);

    // 2. 미로 벽과 모델
    float angleStep = 6.0f; // 구면 간격 (조절 가능)

    for (int r = 0; r < 20; r++) {
        for (int c = 0; c < 20; c++) {
            if (map[r][c] == 0) continue;

            glPushMatrix();
            // 위도, 경도 회전
            float theta = (c - 10) * angleStep;
            float phi = (r - 10) * angleStep;
            glRotatef(theta, 0.0f, 1.0f, 0.0f);
            glRotatef(phi, 1.0f, 0.0f, 0.0f);
            glTranslatef(0.0f, 0.0f, planetRadius); // 표면으로 이동

            if (map[r][c] == 1) {
                // [벽] 갈색 큐브
                GLfloat wallCol[] = { 0.6f, 0.4f, 0.2f, 1.0f };
                glMaterialfv(GL_FRONT, GL_DIFFUSE, wallCol);
                glScalef(1.0f, 1.0f, 4.0f); // 높이 솟게
                glutSolidCube(1.5f);
            }
            else if (map[r][c] == 9) {
                // [모델] 금색
                glRotatef(-90, 1.0f, 0.0f, 0.0f);
                glScalef(0.005f, 0.005f, 0.005f); // 크기 조절

                GLfloat goldDiff[] = { 0.8f, 0.7f, 0.2f, 1.0f };
                GLfloat goldSpec[] = { 1.0f, 1.0f, 1.0f, 1.0f };
                glMaterialfv(GL_FRONT, GL_DIFFUSE, goldDiff);
                glMaterialfv(GL_FRONT, GL_SPECULAR, goldSpec);
                glMaterialf(GL_FRONT, GL_SHININESS, 60.0f);

                glBegin(GL_TRIANGLES);
                for (int i = 0; i < faces.size(); i++) {
                    Point3D p1 = vertices[faces[i].v1];
                    Point3D p2 = vertices[faces[i].v2];
                    Point3D p3 = vertices[faces[i].v3];
                    Point3D n = calculateNormal(p1, p2, p3);
                    glNormal3f(n.x, n.y, n.z);
                    glVertex3f(p1.x, p1.y, p1.z);
                    glVertex3f(p2.x, p2.y, p2.z);
                    glVertex3f(p3.x, p3.y, p3.z);
                }
                glEnd();
            }
            glPopMatrix();
        }
    }
}

// ----------------------------------------------------------
// 화면 출력 (Display) - 화면 분할의 핵심!
// ----------------------------------------------------------
void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // 공통 조명 설정
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    GLfloat lightPos[] = { 100.0f, 100.0f, 100.0f, 1.0f };
    glLightfv(GL_LIGHT0, GL_POSITION, lightPos);

    // ==========================================
    // [왼쪽 화면] 1인칭 시점 (Player View)
    // ==========================================
    glViewport(0, 0, winW / 2, winH); // 화면 왼쪽 절반 사용
    glLoadIdentity();

    // 카메라: 행성 표면 위에 고정
    gluLookAt(0, planetRadius + playerHeight, 0,  // 눈
        0, planetRadius + playerHeight, -1, // 보는 곳 (전방)
        0, 1, 0);                           // 윗 방향

    glPushMatrix();
    // **중요**: 1인칭에서는 내가 움직이는 게 아니라 세상이 반대로 돔
    glRotatef(worldRotX, 1.0f, 0.0f, 0.0f);
    glRotatef(worldRotY, 0.0f, 1.0f, 0.0f);
    drawScene(); // 월드 그리기
    glPopMatrix();


    // ==========================================
    // [오른쪽 화면] 3인칭 시점 (God View)
    // ==========================================
    glViewport(winW / 2, 0, winW / 2, winH); // 화면 오른쪽 절반 사용
    glLoadIdentity();

    // 카메라: 멀리 떨어져서 전체를 조망
    gluLookAt(0, 80, 100,  // 눈 (위, 뒤쪽)
        0, 0, 0,     // 행성 중심을 바라봄
        0, 1, 0);

    // 1. 월드 그리기 (1인칭과 똑같이 회전 상태 반영)
    glPushMatrix();
    glRotatef(worldRotX, 1.0f, 0.0f, 0.0f);
    glRotatef(worldRotY, 0.0f, 1.0f, 0.0f);
    drawScene();
    glPopMatrix();

    // 2. 플레이어 위치 표시 (빨간 박스)
    // 플레이어는 1인칭 시점 기준으로 항상 (0, R+h, 0)에 고정되어 있음
    // 따라서 월드 회전 바깥에 그려야 함
    glDisable(GL_LIGHTING); // 잘 보이게 조명 끔
    glPushMatrix();
    glTranslatef(0.0f, planetRadius + 1.0f, 0.0f);
    glColor3f(1.0f, 0.0f, 0.0f); // 빨간색 (나)
    glutSolidCube(3.0f); // 좀 크게 그림
    glPopMatrix();
    glEnable(GL_LIGHTING);

    // ==========================================
    // 가운데 구분선 (선택사항)
    // ==========================================
    glViewport(0, 0, winW, winH);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, winW, 0, winH);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    glColor3f(1.0f, 1.0f, 1.0f);
    glBegin(GL_LINES);
    glVertex2f(winW / 2, 0); glVertex2f(winW / 2, winH);
    glEnd();
    glEnable(GL_DEPTH_TEST);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);


    glutSwapBuffers();
}

void reshape(int w, int h) {
    winW = w;
    winH = h;
    // 렌즈 설정 (비율은 절반 너비 기준)
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60.0f, (float)(w / 2) / h, 0.1f, 1000.0f);
    glMatrixMode(GL_MODELVIEW);
}

void specialKeys(int key, int x, int y) {
    float speed = 2.0f;
    if (key == GLUT_KEY_UP)    worldRotX -= speed;
    if (key == GLUT_KEY_DOWN)  worldRotX += speed;
    if (key == GLUT_KEY_LEFT)  worldRotY -= speed;
    if (key == GLUT_KEY_RIGHT) worldRotY += speed;
    glutPostRedisplay();
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(winW, winH);
    glutCreateWindow("Debug Mode: Split Screen (Left: 1st / Right: 3rd)");

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_NORMALIZE);

    initMap();
    loadModel("myModel.dat");

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutSpecialFunc(specialKeys);

    glutMainLoop();
    return 0;
}