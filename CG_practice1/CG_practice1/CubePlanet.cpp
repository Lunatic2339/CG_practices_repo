// Visual Studio의 구형 C함수(scanf, fopen 등) 보안 경고 무시
#define _CRT_SECURE_NO_WARNINGS

#include <GL/glut.h>   // OpenGL 및 GLUT 라이브러리 (윈도우 생성, 입력 처리)
#include <vector>      // C++ 동적 배열 (점, 면, 아이템 관리용)
#include <cmath>       // 수학 함수 (sin, cos, sqrt 등)
#include <cstdio>      // 표준 입출력 (printf, fscanf 등)
#include <cstdlib>     // 표준 라이브러리 (rand 등)
#include <string>      // 문자열 처리

// 원주율 파이(PI) 정의 (삼각함수 계산에 필수)
#define M_PI 3.14159265358979323846

// ----------------------------------------------------------
// [데이터 구조체 정의]
// ----------------------------------------------------------

// 3D 공간의 점(Vertex) 좌표
struct Point3D {
    float x, y, z;
};

// 모델의 면(Face) 정보 (점들의 인덱스 저장)
struct Face {
    int v1, v2, v3;
};

// 게임 내 아이템(도자기 등) 정보
struct Item {
    int face, r, c; // 위치: 어느 면(Face)의 몇 행(r), 몇 열(c)에 있는가?
    bool active;    // 상태: 아직 안 먹었으면 true, 먹었으면 false
    float rot;      // 애니메이션: 제자리 회전 각도
};

// ----------------------------------------------------------
// [전역 변수] 게임의 상태 및 데이터 저장소
// ----------------------------------------------------------

// 모델링 데이터 저장소
std::vector<Point3D> vertices;
std::vector<Face> faces;
std::vector<Item> items;

// 게임 환경 설정
float planetRadius = 80.0f; // 행성의 크기 (반지름)
float playerHeight = 3.0f;  // 바닥에서 플레이어 눈높이까지 거리
int score = 0;              // 현재 점수
int totalItems = 3;         // 맵에 배치된 총 아이템 개수 (승리 조건용)

// 텍스처 ID (벽, 바닥)
GLuint texWall, texFloor;

// 화면 모드 (0: 게임 모드, 1: 디버그/지도 모드)
int viewMode = 0;

// [핵심: 월드 회전 행렬]
// 플레이어는 고정되어 있고 행성이 돕니다.
// 단순히 X, Y 각도만 쓰면 축이 꼬이는 '짐벌 락' 현상이 발생하므로,
// 4x4 행렬 전체를 저장하여 회전을 누적시킵니다.
GLfloat planetRotationMatrix[16] = {
    1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1 // 단위 행렬로 초기화
};

// 플레이어 시야각 (WASD 키로 조절)
float cameraYaw = 0.0f;   // 좌우 회전 (Y축 기준)
float cameraPitch = 0.0f; // 상하 회전 (X축 기준)

// 윈도우 크기 및 맵 데이터 설정
int winW = 1200, winH = 800;
const int N = 20; // 맵 한 면의 해상도 (20x20 격자)

// 큐브 맵의 6개 면 이름 정의 (직관적인 인덱싱을 위해)
enum { FACE_BACK = 0, FACE_FRONT = 1, FACE_RIGHT = 2, FACE_LEFT = 3, FACE_TOP = 4, FACE_BOTTOM = 5 };

// 전체 맵 데이터: [면6개][행N][열N]
// 0: 빈 공간, 1: 벽
int map[6][N][N] = { 0 };

// ----------------------------------------------------------
// [텍스처 생성] 이미지 파일 없이 코드로 텍스처 만들기 (절차적 텍스처)
// type 0: 벽 (체크무늬), type 1: 바닥 (노이즈)
// ----------------------------------------------------------
void makeCheckImage(int type) {
    const int width = 64;
    const int height = 64;
    GLubyte image[height][width][3]; // RGB 데이터

    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            int c;
            if (type == 0) {
                // [벽] 네온 벽돌 패턴 (비트 연산 이용)
                // XOR 연산을 이용해 격자 무늬 생성
                if ((i & 16) == 0 ^ (j & 16) == 0) c = 255; else c = 100;
                image[i][j][0] = 0;       // Red (0)
                image[i][j][1] = c;       // Green (Cyan 계열)
                image[i][j][2] = c;       // Blue
            }
            else {
                // [바닥] 우주 금속 패턴
                // 불규칙한 노이즈 느낌을 줌
                c = ((((i & 0x8) == 0) ^ ((j & 0x8)) == 0)) * 255;
                image[i][j][0] = c / 4;   // 어두운 남색 계열
                image[i][j][1] = c / 4;
                image[i][j][2] = c / 2 + 50;
            }
        }
    }

    // OpenGL 텍스처 생성 및 설정
    GLuint* targetTex = (type == 0) ? &texWall : &texFloor;
    glGenTextures(1, targetTex);          // ID 생성
    glBindTexture(GL_TEXTURE_2D, *targetTex); // 활성화

    // 텍스처 반복 및 필터링 설정
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST); // 도트 느낌 유지
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    // 이미지 데이터 전송
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
}

