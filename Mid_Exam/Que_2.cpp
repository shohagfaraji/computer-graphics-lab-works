#include <GL/glut.h>
#include <bits/stdc++.h>
using namespace std;

#define MAX_HISTORY 5

typedef struct {
    float rotationAngle;
    float scaleFactor;
    float translateX;
    float translateY;
} TransformState;

float rotationAngle = 0.0f;
float scaleFactor = 1.0f;
float translateX = 0.0f;
float translateY = 0.0f;

TransformState history[MAX_HISTORY];
int historyTop = 0;
int total_changes = 0;

void pushState() {
    historyTop = (historyTop + 1) % MAX_HISTORY;
    history[historyTop].rotationAngle = rotationAngle;
    history[historyTop].scaleFactor = scaleFactor;
    history[historyTop].translateX = translateX;
    history[historyTop].translateY = translateY;
}

void display() {
    glClear(GL_COLOR_BUFFER_BIT);
    glLoadIdentity();
    glTranslatef(translateX, translateY, 0.0f);
    glTranslatef(225, 175, 0.0f);
    glRotatef(rotationAngle, 0.0f, 0.0f, 1.0f);
    glScalef(scaleFactor, scaleFactor, 1.0f);
    glTranslatef(-225, -175, 0.0f);

    // Roof
    glColor3f(0.20f, 1.0f, 0.0f);
    glBegin(GL_TRIANGLES);
      glVertex2f(115.0f, 200.0f); // L
      glVertex2f(310.0f, 200.0f); // R
      glVertex2f(212.5f, 300.0f); // C
    glEnd();

    // Basement
    glBegin(GL_QUADS);
      glColor3f(0.0, 0.20, 0.80);
      glVertex2i(125, 100); // DL
      glVertex2i(300, 100); // DR
      glVertex2i(300, 200); // UR
      glVertex2i(125, 200); // UL
    glEnd();

    // Door
    glBegin(GL_QUADS);
      glColor3f(1.0, 1.0, 0.0);
      glVertex2i(200, 105); // DL
      glVertex2i(224, 105); // DR
      glVertex2i(224, 140); // UR
      glVertex2i(200, 140); // UL
    glEnd();

    glFlush();
}

void menu(int option) {
    total_changes++;
    cout << "\nTotal Changes: " << total_changes;

    switch (option) {
        case 1:
            cout << " (Reset)\n";
            rotationAngle = 0.0f;
            scaleFactor = 1.0f;
            translateX = 0.0f;
            translateY = 0.0f;
            historyTop = 0;
            history[0].rotationAngle = rotationAngle;
            history[0].scaleFactor = scaleFactor;
            history[0].translateX = translateX;
            history[0].translateY = translateY;
            break;
        case 2:
            cout << " (Rotate Clokwise)\n";
            rotationAngle += -90.0f;
            pushState();
            break;
        case 3:
            cout << " (Rotate Anti-Clokwise)\n";
            rotationAngle += 90.0f;
            pushState();
            break;
        case 4:
            cout << " (Half)\n";
            scaleFactor *= 0.5f;
            pushState();
            break;
        case 5:
            cout << " (Translate)\n";
            translateX += 100.0f;
            translateY += 100.0f;
            pushState();
            break;
        case 6:
            cout << " (double size)\n";
            scaleFactor *= 2.0f;
            pushState();
            break;
        case 7:
            cout << " (undo)\n";
            if (historyTop > 0) {
                historyTop--;
                rotationAngle = history[historyTop].rotationAngle;
                scaleFactor = history[historyTop].scaleFactor;
                translateX = history[historyTop].translateX;
                translateY = history[historyTop].translateY;
            } else {
                historyTop = MAX_HISTORY - 1;
                rotationAngle = history[historyTop].rotationAngle;
                scaleFactor = history[historyTop].scaleFactor;
                translateX = history[historyTop].translateX;
                translateY = history[historyTop].translateY;
            }
            break;
        case 8:
            cout << " (redo)\n";
            historyTop = (historyTop + 1) % MAX_HISTORY;
            rotationAngle = history[historyTop].rotationAngle;
            scaleFactor = history[historyTop].scaleFactor;
            translateX = history[historyTop].translateX;
            translateY = history[historyTop].translateY;
            break;
    }

    cout << "history saved at index: " << historyTop << '\n';
    glutPostRedisplay();
}

void init() {
    glClearColor(0.0, 0.0, 0.0, 1.0);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0.0, 800.0, 0.0, 800.0);
    glMatrixMode(GL_MODELVIEW);
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);
    glutInitWindowSize(800, 800);
    glutInitWindowPosition(100, 100);
    glutCreateWindow("Transformations Demo");

    init();

    history[0].rotationAngle = rotationAngle;
    history[0].scaleFactor = scaleFactor;
    history[0].translateX = translateX;
    history[0].translateY = translateY;
    historyTop = 0;

    glutDisplayFunc(display);

    glutCreateMenu(menu);
    glutAddMenuEntry("Reset", 1);
    glutAddMenuEntry("Rotate 90 degrees colckwise", 2);
    glutAddMenuEntry("Rotate 90 degrees anti-colckwise", 3);
    glutAddMenuEntry("Scale x0.5", 4);
    glutAddMenuEntry("Scale x2.0", 6);
    glutAddMenuEntry("Translate to center", 5);
    glutAddMenuEntry("Undo", 7);
    glutAddMenuEntry("Redo", 8);
    glutAttachMenu(GLUT_RIGHT_BUTTON);

    glutMainLoop();

    return 0;
}
