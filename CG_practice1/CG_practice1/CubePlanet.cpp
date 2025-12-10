// Visual Studio의 구형 C함수(scanf, fopen 등) 보안 경고 무시

#define _CRT_SECURE_NO_WARNINGS



#include "glut.h"  // OpenGL 및 GLUT 라이브러리

#include <vector>      // STL 벡터

#include <cmath>       // 수학 함수

#include <cstdio>      // 표준 입출력

#include <cstdlib>     // 표준 라이브러리

#include <string>      // 문자열

#include <fstream>     // 파일 입출력

#include <sstream>     // 문자열 스트림

#include <iostream>    // 입출력 스트림



// 원주율 정의

#define M_PI 3.14159265358979323846



// ----------------------------------------------------------

// [데이터 구조체]

// ----------------------------------------------------------

struct Point3D { float x, y, z; };

struct Face { int v1, v2, v3; };



// [추가됨] 모델 하나를 저장하는 구조체

struct Model {

    std::vector<Point3D> vertices;

    std::vector<Face> faces;

};



// [수정됨] 아이템 구조체 (모델 인덱스, 색상 추가)

struct Item {

    int face, r, c;

    bool active;

    float rot;

    int modelIdx; // 0~5번 모델 중 어떤 것인지

    float rColor, gColor, bColor; // 아이템 색상

};



// ----------------------------------------------------------

// [전역 변수]

// ----------------------------------------------------------

// [수정됨] 다중 모델 저장소

std::vector<Model> models;

std::vector<Item> items;



float planetRadius = 80.0f;

float playerHeight = 3.0f;

int score = 0;

int totalItems = 3;



GLuint texWall, texFloor;

int viewMode = 0;



GLfloat planetRotationMatrix[16] = { 1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1 };

float cameraYaw = 0.0f, cameraPitch = 0.0f;



// 마우스 설정

float mouseSensitivity = 0.005f;

bool mouseCaptured = true;



int winW = 1200, winH = 800;



// [설정 유지] 맵 해상도 15, 사용자 정의 순서 (FRONT=0)

const int N = 15;

enum { FACE_FRONT = 0, FACE_BACK = 1, FACE_RIGHT = 2, FACE_LEFT = 3, FACE_TOP = 4, FACE_BOTTOM = 5 };



int map[6][N][N] = { 0 };



// ----------------------------------------------------------

// [함수 선언]

// ----------------------------------------------------------

int getNeighborValue(int f, int r, int c);

Point3D getSpherePoint(int face, float u, float v, float r);

Point3D multiplyMatrixVector(Point3D p, GLfloat* m);

bool checkCollision();



// ----------------------------------------------------------

// [텍스처 생성]

// ----------------------------------------------------------

void makeCheckImage(int type) {

    const int width = 64; const int height = 64;

    GLubyte image[height][width][3];

    for (int i = 0; i < height; i++) {

        for (int j = 0; j < width; j++) {

            int c;

            if (type == 0) { // 벽 (Cyan)

                if ((i & 16) == 0 ^ (j & 16) == 0) c = 255; else c = 100;

                image[i][j][0] = 0; image[i][j][1] = c; image[i][j][2] = c;

            }

            else { // 바닥 (Dark Blue)

                c = ((((i & 0x8) == 0) ^ ((j & 0x8)) == 0)) * 255;

                image[i][j][0] = c / 4; image[i][j][1] = c / 4; image[i][j][2] = c / 2 + 50;

            }

        }

    }

    GLuint* targetTex = (type == 0) ? &texWall : &texFloor;

    glGenTextures(1, targetTex); glBindTexture(GL_TEXTURE_2D, *targetTex);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);

}



// ----------------------------------------------------------

// [수학 유틸리티]

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

    case FACE_BACK:  p = { x, y, z }; break;

    case FACE_FRONT:   p = { -x, y, -z }; break;

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

// [CSV 로딩] 0, 1 반전 기능 포함

// ----------------------------------------------------------

