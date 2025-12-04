#define _CRT_SECURE_NO_WARNINGS
#include <GL/glut.h>
#include <vector>
#include <cmath>
#include <cstdio>
#include <cstdlib> 
#include <string>

#define M_PI 3.14159265358979323846

// ----------------------------------------------------------
// 데이터 구조
// ----------------------------------------------------------
struct Point3D { float x, y, z; };
struct Face { int v1, v2, v3; };

// 아이템 구조체
struct Item {
    int face, r, c; // 위치 (면, 행, 열)
    bool active;    // 먹었는지 여부
    float rot;      // 빙글빙글 돌기 위한 각도
};

std::vector<Point3D> vertices;
std::vector<Face> faces;
std::vector<Item> items; // 아이템 목록

// ----------------------------------------------------------
// 전역 변수 (게임 상태)
// ----------------------------------------------------------
float planetRadius = 40.0f;
float playerHeight = 3.0f;
int score = 0; // 게임 점수
int totalItems = 3; // 총 아이템 개수

// 텍스처 ID 저장소
GLuint texWall, texFloor;

// 뷰 모드 (0: 1인칭 풀스크린, 1: 3분할 디버그/네비게이션)
int viewMode = 0;

// 회전 행렬 & 카메라
GLfloat planetRotationMatrix[16] = { 1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1 };
float cameraYaw = 0.0f, cameraPitch = 0.0f;

int winW = 1200, winH = 800;
const int N = 10;
enum { FACE_FRONT = 0, FACE_BACK, FACE_RIGHT, FACE_LEFT, FACE_TOP, FACE_BOTTOM };
int map[6][N][N] = { 0 };

// ----------------------------------------------------------
// [텍스처 생성] 이미지 파일 없이 코드로 텍스처 만들기
// ----------------------------------------------------------
void makeCheckImage(int type) {
    const int width = 64;
    const int height = 64;
    GLubyte image[height][width][3];

    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            int c;
            if (type == 0) { // [벽] 네온 벽돌 패턴
                // 격자 무늬 생성
                if ((i & 16) == 0 ^ (j & 16) == 0) c = 255; else c = 100;
                image[i][j][0] = 0;       // R
                image[i][j][1] = c;       // G (Cyan 계열)
                image[i][j][2] = c;       // B
            }
            else { // [바닥] 우주 금속 패턴 (노이즈 느낌)
                c = ((((i & 0x8) == 0) ^ ((j & 0x8)) == 0)) * 255;
                image[i][j][0] = c / 4;   // Dark Blue
                image[i][j][1] = c / 4;
                image[i][j][2] = c / 2 + 50;
            }
        }
    }

    // 텍스처 생성 및 바인딩
    GLuint* targetTex = (type == 0) ? &texWall : &texFloor;
    glGenTextures(1, targetTex);
    glBindTexture(GL_TEXTURE_2D, *targetTex);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
}

// ----------------------------------------------------------
// 유틸리티 함수들
// ----------------------------------------------------------
Point3D calculateNormal(Point3D p1, Point3D p2, Point3D p3) {
    Point3D u = { p2.x - p1.x, p2.y - p1.y, p2.z - p1.z };
    Point3D v = { p3.x - p1.x, p3.y - p1.y, p3.z - p1.z };
    Point3D n = { u.y * v.z - u.z * v.y, u.z * v.x - u.x * v.z, u.x * v.y - u.y * v.x };
    float len = sqrt(n.x * n.x + n.y * n.y + n.z * n.z);
    if (len > 0) { n.x /= len; n.y /= len; n.z /= len; }
    return n;
}

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

// 행렬 x 벡터 곱셈 (충돌 및 아이템 거리 계산용)
Point3D multiplyMatrixVector(Point3D p, GLfloat* m) {
    Point3D res;
    res.x = m[0] * p.x + m[4] * p.y + m[8] * p.z + m[12];
    res.y = m[1] * p.x + m[5] * p.y + m[9] * p.z + m[13];
    res.z = m[2] * p.x + m[6] * p.y + m[10] * p.z + m[14];
    return res;
}

