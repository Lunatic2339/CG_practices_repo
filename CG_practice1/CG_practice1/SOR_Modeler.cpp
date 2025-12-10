// Visual Studio에서 scanf, fopen 사용 시 발생하는 보안 경고(C4996)를 무시합니다.
#define _CRT_SECURE_NO_WARNINGS 

#include <GL/glut.h>   // OpenGL 및 GLUT 라이브러리 (창 생성, 그리기, 입력 처리)
#include <vector>      // STL 벡터 (점들을 저장하는 가변 배열)
#include <cmath>       // 수학 함수 (sin, cos - 회전 계산용)
#include <iostream>    // 콘솔 출력 (cout)
#include <cstdio>      // 파일 입출력 (fopen, fprintf)

// 원주율(PI) 값 정의
#define M_PI 3.14159265358979323846

// ----------------------------------------------------------
// [데이터 구조 클래스]
// 3차원 공간의 점(Vertex) 하나를 표현하는 클래스
// ----------------------------------------------------------
class Point3D {
public:
    float x, y, z;
    Point3D() { x = 0; y = 0; z = 0; } // 기본 생성자
    Point3D(float _x, float _y, float _z) { // 값 초기화 생성자
        x = _x; y = _y; z = _z;
    }
};

// ----------------------------------------------------------
// [전역 변수]
// 프로그램 전체에서 공유하는 데이터
// ----------------------------------------------------------
std::vector<Point3D> inputPoints;        // 사용자가 2D 모드에서 찍은 점들의 리스트
std::vector<std::vector<Point3D>> mesh;  // 3D로 변환된 메쉬 데이터 (행: 층, 열: 회전각도)
std::vector<Point3D> flattenedVertices;  // 파일 저장을 위해 1차원으로 펼친 점 리스트

int winWidth = 800;   // 창 너비
int winHeight = 600;  // 창 높이
bool is3DMode = false; // 현재 모드 (false: 2D 그리기 모드, true: 3D 결과 보기 모드)

int rotationSteps = 4; // 회전 정밀도 (360도를 36등분 = 10도씩 회전하여 원을 만듦)

// ----------------------------------------------------------
// [모델 생성 함수] (핵심 알고리즘)
// 2D 점들을 Y축 기준으로 뺑 돌려서 3D 좌표를 계산하는 함수
// ----------------------------------------------------------
void generateSOR() {
    if (inputPoints.empty()) return; // 찍은 점이 없으면 아무것도 안 함

    mesh.clear();             // 기존 데이터 초기화
    flattenedVertices.clear();

    // 360도(2*PI)를 조각 개수로 나누어, 한 칸당 몇 라디안인지 계산
    float angleStep = (2.0f * M_PI) / (float)rotationSteps;

    // 1. 사용자가 찍은 점들을 하나씩 꺼냅니다. (층을 쌓는 과정)
    for (int i = 0; i < inputPoints.size(); i++) {
        std::vector<Point3D> row; // 한 층을 구성하는 점들의 리스트
        Point3D p = inputPoints[i];

        // [중요] 좌표 중심 이동
        // 화면 좌표(0~800)를 화면 중앙(0) 기준인 좌표(-400~+400)로 변환합니다.
        // 이렇게 해야 화면 가운데를 축으로 회전합니다.
        float rawX = p.x - (winWidth / 2);
        float rawY = p.y - (winHeight / 2);

        // 2. 해당 점을 0도부터 360도까지 회전시킵니다. (원을 만드는 과정)
        for (int j = 0; j <= rotationSteps; j++) {
            float theta = j * angleStep; // 현재 각도

            // [회전 변환 공식] (Y축 기준 회전)
            // 반경(r)은 rawX가 됩니다.
            float x_new = rawX * cos(theta);
            float y_new = rawY;              // 높이(Y)는 변하지 않음
            float z_new = -rawX * sin(theta);

            Point3D newPt(x_new, y_new, z_new);
            row.push_back(newPt);            // 화면 그리기용(2차원 배열) 저장
            flattenedVertices.push_back(newPt); // 파일 저장용(1차원 배열) 저장
        }
        mesh.push_back(row); // 한 층(원) 완성
    }
    std::cout << "3D Created! (Space: Mode Change, S: Save)" << std::endl;
}



// ----------------------------------------------------------
// [파일 저장 함수]
// 만들어진 모델을 .dat 파일로 내보냅니다.
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

    // 1. 점(VERTEX) 개수와 좌표 저장
    int pnum = flattenedVertices.size();
    fprintf(fout, "VERTEX = %d\n", pnum);
    for (int i = 0; i < pnum; i++) {
        fprintf(fout, "%.1f %.1f %.1f\n", flattenedVertices[i].x, flattenedVertices[i].y, flattenedVertices[i].z);
    }

    // 2. 면(FACE) 개수와 인덱스 저장
    // 점들을 연결해 삼각형을 만드는 과정
    std::vector<int> faces;
    int cols = rotationSteps + 1; // 한 바퀴 도는 데 필요한 점의 개수

    // 격자 한 칸(사각형)마다 삼각형 2개를 만듦
    for (int i = 0; i < mesh.size() - 1; i++) { // 층 루프
        for (int j = 0; j < rotationSteps; j++) { // 각도 루프
            int p1 = i * cols + j;           // 현재 점
            int p2 = i * cols + (j + 1);     // 오른쪽 점
            int p3 = (i + 1) * cols + (j + 1); // 아래 오른쪽 점
            int p4 = (i + 1) * cols + j;     // 아래 점

            // 삼각형 1 (p1-p2-p3)
            faces.push_back(p1); faces.push_back(p2); faces.push_back(p3);
            // 삼각형 2 (p1-p3-p4)
            faces.push_back(p1); faces.push_back(p3); faces.push_back(p4);
        }
    }

    int fnum = faces.size() / 3; // 삼각형 개수
    fprintf(fout, "FACE = %d\n", fnum);
    for (int i = 0; i < fnum; i++) {
        fprintf(fout, "%d %d %d\n", faces[i * 3], faces[i * 3 + 1], faces[i * 3 + 2]);
    }

    fclose(fout);
    std::cout << "Save Complete: myModel.dat" << std::endl;
}