void loadMapFromCSV(int face, const char* filename, int angle) {

    std::ifstream file(filename);

    if (!file.is_open()) return;



    int rot = angle % 360;

    if (rot < 0) rot += 360;



    std::string line;

    int fileRow = 0;

    while (std::getline(file, line) && fileRow < N) {

        std::stringstream ss(line);

        std::string cell;

        int fileCol = 0;

        while (std::getline(ss, cell, ',') && fileCol < N) {

            int val = 0;

            try { val = std::stoi(cell); }

            catch (...) { val = 0; }



            if (val == 0) val = 1; else if (val == 1) val = 0; // 반전



            int tr = fileRow, tc = fileCol;

            switch (rot) {

            case 0:   tr = fileRow; tc = fileCol; break;

            case 90:  tr = fileCol; tc = N - 1 - fileRow; break;

            case 180: tr = N - 1 - fileRow; tc = N - 1 - fileCol; break;

            case 270: tr = N - 1 - fileCol; tc = fileRow; break;

            }

            map[face][tr][tc] = val;

            fileCol++;

        }

        fileRow++;

    }

    file.close();

    std::cout << "Map Loaded: " << filename << std::endl;

}



// ----------------------------------------------------------

// [모델 로딩 수정] 안전 장치 강화 (면 개수 체크 추가)

// 파일이 비어있거나, 읽었는데 면(Face)이 없으면 0번 모델을 복사함

// ----------------------------------------------------------

void loadModel(const char* filename) {

    FILE* fp = fopen(filename, "r");



    // 1. 파일 자체가 없을 때

    if (!fp) {

        if (!models.empty()) {

            models.push_back(models[0]); // 0번 모델 복사

            // (조용히 복사만 함)

        }

        else {

            Model m; models.push_back(m); // 최악의 경우 빈 모델

        }

        return;

    }



    Model m;

    char buf[128]; int numV = 0, numF = 0;



    // 헤더 읽기 시도

    if (fscanf(fp, "%s %s %d", buf, buf, &numV) != 3) numV = 0;



    for (int i = 0; i < numV; i++) {

        Point3D p;

        if (fscanf(fp, "%f %f %f", &p.x, &p.y, &p.z) == 3) m.vertices.push_back(p);

    }



    if (fscanf(fp, "%s %s %d", buf, buf, &numF) != 3) numF = 0;



    for (int i = 0; i < numF; i++) {

        Face f;

        if (fscanf(fp, "%d %d %d", &f.v1, &f.v2, &f.v3) == 3) m.faces.push_back(f);

    }

    fclose(fp);



    // 2. [핵심 수정] 파일은 있었지만, 내용물이 부실한 경우 (점이나 면이 없음)

    // 이 경우에도 망가진 모델을 쓰지 말고 0번 모델을 복사해서 씁니다.

    if ((m.vertices.empty() || m.faces.empty()) && !models.empty()) {

        models.push_back(models[0]);

        std::cout << "Warning: " << filename << " is invalid (No data). Copied default model." << std::endl;

    }

    else {

        models.push_back(m);

        std::cout << "Model Loaded: " << filename << " (V: " << numV << ", F: " << numF << ")" << std::endl;

    }

}



// ----------------------------------------------------------

// [초기화] 6개 모델 로드 및 아이템 배치

// ----------------------------------------------------------