// ----------------------------------------------------------
// [수학 유틸리티] 3D 계산 함수들
// ----------------------------------------------------------

// 법선 벡터(Normal Vector) 계산: 조명 반사 방향을 결정
Point3D calculateNormal(Point3D p1, Point3D p2, Point3D p3) {
    // 두 벡터 구하기
    Point3D u = { p2.x - p1.x, p2.y - p1.y, p2.z - p1.z };
    Point3D v = { p3.x - p1.x, p3.y - p1.y, p3.z - p1.z };

    // 외적(Cross Product)으로 수직 벡터 구하기
    Point3D n = { u.y * v.z - u.z * v.y, u.z * v.x - u.x * v.z, u.x * v.y - u.y * v.x };

    // 정규화 (길이를 1로 만듦)
    float len = sqrt(n.x * n.x + n.y * n.y + n.z * n.z);
    if (len > 0) { n.x /= len; n.y /= len; n.z /= len; }
    return n;
}

// 벡터 정규화 함수
Point3D normalize(Point3D p) {
    float len = sqrt(p.x * p.x + p.y * p.y + p.z * p.z);
    if (len == 0) return { 0, 0, 0 };
    return { p.x / len, p.y / len, p.z / len };
}

// [핵심 좌표 변환] 큐브 맵 좌표 -> 구면 좌표 변환
// 정육면체의 면(Face) 위의 점 (u, v)를 구(Sphere) 표면의 점 (x, y, z)로 매핑
Point3D getSpherePoint(int face, float u, float v, float r) {
    // 1. 0~1 범위의 u,v를 -1~1 범위로 변환
    float x = (u - 0.5f) * 2.0f;
    float y = (v - 0.5f) * 2.0f;
    float z = 1.0f; // 면은 중심에서 떨어져 있음

    Point3D p = { 0, 0, 0 };

    // 2. 각 면의 방향에 따라 좌표축 회전
    switch (face) {
    case FACE_FRONT:  p = { x, y, z }; break;
    case FACE_BACK:   p = { -x, y, -z }; break;
    case FACE_RIGHT:  p = { z, y, -x }; break;
    case FACE_LEFT:   p = { -z, y, x }; break;
    case FACE_TOP:    p = { x, z, -y }; break;
    case FACE_BOTTOM: p = { x, -z, y }; break;
    }

    // 3. 정규화하여 구 형태로 만듦 (Spherification) 후 반지름 적용
    p = normalize(p);
    p.x *= r; p.y *= r; p.z *= r;
    return p;
}

// 행렬 x 벡터 곱셈 함수 (충돌 감지 시 회전된 벽의 위치 계산용)
Point3D multiplyMatrixVector(Point3D p, GLfloat* m) {
    Point3D res;
    // 4x4 행렬과 3x1 벡터의 곱셈 (동차 좌표계 w=1 가정)
    res.x = m[0] * p.x + m[4] * p.y + m[8] * p.z + m[12];
    res.y = m[1] * p.x + m[5] * p.y + m[9] * p.z + m[13];
    res.z = m[2] * p.x + m[6] * p.y + m[10] * p.z + m[14];
    return res;
}

// ----------------------------------------------------------
// [모델 로딩] SOR 모델러로 만든 .dat 파일 읽기
// ----------------------------------------------------------
void loadModel(const char* filename) {
    FILE* fp = fopen(filename, "r");
    if (!fp) return; // 파일 없으면 패스

    char buf[128]; int numV, numF;

    // 정점 개수 및 좌표 읽기
    fscanf(fp, "%s %s %d", buf, buf, &numV);
    for (int i = 0; i < numV; i++) {
        Point3D p; fscanf(fp, "%f %f %f", &p.x, &p.y, &p.z); vertices.push_back(p);
    }

    // 면 개수 및 인덱스 읽기
    fscanf(fp, "%s %s %d", buf, buf, &numF);
    for (int i = 0; i < numF; i++) {
        Face f; fscanf(fp, "%d %d %d", &f.v1, &f.v2, &f.v3); faces.push_back(f);
    }
    fclose(fp);
}