void loadModel(const char* filename) {
    FILE* fp = fopen(filename, "r");
    if (!fp) return;
    char buf[128]; int numV, numF;
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
    // 맵 초기화 & 아이템 배치
    for (int f = 0; f < 6; f++) {
        for (int i = 0; i < N; i++) {
            map[f][0][i] = 1; map[f][N - 1][i] = 1;
            map[f][i][0] = 1; map[f][i][N - 1] = 1;
        }
    }
    // 장애물
    map[FACE_TOP][2][2] = 1; map[FACE_RIGHT][3][3] = 1;
    map[FACE_LEFT][5][5] = 1; map[FACE_BACK][7][7] = 1;

    // 아이템 배치 (맵 배열에는 0으로 두되, 별도 리스트 관리)
    items.push_back({ FACE_FRONT, N / 2, N / 2, true, 0.0f });
    items.push_back({ FACE_BOTTOM, 4, 4, true, 0.0f });
    items.push_back({ FACE_BACK, 2, 2, true, 0.0f });
}

// ----------------------------------------------------------
// 텍스처가 입혀진 곡면 벽 그리기
// ----------------------------------------------------------
void drawTexturedWall(int f, int r, int c) {
    float u1 = (float)c / N; float u2 = (float)(c + 1) / N;
    float v1 = (float)r / N; float v2 = (float)(r + 1) / N;
    float h = 3.0f;

    Point3D p4 = getSpherePoint(f, u1, v1, planetRadius - h);
    Point3D p5 = getSpherePoint(f, u2, v1, planetRadius - h);
    Point3D p6 = getSpherePoint(f, u2, v2, planetRadius - h);
    Point3D p7 = getSpherePoint(f, u1, v2, planetRadius - h);
    Point3D p0 = getSpherePoint(f, u1, v1, planetRadius);
    Point3D p1 = getSpherePoint(f, u2, v1, planetRadius);
    Point3D p2 = getSpherePoint(f, u2, v2, planetRadius);
    Point3D p3 = getSpherePoint(f, u1, v2, planetRadius);

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texWall);
    glColor3f(1, 1, 1); // 텍스처 본연의 색

    glBegin(GL_QUADS);
    // 윗면 (플레이어 정면)
    Point3D n = calculateNormal(p4, p5, p6); glNormal3f(n.x, n.y, n.z);
    glTexCoord2f(0, 0); glVertex3f(p4.x, p4.y, p4.z);
    glTexCoord2f(1, 0); glVertex3f(p5.x, p5.y, p5.z);
    glTexCoord2f(1, 1); glVertex3f(p6.x, p6.y, p6.z);
    glTexCoord2f(0, 1); glVertex3f(p7.x, p7.y, p7.z);

    // 옆면들 (앞, 뒤, 좌, 우) - 텍스처 입히기
    n = calculateNormal(p0, p1, p5); glNormal3f(n.x, n.y, n.z);
    glTexCoord2f(0, 0); glVertex3f(p0.x, p0.y, p0.z);
    glTexCoord2f(1, 0); glVertex3f(p1.x, p1.y, p1.z);
    glTexCoord2f(1, 1); glVertex3f(p5.x, p5.y, p5.z);
    glTexCoord2f(0, 1); glVertex3f(p4.x, p4.y, p4.z);

    n = calculateNormal(p1, p2, p6); glNormal3f(n.x, n.y, n.z);
    glTexCoord2f(0, 0); glVertex3f(p1.x, p1.y, p1.z);
    glTexCoord2f(1, 0); glVertex3f(p2.x, p2.y, p2.z);
    glTexCoord2f(1, 1); glVertex3f(p6.x, p6.y, p6.z);
    glTexCoord2f(0, 1); glVertex3f(p5.x, p5.y, p5.z);

    n = calculateNormal(p3, p0, p4); glNormal3f(n.x, n.y, n.z);
    glTexCoord2f(0, 0); glVertex3f(p3.x, p3.y, p3.z);
    glTexCoord2f(1, 0); glVertex3f(p0.x, p0.y, p0.z);
    glTexCoord2f(1, 1); glVertex3f(p4.x, p4.y, p4.z);
    glTexCoord2f(0, 1); glVertex3f(p7.x, p7.y, p7.z);

    n = calculateNormal(p2, p3, p7); glNormal3f(n.x, n.y, n.z);
    glTexCoord2f(0, 0); glVertex3f(p2.x, p2.y, p2.z);
    glTexCoord2f(1, 0); glVertex3f(p3.x, p3.y, p3.z);
    glTexCoord2f(1, 1); glVertex3f(p7.x, p7.y, p7.z);
    glTexCoord2f(0, 1); glVertex3f(p6.x, p6.y, p6.z);
    glEnd();
    glDisable(GL_TEXTURE_2D);
}