void initMap() {

    loadMapFromCSV(FACE_FRONT, "map_front.csv", 180);

    loadMapFromCSV(FACE_BACK, "map_back.csv", 0);

    loadMapFromCSV(FACE_RIGHT, "map_right.csv", 90);

    loadMapFromCSV(FACE_LEFT, "map_left.csv", -90);

    loadMapFromCSV(FACE_TOP, "map_top.csv", 0);

    loadMapFromCSV(FACE_BOTTOM, "map_bottom.csv", 0);



    // [수정됨] 모델 로드 (myModel.dat + model_1~5.dat)

    models.clear();

    loadModel("myModel.dat"); // 0번 모델 (기본)

    for (int i = 1; i < 7; i++) {

        char filename[64];

        sprintf(filename, "model_%d.dat", i);

        loadModel(filename);

    }



    // 아이템 배치

    items.clear();

    float colors[6][3] = {

        {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f},

        {1.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 1.0f}, {0.0f, 1.0f, 1.0f}

    };



    for (int f = 0; f < 6; f++) {

        Item it;

        it.face = f;

        it.r = N / 2; it.c = N / 2;

        it.active = true;

        it.rot = 0.0f;

        // 로드된 모델 개수만큼 순환해서 할당

        if (!models.empty()) it.modelIdx = f % models.size();

        else it.modelIdx = 0;



        it.rColor = colors[f][0]; it.gColor = colors[f][1]; it.bColor = colors[f][2];



        items.push_back(it);

        map[f][N / 2][N / 2] = 0; // 아이템 자리는 벽 제거

    }

    totalItems = items.size();

    printf("Total Items: %d\n", totalItems);

}



// ----------------------------------------------------------

// [이웃 체크] (사용자 검증 로직 유지)

// ----------------------------------------------------------

int getNeighborValue(int f, int r, int c) {

    if (r >= 0 && r < N && c >= 0 && c < N) return map[f][r][c];



    int targetF = f, tr = r, tc = c;

    switch (f) {

    case FACE_BOTTOM:

        if (r < 0) { targetF = FACE_FRONT; tr = 0; tc = N - 1 - c; }

        else if (r >= N) { targetF = FACE_BACK; tr = 0; tc = c; }

        else if (c < 0) { targetF = FACE_LEFT; tr = 0; tc = r; }

        else if (c >= N) { targetF = FACE_RIGHT; tr = 0; tc = N - 1 - r; }

        break;

    case FACE_FRONT: // 0

        if (r < 0) { targetF = FACE_BOTTOM; tr = 0; tc = N - 1 - c; }

        else if (r >= N) { targetF = FACE_TOP; tr = N - 1; tc = N - 1 - c; }

        else if (c < 0) { targetF = FACE_RIGHT; tr = r; tc = N - 1; }

        else if (c >= N) { targetF = FACE_LEFT; tr = r; tc = 0; }

        break;

    case FACE_BACK: // 1

        if (r < 0) { targetF = FACE_BOTTOM; tr = N - 1; tc = c; }

        else if (r >= N) { targetF = FACE_TOP; tr = 0; tc = c; }

        else if (c < 0) { targetF = FACE_LEFT; tr = r; tc = N - 1; }

        else if (c >= N) { targetF = FACE_RIGHT; tr = r; tc = 0; }

        break;

    case FACE_LEFT:

        if (r < 0) { targetF = FACE_BOTTOM; tr = c; tc = 0; }

        else if (r >= N) { targetF = FACE_TOP; tr = N - 1 - c; tc = 0; }

        else if (c < 0) { targetF = FACE_FRONT; tr = r; tc = N - 1; }

        else if (c >= N) { targetF = FACE_BACK; tr = r; tc = 0; }

        break;

    case FACE_RIGHT:

        if (r < 0) { targetF = FACE_BOTTOM; tr = N - 1 - c; tc = N - 1; }

        else if (r >= N) { targetF = FACE_TOP; tr = c; tc = N - 1; }

        else if (c < 0) { targetF = FACE_BACK; tr = r; tc = N - 1; }

        else if (c >= N) { targetF = FACE_FRONT; tr = r; tc = 0; }

        break;

    case FACE_TOP:

        if (r < 0) { targetF = FACE_BACK; tr = N - 1; tc = c; }

        else if (r >= N) { targetF = FACE_FRONT; tr = N - 1; tc = N - 1 - c; }

        else if (c < 0) { targetF = FACE_LEFT; tr = N - 1; tc = N - 1 - r; }

        else if (c >= N) { targetF = FACE_RIGHT; tr = N - 1; tc = r; }

        break;

    }

    if (tr < 0) tr = 0; if (tr >= N) tr = N - 1;

    if (tc < 0) tc = 0; if (tc >= N) tc = N - 1;

    return map[targetF][tr][tc];

}