// ----------------------------------------------------------
// [맵 초기화] 미로 벽과 아이템 배치
// ----------------------------------------------------------
void initMap() {
    // 1. 초기 테두리 벽 세우기 (사각형 모양)
    // 1번 면(FRONT)을 예시로 전체 테두리를 두름
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            map[1][i][j] = 1; // 일단 다 채우고
        }
    }
    // 십자 모양으로 길 뚫기 (테스트용)
    for (int i = 0; i < N / 2; i++) {
        map[1][i][N / 2] = 0;
        map[1][N / 2][i] = 0;
    }

    // 2. 바닥(5번) 테두리 벽 세우기
    // 중간 통로(i == N/2)는 뚫어둠
    for (int i = 0; i < N; i++) {
        if (i == N / 2) continue; // 길 터주기
        map[5][0][i] = 1;      // 위쪽 벽
        map[5][N - 1][i] = 1;  // 아래쪽 벽
        map[5][i][0] = 1;      // 왼쪽 벽
        map[5][i][N - 1] = 1;  // 오른쪽 벽
    }
}

// ----------------------------------------------------------
// [렌더링] 텍스처가 입혀진 '단순' 벽 그리기 (기본형)
// ----------------------------------------------------------
void drawTexturedWall(int f, int r, int c) {
    // 현재 칸(Cell)의 텍스처 좌표(u,v) 범위 계산
    float u1 = (float)c / N; float u2 = (float)(c + 1) / N;
    float v1 = (float)r / N; float v2 = (float)(r + 1) / N;
    float h = 3.0f; // 벽 높이

    // 8개의 꼭짓점 계산 (바닥면 4개 + 천장면 4개)
    // 큐브 매핑 -> 구면 매핑 변환 적용
    Point3D p4 = getSpherePoint(f, u1, v1, planetRadius - h); // 천장 (내부)
    Point3D p5 = getSpherePoint(f, u2, v1, planetRadius - h);
    Point3D p6 = getSpherePoint(f, u2, v2, planetRadius - h);
    Point3D p7 = getSpherePoint(f, u1, v2, planetRadius - h);
    Point3D p0 = getSpherePoint(f, u1, v1, planetRadius);     // 바닥 (외부)
    Point3D p1 = getSpherePoint(f, u2, v1, planetRadius);
    Point3D p2 = getSpherePoint(f, u2, v2, planetRadius);
    Point3D p3 = getSpherePoint(f, u1, v2, planetRadius);

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texWall);
    glColor3f(1, 1, 1);

    glBegin(GL_QUADS);
    // 각 면마다 법선(Normal) 계산 후 정점과 텍스처 좌표 지정
    Point3D n;

    // 윗면 (플레이어가 가장 많이 보는 면)
    n = calculateNormal(p4, p5, p6); glNormal3f(n.x, n.y, n.z);
    glTexCoord2f(0, 0); glVertex3f(p4.x, p4.y, p4.z);
    glTexCoord2f(1, 0); glVertex3f(p5.x, p5.y, p5.z);
    glTexCoord2f(1, 1); glVertex3f(p6.x, p6.y, p6.z);
    glTexCoord2f(0, 1); glVertex3f(p7.x, p7.y, p7.z);

    // 옆면들 (앞, 뒤, 좌, 우)
    // ... (상세 좌표 지정 생략 - 위와 동일 패턴)
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
// [핵심 로직] 이웃 체크: 면과 면 사이의 연결 관리
// *사용자 직접 검증 완료*: 중앙(5번)을 향해 모든 옆면(0~3)의 머리(Row 0)가 모이는 구조
// ----------------------------------------------------------
int getNeighborValue(int f, int r, int c) {
    // 1. 현재 면 범위 안쪽이면 -> 그냥 배열 값 리턴
    if (r >= 0 && r < N && c >= 0 && c < N) {
        return map[f][r][c];
    }

    int targetF = f; // 목표 면
    int tr = r;      // 목표 행
    int tc = c;      // 목표 열

    // 2. 경계면 연결 로직 (Switch-Case로 분기)
    switch (f) {

        // =======================================================
        // [중심] 5번 면 (Bottom / Floor)
        // =======================================================
    case FACE_BOTTOM:
        if (r < 0) { // 위쪽 -> 1번(Back)의 머리 (180도 회전)
            targetF = FACE_BACK; tr = 0; tc = N - 1 - c;
        }
        else if (r >= N) { // 아래쪽 -> 0번(Front)의 머리 (정방향)
            targetF = FACE_FRONT; tr = 0; tc = c;
        }
        else if (c < 0) { // 왼쪽 -> 3번(Left)의 머리 (시계 90도 회전)
            targetF = FACE_LEFT; tr = 0; tc = r;
        }
        else if (c >= N) { // 오른쪽 -> 2번(Right)의 머리 (반시계 90도 회전)
            targetF = FACE_RIGHT; tr = 0; tc = N - 1 - r;
        }
        break;

        // =======================================================
        // [옆면] 1번 면 (Back)
        // =======================================================
    case FACE_BACK:
        if (r < 0) { // 위쪽 -> 5번(Bottom)의 위 (180도)
            targetF = FACE_BOTTOM; tr = 0; tc = N - 1 - c;
        }
        else if (r >= N) { // 아래쪽 -> 4번(Top)의 아래 (180도)
            targetF = FACE_TOP; tr = N - 1; tc = N - 1 - c;
        }
        else if (c < 0) { // 왼쪽 -> 2번(Right)의 오른쪽 (그대로)
            targetF = FACE_RIGHT; tr = r; tc = N - 1;
        }
        else if (c >= N) { // 오른쪽 -> 3번(Left)의 왼쪽 (그대로)
            targetF = FACE_LEFT; tr = r; tc = 0;
        }
        break;

        // =======================================================
        // [옆면] 0번 면 (Front)
        // =======================================================
    case FACE_FRONT:
        if (r < 0) { // 위쪽 -> 5번(Bottom)의 아래 (정방향)
            targetF = FACE_BOTTOM; tr = N - 1; tc = c;
        }
        else if (r >= N) { // 아래쪽 -> 4번(Top)의 위 (정방향)
            targetF = FACE_TOP; tr = 0; tc = c;
        }
        else if (c < 0) { // 왼쪽 -> 3번(Left)의 오른쪽 (그대로)
            targetF = FACE_LEFT; tr = r; tc = N - 1;
        }
        else if (c >= N) { // 오른쪽 -> 2번(Right)의 왼쪽 (그대로)
            targetF = FACE_RIGHT; tr = r; tc = 0;
        }
        break;

        // =======================================================
        // [옆면] 3번 면 (Left)
        // =======================================================
    case FACE_LEFT:
        if (r < 0) { // 위쪽 -> 5번(Bottom)의 왼쪽 (반시계 90도)
            targetF = FACE_BOTTOM; tr = c; tc = 0;
        }
        else if (r >= N) { // 아래쪽 -> 4번(Top)의 왼쪽 (반시계 90도?)
            targetF = FACE_TOP; tr = N - 1 - c; tc = 0;
        }
        else if (c < 0) { // 왼쪽 -> 1번(Back)의 오른쪽
            targetF = FACE_BACK; tr = r; tc = N - 1;
        }
        else if (c >= N) { // 오른쪽 -> 0번(Front)의 왼쪽
            targetF = FACE_FRONT; tr = r; tc = 0;
        }
        break;

        // =======================================================
        // [옆면] 2번 면 (Right)
        // =======================================================
    case FACE_RIGHT:
        if (r < 0) { // 위쪽 -> 5번(Bottom)의 오른쪽 (시계 90도)
            targetF = FACE_BOTTOM; tr = N - 1 - c; tc = N - 1;
        }
        else if (r >= N) { // 아래쪽 -> 4번(Top)의 오른쪽 (시계 90도?)
            targetF = FACE_TOP; tr = c; tc = N - 1;
        }
        else if (c < 0) { // 왼쪽 -> 0번(Front)의 오른쪽
            targetF = FACE_FRONT; tr = r; tc = N - 1;
        }
        else if (c >= N) { // 오른쪽 -> 1번(Back)의 왼쪽
            targetF = FACE_BACK; tr = r; tc = 0;
        }
        break;

        // =======================================================
        // [천장] 4번 면 (Top / Ceiling)
        // =======================================================
    case FACE_TOP:
        if (r < 0) { // 위쪽 -> 0번(Front)의 아래 (정방향)
            targetF = FACE_FRONT; tr = N - 1; tc = c;
        }
        else if (r >= N) { // 아래쪽 -> 1번(Back)의 아래 (180도)
            targetF = FACE_BACK; tr = N - 1; tc = N - 1 - c;
        }
        else if (c < 0) { // 왼쪽 -> 3번(Left)의 아래 (반시계 90도)
            targetF = FACE_LEFT; tr = N - 1; tc = N - 1 - r;
        }
        else if (c >= N) { // 오른쪽 -> 2번(Right)의 아래 (시계 90도)
            targetF = FACE_RIGHT; tr = N - 1; tc = r;
        }
        break;
    }

    // 인덱스 범위 안전 장치 (혹시 모를 에러 방지)
    if (tr < 0) tr = 0; if (tr >= N) tr = N - 1;
    if (tc < 0) tc = 0; if (tc >= N) tc = N - 1;

    return map[targetF][tr][tc];
}


