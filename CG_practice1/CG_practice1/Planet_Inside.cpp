// Visual Studio에서 scanf, fopen 같은 C언어 함수 사용 시 발생하는 보안 경고를 무시합니다.
#define _CRT_SECURE_NO_WARNINGS 

#include <GL/glut.h>   // OpenGL 및 GLUT 라이브러리 (창 생성, 입력 처리 등)
#include <vector>      // C++ 벡터 (동적 배열, 점과 면 데이터를 담기 위함)
#include <cmath>       // 수학 함수 (sin, cos, sqrt 등)
#include <cstdio>      // 파일 입출력 (fopen, fscanf)
#include <cstdlib>     // 표준 라이브러리

// 원주율 파이(PI) 값 정의 (삼각함수 계산용)
#define M_PI 3.14159265358979323846

// ----------------------------------------------------------
// [데이터 구조체 정의]
// ----------------------------------------------------------

// 3차원 공간의 점(Vertex) 하나를 표현하는 구조체
struct Point3D {
    float x, y, z;
};

// 삼각형 면(Face) 하나를 표현하는 구조체
// 점들의 실제 좌표가 아니라, vertices 배열의 '인덱스 번호' 3개를 저장합니다.
struct Face {
    int v1, v2, v3;
};

// ----------------------------------------------------------
// [전역 변수] - 프로그램 전체에서 공유하는 데이터
// ----------------------------------------------------------

// 모델 데이터를 저장할 벡터들
std::vector<Point3D> vertices; // 불러온 점들의 목록
std::vector<Face> faces;       // 점들을 이어 만든 삼각형 면들의 목록

// 행성 및 플레이어 설정
float planetRadius = 40.0f;    // 구(Sphere)의 반지름
float playerHeight = 3.0f;     // 바닥에서 플레이어 눈까지의 높이

// [핵심: 회전 행렬]
// 공이 굴러간 상태를 단순히 각도(x, y)로만 저장하면 축이 꼬이는 문제(짐벌락)가 발생합니다.
// 따라서 현재 공의 회전 상태 전체를 4x4 행렬로 저장하여 누적시킵니다.
GLfloat planetRotationMatrix[16] = {
    1, 0, 0, 0,  // 초기값: 단위 행렬 (회전하지 않은 상태)
    0, 1, 0, 0,
    0, 0, 1, 0,
    0, 0, 0, 1
};

// 카메라(플레이어 고개) 각도
float cameraYaw = 0.0f;   // 좌우 회전 (Y축 기준)
float cameraPitch = 0.0f; // 상하 회전 (X축 기준)

// 윈도우 창 크기 (화면 분할 비율 계산을 위해 필요)
int winW = 1200;
int winH = 800;

// 미로 맵 데이터
// 큐브의 6개 면을 숫자로 정의 (이해하기 쉽게)
enum {
    FACE_BACK = 0,
    FACE_FRONT,
    FACE_RIGHT,
    FACE_LEFT,
    FACE_TOP,
    FACE_BOTTOM
};

// 한 면의 크기 (N x N)
const int N = 10;

// 맵 데이터: [면6개][행N][열N]
// 기존 20x20 하나를 쓰는 대신, 10x10짜리 6개를 씁니다.
int map[6][N][N] = { 0 };

// 행성의 반지름 (큐브의 중심에서 면까지의 거리)
float planetRadius = 40.0f;

// ----------------------------------------------------------
// [유틸리티 함수 1] 법선 벡터(Normal Vector) 계산
// 빛이 물체에 닿았을 때 어떻게 반사될지 결정하기 위해,
// 면이 바라보는 '수직 방향'을 계산합니다.
// ----------------------------------------------------------
Point3D calculateNormal(Point3D p1, Point3D p2, Point3D p3) {
    // 두 변의 벡터(u, v) 구하기
    Point3D u = { p2.x - p1.x, p2.y - p1.y, p2.z - p1.z };
    Point3D v = { p3.x - p1.x, p3.y - p1.y, p3.z - p1.z };

    // 외적(Cross Product)을 통해 두 벡터에 수직인 벡터 구하기
    Point3D normal = {
        u.y * v.z - u.z * v.y,
        u.z * v.x - u.x * v.z,
        u.x * v.y - u.y * v.x
    };

    // 정규화(Normalization): 벡터의 길이를 1로 만듦 (방향만 남김)
    float len = sqrt(normal.x * normal.x + normal.y * normal.y + normal.z * normal.z);
    if (len > 0) { normal.x /= len; normal.y /= len; normal.z /= len; }

    return normal;
}

