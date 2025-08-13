#include <iostream>
#include <GL/glut.h>
using namespace std;

int radius;
int centerX, centerY;

void plot(int x, int y, int count) {
  if (count % 5 == 0) {
    glBegin(GL_POINTS);
    glVertex2i(x + centerX, y + centerY);
    glEnd();
  }
}

void myInit(void) {
  glClearColor(1.0, 1.0, 1.0, 0.0);
  glColor3f(0.0f, 0.0f, 0.0f);
  glPointSize(2.0);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluOrtho2D(0.0, 640.0, 0.0, 480.0);
}

void midPointCircleAlgo(int r) {
  int x = 0;
  int y = r;
  float decisionParameter = 1 - r;
  int count = 0;
  while (y >= x) {
    plot(x, y, count);
    plot(y, x, count);
    plot(-x, y, count);
    plot(-y, x, count);
    plot(-x, -y, count);
    plot(-y, -x, count);
    plot(x, -y, count);
    plot(y, -x, count);
    if (decisionParameter < 0) {
      x++;
      decisionParameter += (x << 1) + 1;
    } else {
      x++, y--;
      decisionParameter += ((x - y) << 1) + 1;
    }
    count++;
  }
}

void myDisplay(void) {
  glClear(GL_COLOR_BUFFER_BIT);
  midPointCircleAlgo(radius);
  glFlush();
}

int main(int argc, char** argv) {
  cout << "Enter the coordinates of the circle's center (x y): ";
  cin >> centerX >> centerY;
  cout << "Enter the radius of the circle: ";
  cin >> radius;

  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);
  glutInitWindowSize(800, 600);
  glutInitWindowPosition(100, 150);
  glutCreateWindow("Midpoint Dotted Circle");
  myInit();
  glutDisplayFunc(myDisplay);
  glutMainLoop();

  return 0;
}