// ----------------------------------------------------------
// [스마트 월 그리기 헬퍼] 벽의 한 조각(Segment)을 그리는 함수
// 중심 기둥이나 연결되는 날개를 그릴 때 사용
// ----------------------------------------------------------
void drawWallSegment(int f, float u_s, float u_e, float v_s, float v_e) {
    float h = 3.0f; // 벽 높이

    // 1. 꼭짓점 8개 계산 (바닥 4개, 천장 4개)
    Point3D p0 = getSpherePoint(f, u_s, v_s, planetRadius);
    Point3D p1 = getSpherePoint(f, u_e, v_s, planetRadius);
    Point3D p2 = getSpherePoint(f, u_e, v_e, planetRadius);
    Point3D p3 = getSpherePoint(f, u_s, v_e, planetRadius);
    Point3D p4 = getSpherePoint(f, u_s, v_s, planetRadius - h);
    Point3D p5 = getSpherePoint(f, u_e, v_s, planetRadius - h);
    Point3D p6 = getSpherePoint(f, u_e, v_e, planetRadius - h);
    Point3D p7 = getSpherePoint(f, u_s, v_e, planetRadius - h);

    // 2. 텍스처를 입힌 육면체(Quads) 그리기
    glBegin(GL_QUADS);
    Point3D n;
    // 윗면
    n = calculateNormal(p4, p5, p6); glNormal3f(n.x, n.y, n.z);
    glTexCoord2f(0, 0); glVertex3f(p4.x, p4.y, p4.z);
    glTexCoord2f(1, 0); glVertex3f(p5.x, p5.y, p5.z);
    glTexCoord2f(1, 1); glVertex3f(p6.x, p6.y, p6.z);
    glTexCoord2f(0, 1); glVertex3f(p7.x, p7.y, p7.z);

    // 옆면들 (앞/뒤/좌/우) - 상세 생략 (drawTexturedWall과 동일)
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
// [스마트 월 메인] 얇고 연결된 벽 그리기
// 주변에 벽이 있으면 그쪽으로 연결되는 '날개'를 그림
// ----------------------------------------------------------
void drawSmartWall(int f, int r, int c) {
    // 1. 현재 셀의 기본 좌표 계산
    float u1 = (float)c / N;
    float u2 = (float)(c + 1) / N;
    float v1 = (float)r / N;
    float v2 = (float)(r + 1) / N;

    // 2. 얇은 벽 두께 설정 (셀 크기의 20%)
    float cellW = u2 - u1;
    float cellH = v2 - v1;
    float thickW = cellW * 0.2f;
    float thickH = cellH * 0.2f;

    // 중심부(Center) 좌표
    float uc_s = u1 + (cellW - thickW) / 2.0f;
    float uc_e = u1 + (cellW + thickW) / 2.0f;
    float vc_s = v1 + (cellH - thickH) / 2.0f;
    float vc_e = v1 + (cellH + thickH) / 2.0f;

    // 3. 이웃 확인 (getNeighborValue 호출)
    // 상하좌우에 벽이 있으면 연결함 (conn = true)
    bool connL = (getNeighborValue(f, r, c - 1) == 1);
    bool connR = (getNeighborValue(f, r, c + 1) == 1);
    bool connU = (getNeighborValue(f, r - 1, c) == 1);
    bool connD = (getNeighborValue(f, r + 1, c) == 1);

    // 텍스처 및 색상 설정 (붓 씻기)
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texWall);
    GLfloat white[] = { 1,1,1,1 };
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, white);

    // 4. 벽 조각 그리기 (5-Part Rendering)
    drawWallSegment(f, uc_s, uc_e, vc_s, vc_e); // 중심 기둥 (항상 그림)

    if (connL) drawWallSegment(f, u1, uc_s, vc_s, vc_e);   // 왼쪽 날개
    if (connR) drawWallSegment(f, uc_e, u2, vc_s, vc_e);   // 오른쪽 날개
    if (connU) drawWallSegment(f, uc_s, uc_e, v1, vc_s);   // 위쪽 날개
    if (connD) drawWallSegment(f, uc_s, uc_e, vc_e, v2);   // 아래쪽 날개

    glDisable(GL_TEXTURE_2D);
}

