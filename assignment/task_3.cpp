#include <GL/freeglut.h>
#include <bits/stdc++.h>
using namespace std;

// --- Global Constants ---
const int WINDOW_WIDTH = 1500;
const int WINDOW_HEIGHT = 800;
const int UI_HEADER_HEIGHT = 70;
const int AXIS_GAP = 50;

// Define the logical/mathematical range for the coordinate system
// This centers the origin (0,0) in the middle of the drawing area.
const float COORD_RANGE_X = 700.0f; // Half the range for X
const float COORD_RANGE_Y = 365.0f; // Half the range for Y (800 - 70 / 2 = 365)

// Drawing area limits for the UI (70% of the window)
const float DRAWING_AREA_WIDTH = WINDOW_WIDTH * 0.7;
const float DRAWING_AREA_HEIGHT = WINDOW_HEIGHT - UI_HEADER_HEIGHT;

const int WINDOW_INPUT_MODE = 1;
const int LINE_INPUT_MODE = 2;
const int CLIPPING_MODE = 3;

// --- Data Structures ---
struct Point {
    float x, y;
};

struct LineSegment {
    Point p1, p2;
};

// Stores all visible vertices and intersection points for labeling
vector<Point> visible_points;

// --- State Variables ---
int app_mode = WINDOW_INPUT_MODE;
int click_count = 0;
float xmin = 0, ymin = 0, xmax = 0, ymax = 0;

vector<LineSegment> lines_to_clip;
Point current_line_start = {0, 0};
bool drawing_new_segment = true;

// --- Utility Functions ---

// Global variables for coordinate system transformation
float x_center_offset = 0.0f;
float y_center_offset = 0.0f;
float x_scale = 1.0f;
float y_scale = 1.0f;

// Transform logical (mathematical) coordinates to screen (OpenGL) coordinates
Point logical_to_screen(float lx, float ly) {
    Point screen_p;
    screen_p.x = x_center_offset + lx * x_scale;
    screen_p.y = y_center_offset + ly * y_scale;
    return screen_p;
}

// Transform screen (OpenGL) coordinates to logical (mathematical) coordinates
Point screen_to_logical(float sx, float sy) {
    Point logical_p;
    logical_p.x = (sx - x_center_offset) / x_scale;
    logical_p.y = (sy - y_center_offset) / y_scale;
    return logical_p;
}

void draw_text(float x, float y, float r, float g, float b, const string& text,
               void* font) {
    glColor3f(r, g, b);
    glRasterPos2f(x, y);
    for (char c : text) {
        glutBitmapCharacter(font, c);
    }
}

void draw_ui_header(const string& title, const string& instruction,
                    float title_r, float title_g, float title_b) {
    // Draw background (title bar)
    glColor3f(0.85f, 0.85f, 0.85f);
    glBegin(GL_QUADS);
    glVertex2f(0, WINDOW_HEIGHT - UI_HEADER_HEIGHT);
    glVertex2f(WINDOW_WIDTH, WINDOW_HEIGHT - UI_HEADER_HEIGHT);
    glVertex2f(WINDOW_WIDTH, WINDOW_HEIGHT);
    glVertex2f(0, WINDOW_HEIGHT);
    glEnd();

    // Draw separator line
    glColor3f(0.5f, 0.5f, 0.5f);
    glLineWidth(2.0f);
    glBegin(GL_LINES);
    glVertex2f(0, WINDOW_HEIGHT - UI_HEADER_HEIGHT);
    glVertex2f(WINDOW_WIDTH, WINDOW_HEIGHT - UI_HEADER_HEIGHT);
    glEnd();

    draw_text(10, WINDOW_HEIGHT - 25, title_r, title_g, title_b, title,
              GLUT_BITMAP_HELVETICA_18);
    draw_text(10, WINDOW_HEIGHT - 50, 0.1f, 0.1f, 0.1f, instruction,
              GLUT_BITMAP_HELVETICA_12);
}