// ----------------------------------------------------------

// [렌더링] 벽 조각 그리기 Helper

// ----------------------------------------------------------

void drawWallSegment(int f, float u_s, float u_e, float v_s, float v_e) {

    float h = 6.0f; // 벽 높이

    Point3D p0 = getSpherePoint(f, u_s, v_s, planetRadius);

    Point3D p1 = getSpherePoint(f, u_e, v_s, planetRadius);

    Point3D p2 = getSpherePoint(f, u_e, v_e, planetRadius);

    Point3D p3 = getSpherePoint(f, u_s, v_e, planetRadius);

    Point3D p4 = getSpherePoint(f, u_s, v_s, planetRadius - h);

    Point3D p5 = getSpherePoint(f, u_e, v_s, planetRadius - h);

    Point3D p6 = getSpherePoint(f, u_e, v_e, planetRadius - h);

    Point3D p7 = getSpherePoint(f, u_s, v_e, planetRadius - h);



    glBegin(GL_QUADS);

    Point3D n;

    n = calculateNormal(p4, p5, p6); glNormal3f(n.x, n.y, n.z);

    glTexCoord2f(0, 0); glVertex3f(p4.x, p4.y, p4.z); glTexCoord2f(1, 0); glVertex3f(p5.x, p5.y, p5.z);

    glTexCoord2f(1, 1); glVertex3f(p6.x, p6.y, p6.z); glTexCoord2f(0, 1); glVertex3f(p7.x, p7.y, p7.z);



    n = calculateNormal(p0, p1, p5); glNormal3f(n.x, n.y, n.z);

    glTexCoord2f(0, 0); glVertex3f(p0.x, p0.y, p0.z); glTexCoord2f(1, 0); glVertex3f(p1.x, p1.y, p1.z);

    glTexCoord2f(1, 1); glVertex3f(p5.x, p5.y, p5.z); glTexCoord2f(0, 1); glVertex3f(p4.x, p4.y, p4.z);



    n = calculateNormal(p1, p2, p6); glNormal3f(n.x, n.y, n.z);

    glTexCoord2f(0, 0); glVertex3f(p1.x, p1.y, p1.z); glTexCoord2f(1, 0); glVertex3f(p2.x, p2.y, p2.z);

    glTexCoord2f(1, 1); glVertex3f(p6.x, p6.y, p6.z); glTexCoord2f(0, 1); glVertex3f(p5.x, p5.y, p5.z);



    n = calculateNormal(p2, p3, p7); glNormal3f(n.x, n.y, n.z);

    glTexCoord2f(0, 0); glVertex3f(p2.x, p2.y, p2.z); glTexCoord2f(1, 0); glVertex3f(p3.x, p3.y, p3.z);

    glTexCoord2f(1, 1); glVertex3f(p7.x, p7.y, p7.z); glTexCoord2f(0, 1); glVertex3f(p6.x, p6.y, p6.z);



    n = calculateNormal(p3, p0, p4); glNormal3f(n.x, n.y, n.z);

    glTexCoord2f(0, 0); glVertex3f(p3.x, p3.y, p3.z); glTexCoord2f(1, 0); glVertex3f(p0.x, p0.y, p0.z);

    glTexCoord2f(1, 1); glVertex3f(p4.x, p4.y, p4.z); glTexCoord2f(0, 1); glVertex3f(p7.x, p7.y, p7.z);

    glEnd();

}



// ----------------------------------------------------------

// [렌더링] 스마트 월 (연결된 벽 그리기)

// ----------------------------------------------------------