// ----------------------------------------------------------
// [유틸리티 함수 2] 모델 파일 불러오기 (.dat)
// ----------------------------------------------------------
void loadModel(const char* filename) {
    FILE* fp = fopen(filename, "r");
    if (!fp) return; // 파일 없으면 종료

    char buf[128]; // 임시 문자열 저장소
    int numV, numF;

    // 1. 점(Vertex) 개수 읽기
    fscanf(fp, "%s %s %d", buf, buf, &numV); // "VERTEX = 100" 형식을 읽음
    for (int i = 0; i < numV; i++) {
        Point3D p;
        fscanf(fp, "%f %f %f", &p.x, &p.y, &p.z); // x, y, z 좌표 읽기
        vertices.push_back(p); // 벡터에 추가
    }

    // 2. 면(Face) 개수 읽기
    fscanf(fp, "%s %s %d", buf, buf, &numF); // "FACE = 200" 형식을 읽음
    for (int i = 0; i < numF; i++) {
        Face f;
        fscanf(fp, "%d %d %d", &f.v1, &f.v2, &f.v3); // 점 인덱스 3개 읽기
        faces.push_back(f);
    }
    fclose(fp);
}

// ----------------------------------------------------------
// [유틸리티 함수 3] 맵 데이터 초기화
// ----------------------------------------------------------
void initMap() {
    // 모든 면 초기화 (테두리 벽 세우기)
    for (int f = 0; f < 6; f++) {
        for (int i = 0; i < N; i++) {
            // 각 면의 테두리에 벽(1) 세우기
            map[f][0][i] = 1;
            map[f][N - 1][i] = 1;
            map[f][i][0] = 1;
            map[f][i][N - 1] = 1;
        }
    }

    // [테스트] 면 구분을 위해 특정 위치에 표시
    map[FACE_BACK][N / 2][N / 2] = 9; // 앞면에 도자기(모델)
    map[FACE_TOP][2][2] = 1;       // 윗면에 벽
    map[FACE_RIGHT][3][3] = 1;     // 오른쪽에 벽
}


// (u, v)는 0.0 ~ 1.0 사이의 비율 좌표
Point3D getCubePoint(int faceID, float u, float v, float radius) {
    Point3D p = { 0, 0, 0 };

    // 범위 -1.0 ~ +1.0 으로 변환 (중심 기준)
    float x = (u - 0.5f) * 2.0f * radius;
    float y = (v - 0.5f) * 2.0f * radius;
    float z = radius; // 면은 중심에서 반지름만큼 떨어져 있음

    // 면의 방향에 따라 회전시킴
    switch (faceID) {
    case FACE_BACK:  // 앞면 (Z+)
        p = { x, y, z };
        break;
    case FACE_FRONT:   // 뒷면 (Z-) -> 180도 회전
        p = { -x, y, -z };
        break;
    case FACE_RIGHT:  // 오른쪽 (X+) -> Y축 -90도 회전
        p = { z, y, -x };
        break;
    case FACE_LEFT:   // 왼쪽 (X-) -> Y축 +90도 회전
        p = { -z, y, x };
        break;
    case FACE_TOP:    // 윗면 (Y+) -> X축 -90도 회전
        p = { x, z, -y };
        break;
    case FACE_BOTTOM: // 아랫면 (Y-) -> X축 +90도 회전
        p = { x, -z, y };
        break;
    }
    return p;
}