// ----------------------------------------------------------
// [스마트 월] 얇은 벽 조각 그리기 헬퍼 함수
// u_start, v_start ~ u_end, v_end 범위에 얇은 벽을 그립니다.
// ----------------------------------------------------------
void drawWallSegment(int f, float u_s, float u_e, float v_s, float v_e) {
    float h = 3.0f; // 벽 높이

    // 1. 8개 꼭짓점 계산
    Point3D p0 = getSpherePoint(f, u_s, v_s, planetRadius);
    Point3D p1 = getSpherePoint(f, u_e, v_s, planetRadius);
    Point3D p2 = getSpherePoint(f, u_e, v_e, planetRadius);
    Point3D p3 = getSpherePoint(f, u_s, v_e, planetRadius);
    Point3D p4 = getSpherePoint(f, u_s, v_s, planetRadius - h);
    Point3D p5 = getSpherePoint(f, u_e, v_s, planetRadius - h);
    Point3D p6 = getSpherePoint(f, u_e, v_e, planetRadius - h);
    Point3D p7 = getSpherePoint(f, u_s, v_e, planetRadius - h);

    // 2. 그리기 (텍스처 적용)
    // 텍스처 좌표는 단순하게 0~1로 매핑하지 않고, 월드 좌표 비율에 맞추면 더 좋지만
    // 지금은 간단히 0,0 ~ 1,1로 매핑합니다.
    glBegin(GL_QUADS);
    Point3D n;
    // 윗면
    n = calculateNormal(p4, p5, p6); glNormal3f(n.x, n.y, n.z);
    glTexCoord2f(0, 0); glVertex3f(p4.x, p4.y, p4.z);
    glTexCoord2f(1, 0); glVertex3f(p5.x, p5.y, p5.z);
    glTexCoord2f(1, 1); glVertex3f(p6.x, p6.y, p6.z);
    glTexCoord2f(0, 1); glVertex3f(p7.x, p7.y, p7.z);

    // 옆면들 (앞/뒤/좌/우)
    n = calculateNormal(p0, p1, p5); glNormal3f(n.x, n.y, n.z);
    glTexCoord2f(0, 0); glVertex3f(p0.x, p0.y, p0.z);
    glTexCoord2f(1, 0); glVertex3f(p1.x, p1.y, p1.z);
    glTexCoord2f(1, 1); glVertex3f(p5.x, p5.y, p5.z);
    glTexCoord2f(0, 1); glVertex3f(p4.x, p4.y, p4.z);

    n = calculateNormal(p1, p2, p6); glNormal3f(n.x, n.y, n.z);
    glTexCoord2f(0, 0); glVertex3f(p1.x, p1.y, p1.z);
    glTexCoord2f(1, 0); glVertex3f(p2.x, p2.y, p2.z);
    glTexCoord2f(1, 1); glVertex3f(p6.x, p6.y, p6.z);
    glTexCoord2f(0, 1); glVertex3f(p5.x, p5.y, p5.z);

    n = calculateNormal(p2, p3, p7); glNormal3f(n.x, n.y, n.z);
    glTexCoord2f(0, 0); glVertex3f(p2.x, p2.y, p2.z);
    glTexCoord2f(1, 0); glVertex3f(p3.x, p3.y, p3.z);
    glTexCoord2f(1, 1); glVertex3f(p7.x, p7.y, p7.z);
    glTexCoord2f(0, 1); glVertex3f(p6.x, p6.y, p6.z);

    n = calculateNormal(p3, p0, p4); glNormal3f(n.x, n.y, n.z);
    glTexCoord2f(0, 0); glVertex3f(p3.x, p3.y, p3.z);
    glTexCoord2f(1, 0); glVertex3f(p0.x, p0.y, p0.z);
    glTexCoord2f(1, 1); glVertex3f(p4.x, p4.y, p4.z);
    glTexCoord2f(0, 1); glVertex3f(p7.x, p7.y, p7.z);
    glEnd();
}

