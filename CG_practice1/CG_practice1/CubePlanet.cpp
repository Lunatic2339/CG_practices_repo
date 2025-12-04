#define _CRT_SECURE_NO_WARNINGS
#include <GL/glut.h>
#include <vector>
#include <cmath>
#include <cstdio>
#include <cstdlib> 

#define M_PI 3.14159265358979323846

// ----------------------------------------------------------
// 데이터 구조 & 전역 변수
// ----------------------------------------------------------
struct Point3D { float x, y, z; };
struct Face { int v1, v2, v3; };

std::vector<Point3D> vertices;
std::vector<Face> faces;

float planetRadius = 40.0f;
float playerHeight = 3.0f;

// [회전 행렬]
GLfloat planetRotationMatrix[16] = {
    1, 0, 0, 0, 0, 1, 0, 0,
    0, 0, 1, 0, 0, 0, 0, 1
};

float cameraYaw = 0.0f;
float cameraPitch = 0.0f;

int winW = 1200;
int winH = 800;

// 맵 데이터 (6면)
const int N = 10;
enum { FACE_FRONT = 0, FACE_BACK, FACE_RIGHT, FACE_LEFT, FACE_TOP, FACE_BOTTOM };
int map[6][N][N] = { 0 };

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
    // 1. 테두리 벽
    for (int f = 0; f < 6; f++) {
        for (int i = 0; i < N; i++) {
            map[f][0][i] = 1; map[f][N - 1][i] = 1;
            map[f][i][0] = 1; map[f][i][N - 1] = 1;
        }
    }
    // 2. 장애물 및 모델 배치
    map[FACE_FRONT][N / 2][N / 2] = 9; // 앞: 도자기
    map[FACE_TOP][2][2] = 1;       // 위: 벽
    map[FACE_RIGHT][3][3] = 1;     // 우: 벽
    map[FACE_LEFT][5][5] = 1;      // 좌: 벽
    map[FACE_BACK][7][7] = 1;      // 뒤: 벽
    map[FACE_BOTTOM][4][4] = 9;    // 밑: 도자기

    // 추가 장애물 (테스트용)
    map[FACE_FRONT][2][5] = 1;
    map[FACE_FRONT][3][5] = 1;
}

// ----------------------------------------------------------
// 좌표 변환 함수들 (2단계에서 작성한 것)
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
    case FACE_FRONT:  p = { x, y, z }; break;
    case FACE_BACK:   p = { -x, y, -z }; break;
    case FACE_RIGHT:  p = { z, y, -x }; break;
    case FACE_LEFT:   p = { -z, y, x }; break;
    case FACE_TOP:    p = { x, z, -y }; break;
    case FACE_BOTTOM: p = { x, -z, y }; break;
    }
    p = normalize(p);
    p.x *= r; p.y *= r; p.z *= r;
    return p;
}