// ----------------------------------------------------------
// [곡면 벽 그리기] 
// 큐브 대신, 구의 곡률에 맞춰 휘어진 육면체(Wedge)를 그립니다.
// r: 구의 반지름 (바닥 위치)
// h: 벽의 높이 (안쪽으로 얼마나 튀어나올지)
// theta, phi: 현재 위치의 각도
// step: 한 칸의 각도 크기 (예: 6도)
// ----------------------------------------------------------
// ----------------------------------------------------------
// [곡면 벽 그리기 - 수정판]
// 법선 벡터(Normal)를 자동으로 계산하여 빛 반사를 정상화함
// ----------------------------------------------------------
void drawCurvedWall(float r, float h, float theta, float phi, float step) {
    float deg2rad = M_PI / 180.0f;

    // 1. 각도 범위 계산
    float t1 = theta - (step / 2.0f);
    float t2 = theta + (step / 2.0f);
    float p1 = phi - (step / 2.0f);
    float p2 = phi + (step / 2.0f);

    t1 *= deg2rad; t2 *= deg2rad;
    p1 *= deg2rad; p2 *= deg2rad;

    // 2. 반지름 설정
    // r1: 바닥 (Radius 40) - 묻혀있는 쪽
    // r2: 꼭대기 (Radius 37) - 플레이어 눈에 보이는 쪽
    float r1 = r;
    float r2 = r - h;

    // 3. 8개의 꼭짓점 좌표 계산 (구면 좌표계 공식)
    Point3D v[8];

    // [바닥면 4점] (벽의 뿌리 부분)
    v[0] = { r1 * sin(t1) * cos(p1), r1 * sin(p1), r1 * cos(t1) * cos(p1) }; // 좌하
    v[1] = { r1 * sin(t2) * cos(p1), r1 * sin(p1), r1 * cos(t2) * cos(p1) }; // 우하
    v[2] = { r1 * sin(t2) * cos(p2), r1 * sin(p2), r1 * cos(t2) * cos(p2) }; // 우상
    v[3] = { r1 * sin(t1) * cos(p2), r1 * sin(p2), r1 * cos(t1) * cos(p2) }; // 좌상

    // [윗면 4점] (벽의 천장 부분 - 플레이어가 주로 보는 면)
    v[4] = { r2 * sin(t1) * cos(p1), r2 * sin(p1), r2 * cos(t1) * cos(p1) }; // 좌하
    v[5] = { r2 * sin(t2) * cos(p1), r2 * sin(p1), r2 * cos(t2) * cos(p1) }; // 우하
    v[6] = { r2 * sin(t2) * cos(p2), r2 * sin(p2), r2 * cos(t2) * cos(p2) }; // 우상
    v[7] = { r2 * sin(t1) * cos(p2), r2 * sin(p2), r2 * cos(t1) * cos(p2) }; // 좌상

    // 4. 육면체 그리기 (각 면마다 법선 계산 필수!)
    glBegin(GL_QUADS);

    // [중요] 법선 벡터를 계산하기 위한 임시 변수
    Point3D n;

    // (1) 윗면 (Top) - 가장 밝게 보여야 함
    // 순서: v4 -> v5 -> v6 -> v7
    n = calculateNormal(v[4], v[5], v[6]); // 법선 자동 계산
    glNormal3f(n.x, n.y, n.z);             // 적용
    glVertex3f(v[4].x, v[4].y, v[4].z);
    glVertex3f(v[5].x, v[5].y, v[5].z);
    glVertex3f(v[6].x, v[6].y, v[6].z);
    glVertex3f(v[7].x, v[7].y, v[7].z);

    // (2) 앞면 (Front) - 위도선 방향
    // 순서: v0 -> v1 -> v5 -> v4
    n = calculateNormal(v[0], v[1], v[5]);
    glNormal3f(n.x, n.y, n.z);
    glVertex3f(v[0].x, v[0].y, v[0].z);
    glVertex3f(v[1].x, v[1].y, v[1].z);
    glVertex3f(v[5].x, v[5].y, v[5].z);
    glVertex3f(v[4].x, v[4].y, v[4].z);

    // (3) 뒷면 (Back)
    // 순서: v1 -> v2 -> v6 -> v5
    n = calculateNormal(v[1], v[2], v[6]);
    glNormal3f(n.x, n.y, n.z);
    glVertex3f(v[1].x, v[1].y, v[1].z);
    glVertex3f(v[2].x, v[2].y, v[2].z);
    glVertex3f(v[6].x, v[6].y, v[6].z);
    glVertex3f(v[5].x, v[5].y, v[5].z);

    // (4) 왼쪽면 (Left)
    // 순서: v3 -> v0 -> v4 -> v7
    n = calculateNormal(v[3], v[0], v[4]);
    glNormal3f(n.x, n.y, n.z);
    glVertex3f(v[3].x, v[3].y, v[3].z);
    glVertex3f(v[0].x, v[0].y, v[0].z);
    glVertex3f(v[4].x, v[4].y, v[4].z);
    glVertex3f(v[7].x, v[7].y, v[7].z);

    // (5) 오른쪽면 (Right)
    // 순서: v2 -> v1 -> v0 -> v3 (X) -> v2 -> v3 -> v7 -> v6
    n = calculateNormal(v[2], v[3], v[7]);
    glNormal3f(n.x, n.y, n.z);
    glVertex3f(v[2].x, v[2].y, v[2].z);
    glVertex3f(v[3].x, v[3].y, v[3].z);
    glVertex3f(v[7].x, v[7].y, v[7].z);
    glVertex3f(v[6].x, v[6].y, v[6].z);

    // (6) 바닥면 (Bottom) - 파묻혀서 안 보이지만 계산함
    // 순서: v3 -> v2 -> v1 -> v0
    n = calculateNormal(v[3], v[2], v[1]);
    glNormal3f(n.x, n.y, n.z);
    glVertex3f(v[3].x, v[3].y, v[3].z);
    glVertex3f(v[2].x, v[2].y, v[2].z);
    glVertex3f(v[1].x, v[1].y, v[1].z);
    glVertex3f(v[0].x, v[0].y, v[0].z);

    glEnd();
}