void drawSmartWall(int f, int r, int c) {

    float u1 = (float)c / N; float u2 = (float)(c + 1) / N;

    float v1 = (float)r / N; float v2 = (float)(r + 1) / N;

    float cw = u2 - u1; float ch = v2 - v1;

    float tw = cw * 0.2f; float th = ch * 0.2f;



    float uc_s = u1 + (cw - tw) / 2.0f; float uc_e = u1 + (cw + tw) / 2.0f;

    float vc_s = v1 + (ch - th) / 2.0f; float vc_e = v1 + (ch + th) / 2.0f;



    bool connL = (getNeighborValue(f, r, c - 1) == 1);

    bool connR = (getNeighborValue(f, r, c + 1) == 1);

    bool connU = (getNeighborValue(f, r - 1, c) == 1);

    bool connD = (getNeighborValue(f, r + 1, c) == 1);



    glEnable(GL_TEXTURE_2D); glBindTexture(GL_TEXTURE_2D, texWall);

    GLfloat white[] = { 1,1,1,1 }; glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, white);



    drawWallSegment(f, uc_s, uc_e, vc_s, vc_e);

    if (connL) drawWallSegment(f, u1, uc_s, vc_s, vc_e);

    if (connR) drawWallSegment(f, uc_e, u2, vc_s, vc_e);

    if (connU) drawWallSegment(f, uc_s, uc_e, v1, vc_s);

    if (connD) drawWallSegment(f, uc_s, uc_e, vc_e, v2);



    glDisable(GL_TEXTURE_2D);

}



// [장면 그리기 수정] 모델 루프가 items 리스트를 참조하도록 변경

void drawScene(bool isWireMode) {

    glLightModelf(GL_LIGHT_MODEL_TWO_SIDE, 1.0); glDisable(GL_CULL_FACE);

    GLfloat w[] = { 1,1,1,1 }, b[] = { 0,0,0,1 };

    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, w);

    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, b);

    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 0);



    if (isWireMode) {

        glDisable(GL_LIGHTING); glColor3f(0.3f, 0.3f, 0.3f);

        glutWireSphere(planetRadius, 20, 20); glEnable(GL_LIGHTING);

    }

    else {

        glEnable(GL_TEXTURE_2D); glBindTexture(GL_TEXTURE_2D, texFloor);

        glColor3f(0.7f, 0.7f, 0.8f);

        GLUquadric* q = gluNewQuadric(); gluQuadricTexture(q, GL_TRUE);

        gluSphere(q, planetRadius, 50, 50); gluDeleteQuadric(q);

        glDisable(GL_TEXTURE_2D);

    }



    // 벽 그리기

    for (int f = 0; f < 6; f++) {

        for (int r = 0; r < N; r++) {

            for (int c = 0; c < N; c++) {

                if (map[f][r][c] == 1) drawSmartWall(f, r, c);

            }

        }

    }



    // [수정됨] 아이템 그리기 (모델)

    for (auto& item : items) {

        if (!item.active) continue;



        // 1. [높이 조절] 반지름(80)에 오프셋을 줘서 발이 땅에 닿게 조절

        // 모델이 떠 있으면 값을 줄이고(-), 파묻히면 값을 늘리세요.

        float heightOffset = -2.0f;

        Point3D center = getSpherePoint(item.face, (item.c + 0.5f) / N, (item.r + 0.5f) / N, planetRadius + heightOffset);



        glPushMatrix();



        // 2. [위치 이동] 해당 지점으로 이동

        glTranslatef(center.x, center.y, center.z);



        // 3. [회전: 일으켜 세우기] 

        // 현재 위치(center)가 곧 "이 지점에서의 위쪽 방향(Up Vector)"입니다.

        // 모델의 Z축(0,0,1)을 구의 표면 방향(center)으로 일치시키는 계산입니다.



        Point3D modelUp = { 0, 0, 1 };      // 모델의 정수리 방향 (Z축 기준이라고 가정)

        Point3D surfNormal = normalize(center); // 지면이 향하는 방향



        // 두 벡터 사이의 회전축(Axis) 구하기 (외적)

        Point3D axis = {

            modelUp.y * surfNormal.z - modelUp.z * surfNormal.y,

            modelUp.z * surfNormal.x - modelUp.x * surfNormal.z,

            modelUp.x * surfNormal.y - modelUp.y * surfNormal.x

        };



        // 두 벡터 사이의 각도(Angle) 구하기 (내적)

        float dot = modelUp.x * surfNormal.x + modelUp.y * surfNormal.y + modelUp.z * surfNormal.z;

        float angleRad = acos(dot);

        float angleDeg = angleRad * 180.0f / M_PI;



        // 계산된 대로 회전! (이제 모델이 구의 중심 반대 방향을 쳐다봅니다)

        glRotatef(angleDeg, axis.x, axis.y, axis.z);



        // 4. [보정] 만약 이렇게 했는데도 모델이 엎드려 있거나 이상하면

        // 아래 주석을 풀어서 모델 자체의 축을 맞춰주세요. (모델마다 축이 달라서 필요할 수 있음)

        glRotatef(-90, 1, 0, 0);  // X축 기준 90도





        // 5. 크기 및 재질 설정 (기존 코드 유지)

        glScalef(0.005f, 0.005f, 0.005f);



        GLfloat color[] = { item.rColor, item.gColor, item.bColor, 1.0f };

        glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, color);

        glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, color);

        glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 100);



        if (!models.empty()) {

            Model& m = models[item.modelIdx];

            glBegin(GL_TRIANGLES);

            for (auto& fc : m.faces) {

                Point3D p1 = m.vertices[fc.v1], p2 = m.vertices[fc.v2], p3 = m.vertices[fc.v3];

                Point3D n = calculateNormal(p1, p2, p3); glNormal3f(n.x, n.y, n.z);

                glVertex3f(p1.x, p1.y, p1.z); glVertex3f(p2.x, p2.y, p2.z); glVertex3f(p3.x, p3.y, p3.z);

            }

            glEnd();

        }

        glPopMatrix();

    }

}