// ----------------------------------------------------------
// [스마트 월] 연결된 얇은 벽 그리기
// 주변(상하좌우)에 벽이 있으면 그쪽으로 '팔'을 뻗습니다.
// ----------------------------------------------------------
void drawSmartWall(int f, int r, int c) {
    // 1. 기본 좌표 범위 (0.0 ~ 1.0)
    float u1 = (float)c / N;
    float u2 = (float)(c + 1) / N;
    float v1 = (float)r / N;
    float v2 = (float)(r + 1) / N;

    // 2. 얇은 벽 두께 설정 (셀 크기의 20% 정도)
    float cellW = u2 - u1;
    float cellH = v2 - v1;
    float thickW = cellW * 0.2f; // 가로 두께
    float thickH = cellH * 0.2f; // 세로 두께

    // 중심부 범위
    float uc_s = u1 + (cellW - thickW) / 2.0f; // Center Start
    float uc_e = u1 + (cellW + thickW) / 2.0f; // Center End
    float vc_s = v1 + (cellH - thickH) / 2.0f;
    float vc_e = v1 + (cellH + thickH) / 2.0f;

    // 3. 연결 여부 확인 (이웃 체크)
    // 테두리(0, N-1)는 무조건 연결된 것으로 간주 (큐브 모서리 끊김 방지)
    bool connL = (c == 0) || (map[f][r][c - 1] == 1);
    bool connR = (c == N - 1) || (map[f][r][c + 1] == 1);
    bool connD = (r == 0) || (map[f][r - 1][c] == 1); // Down (v 작음)
    bool connU = (r == N - 1) || (map[f][r + 1][c] == 1); // Up (v 큼)

    // 텍스처 설정
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texWall);
    // 붓 씻기 (색상 초기화)
    GLfloat white[] = { 1,1,1,1 };
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, white);

    // 4. 조각 그리기 (5-Part Rendering)

    // (1) 중심 기둥 (Center Pillar) - 항상 그림
    drawWallSegment(f, uc_s, uc_e, vc_s, vc_e);

    // (2) 왼쪽 날개 (Left Wing)
    if (connL) drawWallSegment(f, u1, uc_s, vc_s, vc_e);

    // (3) 오른쪽 날개 (Right Wing)
    if (connR) drawWallSegment(f, uc_e, u2, vc_s, vc_e);

    // (4) 아래쪽 날개 (Down Wing)
    if (connD) drawWallSegment(f, uc_s, uc_e, v1, vc_s);

    // (5) 위쪽 날개 (Up Wing)
    if (connU) drawWallSegment(f, uc_s, uc_e, vc_e, v2);

    glDisable(GL_TEXTURE_2D);
}