void drawSectorWall(int f, int r, int c) {
    float u1 = (float)c / N; float u2 = (float)(c + 1) / N;
    float v1 = (float)r / N; float v2 = (float)(r + 1) / N;
    float h = 3.0f;

    Point3D p0 = getSpherePoint(f, u1, v1, planetRadius);
    Point3D p1 = getSpherePoint(f, u2, v1, planetRadius);
    Point3D p2 = getSpherePoint(f, u2, v2, planetRadius);
    Point3D p3 = getSpherePoint(f, u1, v2, planetRadius);
    Point3D p4 = getSpherePoint(f, u1, v1, planetRadius - h);
    Point3D p5 = getSpherePoint(f, u2, v1, planetRadius - h);
    Point3D p6 = getSpherePoint(f, u2, v2, planetRadius - h);
    Point3D p7 = getSpherePoint(f, u1, v2, planetRadius - h);

    glBegin(GL_QUADS);
    Point3D n;
    // 윗면
    n = calculateNormal(p4, p5, p6); glNormal3f(n.x, n.y, n.z);
    glVertex3f(p4.x, p4.y, p4.z); glVertex3f(p5.x, p5.y, p5.z);
    glVertex3f(p6.x, p6.y, p6.z); glVertex3f(p7.x, p7.y, p7.z);
    // 앞면
    n = calculateNormal(p0, p1, p5); glNormal3f(n.x, n.y, n.z);
    glVertex3f(p0.x, p0.y, p0.z); glVertex3f(p1.x, p1.y, p1.z);
    glVertex3f(p5.x, p5.y, p5.z); glVertex3f(p4.x, p4.y, p4.z);
    // 뒷면
    n = calculateNormal(p1, p2, p6); glNormal3f(n.x, n.y, n.z);
    glVertex3f(p1.x, p1.y, p1.z); glVertex3f(p2.x, p2.y, p2.z);
    glVertex3f(p6.x, p6.y, p6.z); glVertex3f(p5.x, p5.y, p5.z);
    // 왼쪽
    n = calculateNormal(p3, p0, p4); glNormal3f(n.x, n.y, n.z);
    glVertex3f(p3.x, p3.y, p3.z); glVertex3f(p0.x, p0.y, p0.z);
    glVertex3f(p4.x, p4.y, p4.z); glVertex3f(p7.x, p7.y, p7.z);
    // 오른쪽
    n = calculateNormal(p2, p3, p7); glNormal3f(n.x, n.y, n.z);
    glVertex3f(p2.x, p2.y, p2.z); glVertex3f(p3.x, p3.y, p3.z);
    glVertex3f(p7.x, p7.y, p7.z); glVertex3f(p6.x, p6.y, p6.z);
    glEnd();
}

// ----------------------------------------------------------
// [3단계 핵심] 충돌 처리 함수들
// ----------------------------------------------------------

// 1. 행렬 x 벡터 곱셈 (회전된 위치 구하기용)
Point3D multiplyMatrixVector(Point3D p, GLfloat* m) {
    Point3D res;
    res.x = m[0] * p.x + m[4] * p.y + m[8] * p.z + m[12];
    res.y = m[1] * p.x + m[5] * p.y + m[9] * p.z + m[13];
    res.z = m[2] * p.x + m[6] * p.y + m[10] * p.z + m[14];
    return res;
}

// 2. 충돌 체크 (현재 행렬 상태에서 플레이어가 벽과 겹치는지?)
bool checkCollision() {
    // 플레이어의 위치 (월드 좌표계 상 고정 위치)
    // 구의 맨 아래쪽 안쪽 바닥
    Point3D playerPos = { 0.0f, -planetRadius + 1.5f, 0.0f };

    // 충돌 거리 기준 (플레이어 몸집 + 벽 두께 고려)
    float collisionDist = 2.5f;

    // 모든 면의 모든 벽을 순회하며 검사 (전수 조사)
    for (int f = 0; f < 6; f++) {
        for (int r = 0; r < N; r++) {
            for (int c = 0; c < N; c++) {
                if (map[f][r][c] == 1) { // 벽이 있다면

                    // 1) 그 벽의 원래 위치(Local) 구하기 (중심점)
                    Point3D wallLocal = getSpherePoint(f, (c + 0.5f) / N, (r + 0.5f) / N, planetRadius - 1.5f);

                    // 2) 현재 월드 회전 행렬을 적용해서 실제 위치(World) 구하기
                    Point3D wallWorld = multiplyMatrixVector(wallLocal, planetRotationMatrix);

                    // 3) 플레이어와의 거리 계산
                    float dx = wallWorld.x - playerPos.x;
                    float dy = wallWorld.y - playerPos.y;
                    float dz = wallWorld.z - playerPos.z;
                    float dist = sqrt(dx * dx + dy * dy + dz * dz);

                    // 4) 너무 가까우면 충돌!
                    if (dist < collisionDist) {
                        return true;
                    }
                }
            }
        }
    }
    return false;
}