void drawText(const char* str, float x, float y, float r, float g, float b) {

    glDisable(GL_LIGHTING); glDisable(GL_DEPTH_TEST);

    glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity(); gluOrtho2D(0, winW, 0, winH);

    glMatrixMode(GL_MODELVIEW); glPushMatrix(); glLoadIdentity();

    glColor3f(r, g, b); glRasterPos2f(x, y);

    for (const char* c = str; *c != '\0'; c++) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);

    glPopMatrix(); glMatrixMode(GL_PROJECTION); glPopMatrix(); glMatrixMode(GL_MODELVIEW);

    glEnable(GL_LIGHTING); glEnable(GL_DEPTH_TEST);

}



void display() {

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glEnable(GL_LIGHTING); glEnable(GL_LIGHT0);

    GLfloat lPos[] = { 0, 0, 0, 1 };



    float eyeY = -planetRadius + playerHeight;

    float lx = sin(cameraYaw) * cos(cameraPitch);

    float ly = sin(cameraPitch);

    float lz = -cos(cameraYaw) * cos(cameraPitch);



    auto setView = [&](int x, int y, int w, int h) {

        glViewport(x, y, w, h);

        glMatrixMode(GL_PROJECTION); glLoadIdentity();

        gluPerspective(60.0f, (float)w / h, 0.1f, 1000.0f);

        glMatrixMode(GL_MODELVIEW); glLoadIdentity();

        };



    if (viewMode == 0) { // 1인칭 풀스크린

        setView(0, 0, winW, winH);

        gluLookAt(0, eyeY, 0, lx, eyeY + ly, lz, 0, 1, 0);

        glLightfv(GL_LIGHT0, GL_POSITION, lPos);

        glPushMatrix(); glMultMatrixf(planetRotationMatrix); drawScene(false); glPopMatrix();

    }

    else { // 3분할

        setView(0, 0, winW / 2, winH); // 왼쪽 (FPS)

        gluLookAt(0, eyeY, 0, lx, eyeY + ly, lz, 0, 1, 0);

        glLightfv(GL_LIGHT0, GL_POSITION, lPos);

        glPushMatrix(); glMultMatrixf(planetRotationMatrix); drawScene(false); glPopMatrix();



        setView(winW / 2, winH / 2, winW / 2, winH / 2); // 우상 (Navi)

        gluLookAt(0, 0, 0, 0, -1, 0, 0, 0, -1);

        glLightfv(GL_LIGHT0, GL_POSITION, lPos);

        glPushMatrix(); glRotatef(cameraYaw * 180 / M_PI, 0, 1, 0);

        glMultMatrixf(planetRotationMatrix); drawScene(false); glPopMatrix();

        glDisable(GL_LIGHTING); glPushMatrix(); glTranslatef(0, -planetRadius + 1, 0); glColor3f(1, 0, 0); glutSolidCube(2); glPopMatrix(); glEnable(GL_LIGHTING);



        setView(winW / 2, 0, winW / 2, winH / 2); // 우하 (Absolute)

        gluLookAt(0, -150, -150, 0, 0, 0, 0, 1, 0);

        glLightfv(GL_LIGHT0, GL_POSITION, lPos);

        glPushMatrix(); glMultMatrixf(planetRotationMatrix); drawScene(true); glPopMatrix();

        glDisable(GL_LIGHTING); glPushMatrix(); glTranslatef(0, -planetRadius, 0); glColor3f(1, 0, 1); glutSolidCube(3); glPopMatrix(); glEnable(GL_LIGHTING);

    }



    char info[128];

    sprintf(info, "Score: %d | Mode: %s ('V' to switch) | Move: WASD | Look: Mouse | Interact: SPACE | ESC: Toggle Mouse", score, (viewMode == 0 ? "Game" : "Debug"));

    drawText(info, 20, winH - 30, 1, 1, 1);



    if (score == totalItems * 100) {

        static float a = 0; a += 0.05f;

        drawText("MISSION COMPLETE!", winW / 2 - 80, winH / 2, 1, fabs(sin(a)), 0);

    }

    glutSwapBuffers();

}