void draw_clipping_window() {
    // The window coordinates (xmin, ymin, etc.) are in logical space now.
    Point p1 = logical_to_screen(xmin, ymin);
    Point p2 = logical_to_screen(xmax, ymin);
    Point p3 = logical_to_screen(xmax, ymax);
    Point p4 = logical_to_screen(xmin, ymax);

    glColor3f(0.0f, 0.0f, 0.0f); // Black window frame
    glLineWidth(2.0f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(p1.x, p1.y);
    glVertex2f(p2.x, p2.y);
    glVertex2f(p3.x, p3.y);
    glVertex2f(p4.x, p4.y);
    glEnd();
}

// Function to draw coordinate axes and numbering based on AXIS_GAP (50)
void draw_coordinate_system() {
    // 1. Calculate axis screen positions
    Point origin = logical_to_screen(0.0f, 0.0f);
    Point x_max_screen = logical_to_screen(COORD_RANGE_X, 0.0f);
    Point x_min_screen = logical_to_screen(-COORD_RANGE_X, 0.0f);
    Point y_max_screen = logical_to_screen(0.0f, COORD_RANGE_Y);
    Point y_min_screen = logical_to_screen(0.0f, -COORD_RANGE_Y);

    // 2. Draw X and Y Axes
    glColor3f(0.4f, 0.4f, 0.4f); // Darker gray for axes
    glLineWidth(1.5f);

    glBegin(GL_LINES);
    // X-Axis
    glVertex2f(x_min_screen.x, origin.y);
    glVertex2f(x_max_screen.x, origin.y);

    // Y-Axis
    glVertex2f(origin.x, y_min_screen.y);
    glVertex2f(origin.x, y_max_screen.y);
    glEnd();

    // 3. Draw Ticks and Numbers
    glColor3f(0.4f, 0.4f, 0.4f); // Color for text and ticks
    glLineWidth(1.0f);

    // --- X-Axis Ticks and Numbers (Divisible by 50) ---
    // Start at the first multiple of AXIS_GAP that is <= -COORD_RANGE_X
    int start_x = static_cast<int>(ceil(-COORD_RANGE_X / AXIS_GAP)) * AXIS_GAP;
    for (int x = start_x; x <= COORD_RANGE_X; x += AXIS_GAP) {
        if (x == 0)
            continue; // Skip origin

        Point tick_screen = logical_to_screen(x, 0);

        // Draw Tick Mark
        glBegin(GL_LINES);
        glVertex2f(tick_screen.x, origin.y - 4);
        glVertex2f(tick_screen.x, origin.y + 4); // Tick length 8 pixels total
        glEnd();

        // X-axis number
        string label = to_string(x);
        draw_text(tick_screen.x + 2, origin.y + 5, 0.4f, 0.4f, 0.4f, label,
                  GLUT_BITMAP_HELVETICA_10);
    }

    // --- Y-Axis Ticks and Numbers (Divisible by 50) ---
    // Start at the first multiple of AXIS_GAP that is <= -COORD_RANGE_Y
    int start_y = static_cast<int>(ceil(-COORD_RANGE_Y / AXIS_GAP)) * AXIS_GAP;
    for (int y = start_y; y <= COORD_RANGE_Y; y += AXIS_GAP) {
        if (y == 0)
            continue; // Skip origin

        Point tick_screen = logical_to_screen(0, y);

        // Draw Tick Mark
        glBegin(GL_LINES);
        glVertex2f(origin.x - 4, tick_screen.y);
        glVertex2f(origin.x + 4, tick_screen.y); // Tick length 8 pixels total
        glEnd();

        // Y-axis number
        string label = to_string(y);
        draw_text(origin.x + 5, tick_screen.y + 2, 0.4f, 0.4f, 0.4f, label,
                  GLUT_BITMAP_HELVETICA_10);
    }

    // Label Origin
    draw_text(origin.x + 5, origin.y + 5, 0.4f, 0.4f, 0.4f, "(0,0)",
              GLUT_BITMAP_HELVETICA_10);
}

// --- Liang-Barsky Algorithm (Uses logical coordinates) ---

bool liang_barsky_clip(float x0, float y0, float x1, float y1, float& tx0,
                       float& tx1) {
    float dx = x1 - x0;
    float dy = y1 - y0;
    float p[4], q[4];
    float t0 = 0.0, t1 = 1.0;

    p[0] = -dx;
    q[0] = x0 - xmin;
    p[1] = dx;
    q[1] = xmax - x0;
    p[2] = -dy;
    q[2] = y0 - ymin;
    p[3] = dy;
    q[3] = ymax - y0;

    for (int i = 0; i < 4; i++) {
        if (fabs(p[i]) < 1e-6) {
            if (q[i] < 0.0f)
                return false;
        } else {
            float t = q[i] / p[i];
            if (p[i] < 0.0f) {
                t0 = max(t0, t);
            } else {
                t1 = min(t1, t);
            }
        }
    }

    if (t0 > t1) {
        return false;
    }

    tx0 = t0;
    tx1 = t1;
    return true;
}

// --- Display Modes ---

void draw_clipping_mode() {
    glClear(GL_COLOR_BUFFER_BIT);
    visible_points.clear(); // Clear points list for this drawing cycle

    // Divide window: 70% for drawing, 30% for coordinate list
    const float LIST_AREA_START_X = WINDOW_WIDTH * 0.7;

    draw_ui_header("STEP 3: Clipping Results (Liang-Barsky) - All Quadrants",
                   "Original Lines (RED) | Clipped Segments (GREEN) | "
                   "Intersections (BLUE/P#)",
                   0.0f, 0.5f, 0.0f);

    // 1. Draw Axis/Grid in the main area
    draw_coordinate_system();

    // 2. Draw Clipping Window
    draw_clipping_window();

    // 3. Process and Draw Lines
    for (const auto& line : lines_to_clip) {
        float x0 = line.p1.x; // Logical x0
        float y0 = line.p1.y; // Logical y0
        float x1 = line.p2.x; // Logical x1
        float y1 = line.p2.y; // Logical y1

        Point screen_p1 = logical_to_screen(x0, y0);
        Point screen_p2 = logical_to_screen(x1, y1);

        float tx0, tx1;

        // Draw the ORIGINAL line segment (RED) using screen coordinates
        glColor3f(1.0f, 0.0f, 0.0f);
        glLineWidth(1.0f);
        glBegin(GL_LINES);
        glVertex2f(screen_p1.x, screen_p1.y);
        glVertex2f(screen_p2.x, screen_p2.y);
        glEnd();

        // Execute Liang-Barsky using logical coordinates
        if (liang_barsky_clip(x0, y0, x1, y1, tx0, tx1)) {
            // Calculate clipped endpoints in logical coordinates
            float clipped_lx0 = x0 + tx0 * (x1 - x0);
            float clipped_ly0 = y0 + tx0 * (y1 - y0);
            float clipped_lx1 = x0 + tx1 * (x1 - x0);
            float clipped_ly1 = y0 + tx1 * (y1 - y0);

            // Convert clipped endpoints to screen coordinates for drawing
            Point clipped_screen_p1 =
                logical_to_screen(clipped_lx0, clipped_ly0);
            Point clipped_screen_p2 =
                logical_to_screen(clipped_lx1, clipped_ly1);

            // Add the visible endpoints (logical coordinates) to the list
            visible_points.push_back({clipped_lx0, clipped_ly0});
            visible_points.push_back({clipped_lx1, clipped_ly1});

            // Draw the CLIPPED segment (GREEN, THICK)
            glColor3f(0.0f, 0.8f, 0.0f);
            glLineWidth(4.0f);
            glBegin(GL_LINES);
            glVertex2f(clipped_screen_p1.x, clipped_screen_p1.y);
            glVertex2f(clipped_screen_p2.x, clipped_screen_p2.y);
            glEnd();

            // Draw large dots on the intersection/visible endpoints
            glColor3f(0.0f, 0.0f, 0.8f);
            glPointSize(8.0f);
            glBegin(GL_POINTS);
            glVertex2f(clipped_screen_p1.x, clipped_screen_p1.y);
            glVertex2f(clipped_screen_p2.x, clipped_screen_p2.y);
            glEnd();
        }
    }

    // --- 4. Label Visible/Intersection Points on the Graph ---
    for (size_t i = 0; i < visible_points.size(); ++i) {
        const auto& p_logical = visible_points[i];
        Point p_screen = logical_to_screen(p_logical.x, p_logical.y);

        string label = "P" + to_string(i + 1);
        // Draw the label near the point (offset by 8 units)
        draw_text(p_screen.x + 8, p_screen.y + 8, 0.0f, 0.0f, 0.8f, label,
                  GLUT_BITMAP_HELVETICA_12);
    }

    // --- 5. Draw Coordinate List ---

    // Draw background for the list area
    glColor3f(0.95f, 0.95f, 0.95f);
    glBegin(GL_QUADS);
    glVertex2f(LIST_AREA_START_X, 0);
    glVertex2f(WINDOW_WIDTH, 0);
    glVertex2f(WINDOW_WIDTH, WINDOW_HEIGHT - UI_HEADER_HEIGHT);
    glVertex2f(LIST_AREA_START_X, WINDOW_HEIGHT - UI_HEADER_HEIGHT);
    glEnd();

    // Draw list separator
    glColor3f(0.5f, 0.5f, 0.5f);
    glLineWidth(1.0f);
    glBegin(GL_LINES);
    glVertex2f(LIST_AREA_START_X, 0);
    glVertex2f(LIST_AREA_START_X, WINDOW_HEIGHT - UI_HEADER_HEIGHT);
    glEnd();

    draw_text(LIST_AREA_START_X + 10, WINDOW_HEIGHT - UI_HEADER_HEIGHT - 10,
              0.2f, 0.2f, 0.2f, "Visible/Intersection Points (Logical Coords):",
              GLUT_BITMAP_HELVETICA_12);

    float current_y = WINDOW_HEIGHT - UI_HEADER_HEIGHT - 30;

    stringstream ss;
    ss << fixed << setprecision(1); // Set precision to 1 decimal place

    for (size_t i = 0; i < visible_points.size(); ++i) {
        const auto& p = visible_points[i];

        ss.str(""); // Clear stringstream
        ss << "P" << (i + 1) << " (" << p.x << ", " << p.y << ")";

        draw_text(LIST_AREA_START_X + 10, current_y, 0.0f, 0.0f, 0.0f, ss.str(),
                  GLUT_BITMAP_HELVETICA_10);

        current_y -= 15; // Move down for the next point

        if (current_y < 10)
            break; // Stop if we run out of space
    }

    glFlush();
}

void draw_window_input_mode() {
    glClear(GL_COLOR_BUFFER_BIT);

    string prompt;
    if (click_count == 0) {
        prompt =
            "Click 1/2: Select first diagonal corner of the Clipping Window.";
    } else {
        // Show the currently selected point in screen coordinates for feedback
        // Use logical coordinates for display
        stringstream ss;
        ss << fixed << setprecision(1);
        ss << "Click 2/2: Select the opposite diagonal corner to complete the "
              "window. P1: ("
           << xmin << ", " << ymin << ")";
        prompt = ss.str();
    }

    draw_ui_header("STEP 1: Define Clipping Window (All Quadrants)", prompt,
                   0.0f, 0.5f, 0.0f);

    draw_coordinate_system(); // Show the axes even in input mode

    if (click_count == 1) {
        // Draw the point in screen coordinates
        Point p_screen = logical_to_screen(xmin, ymin);
        glColor3f(0.8f, 0.2f, 0.2f);
        glPointSize(8.0f);
        glBegin(GL_POINTS);
        glVertex2f(p_screen.x, p_screen.y);
        glEnd();
    }

    glFlush();
}

void draw_line_input_mode() {
    glClear(GL_COLOR_BUFFER_BIT);

    string prompt;
    if (drawing_new_segment) {
        prompt = "Click P1 to start a new line segment. Press SPACE to "
                 "reset/start a new line.";
    } else {
        stringstream ss;
        ss << fixed << setprecision(1);
        ss << "Click P2 to connect to the previous point. P1: ("
           << current_line_start.x << ", " << current_line_start.y
           << "). Press SPACE to start a new line.";
        prompt = ss.str();
    }

    draw_ui_header("STEP 2: Draw Line Segments (Total Lines: " +
                       to_string(lines_to_clip.size()) + ")",
                   prompt + " Press ENTER to Clip.", 0.8f, 0.4f, 0.0f);

    draw_coordinate_system();

    // Draw the clipping window
    draw_clipping_window();

    // Draw all lines currently stored
    glColor3f(0.5f, 0.5f, 0.5f); // Gray for input lines
    glLineWidth(1.0f);
    glBegin(GL_LINES);
    for (const auto& line : lines_to_clip) {
        Point p1_screen = logical_to_screen(line.p1.x, line.p1.y);
        Point p2_screen = logical_to_screen(line.p2.x, line.p2.y);
        glVertex2f(p1_screen.x, p1_screen.y);
        glVertex2f(p2_screen.x, p2_screen.y);
    }
    glEnd();

    // Draw the start point of the current line
    if (!drawing_new_segment) {
        Point p_screen =
            logical_to_screen(current_line_start.x, current_line_start.y);
        glColor3f(0.0f, 0.0f, 0.8f);
        glPointSize(5.0f);
        glBegin(GL_POINTS);
        glVertex2f(p_screen.x, p_screen.y);
        glEnd();
    }

    glFlush();
}

void display() {
    if (app_mode == WINDOW_INPUT_MODE) {
        draw_window_input_mode();
    } else if (app_mode == LINE_INPUT_MODE) {
        draw_line_input_mode();
    } else {
        draw_clipping_mode();
    }
}

// --- Event Callbacks ---

void mouse(int button, int state, int x, int y) {
    if (button != GLUT_LEFT_BUTTON || state != GLUT_DOWN)
        return;

    float screen_x = (float)x;
    float screen_y = (float)(WINDOW_HEIGHT - y); // Flip Y

    // Convert screen coordinates to logical (mathematical) coordinates
    Point click_point_logical = screen_to_logical(screen_x, screen_y);

    if (app_mode == WINDOW_INPUT_MODE) {
        if (click_count == 0) {
            xmin = click_point_logical.x;
            ymin = click_point_logical.y;
            click_count = 1;
        } else {
            xmax = click_point_logical.x;
            ymax = click_point_logical.y;

            // Ensure xmin < xmax and ymin < ymax for clipping algorithm
            if (xmin > xmax)
                swap(xmin, xmax);
            if (ymin > ymax)
                swap(ymin, ymax);

            app_mode = LINE_INPUT_MODE;
            click_count = 0;
        }
    } else if (app_mode == LINE_INPUT_MODE) {
        // Restrict clicks to the drawing area, ignoring the UI header (in
        // screen coordinates)
        if (screen_y > WINDOW_HEIGHT - UI_HEADER_HEIGHT)
            return;

        // Also restrict clicks to the left 70% of the window
        if (screen_x > DRAWING_AREA_WIDTH)
            return;

        if (drawing_new_segment) {
            current_line_start = click_point_logical;
            drawing_new_segment = false;
        } else {
            LineSegment new_segment;
            new_segment.p1 = current_line_start;
            new_segment.p2 = click_point_logical;
            lines_to_clip.push_back(new_segment);

            current_line_start = click_point_logical;
        }
    }

    glutPostRedisplay();
}

void keyboard(unsigned char key, int x, int y) {
    if (app_mode == LINE_INPUT_MODE) {
        if (key == 13) { // Enter key
            app_mode = CLIPPING_MODE;
        } else if (key == ' ') {
            drawing_new_segment = true;
        }
    } else if (key == 'r' || key == 'R') { // Reset everything
        app_mode = WINDOW_INPUT_MODE;
        click_count = 0;
        xmin = ymin = xmax = ymax = 0.0f;
        lines_to_clip.clear();
        visible_points.clear();
        drawing_new_segment = true;
    }
    glutPostRedisplay();
}

void init() {
    glClearColor(1.0, 1.0, 1.0, 1.0);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    // Set the projection to be based on the screen's pixel coordinates
    gluOrtho2D(0, WINDOW_WIDTH, 0, WINDOW_HEIGHT);

    // --- Setup Transformation from Logical (-X to +X, -Y to +Y) to Screen (0
    // to W, 0 to H) ---
    float drawing_width = DRAWING_AREA_WIDTH;
    float drawing_height = DRAWING_AREA_HEIGHT;

    // Calculate scaling factors: logical range / screen range
    x_scale = drawing_width / (2.0f * COORD_RANGE_X);
    y_scale = drawing_height / (2.0f * COORD_RANGE_Y);

    // Center offsets
    x_center_offset = drawing_width / 2.0f;
    y_center_offset = drawing_height / 2.0f;
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);
    glutInitWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);
    glutInitWindowPosition(100, 100);
    glutCreateWindow("Liang-Barsky Line Clipping (All Quadrants)");

    init();
    glutDisplayFunc(display);
    glutMouseFunc(mouse);
    glutKeyboardFunc(keyboard);
    glutMainLoop();

    return 0;
}
