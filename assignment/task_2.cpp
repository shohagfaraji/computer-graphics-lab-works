#include <GL/freeglut.h>
#include <bits/stdc++.h>
using namespace std;

// --- Global Constants ---
const int WINDOW_SIZE = 700;
const int CENTER_X = WINDOW_SIZE / 2;
const int CENTER_Y = WINDOW_SIZE / 2;
const int MAX_RADIUS = 300;
const int NUM_CIRCLES =
    100; // Increased count for finer gradient and thickness steps
const int MIN_RADIUS = 10;
const int RADIUS_STEP = (MAX_RADIUS - MIN_RADIUS) / (NUM_CIRCLES - 1);
const float MAX_THICKNESS = 5.0f; // Max thickness in glPointSize units

// --- Utility Functions ---

/**
 * @brief Converts HSV to RGB color model.
 * @param h Hue (0-360 degrees)
 * @param s Saturation (0-1)
 * @param v Value (0-1)
 * @param r Pointer to store Red component (0-1)
 * @param g Pointer to store Green component (0-1)
 * @param b Pointer to store Blue component (0-1)
 */
void hsvToRgb(float h, float s, float v, float* r, float* g, float* b) {
    if (s == 0.0f) {
        *r = *g = *b = v;
        return;
    }

    // Normalize H to [0, 6)
    float h_norm = h / 60.0f;
    int i = (int)floor(h_norm);
    float f = h_norm - i;

    float p = v * (1.0f - s);
    float q = v * (1.0f - s * f);
    float t = v * (1.0f - s * (1.0f - f));

    switch (i % 6) {
    case 0:
        *r = v;
        *g = t;
        *b = p;
        break;
    case 1:
        *r = q;
        *g = v;
        *b = p;
        break;
    case 2:
        *r = p;
        *g = v;
        *b = t;
        break;
    case 3:
        *r = p;
        *g = q;
        *b = v;
        break;
    case 4:
        *r = t;
        *g = p;
        *b = v;
        break;
    case 5:
        *r = v;
        *g = p;
        *b = q;
        break;
    }
}

// Function to set the color based on the current circle index (i)
void set_gradient_color(int i) {
    // Normalized position of the circle (0.0 for inner, 1.0 for outer)
    float normalized_i = (float)i / (NUM_CIRCLES - 1);

    // Use HSV model for smooth gradient.
    // Hue (H) changes from 0 (Red) to 360 (Red) across the circles.
    // We can use 0 to 360 degrees for a full color wheel cycle.
    float hue = normalized_i * 360.0f;

    float saturation = 1.0f; // Full saturation
    float value = 1.0f;      // Full brightness

    float red, green, blue;
    hsvToRgb(hue, saturation, value, &red, &green, &blue);

    glColor3f(red, green, blue);
}

// Function to draw a pixel on the screen
void draw_pixel(int x, int y, int cx, int cy, float size) {
    glPointSize(size);
    glBegin(GL_POINTS);
    // Draw the point relative to the center (cx, cy)
    glVertex2i(cx + x, cy + y);
    glEnd();
}

// --- Midpoint Circle Drawing Functions ---

// Draws the 8 symmetrical points
void circle_plot_points(int cx, int cy, int x, int y, float size) {
    draw_pixel(x, y, cx, cy, size);
    draw_pixel(-x, y, cx, cy, size);
    draw_pixel(x, -y, cx, cy, size);
    draw_pixel(-x, -y, cx, cy, size);
    draw_pixel(y, x, cx, cy, size);
    draw_pixel(-y, x, cx, cy, size);
    draw_pixel(y, -x, cx, cy, size);
    draw_pixel(-y, -x, cx, cy, size);
}

// Midpoint Circle Algorithm implementation
void midpoint_circle(int cx, int cy, int radius, float thickness) {
    int x = 0;
    int y = radius;
    // Initial decision parameter P0 = 1 - r (for integer math)
    int p = 1 - radius;

    circle_plot_points(cx, cy, x, y, thickness);

    while (x < y) {
        x++;
        if (p < 0) {
            // Choose E (East)
            p += 2 * x + 1;
        } else {
            // Choose SE (South-East)
            y--;
            p += 2 * (x - y) + 1;
        }
        circle_plot_points(cx, cy, x, y, thickness);
    }
}

// --- Drawing Function ---

void display() {
    glClear(GL_COLOR_BUFFER_BIT);

    for (int i = 0; i < NUM_CIRCLES; ++i) {
        // Calculate the radius for the current circle
        int radius = MIN_RADIUS + i * RADIUS_STEP;

        // Thickness increases gradually (from 1.0 to MAX_THICKNESS)
        // Normalized thickness based on circle index 'i'
        float normalized_i = (float)i / (NUM_CIRCLES - 1);
        float thickness = 1.0f + normalized_i * (MAX_THICKNESS - 1.0f);

        // Set the smooth color gradient using HSV
        set_gradient_color(i);

        // Draw the circle
        midpoint_circle(CENTER_X, CENTER_Y, radius, thickness);
    }

    glFlush();
}

// --- Setup Functions ---

void init() {
    // Set a clean white background
    glClearColor(1.0, 1.0, 1.0, 1.0);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    // Set up a 2D orthographic view
    gluOrtho2D(0, WINDOW_SIZE, 0, WINDOW_SIZE);
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);
    glutInitWindowSize(WINDOW_SIZE, WINDOW_SIZE);
    glutInitWindowPosition(100, 100);
    glutCreateWindow("Smooth Concentric Circles (Midpoint + HSV Gradient)");

    init();
    glutDisplayFunc(display);
    glutMainLoop();

    return 0;
}
