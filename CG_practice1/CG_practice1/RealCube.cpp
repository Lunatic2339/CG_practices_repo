#define _CRT_SECURE_NO_WARNINGS

#include <GL/glut.h>
#include <vector>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

#define M_PI 3.14159265358979323846

// ----------------------------------------------------------
// [데이터 구조체]
// ----------------------------------------------------------
struct Point3D { float x, y, z; };
struct Face { int v1, v2, v3; };

// ----------------------------------------------------------
// [전역 변수]
// ----------------------------------------------------------
float planetRadius = 80.0f;
float playerHeight = 3.0f;
float wallDepth = 20.0f; // 벽이 안쪽으로 파고드는 깊이 (충돌 확인용으로 깊게 설정)

// [핵심 기능] B키 토글 변수
bool useSmartWall = true;

// 행성 회전 행렬 (플레이어 이동용)
GLfloat planetRotationMatrix[16] = { 1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1 };
float cameraYaw = 0.0f, cameraPitch = 0.0f;

// 마우스 설정
float mouseSensitivity = 0.005f;
bool mouseCaptured = true;

int winW = 1200, winH = 800;

// 맵 설정
const int N = 15;
enum { FACE_FRONT = 0, FACE_BACK = 1, FACE_RIGHT = 2, FACE_LEFT = 3, FACE_TOP = 4, FACE_BOTTOM = 5 };
int map[6][N][N] = { 0 };

// ----------------------------------------------------------
// [수학 함수]
// ----------------------------------------------------------
Point3D normalize(Point3D p) {
    float len = sqrt(p.x * p.x + p.y * p.y + p.z * p.z);
    if (len == 0) return { 0, 0, 0 };
    return { p.x / len, p.y / len, p.z / len };
}

Point3D getSpherePoint(int face, float u, float v, float r) {
    float x = (u - 0.5f) * 2.0f;
    float y = (v - 0.5f) * 2.0f;
    float z = 1.0f;
    Point3D p = { 0, 0, 0 };
    switch (face) {
    case FACE_BACK:   p = { x, y, z }; break;
    case FACE_FRONT:  p = { -x, y, -z }; break;
    case FACE_RIGHT:  p = { z, y, -x }; break;
    case FACE_LEFT:   p = { -z, y, x }; break;
    case FACE_TOP:    p = { x, z, -y }; break;
    case FACE_BOTTOM: p = { x, -z, y }; break;
    }
    p = normalize(p);
    p.x *= r; p.y *= r; p.z *= r;
    return p;
}

Point3D multiplyMatrixVector(Point3D p, GLfloat* m) {
    Point3D res;
    res.x = m[0] * p.x + m[4] * p.y + m[8] * p.z + m[12];
    res.y = m[1] * p.x + m[5] * p.y + m[9] * p.z + m[13];
    res.z = m[2] * p.x + m[6] * p.y + m[10] * p.z + m[14];
    return res;
}

// ----------------------------------------------------------
// [맵 로딩]
// ----------------------------------------------------------
void loadMapFromCSV(int face, const char* filename, int angle) {
    std::ifstream file(filename);
    if (!file.is_open()) return;

    int rot = angle % 360; if (rot < 0) rot += 360;
    std::string line; int fileRow = 0;

    while (std::getline(file, line) && fileRow < N) {
        std::stringstream ss(line); std::string cell; int fileCol = 0;
        while (std::getline(ss, cell, ',') && fileCol < N) {
            int val = 0; try { val = std::stoi(cell); }
            catch (...) {}
            if (val == 0) val = 1; else if (val == 1) val = 0; // 반전

            int tr = fileRow, tc = fileCol;
            switch (rot) {
            case 0: tr = fileRow; tc = fileCol; break;
            case 90: tr = fileCol; tc = N - 1 - fileRow; break;
            case 180: tr = N - 1 - fileRow; tc = N - 1 - fileCol; break;
            case 270: tr = N - 1 - fileCol; tc = fileRow; break;
            }
            map[face][tr][tc] = val; fileCol++;
        }
        fileRow++;
    }
    file.close();
}

void initMap() {
    // 맵 파일이 없으면 그냥 벽이 없거나 기본값으로 실행됩니다.
    // 기존 프로젝트에 있던 map_*.csv 파일이 같은 폴더에 있어야 합니다.
    loadMapFromCSV(FACE_FRONT, "map_front.csv", 180);
    loadMapFromCSV(FACE_BACK, "map_back.csv", 0);
    loadMapFromCSV(FACE_RIGHT, "map_right.csv", 90);
    loadMapFromCSV(FACE_LEFT, "map_left.csv", -90);
    loadMapFromCSV(FACE_TOP, "map_top.csv", 0);
    loadMapFromCSV(FACE_BOTTOM, "map_bottom.csv", 0);
}

