#include <GL/freeglut.h>
#include <bits/stdc++.h>
using namespace std;

// --- Global Constants ---
const int WINDOW_SIZE = 700;
const int GRID_SPACING = 50;
const int MAX_COORD = WINDOW_SIZE / 2; // 350
const int INPUT_MODE = 1;
const int DRAWING_MODE = 2;

// --- State and Input Variables ---
int app_mode = INPUT_MODE; // 1: Input Screen, 2: Drawing Screen
int input_step =
    1; // 1: Choice, 2: x1, 3: y1, 4: x2, 5: y2, 6: width (if needed)

// Strings to hold the input values as the user types
string input_buffer = ""; // Holds the currently typed text
string choice_str = "";
string x1_str = "";
string y1_str = "";
string x2_str = "";
string y2_str = "";
string width_str = "";

// Final parsed integer values
int choice = 0;
int x1_in, y1_in, x2_in, y2_in;
int width_in = 1;
bool has_input = false;

// --- Utility Functions ---

int to_screen_x(int x) { return x + MAX_COORD; }
int to_screen_y(int y) { return y + MAX_COORD; }

void draw_pixel(int x, int y, float r, float g, float b) {
    glColor3f(r, g, b);
    glBegin(GL_POINTS);
    glVertex2i(to_screen_x(x), to_screen_y(y));
    glEnd();
}

// Function to draw large text for prompts
void draw_text_large(float x, float y, float r, float g, float b,
                     const string& text) {
    glColor3f(r, g, b);
    glRasterPos2f(x, y);
    for (char c : text) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, c);
    }
}

// Function to draw small text for axes labels
void draw_text_small(float x, float y, float r, float g, float b,
                     const string& text) {
    glColor3f(r, g, b);
    glRasterPos2f(x, y);
    for (char c : text) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_10, c);
    }
}

// Helper to convert string to int safely
int parse_int(const string& str) {
    try {
        // Only allow parsing if the string is not empty and not just "-"
        if (!str.empty() && (str.length() > 1 || str[0] != '-')) {
            return stoi(str);
        }
    } catch (...) {
        // Conversion error (e.g., overflow or invalid format)
    }
    return 0;
}

// --- Bresenham's Algorithms (Unchanged) ---

void standard_bresenham(int x1, int y1, int x2, int y2) {
    int dx = abs(x2 - x1);
    int dy = abs(y2 - y1);
    int sx = (x1 < x2) ? 1 : -1;
    int sy = (y1 < y2) ? 1 : -1;
    int err = dx - dy;
    int x = x1;
    int y = y1;

    while (true) {
        draw_pixel(x, y, 0.0, 0.0, 0.0);
        if (x == x2 && y == y2)
            break;
        int e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x += sx;
        }
        if (e2 < dx) {
            err += dx;
            y += sy;
        }
    }
}

void thick_bresenham(int x1, int y1, int x2, int y2, int w) {
    int half_w = (w - 1) / 2;
    int dx = abs(x2 - x1);
    int dy = abs(y2 - y1);
    bool steep = dy > dx;

    if (steep) {
        for (int offset = -half_w; offset <= half_w; ++offset) {
            standard_bresenham(x1 + offset, y1, x2 + offset, y2);
        }
    } else {
        for (int offset = -half_w; offset <= half_w; ++offset) {
            standard_bresenham(x1, y1 + offset, x2, y2 + offset);
        }
    }
}

// --- Drawing Screen Functions (Unchanged) ---

void draw_coordinate_system() {
    glColor3f(0.0f, 0.0f, 1.0f);

    // 1. Draw X and Y Axes (blue)
    glBegin(GL_LINES);
    glVertex2i(0, MAX_COORD);
    glVertex2i(WINDOW_SIZE, MAX_COORD);
    glVertex2i(MAX_COORD, 0);
    glVertex2i(MAX_COORD, WINDOW_SIZE);
    glEnd();

    // 2. Draw Grid and Numbers (blue)
    for (int current_coord = -MAX_COORD + GRID_SPACING;
         current_coord < MAX_COORD; current_coord += GRID_SPACING) {
        if (current_coord == 0)
            continue;

        int screen_pos_x = to_screen_x(current_coord);
        int screen_pos_y = to_screen_y(current_coord);

        glBegin(GL_LINES);
        glVertex2i(screen_pos_x, MAX_COORD - 3);
        glVertex2i(screen_pos_x, MAX_COORD + 3);
        glVertex2i(MAX_COORD - 3, screen_pos_y);
        glVertex2i(MAX_COORD + 3, screen_pos_y);
        glEnd();

        if (current_coord % GRID_SPACING == 0) {
            draw_text_small(screen_pos_x - 10, MAX_COORD - 15, 0.0f, 0.0f, 1.0f,
                            to_string(current_coord));
            draw_text_small(MAX_COORD + 5, screen_pos_y - 3, 0.0f, 0.0f, 1.0f,
                            to_string(current_coord));
        }
    }
}

// --- Input Screen Functions (Refined) ---