// ----------------------------------------------------------
// 장면 그리기 (수정됨)
// ----------------------------------------------------------
void drawScene(bool isWireMode) {
    glLightModelf(GL_LIGHT_MODEL_TWO_SIDE, 1.0);
    glDisable(GL_CULL_FACE);

    // 재질 초기화 (필수!)
    GLfloat white[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    GLfloat black[] = { 0.0f, 0.0f, 0.0f, 1.0f };
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, white);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, black);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 0.0f);

    // 1. 바닥
    if (isWireMode) {
        glDisable(GL_LIGHTING); glColor3f(0.3f, 0.3f, 0.3f);
        glutWireSphere(planetRadius, 20, 20); glEnable(GL_LIGHTING);
    }
    else {
        glEnable(GL_TEXTURE_2D); glBindTexture(GL_TEXTURE_2D, texFloor);
        // 바닥 텍스처가 너무 밝으면 눈아프니 약간 어둡게
        glColor3f(0.7f, 0.7f, 0.8f);

        GLUquadric* quad = gluNewQuadric();
        gluQuadricTexture(quad, GL_TRUE);
        gluSphere(quad, planetRadius, 50, 50);
        gluDeleteQuadric(quad);
        glDisable(GL_TEXTURE_2D);
    }

    // 2. 벽 & 모델
    for (int f = 0; f < 6; f++) {
        for (int r = 0; r < N; r++) {
            for (int c = 0; c < N; c++) {
                if (map[f][r][c] == 0) continue;

                if (map[f][r][c] == 1) {
                    // [변경] 기존 drawTexturedWall 대신 drawSmartWall 호출
                    // 두께 20%의 얇고 연결된 벽을 그립니다.
                    drawSmartWall(f, r, c);
                }
                else if (map[f][r][c] == 9) {
                    // 모델 (기존 코드와 동일)
                    Point3D center = getSpherePoint(f, (c + 0.5f) / N, (r + 0.5f) / N, planetRadius);
                    glPushMatrix();
                    glTranslatef(center.x, center.y, center.z);
                    glTranslatef(0.0f, 0.0f, -2.0f);

                    // 아이템 회전
                    // 주의: 여기서 items 벡터를 뒤져서 회전값을 가져오는 게 정석이지만
                    // 단순화를 위해 그냥 시간(clock)기반으로 돌려도 됨.
                    // 하지만 items 구조체에 active 체크가 있으므로 아래처럼 하는게 맞음.
                    // *렌더링 루프에서는 맵 배열만 보고 그리면 이미 먹은 아이템도 그려질 수 있음*
                    // -> drawScene에서는 모델(9)을 그리지 말고, 
                    // display() 함수의 "3. 아이템 그리기" 루프에 맡기는 게 더 안전함.
                    // (하지만 기존 구조 유지를 위해 일단 여기서 그림)

                    glRotatef(-90, 1.0f, 0.0f, 0.0f);
                    glScalef(0.005f, 0.005f, 0.005f);

                    GLfloat gold[] = { 1.0f, 0.8f, 0.0f, 1.0f };
                    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, gold);
                    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, gold);
                    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 100.0f);

                    glBegin(GL_TRIANGLES);
                    for (auto& face : faces) {
                        Point3D p1 = vertices[face.v1];
                        Point3D p2 = vertices[face.v2];
                        Point3D p3 = vertices[face.v3];
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
// 텍스트 출력 함수 (점수판)
// ----------------------------------------------------------
void drawText(const char* str, float x, float y, float r, float g, float b) {
    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity();
    gluOrtho2D(0, winW, 0, winH);
    glMatrixMode(GL_MODELVIEW); glPushMatrix(); glLoadIdentity();

    glColor3f(r, g, b);
    glRasterPos2f(x, y);
    for (const char* c = str; *c != '\0'; c++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);
    }

    glPopMatrix(); glMatrixMode(GL_PROJECTION); glPopMatrix(); glMatrixMode(GL_MODELVIEW);
    glEnable(GL_LIGHTING); glEnable(GL_DEPTH_TEST);
}

void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_LIGHTING); glEnable(GL_LIGHT0);
    GLfloat lightPos[] = { 0.0f, 0.0f, 0.0f, 1.0f };

    // 시점 설정 (FPS View Common)
    float eyeY = -planetRadius + playerHeight;
    float lookX = sin(cameraYaw) * cos(cameraPitch);
    float lookY = sin(cameraPitch);
    float lookZ = -cos(cameraYaw) * cos(cameraPitch);

    if (viewMode == 0) {
        // [모드 0] 게임 모드 (1인칭 전체 화면)
        glViewport(0, 0, winW, winH);
        glMatrixMode(GL_PROJECTION); glLoadIdentity();
        gluPerspective(60.0f, (float)winW / winH, 0.1f, 1000.0f);

        glMatrixMode(GL_MODELVIEW); glLoadIdentity();
        gluLookAt(0, eyeY, 0, lookX, eyeY + lookY, lookZ, 0, 1, 0);
        glLightfv(GL_LIGHT0, GL_POSITION, lightPos);

        glPushMatrix();
        glMultMatrixf(planetRotationMatrix);
        drawScene(false);
        glPopMatrix();
    }
    else {
        // [모드 1] 디버그 모드 (3분할 화면 - 기존 코드)
        // 1. 왼쪽
        glViewport(0, 0, winW / 2, winH);
        glMatrixMode(GL_PROJECTION); glLoadIdentity();
        gluPerspective(60.0f, (float)(winW / 2) / winH, 0.1f, 1000.0f);
        glMatrixMode(GL_MODELVIEW); glLoadIdentity();
        gluLookAt(0, eyeY, 0, lookX, eyeY + lookY, lookZ, 0, 1, 0);
        glLightfv(GL_LIGHT0, GL_POSITION, lightPos);
        glPushMatrix(); glMultMatrixf(planetRotationMatrix); drawScene(false); glPopMatrix();

        // 2. 우상단 (Navi)
        glViewport(winW / 2, winH / 2, winW / 2, winH / 2);
        glMatrixMode(GL_PROJECTION); glLoadIdentity();
        gluPerspective(60.0f, (float)(winW / 2) / (winH / 2), 0.1f, 1000.0f);
        glMatrixMode(GL_MODELVIEW); glLoadIdentity();
        gluLookAt(0, 0, 0, 0, -1, 0, 0, 0, -1);
        glLightfv(GL_LIGHT0, GL_POSITION, lightPos);
        glPushMatrix();
        glRotatef(cameraYaw * 180.0f / M_PI, 0.0f, 1.0f, 0.0f);
        glMultMatrixf(planetRotationMatrix); drawScene(false);
        glPopMatrix();
        glDisable(GL_LIGHTING);
        glPushMatrix(); glTranslatef(0, -planetRadius + 1, 0); glColor3f(1, 0, 0); glutSolidCube(2); glPopMatrix();
        glEnable(GL_LIGHTING);

        // 3. 우하단 (Absolute)
        glViewport(winW / 2, 0, winW / 2, winH / 2);
        glMatrixMode(GL_PROJECTION); glLoadIdentity();
        gluPerspective(60.0f, (float)(winW / 2) / (winH / 2), 0.1f, 1000.0f);
        glMatrixMode(GL_MODELVIEW); glLoadIdentity();
        gluLookAt(0, -100, 100, 0, 0, 0, 0, 1, 0);
        glLightfv(GL_LIGHT0, GL_POSITION, lightPos);
        glPushMatrix(); glMultMatrixf(planetRotationMatrix); drawScene(true); glPopMatrix();
        glDisable(GL_LIGHTING);
        glPushMatrix(); glTranslatef(0, -planetRadius, 0); glColor3f(1, 0, 1); glutSolidCube(3); glPopMatrix();
        glEnable(GL_LIGHTING);
    }

    // UI (점수 및 안내)
    char info[128];
    sprintf(info, "Score: %d | Mode: %s ('V' to switch) | Move: Arrows+WASD | Interact: SPACE",
        score, (viewMode == 0 ? "Game View" : "Debug View"));
    drawText(info, 20, winH - 30, 1.0f, 1.0f, 1.0f);

    // [승리 조건 체크 및 메시지 출력]
    // 현재 점수 == 아이템 개수 * 100점 이면 클리어
    if (score == totalItems * 100) {
        // 화면 중앙에 큰 글씨 띄우기
        char msg[] = "MISSION COMPLETE!";
        char sub[] = "Press ESC to Exit";

        // 깜빡이는 효과 (선택사항)
        static float alpha = 0.0f;
        alpha += 0.05f;
        float blink = fabs(sin(alpha));

        drawText(msg, winW / 2 - 80, winH / 2, 1.0f, blink, 0.0f); // 노란색 깜빡임
        drawText(sub, winW / 2 - 60, winH / 2 - 30, 1.0f, 1.0f, 1.0f);
    }

    glutSwapBuffers();
}