// ----------------------------------------------------------
// [장면 그리기 함수] - 핵심 렌더링 파트
// 구체, 벽, 모델 등 월드 전체를 그립니다.
// isWireMode: true면 구를 철사(Wireframe)로 그림 (절대 시점용)
// ----------------------------------------------------------
// ----------------------------------------------------------
// [수정된 장면 그리기] 색상 버그 완벽 수정
// ----------------------------------------------------------
void drawScene(bool isWireMode) {
    // 1. 양면 조명 설정
    glLightModelf(GL_LIGHT_MODEL_TWO_SIDE, 1.0);
    glDisable(GL_CULL_FACE);

    // 2. [바닥] 거대한 구 그리기
    if (isWireMode) {
        glDisable(GL_LIGHTING);
        glColor3f(0.3f, 0.3f, 0.3f);
        glutWireSphere(planetRadius, 30, 30);
        glEnable(GL_LIGHTING);
    }
    else {
        // [바닥 색상] 어두운 남색
        GLfloat wallColor[] = { 0.1f, 0.1f, 0.3f, 1.0f };

        // [중요 수정] 앞면/뒷면 모두, 그리고 주변광/분산광 모두 설정
        glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, wallColor);

        glutSolidSphere(planetRadius, 50, 50);
    }

    float angleStep = 6.0f;

    for (int r = 0; r < 20; r++) {
        for (int c = 0; c < 20; c++) {
            if (map[r][c] == 0) continue;

            // CASE A: 곡면 벽 (Curved Wall)
            if (map[r][c] == 1) {
                // [벽 색상] 형광 하늘색 (Cyan)
                GLfloat boxColor[] = { 0.0f, 1.0f, 1.0f, 1.0f };

                // [핵심 수정] 여기서도 GL_FRONT_AND_BACK을 써야 남색이 묻어나지 않음!
                glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, boxColor);

                float theta = (c - 10) * angleStep;
                float phi = (r - 10) * angleStep;

                drawCurvedWall(planetRadius, 3.0f, theta, phi, angleStep);
            }

            // CASE B: 도자기 모델 (Model)
            else if (map[r][c] == 9) {
                glPushMatrix();
                float theta = (c - 10) * angleStep;
                float phi = (r - 10) * angleStep;

                glRotatef(theta, 0.0f, 1.0f, 0.0f);
                glRotatef(phi, 1.0f, 0.0f, 0.0f);
                glTranslatef(0.0f, 0.0f, planetRadius);
                glTranslatef(0.0f, 0.0f, -2.0f);
                glRotatef(-90, 1.0f, 0.0f, 0.0f);
                glScalef(0.005f, 0.005f, 0.005f);

                // [모델 색상] 금색
                GLfloat goldDiff[] = { 1.0f, 0.8f, 0.0f, 1.0f };
                GLfloat goldSpec[] = { 1.0f, 1.0f, 1.0f, 1.0f };

                // 모델도 앞뒤 확실하게 설정
                glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, goldDiff);
                glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, goldSpec);
                glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 100.0f);

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
                glPopMatrix();
            }
        }
    }
}