void reshape(int w, int h) { winW = w; winH = h; }



// ----------------------------------------------------------

// [충돌 체크 수정] 스마트 월 충돌 감지 (중요!)

// *사용자님의 getNeighborValue를 사용하여 빈틈까지 막음*

// ----------------------------------------------------------

bool checkCollision() {

    Point3D playerPos = { 0.0f, -planetRadius + 1.5f, 0.0f };

    float collisionDist = 3.5f;



    for (int f = 0; f < 6; f++) {

        for (int r = 0; r < N; r++) {

            for (int c = 0; c < N; c++) {

                if (map[f][r][c] == 1) {

                    // 1. 중심 기둥 체크

                    Point3D pC = multiplyMatrixVector(getSpherePoint(f, (c + 0.5f) / N, (r + 0.5f) / N, planetRadius - 1.5f), planetRotationMatrix);

                    float dC = sqrt(pow(pC.x - playerPos.x, 2) + pow(pC.y - playerPos.y, 2) + pow(pC.z - playerPos.z, 2));

                    if (dC < collisionDist) return true;



                    // 2. 연결된 날개(Edge) 체크

                    if (getNeighborValue(f, r, c - 1) == 1) { // Left

                        Point3D pL = multiplyMatrixVector(getSpherePoint(f, (float)c / N, (r + 0.5f) / N, planetRadius - 1.5f), planetRotationMatrix);

                        if (sqrt(pow(pL.x - playerPos.x, 2) + pow(pL.y - playerPos.y, 2) + pow(pL.z - playerPos.z, 2)) < collisionDist) return true;

                    }

                    if (getNeighborValue(f, r, c + 1) == 1) { // Right

                        Point3D pR = multiplyMatrixVector(getSpherePoint(f, (float)(c + 1) / N, (r + 0.5f) / N, planetRadius - 1.5f), planetRotationMatrix);

                        if (sqrt(pow(pR.x - playerPos.x, 2) + pow(pR.y - playerPos.y, 2) + pow(pR.z - playerPos.z, 2)) < collisionDist) return true;

                    }

                    if (getNeighborValue(f, r - 1, c) == 1) { // Up

                        Point3D pU = multiplyMatrixVector(getSpherePoint(f, (c + 0.5f) / N, (float)r / N, planetRadius - 1.5f), planetRotationMatrix);

                        if (sqrt(pow(pU.x - playerPos.x, 2) + pow(pU.y - playerPos.y, 2) + pow(pU.z - playerPos.z, 2)) < collisionDist) return true;

                    }

                    if (getNeighborValue(f, r + 1, c) == 1) { // Down

                        Point3D pD = multiplyMatrixVector(getSpherePoint(f, (c + 0.5f) / N, (float)(r + 1) / N, planetRadius - 1.5f), planetRotationMatrix);

                        if (sqrt(pow(pD.x - playerPos.x, 2) + pow(pD.y - playerPos.y, 2) + pow(pD.z - playerPos.z, 2)) < collisionDist) return true;

                    }

                }

            }

        }

    }

    return false;

}