// ----------------------------------------------------------
// [렌더링 1] Smart Wall (정상: 부채꼴 모양)
// - 안쪽 좌표(b)와 바깥쪽 좌표(t)를 따로 계산해서 잇는다.
// ----------------------------------------------------------
void drawSmartWall(int f, int r, int c) {
    float u1 = (float)c / N; float u2 = (float)(c + 1) / N;
    float v1 = (float)r / N; float v2 = (float)(r + 1) / N;

    // 바깥쪽 점 (Radius)
    Point3D t1 = getSpherePoint(f, u1, v1, planetRadius);
    Point3D t2 = getSpherePoint(f, u2, v1, planetRadius);
    Point3D t3 = getSpherePoint(f, u2, v2, planetRadius);
    Point3D t4 = getSpherePoint(f, u1, v2, planetRadius);

    // 안쪽 점 (Radius - Depth) -> 반지름이 작으니 자동으로 좁아짐
    Point3D b1 = getSpherePoint(f, u1, v1, planetRadius - wallDepth);
    Point3D b2 = getSpherePoint(f, u2, v1, planetRadius - wallDepth);
    Point3D b3 = getSpherePoint(f, u2, v2, planetRadius - wallDepth);
    Point3D b4 = getSpherePoint(f, u1, v2, planetRadius - wallDepth);

    glBegin(GL_QUADS);
    // 윗면 (Cyan)
    glColor3f(0.0f, 1.0f, 1.0f);
    Point3D n = { t1.x, t1.y, t1.z }; glNormal3f(n.x, n.y, n.z);
    glVertex3f(t1.x, t1.y, t1.z); glVertex3f(t2.x, t2.y, t2.z);
    glVertex3f(t3.x, t3.y, t3.z); glVertex3f(t4.x, t4.y, t4.z);

    // 옆면 (Dark Cyan)
    glColor3f(0.0f, 0.5f, 0.5f);
    glVertex3f(b1.x, b1.y, b1.z); glVertex3f(b2.x, b2.y, b2.z); glVertex3f(t2.x, t2.y, t2.z); glVertex3f(t1.x, t1.y, t1.z);
    glVertex3f(b2.x, b2.y, b2.z); glVertex3f(b3.x, b3.y, b3.z); glVertex3f(t3.x, t3.y, t3.z); glVertex3f(t2.x, t2.y, t2.z);
    glVertex3f(b3.x, b3.y, b3.z); glVertex3f(b4.x, b4.y, b4.z); glVertex3f(t4.x, t4.y, t4.z); glVertex3f(t3.x, t3.y, t3.z);
    glVertex3f(b4.x, b4.y, b4.z); glVertex3f(b1.x, b1.y, b1.z); glVertex3f(t1.x, t1.y, t1.z); glVertex3f(t4.x, t4.y, t4.z);
    glEnd();

    // 테두리 (검은색)
    glColor3f(0.0f, 0.0f, 0.0f); glLineWidth(1.0f);
    glBegin(GL_LINE_LOOP); glVertex3f(t1.x, t1.y, t1.z); glVertex3f(t2.x, t2.y, t2.z); glVertex3f(t3.x, t3.y, t3.z); glVertex3f(t4.x, t4.y, t4.z); glEnd();
}

// ----------------------------------------------------------
// [렌더링 2] Naive Cube (문제: 직육면체)
// - 단순히 큐브를 회전시켜 박아넣는다. 안쪽이 안 좁아져서 겹침.
// ----------------------------------------------------------
void drawNaiveCube(int f, int r, int c) {
    float u = (c + 0.5f) / N;
    float v = (r + 0.5f) / N;

    // 벽의 중심 위치 (표면에서 깊이의 절반만큼 들어간 곳)
    Point3D center = getSpherePoint(f, u, v, planetRadius - wallDepth / 2.0f);

    glPushMatrix();
    glTranslatef(center.x, center.y, center.z);

    // 구 표면에 맞게 회전
    Point3D up = { 0, 0, 1 };
    Point3D normal = normalize(center);
    Point3D axis = {
        up.y * normal.z - up.z * normal.y,
        up.z * normal.x - up.x * normal.z,
        up.x * normal.y - up.y * normal.x
    };
    float angle = acos(up.x * normal.x + up.y * normal.y + up.z * normal.z) * 180.0f / M_PI;
    glRotatef(angle, axis.x, axis.y, axis.z);

    // [빨간색] 직육면체 그리기
    glColor3f(1.0f, 0.2f, 0.2f);

    // 크기는 '표면(가장 넓은 곳)' 기준으로 잡음
    // 이러면 깊은 곳(좁은 곳)에서는 서로 겹치게 됨 -> 이것이 문제점!
    float size = (planetRadius * 2.0f * M_PI) / (N * 4);
    glScalef(size, size, wallDepth);

    glutSolidCube(1.0f); // 위아래 굵기 똑같은 큐브

    // 검은 테두리 (겹침 현상 확인용)
    glColor3f(0.0f, 0.0f, 0.0f); glLineWidth(1.0f);
    glutWireCube(1.01f);

    glPopMatrix();
}