// ----------------------------------------------------------
// [화면 그리기 함수]
// OpenGL이 화면을 그릴 때마다 호출하는 함수
// ----------------------------------------------------------
void display() {
    // 화면을 검은색으로 지우고, 깊이 버퍼 초기화
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glLoadIdentity(); // 좌표계 초기화

    if (!is3DMode) {
        // [2D 모드] 평면 그리기
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        gluOrtho2D(0, winWidth, 0, winHeight); // 원근감 없는 2D 좌표계 설정

        // 1. 중앙 회전축 그리기 (회색 선)
        glColor3f(0.5f, 0.5f, 0.5f);
        glBegin(GL_LINES);
        glVertex2f(winWidth / 2, 0); glVertex2f(winWidth / 2, winHeight);
        glEnd();

        // 2. 찍은 점들 그리기 (흰색 점)
        glColor3f(1.0f, 1.0f, 1.0f);
        glPointSize(5.0f);
        glBegin(GL_POINTS);
        for (int i = 0; i < inputPoints.size(); i++) glVertex2f(inputPoints[i].x, inputPoints[i].y);
        glEnd();

        // 3. 점들을 잇는 선 그리기
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

        // [중요] 선과 면이 겹칠 때 깜빡거리는 현상(Z-Fighting) 방지
        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(1.0f, 1.0f);

        for (int i = 0; i < mesh.size() - 1; i++) {
            for (int j = 0; j < mesh[i].size() - 1; j++) {
                
                // 1단계: 검은색 면 그리기 (뒤쪽 선을 가리기 위함)
                glColor3f(0.0f, 0.0f, 0.0f); // 검은색 (배경색과 동일)
                glBegin(GL_QUADS);
                glVertex3f(mesh[i][j].x, mesh[i][j].y, mesh[i][j].z);
                glVertex3f(mesh[i][j + 1].x, mesh[i][j + 1].y, mesh[i][j + 1].z);
                glVertex3f(mesh[i + 1][j + 1].x, mesh[i + 1][j + 1].y, mesh[i + 1][j + 1].z);
                glVertex3f(mesh[i + 1][j].x, mesh[i + 1][j].y, mesh[i + 1][j].z);
                glEnd();

                // 2단계: 녹색 선 그리기 (테두리)
                glColor3f(0.0f, 1.0f, 0.0f); // 녹색
                glBegin(GL_LINE_LOOP);
                glVertex3f(mesh[i][j].x, mesh[i][j].y, mesh[i][j].z);
                glVertex3f(mesh[i][j + 1].x, mesh[i][j + 1].y, mesh[i][j + 1].z);
                glVertex3f(mesh[i + 1][j + 1].x, mesh[i + 1][j + 1].y, mesh[i + 1][j + 1].z);
                glVertex3f(mesh[i + 1][j].x, mesh[i + 1][j].y, mesh[i + 1][j].z);
                glEnd();
            }
        }
        glDisable(GL_POLYGON_OFFSET_FILL);
    }
    glutSwapBuffers(); // 그린 화면을 모니터에 출력
}

// ----------------------------------------------------------
// [키보드 입력 처리]
// ----------------------------------------------------------
void keyboard(unsigned char key, int x, int y) {
    if (key == ' ') { // 스페이스바
        is3DMode = !is3DMode; // 모드 전환 (2D <-> 3D)
        if (is3DMode) generateSOR(); // 3D로 갈 때 모델 생성
        else inputPoints.clear();    // 2D로 돌아오면 점 초기화
        glutPostRedisplay();         // 화면 다시 그리기
    }
    else if (key == 's' || key == 'S') { // S키
        if (is3DMode) saveModel(); // 3D 상태일 때만 저장
    }
    else if (key == 27) exit(0); // ESC키: 프로그램 종료
}

// ----------------------------------------------------------
// [마우스 클릭 처리]
// ----------------------------------------------------------
void mouse(int button, int state, int x, int y) {
    // 2D 모드이고, 왼쪽 버튼을 눌렀을 때만 실행
    if (!is3DMode && button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
        float inputX = (float)x;
        // [중요] 좌표계 변환: 윈도우(왼쪽 위 0,0) -> OpenGL(왼쪽 아래 0,0)
        float inputY = (float)(winHeight - y);

        Point3D pt(inputX, inputY, 0);
        inputPoints.push_back(pt); // 리스트에 점 추가
        glutPostRedisplay();       // 점 찍었으니 화면 다시 그리기
    }
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH); // 더블버퍼, RGB, 깊이 사용
    glutInitWindowSize(winWidth, winHeight); // 창 크기 설정
    glutCreateWindow("SOR Modeler");         // 창 생성

    glEnable(GL_DEPTH_TEST); // 3D 깊이 테스팅 켜기 (앞뒤 구분)

    // 콜백 함수 등록 (어떤 일이 생기면 이 함수를 불러라)
    glutDisplayFunc(display);
    glutMouseFunc(mouse);
    glutKeyboardFunc(keyboard);

    // glutSpecialFunc는 삭제되었습니다 (방향키 사용 안 함)

    glutMainLoop(); // 무한 루프 시작
    return 0;
}