// ----------------------------------------------------------
// [장면 그리기] 전체 월드 렌더링
// ----------------------------------------------------------
void drawScene(bool isWireMode) {
    // 양면 조명 활성화 (내부 탐험 필수)
    glLightModelf(GL_LIGHT_MODEL_TWO_SIDE, 1.0);
    glDisable(GL_CULL_FACE);

    // 재질 초기화 (이전 프레임 색상 잔상 방지)
    GLfloat white[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    GLfloat black[] = { 0.0f, 0.0f, 0.0f, 1.0f };
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, white);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, black);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 0.0f);

    // 1. 바닥(행성 표면) 그리기
    if (isWireMode) {
        // 절대 시점에서는 내부가 보여야 하므로 선으로 그림
        glDisable(GL_LIGHTING); glColor3f(0.3f, 0.3f, 0.3f);
        glutWireSphere(planetRadius, 20, 20); glEnable(GL_LIGHTING);
    }
    else {
        // 1인칭 시점에서는 텍스처 입힌 구체
        glEnable(GL_TEXTURE_2D); glBindTexture(GL_TEXTURE_2D, texFloor);
        glColor3f(0.7f, 0.7f, 0.8f); // 약간 어둡게 틴트

        GLUquadric* quad = gluNewQuadric();
        gluQuadricTexture(quad, GL_TRUE);
        gluSphere(quad, planetRadius, 50, 50); // 구체 생성
        gluDeleteQuadric(quad);
        glDisable(GL_TEXTURE_2D);
    }

    // 2. 벽 & 모델 그리기
    for (int f = 0; f < 6; f++) {
        for (int r = 0; r < N; r++) {
            for (int c = 0; c < N; c++) {
                if (map[f][r][c] == 0) continue; // 빈 공간

                if (map[f][r][c] == 1) {
                    // [벽] 스마트 월 방식으로 그림
                    drawSmartWall(f, r, c);
                }
                else if (map[f][r][c] == 9) {
                    // [모델] (기존 모델링 파일 렌더링)
                    Point3D center = getSpherePoint(f, (c + 0.5f) / N, (r + 0.5f) / N, planetRadius);
                    glPushMatrix();
                    glTranslatef(center.x, center.y, center.z);
                    glTranslatef(0.0f, 0.0f, -2.0f); // 바닥 위로 띄움

                    // 아이템 배치 설정
                    glRotatef(-90, 1.0f, 0.0f, 0.0f); // 세우기
                    glScalef(0.005f, 0.005f, 0.005f); // 크기 조절

                    // 금색 재질
                    GLfloat gold[] = { 1.0f, 0.8f, 0.0f, 1.0f };
                    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, gold);
                    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, gold);
                    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 100.0f);

                    // 모델 데이터 그리기
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
// 텍스트 출력 함수 (화면 UI용)
// ----------------------------------------------------------
void drawText(const char* str, float x, float y, float r, float g, float b) {
    // 조명과 3D 설정을 끄고 2D 오버레이 모드로 전환
    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity();
    gluOrtho2D(0, winW, 0, winH); // 2D 좌표계
    glMatrixMode(GL_MODELVIEW); glPushMatrix(); glLoadIdentity();

    glColor3f(r, g, b);
    glRasterPos2f(x, y);
    for (const char* c = str; *c != '\0'; c++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c); // 글자 출력
    }

    // 설정 복구
    glPopMatrix(); glMatrixMode(GL_PROJECTION); glPopMatrix(); glMatrixMode(GL_MODELVIEW);
    glEnable(GL_LIGHTING); glEnable(GL_DEPTH_TEST);
}