void reshape(int w, int h) { winW = w; winH = h; }

// 충돌 체크
bool checkCollision() {
    Point3D playerPos = { 0.0f, -planetRadius + 1.5f, 0.0f };
    float collisionDist = 3.5f;

    for (int f = 0; f < 6; f++) {
        for (int r = 0; r < N; r++) {
            for (int c = 0; c < N; c++) {
                if (map[f][r][c] == 1) { // 벽
                    Point3D wallLocal = getSpherePoint(f, (c + 0.5f) / N, (r + 0.5f) / N, planetRadius - 1.5f);
                    Point3D wallWorld = multiplyMatrixVector(wallLocal, planetRotationMatrix);
                    float dx = wallWorld.x - playerPos.x;
                    float dy = wallWorld.y - playerPos.y;
                    float dz = wallWorld.z - playerPos.z;
                    if (sqrt(dx * dx + dy * dy + dz * dz) < collisionDist) return true;
                }
            }
        }
    }
    return false;
}

// 아이템 획득 체크
void checkInteraction() {
    Point3D playerPos = { 0.0f, -planetRadius + 1.5f, 0.0f };
    float interactDist = 4.0f; // 약간 관대하게

    for (auto& item : items) {
        if (!item.active) continue;

        Point3D itemLocal = getSpherePoint(item.face, (item.c + 0.5f) / N, (item.r + 0.5f) / N, planetRadius - 2.0f);
        Point3D itemWorld = multiplyMatrixVector(itemLocal, planetRotationMatrix);

        float dx = itemWorld.x - playerPos.x;
        float dy = itemWorld.y - playerPos.y;
        float dz = itemWorld.z - playerPos.z;

        if (sqrt(dx * dx + dy * dy + dz * dz) < interactDist) {
            item.active = false; // 획득!
            score += 100;
            printf("Item Collected! Score: %d\n", score);
        }
    }
}

