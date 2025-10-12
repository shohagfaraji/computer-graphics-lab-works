#include <GL/glut.h>
#include <bits/stdc++.h>

using namespace std;

#define WINDOW_SIZE 700

float FILL_COLOR[3] = {1.0, 0.0, 1.0};
float BACKGROUND_COLOR[3] = {1.0, 1.0, 1.0};

bool visited[WINDOW_SIZE][WINDOW_SIZE];

void getPixelColor(int x, int y, float color[3]) {
    glReadPixels(x, y, 1, 1, GL_RGB, GL_FLOAT, color);
}

void setPixelColor(int x, int y, float color[3]) {
    glColor3f(color[0], color[1], color[2]);
    glBegin(GL_POINTS);
        glVertex2i(x, y);
    glEnd();
    glFlush();
}

int isSameColor(float c1[3], float c2[3]) {
    float epsilon = 0.001;
    if (fabs(c1[0] - c2[0]) > epsilon) return 0;
    if (fabs(c1[1] - c2[1]) > epsilon) return 0;
    if (fabs(c1[2] - c2[2]) > epsilon) return 0;
    return 1;
}

void floodFill4N(int x, int y, float fillColor[3], float oldColor[3]) {
    if (x < 0 || x >= WINDOW_SIZE || y < 0 || y >= WINDOW_SIZE) {
        return;
    }

    if (visited[x][y]) {
        return;
    }

    float currentColor[3];
    getPixelColor(x, y, currentColor);

    if (!isSameColor(currentColor, oldColor)) {
        return;
    }

    setPixelColor(x, y, fillColor);
    visited[x][y] = true;

    floodFill4N(x + 1, y, fillColor, oldColor);
    floodFill4N(x - 1, y, fillColor, oldColor);
    floodFill4N(x, y + 1, fillColor, oldColor);
    floodFill4N(x, y - 1, fillColor, oldColor);
}

void mouse(int button, int state, int x, int y) {
    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
        int click_y = WINDOW_SIZE - 1 - y;

        float clickedColor[3];
        getPixelColor(x, click_y, clickedColor);

        if (isSameColor(clickedColor, BACKGROUND_COLOR)) {
            cout << "Starting 4-Neighbor Flood Fill at (" << x << ", " << click_y << ")..." << endl;

            for (int i = 0; i < WINDOW_SIZE; i++) {
                for (int j = 0; j < WINDOW_SIZE; j++) {
                    visited[i][j] = false;
                }
            }

            floodFill4N(x, click_y, FILL_COLOR, BACKGROUND_COLOR);
            glutPostRedisplay();
        } else {
            cout << "Clicked on a colored pixel (Boundary). Fill not started." << endl;
        }
    }
}

void initOpenGL() {
    glClearColor(BACKGROUND_COLOR[0], BACKGROUND_COLOR[1], BACKGROUND_COLOR[2], 0.0);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0.0, (double)WINDOW_SIZE, 0.0, (double)WINDOW_SIZE);
}

void drawCircleMidpoint(int cx, int cy, int r) {
    int x = 0;
    int y = r;
    int d = 1 - r;

    glColor3f(0.0, 0.0, 1.0);
    glBegin(GL_POINTS);

    while (x <= y) {
        glVertex2i(cx + x, cy + y);
        glVertex2i(cx + y, cy + x);
        glVertex2i(cx - x, cy + y);
        glVertex2i(cx - y, cy + x);
        glVertex2i(cx + x, cy - y);
        glVertex2i(cx + y, cy - x);
        glVertex2i(cx - x, cy - y);
        glVertex2i(cx - y, cy - x);

        if (d < 0) {
            d += 2 * x + 3;
        } else {
            d += 2 * (x - y) + 5;
            y--;
        }
        x++;
    }

    glEnd();
}

void display() {
    glClear(GL_COLOR_BUFFER_BIT);

    glColor3f(1.0, 0.0, 0.0);
    glBegin(GL_LINE_LOOP);
        glVertex2i(180, 400);
        glVertex2i(330, 400);
        glVertex2i(330, 250);
        glVertex2i(180, 250);
    glEnd();

    glColor3f(0.0, 1.0, 0.0);
    glBegin(GL_LINE_LOOP);
        glVertex2i(445, 400);
        glVertex2i(370, 250);
        glVertex2i(520, 250);
    glEnd();

    drawCircleMidpoint(380, 480, 70);

    glFlush();
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);
    glutInitWindowSize(WINDOW_SIZE, WINDOW_SIZE);
    glutCreateWindow("Flood Fill with Visited Tracking (4-Neighbor)");

    initOpenGL();
    glutDisplayFunc(display);
    glutMouseFunc(mouse);

    cout << "Click inside a shape to fill it (Fill Color: Magenta)." << endl;

    glutMainLoop();

    return 0;
}