// ----------------------------------------------------------
// [화면 출력] 뷰포트 분할 및 시점 처리
// ----------------------------------------------------------
void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_LIGHTING); glEnable(GL_LIGHT0);
    GLfloat lightPos[] = { 0.0f, 0.0f, 0.0f, 1.0f }; // 구의 정중앙 광원 (태양)

    // 1인칭 카메라 위치 계산 (Player Height 적용)
    float eyeY = -planetRadius + playerHeight;
    float lookX = sin(cameraYaw) * cos(cameraPitch);
    float lookY = sin(cameraPitch);
    float lookZ = -cos(cameraYaw) * cos(cameraPitch);

    if (viewMode == 0) {
        // [모드 0] 게임 모드 (전체 화면 1인칭)
        glViewport(0, 0, winW, winH);
        glMatrixMode(GL_PROJECTION); glLoadIdentity();
        gluPerspective(60.0f, (float)winW / winH, 0.1f, 1000.0f);

        glMatrixMode(GL_MODELVIEW); glLoadIdentity();
        gluLookAt(0, eyeY, 0, lookX, eyeY + lookY, lookZ, 0, 1, 0);
        glLightfv(GL_LIGHT0, GL_POSITION, lightPos);

        glPushMatrix();
        glMultMatrixf(planetRotationMatrix); // 회전된 월드 적용
        drawScene(false);
        glPopMatrix();
    }
    else {
        // [모드 1] 디버그 모드 (3분할 화면)

        // 1. 왼쪽: 1인칭 화면
        glViewport(0, 0, winW / 2, winH);
        glMatrixMode(GL_PROJECTION); glLoadIdentity();
        gluPerspective(60.0f, (float)(winW / 2) / winH, 0.1f, 1000.0f);
        glMatrixMode(GL_MODELVIEW); glLoadIdentity();
        gluLookAt(0, eyeY, 0, lookX, eyeY + lookY, lookZ, 0, 1, 0);
        glLightfv(GL_LIGHT0, GL_POSITION, lightPos);
        glPushMatrix(); glMultMatrixf(planetRotationMatrix); drawScene(false); glPopMatrix();

        // 2. 우상단: 네비게이션 (진행 방향이 위로 오도록 회전)
        glViewport(winW / 2, winH / 2, winW / 2, winH / 2);
        glMatrixMode(GL_PROJECTION); glLoadIdentity();
        gluPerspective(60.0f, (float)(winW / 2) / (winH / 2), 0.1f, 1000.0f);
        glMatrixMode(GL_MODELVIEW); glLoadIdentity();
        gluLookAt(0, 0, 0, 0, -1, 0, 0, 0, -1); // 위에서 아래로
        glLightfv(GL_LIGHT0, GL_POSITION, lightPos);
        glPushMatrix();
        glRotatef(cameraYaw * 180.0f / M_PI, 0.0f, 1.0f, 0.0f); // 맵 회전
        glMultMatrixf(planetRotationMatrix); drawScene(false);
        glPopMatrix();

        // 플레이어 표시
        glDisable(GL_LIGHTING);
        glPushMatrix(); glTranslatef(0, -planetRadius + 1, 0); glColor3f(1, 0, 0); glutSolidCube(2); glPopMatrix();
        glEnable(GL_LIGHTING);

        // 3. 우하단: 절대 시점 (외부에서 투시)
        glViewport(winW / 2, 0, winW / 2, winH / 2);
        glMatrixMode(GL_PROJECTION); glLoadIdentity();
        gluPerspective(60.0f, (float)(winW / 2) / (winH / 2), 0.1f, 1000.0f);
        glMatrixMode(GL_MODELVIEW); glLoadIdentity();
        gluLookAt(0, -100, 100, 0, 0, 0, 0, 1, 0); // 멀리서
        glLightfv(GL_LIGHT0, GL_POSITION, lightPos);
        glPushMatrix(); glMultMatrixf(planetRotationMatrix); drawScene(true); glPopMatrix(); // 와이어프레임
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
    if (score == totalItems * 100) {
        char msg[] = "MISSION COMPLETE!";
        char sub[] = "Press ESC to Exit";

        // 깜빡이는 효과
        static float alpha = 0.0f;
        alpha += 0.05f;
        float blink = fabs(sin(alpha));

        drawText(msg, winW / 2 - 80, winH / 2, 1.0f, blink, 0.0f);
        drawText(sub, winW / 2 - 60, winH / 2 - 30, 1.0f, 1.0f, 1.0f);
    }

    glutSwapBuffers();
}

