#include <iostream>
#include <GL/glut.h>
using namespace std;

int noOfCir;
int pntX1 = 400, pntY1 = 300;

float colors[7][3] = {
    {1.0f, 0.0f, 0.0f},
    {1.0f, 0.5f, 0.0f},
    {1.0f, 1.0f, 0.0f},
    {0.0f, 1.0f, 0.0f},
    {0.0f, 0.0f, 1.0f},
    {0.29f, 0.0f, 0.51f},
    {0.93f, 0.51f, 0.93f}
};

void setColor(int index) {
    int colorIndex = index % 7;
    glColor3f(colors[colorIndex][0], colors[colorIndex][1], colors[colorIndex][2]);
}

void plot(int x, int y) {
    glBegin(GL_POINTS);
    glVertex2i(x + pntX1, y + pntY1);
    glEnd();
}

void myInit(void) {
    glClearColor(1.0, 1.0, 1.0, 0.0);
    glPointSize(5.0);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0.0, 800.0, 0.0, 600.0);
}

void midPointCircleAlgo(int r, int colorIndex) {
    int x = 0;
    int y = r;
    float decision = 1.25 - r;

    setColor(colorIndex);

    plot(x, y);

    while (y > x) {
        if (decision < 0) {
            x++;
            decision += (x << 1) + 1;
        } else {
            x++, y--;
            decision += ((x - y) << 1) + 1;
        }

        plot(x, y);
        plot(x, -y);
        plot(-x, y);
        plot(-x, -y);
        plot(y, x);
        plot(-y, x);
        plot(y, -x);
        plot(-y, -x);
    }
}

void myDisplay(void) {
    glClear(GL_COLOR_BUFFER_BIT);
    glPointSize(1.0);

    int radius = 30;
    for (int i = 0; i < noOfCir; i++) {
        midPointCircleAlgo(radius, i);
        radius += 30;
    }

    glFlush();
}

int main(int argc, char** argv) {
    cout << "Enter the number of circles you need: ";
    cin >> noOfCir;

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);
    glutInitWindowSize(800, 600);
    glutInitWindowPosition(100, 150);
    glutCreateWindow("Midpoint Circle Drawing Algorithm with Rainbow Colors");

    glutDisplayFunc(myDisplay);
    myInit();
    glutMainLoop();

    return 0;
}
