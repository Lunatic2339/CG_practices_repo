#define _CRT_SECURE_NO_WARNINGS
#include <GL/glut.h>
#include <vector>
#include <cmath>
#include <cstdio>
#include <iostream>

#define M_PI 3.14159265358979323846

// ----------------------------------------------------------
// 데이터 구조 & 전역 변수
// ----------------------------------------------------------
struct Point3D { float x, y, z; };
struct Face { int v1, v2, v3; };

std::vector<Point3D> vertices;
std::vector<Face> faces;

// 플레이어 설정
float playerX = 1.5f;
float playerZ = 1.5f;
float playerAngle = 0.0f; // 좌우 회전 (Yaw)
float playerPitch = 0.0f; // 위아래 시야 (Pitch) - 추가된 변수

// 미로 맵 데이터 (1:벽, 0:길, 9:모델 위치)
int map[10][10] = {
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    {1, 0, 0, 0, 1, 0, 0, 0, 0, 1},
    {1, 0, 1, 0, 1, 0, 1, 1, 0, 1},
    {1, 0, 1, 0, 0, 0, 1, 9, 0, 1},
    {1, 0, 1, 1, 1, 1, 1, 1, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 1, 1, 1, 0, 1, 1, 1, 1},
    {1, 0, 1, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 1, 1, 0, 1, 0, 1},
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1}
};

// ----------------------------------------------------------
// 모델 로딩
// ----------------------------------------------------------
void loadModel(const char* filename) {
    FILE* fp = fopen(filename, "r");
    if (!fp) {
        std::cout << "ERROR: myModel.dat 파일이 없습니다!" << std::endl;
        return;
    }
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

// ----------------------------------------------------------
// 미로 및 모델 그리기
// ----------------------------------------------------------
void drawMazeAndModel() {
    // 바닥
    glColor3f(0.3f, 0.3f, 0.3f);
    glBegin(GL_QUADS);
    glVertex3f(0, 0, 0); glVertex3f(0, 0, 10);
    glVertex3f(10, 0, 10); glVertex3f(10, 0, 0);
    glEnd();

    // 벽과 모델
    for (int z = 0; z < 10; z++) {
        for (int x = 0; x < 10; x++) {
            glPushMatrix();
            glTranslatef((float)x + 0.5f, 0.0f, (float)z + 0.5f);

            if (map[z][x] == 1) { // 벽
                glColor3f(0.6f, 0.4f, 0.2f);
                glTranslatef(0.0f, 0.5f, 0.0f);
                glutSolidCube(1.0f);
            }
            else if (map[z][x] == 9) { // 모델
                glScalef(0.003f, 0.003f, 0.003f); // 크기 조절 필요시 수정
                glColor3f(1.0f, 0.8f, 0.0f);
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
            }
            glPopMatrix();
        }
    }
}

// ----------------------------------------------------------
// 화면 출력
// ----------------------------------------------------------
void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    // [카메라 시야 계산]
    // lookX, lookZ: 좌우 회전 반영
    // lookY: 위아래(Pitch) 회전 반영
    float lookX = playerX + cos(playerAngle);
    float lookY = 0.5f + tan(playerPitch); // 눈높이(0.5)에서 위아래 각도 추가
    float lookZ = playerZ + sin(playerAngle);

    gluLookAt(playerX, 0.5f, playerZ, lookX, lookY, lookZ, 0.0f, 1.0f, 0.0f);

    drawMazeAndModel();

    glutSwapBuffers();
}

// ----------------------------------------------------------
// [WASD 키] 시야 조절 (고개 돌리기)
// ----------------------------------------------------------
void keyboard(unsigned char key, int x, int y) {
    float rotSpeed = 0.1f;

    if (key == 'a' || key == 'A') playerAngle -= rotSpeed; // 왼쪽 보기
    if (key == 'd' || key == 'D') playerAngle += rotSpeed; // 오른쪽 보기
    if (key == 'w' || key == 'W') playerPitch += rotSpeed; // 위 보기
    if (key == 's' || key == 'S') playerPitch -= rotSpeed; // 아래 보기

    // 고개가 너무 뒤로 꺾이지 않게 제한
    if (playerPitch > 1.5f) playerPitch = 1.5f;
    if (playerPitch < -1.5f) playerPitch = -1.5f;

    if (key == 27) exit(0); // ESC 종료

    glutPostRedisplay();
}

// ----------------------------------------------------------
// [방향키] 4방향 이동 (Move & Strafe)
// ----------------------------------------------------------
void specialKeys(int key, int x, int y) {
    float moveSpeed = 0.1f;
    float dx = 0.0f;
    float dz = 0.0f;

    // 현재 바라보는 방향(Angle)을 기준으로 이동량 계산
    if (key == GLUT_KEY_UP) { // 전진
        dx = cos(playerAngle) * moveSpeed;
        dz = sin(playerAngle) * moveSpeed;
    }
    else if (key == GLUT_KEY_DOWN) { // 후진
        dx = -cos(playerAngle) * moveSpeed;
        dz = -sin(playerAngle) * moveSpeed;
    }
    else if (key == GLUT_KEY_LEFT) { // 왼쪽으로 게걸음 (Strafe Left)
        // 현재 각도에서 90도 왼쪽(-90도)으로 이동
        dx = sin(playerAngle) * moveSpeed;
        dz = -cos(playerAngle) * moveSpeed;
    }
    else if (key == GLUT_KEY_RIGHT) { // 오른쪽으로 게걸음 (Strafe Right)
        // 현재 각도에서 90도 오른쪽(+90도)으로 이동
        dx = -sin(playerAngle) * moveSpeed;
        dz = cos(playerAngle) * moveSpeed;
    }

    // 충돌 체크 및 이동
    float nextX = playerX + dx;
    float nextZ = playerZ + dz;

    // 맵 밖으로 안 나가고, 벽(1)이 아니면 이동 허용
    if (nextX >= 0 && nextX < 10 && nextZ >= 0 && nextZ < 10) {
        if (map[(int)nextZ][(int)nextX] != 1) {
            playerX = nextX;
            playerZ = nextZ;
        }
    }

    glutPostRedisplay();
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(800, 600);
    glutCreateWindow("Maze Explorer (Arrows: Move, WASD: Look)");

    glEnable(GL_DEPTH_TEST);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60.0f, 800.0f / 600.0f, 0.1f, 100.0f);
    glMatrixMode(GL_MODELVIEW);

    loadModel("myModel.dat");

    glutDisplayFunc(display);
    glutKeyboardFunc(keyboard);   // WASD 등록
    glutSpecialFunc(specialKeys); // 방향키 등록

    glutMainLoop();
    return 0;
}