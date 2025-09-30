#include <GL/glut.h>
#include <bits/stdc++.h>
#define MAX_HISTORY 5
using namespace std;

typedef struct {
    float rotationAngle;
    float scaleFactor;
    float translateX;
    float translateY;
} TransformState;

// Transformation state variables
float rotationAngle = 0.0f;
float scaleFactor = 1.0f;
float translateX = 0.0f;
float translateY = 0.0f;

// History stack for undo/redo
TransformState history[MAX_HISTORY];
int historyTop = 0;
int total_changes = 0;

// Push current state into history
void pushState() {
    historyTop = (historyTop + 1) % MAX_HISTORY;
    history[historyTop].rotationAngle = rotationAngle;
    history[historyTop].scaleFactor = scaleFactor;
    history[historyTop].translateX = translateX;
    history[historyTop].translateY = translateY;
}

void display() {
    glClear(GL_COLOR_BUFFER_BIT);
    // Reset transformations
    glLoadIdentity();
    // Apply transformations in order: translate, rotate, scale
    glTranslatef(translateX, translateY, 0.0f);
    glTranslatef(225, 175, 0.0f);
    glRotatef(rotationAngle, 0.0f, 0.0f, 1.0f);
    glScalef(scaleFactor, scaleFactor, 1.0f);
    glTranslatef(-225, -175, 0.0f);

    // Set color and draw polygon (triangle)
    glColor3f(1.0f, 0.0f, 1.0f); // Purple
    glBegin(GL_TRIANGLES);
    glVertex2f(150.0f, 150.0f);
    glVertex2f(300.0f, 150.0f);
    glVertex2f(225.0f, 225.0f);
    glEnd();
    glFlush();
}

void menu(int option) {
    total_changes++;
    cout << "\nTotal Changes: " << total_changes;

    switch (option) {
    case 1: // Reset transformations
        cout << " (Reset)\n";
        rotationAngle = 0.0f;
        scaleFactor = 1.0f;
        translateX = 0.0f;
        translateY = 0.0f;
        // Reset history
        historyTop = 0;
        history[0].rotationAngle = rotationAngle;
        history[0].scaleFactor = scaleFactor;
        history[0].translateX = translateX;
        history[0].translateY = translateY;
        break;

    case 2: // Rotate 90 degrees
        cout << " (Rotate Clokwise)\n";
        rotationAngle -= 90.0f;
        pushState();
        break;

    case 3: // Rotate 90 degrees
        cout << " (Rotate Anti-Clokwise)\n";
        rotationAngle += 90.0f;
        pushState();
        break;

    case 4: // Scale up by 0.5x
        cout << " (Half)\n";
        scaleFactor *= 0.5f;
        pushState();
        break;

    case 5: // Translate by (180, 220)
        cout << " (Translate)\n";
        translateX += 180.0f;
        translateY += 220.0f;
        pushState();
        break;

    case 6: // double the size
        cout << " (double size)\n";
        scaleFactor *= 2.0f;
        pushState();
        break;

    case 7: // Undo
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

    case 8: // redo
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
    glClearColor(0.0, 0.0, 0.0, 1.0); // Black background
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0.0, 800.0, 0.0, 800.0);
    glMatrixMode(GL_MODELVIEW);
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);
    glutInitWindowSize(800, 650);
    glutInitWindowPosition(100, 100);
    glutCreateWindow("Transformations Demo");
    init();

    // Initialize history stack with initial state
    history[0].rotationAngle = rotationAngle;
    history[0].scaleFactor = scaleFactor;
    history[0].translateX = translateX;
    history[0].translateY = translateY;
    historyTop = 0;
    glutDisplayFunc(display);

    // Create right-click menu
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