// ----------------------------------------------------------
// 장면 그리기
// ----------------------------------------------------------
void drawScene(bool isWireMode) {
    glLightModelf(GL_LIGHT_MODEL_TWO_SIDE, 1.0);
    glDisable(GL_CULL_FACE);

    // 1. 구체 (바닥)
    if (isWireMode) {
        glDisable(GL_LIGHTING);
        glColor3f(0.3f, 0.3f, 0.3f);
        glutWireSphere(planetRadius, 20, 20);
        glEnable(GL_LIGHTING);
    }
    else {
        GLfloat wallColor[] = { 0.1f, 0.1f, 0.3f, 1.0f };
        glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, wallColor);
        glutSolidSphere(planetRadius, 50, 50);
    }

    // 2. 맵 그리기
    for (int f = 0; f < 6; f++) {
        for (int r = 0; r < N; r++) {
            for (int c = 0; c < N; c++) {
                if (map[f][r][c] == 0) continue;

                if (map[f][r][c] == 1) {
                    // [벽]
                    GLfloat boxCol[] = { 0.0f, 1.0f, 1.0f, 1.0f };
                    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, boxCol);
                    drawSectorWall(f, r, c);
                }
                else if (map[f][r][c] == 9) {
                    // [모델]
                    Point3D center = getSpherePoint(f, (c + 0.5f) / N, (r + 0.5f) / N, planetRadius);
                    glPushMatrix();
                    glTranslatef(center.x, center.y, center.z);
                    // 모델 방향 간단 보정 (중심에서 바깥을 향하게)
                    // 정밀한 회전은 복잡하므로 위치만 잡음
                    glTranslatef(0, 0, 0);
                    glScalef(0.005f, 0.005f, 0.005f);

                    GLfloat goldDiff[] = { 1.0f, 0.8f, 0.0f, 1.0f };
                    GLfloat goldSpec[] = { 1.0f, 1.0f, 1.0f, 1.0f };
                    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, goldDiff);
                    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, goldSpec);
                    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 100.0f);

                    glBegin(GL_TRIANGLES);
                    for (int k = 0; k < faces.size(); k++) {
                        Point3D p1 = vertices[faces[k].v1];
                        Point3D p2 = vertices[faces[k].v2];
                        Point3D p3 = vertices[faces[k].v3];
                        Point3D n = calculateNormal(p1, p2, p3);
                        glNormal3f(n.x, n.y, n.z);
                        glVertex3f(p1.x, p1.y, p1.z);
                        glVertex3f(p2.x, p2.y, p2.z);
                        glVertex3f(p3.x, p3.y, p3.z);
                    }
                    glEnd();
                    glPopMatrix();
                }
            }
        }
    }
}

// ----------------------------------------------------------
// 화면 출력
// ----------------------------------------------------------
void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_LIGHTING); glEnable(GL_LIGHT0);

    // [1] 왼쪽 화면
    glViewport(0, 0, winW / 2, winH);
    glMatrixMode(GL_PROJECTION); glLoadIdentity();
    gluPerspective(60.0f, (float)(winW / 2) / winH, 0.1f, 1000.0f);
    glMatrixMode(GL_MODELVIEW); glLoadIdentity();

    float eyeY = -planetRadius + playerHeight;
    float lookX = sin(cameraYaw) * cos(cameraPitch);
    float lookY = sin(cameraPitch);
    float lookZ = -cos(cameraYaw) * cos(cameraPitch);
    gluLookAt(0, eyeY, 0, lookX, eyeY + lookY, lookZ, 0, 1, 0);

    GLfloat lightPos[] = { 0.0f, 0.0f, 0.0f, 1.0f };
    glLightfv(GL_LIGHT0, GL_POSITION, lightPos);

    glPushMatrix();
    glMultMatrixf(planetRotationMatrix);
    drawScene(false);
    glPopMatrix();

    // [2] 오른쪽 위
    glViewport(winW / 2, winH / 2, winW / 2, winH / 2);
    glMatrixMode(GL_PROJECTION); glLoadIdentity();
    gluPerspective(60.0f, (float)(winW / 2) / (winH / 2), 0.1f, 1000.0f);
    glMatrixMode(GL_MODELVIEW); glLoadIdentity();
    gluLookAt(0, 0, 0, 0, -1, 0, 0, 0, -1);
    glLightfv(GL_LIGHT0, GL_POSITION, lightPos);

    glPushMatrix();
    glRotatef(cameraYaw * 180.0f / M_PI, 0.0f, 1.0f, 0.0f);
    glMultMatrixf(planetRotationMatrix);
    drawScene(false);
    glPopMatrix();

    glDisable(GL_LIGHTING);
    glPushMatrix();
    glTranslatef(0.0f, -planetRadius + 1.0f, 0.0f);
    glColor3f(1.0f, 0.0f, 0.0f); glutSolidCube(2.0f);
    glPopMatrix();
    glEnable(GL_LIGHTING);

    // [3] 오른쪽 아래
    glViewport(winW / 2, 0, winW / 2, winH / 2);
    glMatrixMode(GL_PROJECTION); glLoadIdentity();
    gluPerspective(60.0f, (float)(winW / 2) / (winH / 2), 0.1f, 1000.0f);
    glMatrixMode(GL_MODELVIEW); glLoadIdentity();
    gluLookAt(0, -100, 100, 0, 0, 0, 0, 1, 0);
    glLightfv(GL_LIGHT0, GL_POSITION, lightPos);

    glPushMatrix();
    glMultMatrixf(planetRotationMatrix);
    drawScene(true);
    glPopMatrix();

    glDisable(GL_LIGHTING);
    glPushMatrix();
    glTranslatef(0.0f, -planetRadius, 0.0f);
    glColor3f(1.0f, 0.0f, 1.0f); glutSolidCube(3.0f);
    glPopMatrix();
    glEnable(GL_LIGHTING);

    glutSwapBuffers();
}

