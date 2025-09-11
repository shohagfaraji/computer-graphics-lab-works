#include <GL/glut.h>
#include <cmath>
#include <bits/stdc++.h>
using namespace std;

const int WIDTH = 1000;
const int HEIGHT = 750;

const float GRAVITY = 0.5f;
const float PUSH_FORCE = 2.0f;

const int radii[] = {15, 20, 25, 18, 22, 28, 16};
const int array_size = sizeof(radii) / sizeof(radii[0]);

float colors[31][3] = {
    {0.9f, 0.2f, 0.3f}, {0.2f, 0.9f, 0.4f}, {0.3f, 0.4f, 0.95f}, {0.95f, 0.85f, 0.3f},
    {0.7f, 0.4f, 0.5f}, {0.4f, 0.8f, 0.7f}, {0.5f, 0.3f, 0.7f}, {0.6f, 0.7f, 0.2f},
    {0.3f, 0.6f, 0.9f}, {0.85f, 0.5f, 0.2f}, {0.7f, 0.9f, 0.3f}, {0.2f, 0.5f, 0.4f},
    {0.6f, 0.2f, 0.3f}, {0.5f, 0.5f, 0.5f}, {0.3f, 0.3f, 0.6f}, {0.9f, 0.7f, 0.4f},
    {0.7f, 0.2f, 0.5f}, {0.4f, 0.6f, 0.8f}, {0.6f, 0.3f, 0.3f}, {0.2f, 0.7f, 0.6f},
    {0.5f, 0.4f, 0.2f}, {0.8f, 0.4f, 0.4f}, {0.6f, 0.8f, 0.5f}, {0.4f, 0.3f, 0.5f},
    {0.3f, 0.7f, 0.3f}, {0.7f, 0.3f, 0.6f}, {0.8f, 0.6f, 0.7f}, {0.6f, 0.5f, 0.9f},
    {0.9f, 0.6f, 0.3f}, {0.4f, 0.5f, 0.3f}, {0.5f, 0.8f, 0.6f}
};

int click_count = 0;

struct Circle {
    float x, y;
    float radius;
    float vy;
    float r, g, b;
    bool settled;

    Circle(float x_, float y_, float radius_, float r_, float g_, float b_)
        : x(x_), y(y_), radius(radius_), vy(0.0f),
          r(r_), g(g_), b(b_), settled(false) {}
};

vector<Circle> circles;

void drawCircle(float x, float y, float radius) {
    const int segments = 50;
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(x, y);
    for (int i = 0; i <= segments; ++i) {
        float angle = i * 2.0f * 3.14159f / segments;
        glVertex2f(x + cos(angle) * radius, y + sin(angle) * radius);
    }
    glEnd();
}

bool isColliding(Circle& a, Circle& b) {
    float dx = a.x - b.x;
    float dy = a.y - b.y;
    float dist = sqrt(dx * dx + dy * dy);
    return dist < (a.radius + b.radius);
}

void resolvePush(Circle& falling, Circle& other) {
    if (falling.radius <= other.radius || !other.settled)
        return;

    float dx = other.x - falling.x;
    float pushDir = (dx >= 0) ? 1.0f : -1.0f;

    other.x += pushDir * PUSH_FORCE;

    // Keep within screen bounds
    if (other.x - other.radius < 0)
        other.x = other.radius;
    else if (other.x + other.radius > WIDTH)
        other.x = WIDTH - other.radius;
}

void display() {
    glClear(GL_COLOR_BUFFER_BIT);
    for (auto& c : circles) {
        glColor3f(c.r, c.g, c.b);
        drawCircle(c.x, c.y, c.radius);
    }
    glutSwapBuffers();
}

void update(int) {
    for (auto& c : circles) {
        if (c.settled) continue;

        c.vy -= GRAVITY;
        float nextY = c.y + c.vy;
        bool settledThisFrame = false;

        for (auto& other : circles) {
            if (&c == &other || !other.settled) continue;

            float dx = c.x - other.x;
            float dy = nextY - other.y;
            float distance = sqrt(dx * dx + dy * dy);

            if (distance < (c.radius + other.radius)) {
                // Push logic
                resolvePush(c, other);

                // Place on top of the other circle
                c.y = other.y + other.radius + c.radius;
                c.vy = 0;
                c.settled = true;
                settledThisFrame = true;
                break;
            }
        }

        if (!settledThisFrame) {
            if (nextY - c.radius <= 0) {
                c.y = c.radius;
                c.vy = 0;
                c.settled = true;
            } else {
                c.y = nextY;
            }
        }
    }

    glutPostRedisplay();
    glutTimerFunc(16, update, 0);
}

void mouse(int button, int state, int x, int y) {
    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
        float fx = static_cast<float>(x);
        float fy = static_cast<float>(HEIGHT - y); // OpenGL y

        float radius = radii[click_count % array_size];
        int color_index = click_count % 31;
        float r = colors[color_index][0];
        float g = colors[color_index][1];
        float b = colors[color_index][2];

        circles.emplace_back(fx, fy, radius, r, g, b);
        click_count++;
    }
}

void init() {
    glClearColor(1.0, 1.0, 1.0, 1.0);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, WIDTH, 0, HEIGHT);
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(WIDTH, HEIGHT);
    glutCreateWindow("Falling Circles - Drop From Click Position");
    init();
    glutDisplayFunc(display);
    glutMouseFunc(mouse);
    glutTimerFunc(0, update, 0);
    glutMainLoop();
    return 0;
}