void checkInteraction() {

    Point3D pPos = { 0, -planetRadius + 1.5f, 0 };

    for (auto& item : items) {

        if (!item.active) continue;

        Point3D iPos = multiplyMatrixVector(getSpherePoint(item.face, (item.c + 0.5f) / N, (item.r + 0.5f) / N, planetRadius - 2), planetRotationMatrix);

        if (sqrt(pow(iPos.x - pPos.x, 2) + pow(iPos.y - pPos.y, 2) + pow(iPos.z - pPos.z, 2)) < 4.0f) {

            item.active = false; score += 100;

        }

    }

}



void movePlayer(float moveSpeed, float strafeSpeed) {

    float axisX = 0.0f, axisZ = 0.0f, angle = 0.0f;

    float sinYaw = sin(cameraYaw), cosYaw = cos(cameraYaw);



    if (moveSpeed != 0.0f) {

        axisX = cosYaw;

        axisZ = sinYaw;

        angle = moveSpeed;

    }

    else if (strafeSpeed != 0.0f) {

        axisX = sinYaw;

        axisZ = -cosYaw;

        angle = strafeSpeed;

    }



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



    if (key == 'v' || key == 'V') viewMode = !viewMode;

    if (key == ' ') checkInteraction();



    if (key == 27) {

        mouseCaptured = false;

        glutSetCursor(GLUT_CURSOR_INHERIT);

    }

    glutPostRedisplay();

}



void mouseMotion(int x, int y) {

    if (!mouseCaptured) return;

    int centerX = winW / 2;

    int centerY = winH / 2;

    int dx = x - centerX;

    int dy = y - centerY;

    if (dx == 0 && dy == 0) return;



    cameraYaw += dx * mouseSensitivity;

    cameraPitch -= dy * mouseSensitivity;



    if (cameraPitch > 1.5f) cameraPitch = 1.5f;

    if (cameraPitch < -1.5f) cameraPitch = -1.5f;



    glutWarpPointer(centerX, centerY);

    glutPostRedisplay();

}



void mouseClick(int button, int state, int x, int y) {

    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {

        if (!mouseCaptured) {

            mouseCaptured = true;

            glutSetCursor(GLUT_CURSOR_NONE);

            glutWarpPointer(winW / 2, winH / 2);

        }

    }

}



void specialKeys(int key, int x, int y) {}



int main(int argc, char** argv) {

    glutInit(&argc, argv);

    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);

    glutInitWindowSize(winW, winH);

    glutCreateWindow("Final Game: Planet Explorer");

    glEnable(GL_DEPTH_TEST); glEnable(GL_NORMALIZE);

    glutSetCursor(GLUT_CURSOR_NONE);



    makeCheckImage(0); makeCheckImage(1);

    initMap();

    loadModel("myModel.dat"); // 기본 모델 로드 (없어도 initMap에서 처리됨)



    glutDisplayFunc(display);

    glutReshapeFunc(reshape);

    glutKeyboardFunc(keyboard);

    glutPassiveMotionFunc(mouseMotion);

    glutMouseFunc(mouseClick);

    glutSpecialFunc(specialKeys);



    glutWarpPointer(winW / 2, winH / 2);

    glutMainLoop();

    return 0;

}