// ----------------------------------------------------------
// [시야각 표시 함수] 절대 시점에서 내가 어디를 보는지 노란선으로 그림
// ----------------------------------------------------------
void drawViewFrustum() {
    glPushMatrix();
    // 1. 플레이어 눈 위치로 이동
    glTranslatef(0.0f, -planetRadius + playerHeight, 0.0f);

    // 2. 카메라 각도만큼 회전 (라디안 -> 도 변환 필요)
    // Yaw: 좌우 회전 (Y축)
    glRotatef(cameraYaw * 180.0f / M_PI, 0.0f, 1.0f, 0.0f);
    // Pitch: 상하 회전 (X축) - OpenGL 좌표계 특성상 부호 반대 주의
    glRotatef(-cameraPitch * 180.0f / M_PI, 1.0f, 0.0f, 0.0f);

    // 3. 노란색 선 그리기
    glDisable(GL_LIGHTING); // 빛나게 하기 위해 조명 끔
    glColor3f(1.0f, 1.0f, 0.0f); // 노란색
    glLineWidth(2.0f);

    float len = 15.0f; // 시야 거리 (레이저 길이)
    float w = 5.0f;    // 시야 폭

    glBegin(GL_LINES);
    // (1) 시선 정중앙 (레이저 포인터)
    glVertex3f(0, 0, 0);
    glVertex3f(0, 0, -len); // -Z 방향이 정면

    // (2) 시야각 테두리 (피라미드 모양)
    // 왼쪽 위
    glVertex3f(0, 0, 0); glVertex3f(-w, w, -len);
    // 오른쪽 위
    glVertex3f(0, 0, 0); glVertex3f(w, w, -len);
    // 왼쪽 아래
    glVertex3f(0, 0, 0); glVertex3f(-w, -w, -len);
    // 오른쪽 아래
    glVertex3f(0, 0, 0); glVertex3f(w, -w, -len);

    // (3) 끝부분을 네모로 연결 (화면 프레임 느낌)
    glVertex3f(-w, w, -len); glVertex3f(w, w, -len);
    glVertex3f(w, w, -len); glVertex3f(w, -w, -len);
    glVertex3f(w, -w, -len); glVertex3f(-w, -w, -len);
    glVertex3f(-w, -w, -len); glVertex3f(-w, w, -len);
    glEnd();

    glEnable(GL_LIGHTING); // 조명 다시 켜기
    glPopMatrix();
}