void draw_input_screen() {
    // Light background for input
    glClearColor(0.9f, 0.9f, 0.9f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // Dynamic Prompt
    string prompt_line1 = "";
    string prompt_line2 = "Enter value and press ENTER ⏎";
    string current_value = "";
    float text_r = 0.0f, text_g = 0.0f, text_b = 0.0f;

    switch (input_step) {
        case 1:
            prompt_line1 = "STEP 1: Select Line Type";
            prompt_line2 = "Type 1 (Standard) or 2 (Thick) and press ENTER";
            current_value = "Choice: " + input_buffer;
            text_r = 0.5f;
            text_g = 0.0f;
            text_b = 0.0f;
            break;
        case 2:
            prompt_line1 = "STEP 2: Enter Start X (x1)";
            prompt_line2 = "Coordinates in range (-" + to_string(MAX_COORD) +
                           " to " + to_string(MAX_COORD) + ")";
            current_value = "x1: " + input_buffer;
            break;
        case 3:
            prompt_line1 = "STEP 3: Enter Start Y (y1)";
            current_value = "y1: " + input_buffer;
            break;
        case 4:
            prompt_line1 = "STEP 4: Enter End X (x2)";
            current_value = "x2: " + input_buffer;
            break;
        case 5:
            prompt_line1 = "STEP 5: Enter End Y (y2)";
            current_value = "y2: " + input_buffer;
            break;
        case 6:
            prompt_line1 = "STEP 6: Enter Line Width (W)";
            prompt_line2 = "Width W (e.g., 3, 5, 7). Must be > 0.";
            current_value = "Width W: " + input_buffer;
            text_r = 0.0f;
            text_g = 0.5f;
            text_b = 0.0f;
            break;
    }

    // Display Text
    draw_text_large(100, 600, 0.1f, 0.1f, 0.1f,
                    "Bresenham's Algorithm Input Wizard");

    draw_text_large(100, 500, text_r, text_g, text_b, prompt_line1);
    draw_text_large(100, 450, 0.5f, 0.5f, 0.5f, prompt_line2);

    draw_text_large(100, 300, 0.0f, 0.0f, 0.8f,
                    "Typing: " + current_value + "_"); // Current input buffer

    // Status box (optional, but helpful)
    string status = "P1(" + x1_str + ", " + y1_str + ") P2(" + x2_str + ", " +
                    y2_str + ") W:" + width_str;
    draw_text_small(100, 50, 0.3f, 0.3f, 0.3f, "Current Status: " + status);

    glFlush();
}

// Function to process the input string when ENTER is pressed
void process_input() {
    bool input_valid = true;

    // Check for empty input (except for the option step, which must be 1 or 2)
    if (input_buffer.empty() && input_step != 1) {
        input_valid = false;
        cerr << "Input cannot be empty.\n";
    }

    if (input_valid) {
        if (input_step == 1) {
            if (input_buffer == "1" || input_buffer == "2") {
                choice_str = input_buffer;
                input_step = 2;
            } else {
                input_valid = false;
                cerr << "Invalid choice. Must be 1 or 2.\n";
            }
        } else if (input_step == 2) {
            x1_str = input_buffer;
            input_step = 3;
        } else if (input_step == 3) {
            y1_str = input_buffer;
            input_step = 4;
        } else if (input_step == 4) {
            x2_str = input_buffer;
            input_step = 5;
        } else if (input_step == 5) {
            y2_str = input_buffer;
            if (choice_str == "1") {
                has_input = true;
                app_mode = DRAWING_MODE; // Option 1 is done
            } else {
                input_step = 6; // Go to width input
            }
        } else if (input_step == 6) {
            width_str = input_buffer;
            has_input = true;
            app_mode = DRAWING_MODE; // Option 2 is done
        }
    }

    if (input_valid) {
        input_buffer = ""; // Clear buffer for next step
    } else {
        // If input was invalid (e.g., choice != 1 or 2), clear the buffer but
        // stay on step
        input_buffer = "";
    }

    // Final Parsing before drawing
    if (app_mode == DRAWING_MODE) {
        choice = parse_int(choice_str);
        x1_in = parse_int(x1_str);
        y1_in = parse_int(y1_str);
        x2_in = parse_int(x2_str);
        y2_in = parse_int(y2_str);
        width_in = (choice == 2) ? max(1, parse_int(width_str)) : 1;

        cout << "Drawing with: P1(" << x1_in << ", " << y1_in << "), P2("
             << x2_in << ", " << y2_in << "), Width: " << width_in << "\n";
    }

    glutPostRedisplay(); // Redraw window
}

// --- OpenGL Callback Functions ---

void display() {
    if (app_mode == INPUT_MODE) {
        draw_input_screen();
    } else if (app_mode == DRAWING_MODE) {
        // Drawing Screen Setup
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glPointSize(1.0);
        draw_coordinate_system();

        if (has_input) {
            glPointSize(1.5);
            if (choice == 1) {
                standard_bresenham(x1_in, y1_in, x2_in, y2_in);
            } else if (choice == 2) {
                thick_bresenham(x1_in, y1_in, x2_in, y2_in, width_in);
            }
        }

        // Simple instruction to close the window
        draw_text_small(10, 10, 0.5f, 0.0f, 0.0f,
                        "Line Drawn. Close window to exit.");

        glFlush();
    }
}

void keyboard(unsigned char key, int x, int y) {
    if (app_mode != INPUT_MODE)
        return;

    if (key == 13) { // Enter Key
        process_input();
    } else if (key == 8 || key == 127) { // Backspace or Delete
        if (!input_buffer.empty()) {
            input_buffer.pop_back();
            glutPostRedisplay();
        }
    } else if ((key >= '0' && key <= '9') ||
               key == '-') { // Numbers and Minus sign
        // Only allow '-' at the beginning, and restrict length
        if (key == '-' && !input_buffer.empty())
            return;
        if (input_buffer.length() < 5) {
            input_buffer += key;
            glutPostRedisplay();
        }
    }
}

void init() {
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, WINDOW_SIZE, 0, WINDOW_SIZE);
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);
    glutInitWindowSize(WINDOW_SIZE, WINDOW_SIZE);
    glutInitWindowPosition(100, 100);
    glutCreateWindow("Bresenham's Line Drawing");

    init();
    glutDisplayFunc(display);
    glutKeyboardFunc(keyboard);
    glutMainLoop();

    return 0;
}
