#include <GL/glut.h>
#include <cmath>
#include <bits/stdc++.h>
using namespace std;

// Screen size
const int WIDTH = 800;
const int HEIGHT = 600;

// Gravity acceleration
const float GRAVITY = 0.5f;

// Radii array
const int radii[] = {15, 20, 25, 18, 22, 28, 16};
const int array_size = sizeof(radii) / sizeof(radii[0]);

int click_count = 0;

float colors[31][3] = {
    {0.9f, 0.2f, 0.3f},  {0.2f, 0.9f, 0.4f},  {0.3f, 0.4f, 0.95f},
    {0.95f, 0.85f, 0.3f}, {0.7f, 0.4f, 0.5f},  {0.4f, 0.8f, 0.7f},
    {0.5f, 0.3f, 0.7f},  {0.6f, 0.7f, 0.2f},  {0.3f, 0.6f, 0.9f},
    {0.85f, 0.5f, 0.2f}, {0.7f, 0.9f, 0.3f},  {0.2f, 0.5f, 0.4f},
    {0.6f, 0.2f, 0.3f},  {0.5f, 0.5f, 0.5f},  {0.3f, 0.3f, 0.6f},
    {0.9f, 0.7f, 0.4f},  {0.7f, 0.2f, 0.5f},  {0.4f, 0.6f, 0.8f},
    {0.6f, 0.3f, 0.3f},  {0.2f, 0.7f, 0.6f},  {0.5f, 0.4f, 0.2f},
    {0.8f, 0.4f, 0.4f},  {0.6f, 0.8f, 0.5f},  {0.4f, 0.3f, 0.5f},
    {0.3f, 0.7f, 0.3f},  {0.7f, 0.3f, 0.6f},  {0.8f, 0.6f, 0.7f},
    {0.6f, 0.5f, 0.9f},  {0.9f, 0.6f, 0.3f},  {0.4f, 0.5f, 0.3f},
    {0.5f, 0.8f, 0.6f}
};

// Circle structure
struct Circle {
    float x, y;
    float radius;
    float vy; // vertical speed
    float r, g, b;

    Circle(float x_, float y_, float r_, float red, float green, float blue)
        : x(x_), y(y_), radius(r_), vy(0.0f), r(red), g(green), b(blue) {}
};

vector<Circle> circles;

// Draw a filled circle
void drawCircle(float x, float y, float radius) {
    const int num_segments = 50;
    float angle;

    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(x, y);
    for (int i = 0; i <= num_segments; i++) {
        angle = i * 2.0f * 3.141592f / num_segments;
        glVertex2f(x + cos(angle) * radius, y + sin(angle) * radius);
    }
    glEnd();
}

// Display callback
void display() {
    glClear(GL_COLOR_BUFFER_BIT);

    for (const auto& c : circles) {
        glColor3f(c.r, c.g, c.b);
        drawCircle(c.x, c.y, c.radius);
    }

    glutSwapBuffers();
}

// Update animation (gravity)
void update(int value) {
    for (auto& c : circles) {
        c.vy -= GRAVITY;
        c.y += c.vy;

        // Stop at bottom
        if (c.y - c.radius < 0) {
            c.y = c.radius;
            c.vy = 0;
        }
    }

    glutPostRedisplay();
    glutTimerFunc(16, update, 0); // ~60 FPS
}

void mouse(int button, int state, int x, int y) {
    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
        float worldX = static_cast<float>(x);
        float worldY = static_cast<float>(HEIGHT - y); // flip Y

        float radius = radii[click_count % array_size];
        int color_index = click_count % 31;

        float r = colors[color_index][0];
        float g = colors[color_index][1];
        float b = colors[color_index][2];

        circles.emplace_back(worldX, worldY, radius, r, g, b);
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
    glutCreateWindow("Falling Colored Circles - 31 Colors");

    init();

    glutDisplayFunc(display);
    glutMouseFunc(mouse);
    glutTimerFunc(0, update, 0);

    glutMainLoop();
    return 0;
}