// ----------------------------------------------------------
// [화면 출력 함수] - 3분할 화면(Viewport) 구성
// ----------------------------------------------------------
void display() {
    // 화면과 깊이 버퍼(앞뒤 관계) 초기화
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // 조명 활성화
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    // ==========================================
    // [1] 왼쪽 화면: 1인칭 시점 (Main Game View)
    // ==========================================
    glViewport(0, 0, winW / 2, winH); // 화면의 왼쪽 절반 사용

    // 렌즈 설정 (원근감)
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60.0f, (float)(winW / 2) / winH, 0.1f, 1000.0f);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // 1인칭 카메라 위치 계산
    // 플레이어는 구 바닥(-Radius)에 있고, 시선(Look) 벡터를 더해 보는 곳을 정함
    float eyeY = -planetRadius + playerHeight;
    float lookX = sin(cameraYaw) * cos(cameraPitch);
    float lookY = sin(cameraPitch);
    float lookZ = -cos(cameraYaw) * cos(cameraPitch);

    // 카메라 설치
    gluLookAt(0, eyeY, 0,           // 눈 위치 (고정)
        lookX, eyeY + lookY, lookZ, // 보는 곳 (WASD로 변함)
        0, 1, 0);             // 윗 방향

    // [중요] 카메라 설치 후 조명 위치 재설정
    // 조명을 (0,0,0) 즉 구의 정중앙에 고정해야 '태양'처럼 작동함
    GLfloat lightPos[] = { 0.0f, 0.0f, 0.0f, 1.0f };
    glLightfv(GL_LIGHT0, GL_POSITION, lightPos);

    glPushMatrix();
    // **핵심**: 누적된 회전 행렬을 적용하여 월드 전체를 돌림
    // (나는 가만히 있고 지구가 도는 상대성 원리)
    glMultMatrixf(planetRotationMatrix);
    drawScene(false); // 꽉 찬 구 그리기
    glPopMatrix();


    // ==========================================
    // [2] 오른쪽 위: 네비게이션 뷰 (Mini-Map)
    // ==========================================
    glViewport(winW / 2, winH / 2, winW / 2, winH / 2); // 오른쪽 상단

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60.0f, (float)(winW / 2) / (winH / 2), 0.1f, 1000.0f);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    // 하늘에서 수직으로 내려다보는 카메라
    gluLookAt(0, 0, 0, 0, -1, 0, 0, 0, -1);

    // 여기도 조명 위치 고정
    glLightfv(GL_LIGHT0, GL_POSITION, lightPos);

    glPushMatrix();
    // **네비게이션 효과**: 내가 고개를 돌린 만큼(Yaw), 지도를 반대로 회전시킴
    // 이렇게 하면 화면의 위쪽이 항상 내가 가는 방향이 됨
    glRotatef(cameraYaw * 180.0f / M_PI, 0.0f, 1.0f, 0.0f);

    glMultMatrixf(planetRotationMatrix); // 공 굴리기 적용
    drawScene(false);
    glPopMatrix();

    // 플레이어 표시 (중앙에 빨간 점)
    glDisable(GL_LIGHTING); // 잘 보이게 조명 끔
    glPushMatrix();
    glTranslatef(0.0f, -planetRadius + 1.0f, 0.0f);
    glColor3f(1.0f, 0.0f, 0.0f); // 빨간색
    glutSolidCube(2.0f);
    glPopMatrix();
    glEnable(GL_LIGHTING);


    // ... (위쪽 코드는 동일) ...

        // ==========================================
        // [3] 오른쪽 아래: 절대 관찰자 시점 (Absolute View)
        // ==========================================
    glViewport(winW / 2, 0, winW / 2, winH / 2);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60.0f, (float)(winW / 2) / (winH / 2), 0.1f, 1000.0f);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(0, -80, 80, 0, 0, 0, 0, 1, 0);

    glLightfv(GL_LIGHT0, GL_POSITION, lightPos);

    glPushMatrix();
    glMultMatrixf(planetRotationMatrix);
    drawScene(true); // Wireframe World
    glPopMatrix();

    // 플레이어 표시
    glDisable(GL_LIGHTING);
    glPushMatrix();
    glTranslatef(0.0f, -planetRadius, 0.0f);
    glColor3f(1.0f, 0.0f, 1.0f);
    glutSolidCube(3.0f);
    glPopMatrix();

    // [추가됨] 여기서 시야각 노란색 원뿔을 그립니다!
    // 플레이어와 마찬가지로 월드 회전(Matrix) 바깥에 그려야 합니다.
    drawViewFrustum();

    glEnable(GL_LIGHTING);

    glutSwapBuffers();
}

// 창 크기가 바뀔 때 호출되는 함수
void reshape(int w, int h) {
    winW = w; winH = h;
}

