#include<bits/stdc++.h>
#include <GL/glut.h>
#include <cmath>
using namespace std;

int x_start, y_start;
int x_end, y_end;

void putPixel(int x, int y) {
    glBegin(GL_POINTS);
    glVertex2i(x, y);
    glEnd();
}

void drawBoldPixel(int x, int y) {
    putPixel(x, y);
    putPixel(x + 1, y);
    putPixel(x - 1, y);
    putPixel(x, y + 1);
    putPixel(x, y - 1);
    putPixel(x + 1, y + 1);
    putPixel(x - 1, y - 1);
    putPixel(x + 1, y - 1);
    putPixel(x - 1, y + 1);
}

void drawLine8Neighbour(int x1, int y1, int x2, int y2) {
    int dx = abs(x2 - x1);
    int dy = abs(y2 - y1);
    int sx = (x2 >= x1) ? 1 : -1;
    int sy = (y2 >= y1) ? 1 : -1;

    int err = dx - dy;

    while (true) {
        drawBoldPixel(x1, y1);
        if (x1 == x2 && y1 == y2) break;
        int e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x1 += sx;
        }
        if (e2 < dx) {
            err += dx;
            y1 += sy;
        }
    }
}

void display() {
    glClear(GL_COLOR_BUFFER_BIT);
    glColor3f(0.0, 1.0, 0.0);
    drawLine8Neighbour(x_start, y_start, x_end, y_end);
    glFlush();
}

void init() {
    glClearColor(1.0, 1.0, 1.0, 1.0);
    glColor3f(0.0, 0.0, 0.0);
    gluOrtho2D(0, 500, 0, 500);
    glPointSize(1.0);
}

int main(int argc, char** argv) {
    cout << "Enter start point (x1, y1): ";
    cin >> x_start >> y_start;

    cout << '\n';

    cout << "Enter End point (x2, y2): ";
    cin >> x_end >> y_end;

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);
    glutInitWindowSize(500, 500);
    glutInitWindowPosition(100, 100);
    glutCreateWindow("Bold Line using 8-Neighbourhood");
    init();
    glutDisplayFunc(display);
    glutMainLoop();
    return 0;
}