void keyboard(unsigned char key, int x, int y) {
    float rotSpeed = 0.05f;
    if (key == 'a' || key == 'A') cameraYaw -= rotSpeed;
    if (key == 'd' || key == 'D') cameraYaw += rotSpeed;
    if (key == 'w' || key == 'W') cameraPitch += rotSpeed;
    if (key == 's' || key == 'S') cameraPitch -= rotSpeed;

    // [V키] 시점 변환 (Toggle)
    if (key == 'v' || key == 'V') viewMode = !viewMode;

    // [SPACE] 상호작용 (아이템 줍기)
    if (key == ' ') checkInteraction();

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

    GLfloat backupMatrix[16];
    for (int i = 0; i < 16; i++) backupMatrix[i] = planetRotationMatrix[i];

    glPushMatrix(); glLoadIdentity();
    glRotatef(angle, axisX, 0.0f, axisZ);
    glMultMatrixf(planetRotationMatrix);
    glGetFloatv(GL_MODELVIEW_MATRIX, planetRotationMatrix);
    glPopMatrix();

    if (checkCollision()) {
        for (int i = 0; i < 16; i++) planetRotationMatrix[i] = backupMatrix[i];
    }
    glutPostRedisplay();
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(winW, winH);
    glutCreateWindow("Final Game: Planet Explorer");

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_NORMALIZE);

    // 텍스처 생성
    makeCheckImage(0); // 벽
    makeCheckImage(1); // 바닥

    initMap();
    loadModel("myModel.dat");

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(specialKeys);

    glutMainLoop();
    return 0;
}