void reshape(int w, int h) { winW = w; winH = h; }

// ----------------------------------------------------------
// [충돌 체크] 플레이어와 벽 사이의 거리 계산
// ----------------------------------------------------------
bool checkCollision() {
    // 플레이어는 항상 화면 중앙 하단 고정 위치에 있음
    Point3D playerPos = { 0.0f, -planetRadius + 1.5f, 0.0f };
    float collisionDist = 3.5f; // 충돌 반경

    for (int f = 0; f < 6; f++) {
        for (int r = 0; r < N; r++) {
            for (int c = 0; c < N; c++) {
                if (map[f][r][c] == 1) { // 벽이 있다면
                    // 1. 벽의 로컬 좌표 구하기
                    Point3D wallLocal = getSpherePoint(f, (c + 0.5f) / N, (r + 0.5f) / N, planetRadius - 1.5f);

                    // 2. 현재 월드 회전 행렬을 적용해 실제 월드 좌표 구하기
                    Point3D wallWorld = multiplyMatrixVector(wallLocal, planetRotationMatrix);

                    // 3. 거리 계산
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

// [아이템 상호작용]
void checkInteraction() {
    Point3D playerPos = { 0.0f, -planetRadius + 1.5f, 0.0f };
    float interactDist = 4.0f;

    for (auto& item : items) {
        if (!item.active) continue; // 이미 먹은 건 패스

        Point3D itemLocal = getSpherePoint(item.face, (item.c + 0.5f) / N, (item.r + 0.5f) / N, planetRadius - 2.0f);
        Point3D itemWorld = multiplyMatrixVector(itemLocal, planetRotationMatrix);

        float dx = itemWorld.x - playerPos.x;
        float dy = itemWorld.y - playerPos.y;
        float dz = itemWorld.z - playerPos.z;

        if (sqrt(dx * dx + dy * dy + dz * dz) < interactDist) {
            item.active = false; // 획득 처리
            score += 100;
            printf("Item Collected! Score: %d\n", score);
        }
    }
}

// 키보드 입력 (시점 조절)
void keyboard(unsigned char key, int x, int y) {
    float rotSpeed = 0.05f;
    if (key == 'a' || key == 'A') cameraYaw -= rotSpeed;
    if (key == 'd' || key == 'D') cameraYaw += rotSpeed;
    if (key == 'w' || key == 'W') cameraPitch += rotSpeed;
    if (key == 's' || key == 'S') cameraPitch -= rotSpeed;

    // [V키] 시점 변환
    if (key == 'v' || key == 'V') viewMode = !viewMode;

    // [SPACE] 아이템 획득
    if (key == ' ') checkInteraction();

    // 고개 각도 제한 (목 꺾임 방지)
    if (cameraPitch > 1.5f) cameraPitch = 1.5f;
    if (cameraPitch < -1.5f) cameraPitch = -1.5f;
    if (key == 27) exit(0);
    glutPostRedisplay();
}

// 특수 키 입력 (이동)
void specialKeys(int key, int x, int y) {
    float moveSpeed = 1.5f;
    // 현재 시선(Yaw) 기준 이동 축 계산
    float axisX = 0.0f, axisZ = 0.0f, angle = 0.0f;
    float sinYaw = sin(cameraYaw), cosYaw = cos(cameraYaw);

    if (key == GLUT_KEY_UP) { axisX = cosYaw; axisZ = sinYaw; angle = -moveSpeed; }
    else if (key == GLUT_KEY_DOWN) { axisX = cosYaw; axisZ = sinYaw; angle = moveSpeed; }
    else if (key == GLUT_KEY_RIGHT) { axisX = sinYaw; axisZ = -cosYaw; angle = moveSpeed; }
    else if (key == GLUT_KEY_LEFT) { axisX = sinYaw; axisZ = -cosYaw; angle = -moveSpeed; }

    // [이동 로직: 행렬 누적]
    // 1. 현재 행렬 백업 (충돌 시 복구용)
    GLfloat backupMatrix[16];
    for (int i = 0; i < 16; i++) backupMatrix[i] = planetRotationMatrix[i];

    // 2. 가상으로 이동 적용
    glPushMatrix(); glLoadIdentity();
    glRotatef(angle, axisX, 0.0f, axisZ);
    glMultMatrixf(planetRotationMatrix);
    glGetFloatv(GL_MODELVIEW_MATRIX, planetRotationMatrix);
    glPopMatrix();

    // 3. 충돌 검사
    if (checkCollision()) {
        // 충돌 시 원상 복구 (이동 취소)
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