// ----------------------------------------------------------
// [장면 그리기]
// ----------------------------------------------------------
void drawScene() {
    glDisable(GL_CULL_FACE);

    // 조명 끄기 (색상 잘 보이게)
    glDisable(GL_LIGHTING);

    // 1. 행성 바닥 (회색)
    glColor3f(0.3f, 0.3f, 0.3f);
    glutSolidSphere(planetRadius - wallDepth - 0.1f, 50, 50); // 안쪽 핵

    // 2. 벽 그리기
    for (int f = 0; f < 6; f++) {
        for (int r = 0; r < N; r++) {
            for (int c = 0; c < N; c++) {
                if (map[f][r][c] == 1) {
                    if (useSmartWall) drawSmartWall(f, r, c);
                    else drawNaiveCube(f, r, c);
                }
            }
        }
    }
}

void display() {
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    float eyeY = -planetRadius + playerHeight;
    float lx = sin(cameraYaw) * cos(cameraPitch);
    float ly = sin(cameraPitch);
    float lz = -cos(cameraYaw) * cos(cameraPitch);

    glLoadIdentity();
    gluLookAt(0, eyeY, 0, lx, eyeY + ly, lz, 0, 1, 0);

    glPushMatrix();
    glMultMatrixf(planetRotationMatrix); // 플레이어 대신 행성 회전
    drawScene();
    glPopMatrix();

    glutSwapBuffers();
}

// ----------------------------------------------------------
// [플레이어 이동 및 입력] (원본 로직 유지)
// ----------------------------------------------------------
bool checkCollision() {
    Point3D playerPos = { 0.0f, -planetRadius + 1.5f, 0.0f };
    float collisionDist = 3.5f;
    for (int f = 0; f < 6; f++) {
        for (int r = 0; r < N; r++) {
            for (int c = 0; c < N; c++) {
                if (map[f][r][c] == 1) {
                    Point3D pC = multiplyMatrixVector(getSpherePoint(f, (c + 0.5f) / N, (r + 0.5f) / N, planetRadius - 1.5f), planetRotationMatrix);
                    if (sqrt(pow(pC.x - playerPos.x, 2) + pow(pC.y - playerPos.y, 2) + pow(pC.z - playerPos.z, 2)) < collisionDist) return true;
                }
            }
        }
    }
    return false;
}

void movePlayer(float moveSpeed, float strafeSpeed) {
    float axisX = 0.0f, axisZ = 0.0f, angle = 0.0f;
    float sinYaw = sin(cameraYaw), cosYaw = cos(cameraYaw);

    if (moveSpeed != 0.0f) { axisX = cosYaw; axisZ = sinYaw; angle = moveSpeed; }
    else if (strafeSpeed != 0.0f) { axisX = sinYaw; axisZ = -cosYaw; angle = strafeSpeed; }

    if (angle == 0.0f) return;

    GLfloat bk[16]; for (int i = 0; i < 16; i++) bk[i] = planetRotationMatrix[i];
    glPushMatrix(); glLoadIdentity(); glRotatef(angle, axisX, 0.0f, axisZ);
    glMultMatrixf(planetRotationMatrix); glGetFloatv(GL_MODELVIEW_MATRIX, planetRotationMatrix); glPopMatrix();

    if (checkCollision()) for (int i = 0; i < 16; i++) planetRotationMatrix[i] = bk[i];
}

void keyboard(unsigned char key, int x, int y) {
    float speed = 1.5f;
    if (key == 'w' || key == 'W') movePlayer(-speed, 0);
    if (key == 's' || key == 'S') movePlayer(speed, 0);
    if (key == 'a' || key == 'A') movePlayer(0, -speed);
    if (key == 'd' || key == 'D') movePlayer(0, speed);

    // [B키 토글] Smart Wall <-> Naive Cube
    if (key == 'b' || key == 'B') useSmartWall = !useSmartWall;

    if (key == 27) {
        mouseCaptured = !mouseCaptured;
        if (mouseCaptured) glutSetCursor(GLUT_CURSOR_NONE);
        else glutSetCursor(GLUT_CURSOR_INHERIT);
    }
    glutPostRedisplay();
}

void mouseMotion(int x, int y) {
    if (!mouseCaptured) return;
    int centerX = winW / 2; int centerY = winH / 2;
    int dx = x - centerX; int dy = y - centerY;
    if (dx == 0 && dy == 0) return;

    cameraYaw += dx * mouseSensitivity;
    cameraPitch -= dy * mouseSensitivity;
    if (cameraPitch > 1.5f) cameraPitch = 1.5f;
    if (cameraPitch < -1.5f) cameraPitch = -1.5f;

    glutWarpPointer(centerX, centerY);
    glutPostRedisplay();
}

void reshape(int w, int h) {
    winW = w; winH = h;
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION); glLoadIdentity();
    gluPerspective(60.0f, (float)w / h, 0.1f, 1000.0f);
    glMatrixMode(GL_MODELVIEW);
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(winW, winH);
    glutCreateWindow("Game View: Press 'B' to toggle Cube/SmartWall");

    glEnable(GL_DEPTH_TEST);
    glutSetCursor(GLUT_CURSOR_NONE);
    glutWarpPointer(winW / 2, winH / 2);

    initMap(); // CSV 맵 로드

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutPassiveMotionFunc(mouseMotion);

    glutMainLoop();
    return 0;
}