void reshape(int w, int h) { winW = w; winH = h; }

void keyboard(unsigned char key, int x, int y) {
    float rotSpeed = 0.05f;
    if (key == 'a' || key == 'A') cameraYaw -= rotSpeed;
    if (key == 'd' || key == 'D') cameraYaw += rotSpeed;
    if (key == 'w' || key == 'W') cameraPitch += rotSpeed;
    if (key == 's' || key == 'S') cameraPitch -= rotSpeed;

    if (cameraPitch > 1.5f) cameraPitch = 1.5f;
    if (cameraPitch < -1.5f) cameraPitch = -1.5f;

    if (key == 27) exit(0);
    glutPostRedisplay();
}

void specialKeys(int key, int x, int y) {
    float moveSpeed = 1.5f;
    float axisX = 0.0f, axisZ = 0.0f, angle = 0.0f;
    float sinYaw = sin(cameraYaw), cosYaw = cos(cameraYaw);

    if (key == GLUT_KEY_UP) { axisX = cosYaw; axisZ = sinYaw; angle = -moveSpeed; }
    else if (key == GLUT_KEY_DOWN) { axisX = cosYaw; axisZ = sinYaw; angle = moveSpeed; }
    else if (key == GLUT_KEY_RIGHT) { axisX = sinYaw; axisZ = -cosYaw; angle = moveSpeed; }
    else if (key == GLUT_KEY_LEFT) { axisX = sinYaw; axisZ = -cosYaw; angle = -moveSpeed; }

    // [충돌 감지 핵심 로직]
    // 1. 현재 행렬 백업 (되돌리기 위해)
    GLfloat backupMatrix[16];
    for (int i = 0; i < 16; i++) backupMatrix[i] = planetRotationMatrix[i];

    // 2. 일단 이동시켜 봄 (가상 이동)
    glPushMatrix();
    glLoadIdentity();
    glRotatef(angle, axisX, 0.0f, axisZ);
    glMultMatrixf(planetRotationMatrix);
    glGetFloatv(GL_MODELVIEW_MATRIX, planetRotationMatrix);
    glPopMatrix();

    // 3. 이동한 상태에서 충돌 검사
    if (checkCollision()) {
        // 4-A. 충돌 발생! -> 백업해둔 행렬로 원상복구 (이동 취소)
        for (int i = 0; i < 16; i++) planetRotationMatrix[i] = backupMatrix[i];
        // (선택사항) 쾅 소리나 진동 효과 등을 여기에 넣을 수 있음
    }
    else {
        // 4-B. 충돌 없음 -> 이동 확정 (아무것도 안 해도 됨, 이미 planetRotationMatrix가 변했으므로)
    }

    glutPostRedisplay();
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(winW, winH);
    glutCreateWindow("Final Project Complete: Collision Detection");

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_NORMALIZE);

    initMap();
    loadModel("myModel.dat");

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(specialKeys);

    glutMainLoop();
    return 0;
}