// ----------------------------------------------------------
// [입력 처리 1] 일반 키보드 (WASD) - 시야 조절
// ----------------------------------------------------------
void keyboard(unsigned char key, int x, int y) {
    float rotSpeed = 0.05f;

    // 시야각 변경
    if (key == 'a' || key == 'A') cameraYaw -= rotSpeed; // 왼쪽 보기
    if (key == 'd' || key == 'D') cameraYaw += rotSpeed; // 오른쪽 보기
    if (key == 'w' || key == 'W') cameraPitch += rotSpeed; // 위 보기
    if (key == 's' || key == 'S') cameraPitch -= rotSpeed; // 아래 보기

    // 고개가 너무 뒤로 꺾이지 않게 제한 (상하 90도 안쪽)
    if (cameraPitch > 1.5f) cameraPitch = 1.5f;
    if (cameraPitch < -1.5f) cameraPitch = -1.5f;

    if (key == 27) exit(0); // ESC키 누르면 종료
    glutPostRedisplay();
}

// ----------------------------------------------------------
// [입력 처리 2] 특수 키보드 (화살표) - 이동
// **핵심 알고리즘**: 내가 보는 방향으로 공을 굴리는 로직
// ----------------------------------------------------------
void specialKeys(int key, int x, int y) {
    float moveSpeed = 1.5f;

    // 회전할 축(Axis)과 각도(Angle) 계산
    float axisX = 0.0f;
    float axisZ = 0.0f;
    float angle = 0.0f;

    // 현재 내가 보고 있는 방향(Yaw)의 사인, 코사인 값
    float sinYaw = sin(cameraYaw);
    float cosYaw = cos(cameraYaw);

    if (key == GLUT_KEY_UP) {
        // 전진: 나의 '오른쪽 벡터'를 축으로 공을 앞으로(-각도) 굴림
        axisX = cosYaw; axisZ = sinYaw; angle = -moveSpeed;
    }
    else if (key == GLUT_KEY_DOWN) {
        // 후진: 나의 '오른쪽 벡터'를 축으로 공을 뒤로(+각도) 굴림
        axisX = cosYaw; axisZ = sinYaw; angle = moveSpeed;
    }
    else if (key == GLUT_KEY_RIGHT) {
        // 우측 게걸음: 나의 '앞쪽 벡터'를 축으로 공을 오른쪽으로 굴림
        // (실제 바닥은 왼쪽으로 움직여야 함 -> 양수 회전?)
        // 테스트 결과: 오른쪽으로 가려면 바닥을 반대로 밀어야 함
        axisX = sinYaw; axisZ = -cosYaw; angle = moveSpeed;
    }
    else if (key == GLUT_KEY_LEFT) {
        // 좌측 게걸음
        axisX = sinYaw; axisZ = -cosYaw; angle = -moveSpeed;
    }

    // [행렬 누적]
    glPushMatrix();
    glLoadIdentity();
    // 1. 계산된 축과 각도로 새로운 회전 생성
    glRotatef(angle, axisX, 0.0f, axisZ);
    // 2. 기존의 공 회전 상태(행렬)를 곱함
    glMultMatrixf(planetRotationMatrix);
    // 3. 합쳐진 결과를 다시 전역 변수에 저장
    glGetFloatv(GL_MODELVIEW_MATRIX, planetRotationMatrix);
    glPopMatrix();

    glutPostRedisplay(); // 화면 다시 그리기 요청
}

// 메인 함수
int main(int argc, char** argv) {
    glutInit(&argc, argv);
    // 더블 버퍼링(깜빡임 방지), RGB 컬러, 깊이 버퍼(3D 앞뒤 구분) 사용
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(winW, winH);
    glutCreateWindow("Final Project: Inside Sphere + Triple View");

    glEnable(GL_DEPTH_TEST); // 깊이 테스트 켜기 (앞에 있는 물체가 뒤를 가림)
    glEnable(GL_NORMALIZE);  // 법선 벡터 정규화 (조명 계산 정확도 향상)

    initMap();               // 맵 생성
    loadModel("myModel.dat");// 모델 파일 불러오기

    // 콜백 함수 등록
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);   // 일반 키 (WASD)
    glutSpecialFunc(specialKeys); // 특수 키 (화살표)

    glutMainLoop(); // 무한 루프 시작
    return 0;
}