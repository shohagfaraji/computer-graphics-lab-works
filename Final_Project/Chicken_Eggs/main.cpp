#include <GL/glut.h>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <string>
#include <vector>

struct Vec2 {
    float x, y;
};

float frand(float a, float b) {
    return a + (b - a) * (rand() / (float)RAND_MAX);
}

// Reworked object types: Removed all square 'Perk' types
enum class ObjType { NormalEgg, BlueEgg, GoldenEgg, Bomb };
enum class Screen { Menu, Help, Playing, Paused, GameOver };

struct Falling {
    ObjType type;
    Vec2 pos;
    float vy;
    float radius;
    float rot = 0.0f, rotSpd = 0.0f;
    bool active = true;
};

struct Basket {
    float x = 0.0f, y = -0.8f;
    float halfW = 0.16f, h = 0.09f;
};

struct Chicken {
    float x = 0.0f, y = 0.70f;
    float vx = 0.45f;
    float bob = 0.0f;
};

struct Particle {
    Vec2 p, v;
    float life, maxLife;
    float size;
    float r, g, b, a;
};

struct FloatText {
    Vec2 p;
    std::string s;
    float life = 1.0f;
    float vy = 0.35f;
    float r = 0, g = 0, b = 0;
};

struct Cloud {
    Vec2 pos;
    float scale;
    float speed;
};

// New: Struct to hold static flower data
struct Flower {
    float x, y, scale, r, g, b;
};

static int winW = 900, winH = 600;
static float worldL = -1, worldR = 1, worldB = -1, worldT = 1;

static Screen screenState = Screen::Menu;
static int menuIndex = 0;

static Basket basket;
static Chicken chicken;
static std::vector<Falling> objs;
static std::vector<Particle> parts;
static std::vector<FloatText> floatTexts;
static std::vector<Cloud> clouds;
static std::vector<Flower> flowers; // New: Vector to store static flowers

static int score = 0, highScore = 0;
static int timeLeft = 60;
static int lives = 3;
static const int MAX_BOMBS_ALLOWED = 3;
static int bombsCaught = 0;

static float spawnTimer = 0, spawnEvery = 0.65f;
static int lastTick = 0;

static float shake = 0.0f;

// --- Wind Variables
static float windForce = 0.0f;    // Current horizontal force applied to objects
static float maxWindForce = 0.5f; // Max strength of the wind
static float windChangeTimer = 0.0f;
static float windChangeEvery = 4.0f; // Wind changes every 4 seconds

// --- Mouse / Button support
struct Button {
    float x1, y1, x2, y2; // in world‑coords
    std::string label;
    void (*action)();
};

static std::vector<Button> menuButtons;

// Utility to map window coordinate to world coordinate
Vec2 windowToWorld(int mx, int my) {
    float wx = worldL + (mx / (float)winW) * (worldR - worldL);
    float wy = worldB + ((winH - my) / (float)winH) * (worldT - worldB);
    return {wx, wy};
}

void startGame();
void helpScreen();
void exitGame();

bool isOver(const Button& b, const Vec2& wpos) {
    return (wpos.x >= b.x1 && wpos.x <= b.x2 && wpos.y >= b.y1 &&
            wpos.y <= b.y2);
}

void mouseClick(int button, int state, int x, int y) {
    if (state != GLUT_DOWN)
        return;
    Vec2 wpos = windowToWorld(x, y);
    if (screenState == Screen::Menu) {
        for (auto& b : menuButtons) {
            if (isOver(b, wpos)) {
                b.action();
                return;
            }
        }
    } else if (screenState == Screen::GameOver) {
        if (wpos.x < 0) {
            screenState = Screen::Menu;
        } else {
            startGame();
        }
    } else if (screenState == Screen::Help) {
        screenState = Screen::Menu;
    }
}

// --- Setup buttons for menu
void setupMenuButtons() {
    float bw = 0.36f, bh = 0.07f;
    float cx = 0.0f;
    float baseY = 0.15f;
    menuButtons.clear();
    menuButtons.push_back(
        {cx - bw, baseY - bh, cx + bw, baseY + bh, "Start", &startGame});
    menuButtons.push_back({cx - bw, baseY - 0.12f - bh, cx + bw,
                           baseY - 0.12f + bh, "Resume",
                           []() { screenState = Screen::Playing; }});
    menuButtons.push_back({cx - bw, baseY - 0.24f - bh, cx + bw,
                           baseY - 0.24f + bh, "Help", &helpScreen});
    menuButtons.push_back({cx - bw, baseY - 0.36f - bh, cx + bw,
                           baseY - 0.36f + bh, "Exit", &exitGame});
}

// Function to initialize flowers ONCE
void initializeFlowers() {
    flowers.clear();
    for (int i = 0; i < 20; ++i) { // Create 20 static flowers
        Flower f;
        f.x = frand(worldL + 0.05f, worldR - 0.05f);
        f.y = frand(worldB + 0.1f,
                    worldB + 0.2f); // Place them on the foreground grass
        f.scale = frand(0.8f, 1.2f);
        f.r = frand(0.7f, 1.0f);
        f.g = frand(0.2f, 0.8f);
        f.b = frand(0.7f, 1.0f);
        flowers.push_back(f);
    }
}

// Action functions
void startGame() {
    // Reset game state
    objs.clear();
    parts.clear();
    floatTexts.clear();
    score = 0;
    timeLeft = 60;
    lives = 3;
    bombsCaught = 0;
    spawnTimer = 0;
    spawnEvery = 0.65f;
    basket = Basket();
    chicken = Chicken();
    shake = 0.0f;

    // Reset and initialize clouds
    clouds.clear();
    for (int i = 0; i < 5; ++i) { // 5 clouds
        clouds.push_back({{frand(worldL, worldR), frand(0.5f, worldT - 0.1f)},
                          frand(0.5f, 1.2f),
                          frand(0.02f, 0.08f)});
    }

    // Flowers are initialized only once in main, but this ensures they exist if
    // main wasn't properly run.
    if (flowers.empty()) {
        initializeFlowers();
    }

    lastTick = glutGet(GLUT_ELAPSED_TIME);
    screenState = Screen::Playing;
}

void helpScreen() { screenState = Screen::Help; }

void exitGame() { exit(0); }

// --- Rendering / logic functions (mostly unchanged) …

void ortho() {
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(worldL, worldR, worldB, worldT);
    glMatrixMode(GL_MODELVIEW);
}

void enableSmooth() {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    glEnable(GL_POINT_SMOOTH);
    glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);
}

void drawRect(float x, float y, float w, float h) {
    glBegin(GL_QUADS);
    glVertex2f(x - w, y - h);
    glVertex2f(x + w, y - h);
    glVertex2f(x + w, y + h);
    glVertex2f(x - w, y + h);
    glEnd();
}

void drawCircle(float x, float y, float r, int seg = 48) {
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(x, y);
    for (int i = 0; i <= seg; i++) {
        float th = 2.0f * 3.1415926f * i / seg;
        glVertex2f(x + cosf(th) * r, y + sinf(th) * r);
    }
    glEnd();
}

void drawRing(float x, float y, float r, float t, int seg = 64) {
    glBegin(GL_TRIANGLE_STRIP);
    for (int i = 0; i <= seg; i++) {
        float th = 2.0f * 3.1415926f * i / seg;
        float cx = cosf(th), sy = sinf(th);
        glVertex2f(x + (r - t) * cx, y + (r - t) * sy);
        glVertex2f(x + r * cx, y + r * sy);
    }
    glEnd();
}

void drawText(float x, float y, const std::string& s,
              void* font = GLUT_BITMAP_HELVETICA_18) {
    glRasterPos2f(x, y);
    for (char c : s)
        glutBitmapCharacter(font, c);
}

void addParticles(Vec2 p, int n, float r, float g, float b) {
    for (int i = 0; i < n; i++) {
        Particle q;
        q.p = p;
        float ang = frand(0, 2 * 3.14159f);
        float sp = frand(0.6f, 1.6f);
        q.v = {cosf(ang) * sp, sinf(ang) * sp};
        q.life = q.maxLife = frand(0.35f, 0.75f);
        q.size = frand(0.008f, 0.02f);
        q.r = r;
        q.g = g;
        q.b = b;
        q.a = 1.0f;
        parts.push_back(q);
    }
}

void addFloatText(Vec2 p, const std::string& s, float r, float g, float b) {
    FloatText ft;
    ft.p = p;
    ft.s = s;
    ft.r = r;
    ft.g = g;
    ft.b = b;
    ft.life = 1.1f;
    floatTexts.push_back(ft);
}

Falling makeObj(ObjType t) {
    Falling f;
    f.type = t;
    f.pos = {chicken.x + frand(-0.05f, 0.05f), chicken.y - 0.06f};
    f.vy = -frand(0.45f, 0.65f);
    f.radius = (t == ObjType::GoldenEgg ? 0.045f
                : t == ObjType::Bomb    ? 0.032f
                                        : 0.038f);
    f.rotSpd = frand(-120, 120);
    return f;
}

ObjType getRandomObjType() {
    int r = rand() % 100; // 0 to 99
    if (r < 5)
        return ObjType::GoldenEgg; // 5% chance
    if (r < 15)
        return ObjType::BlueEgg; // 10% chance
    if (r < 30)
        return ObjType::Bomb;  // 15% chance
    return ObjType::NormalEgg; // 70% chance
}

bool aabbCircleCollide(const Basket& b, const Falling& f) {
    float cx = std::max(b.x - b.halfW, std::min(f.pos.x, b.x + b.halfW));
    float cy = std::max(b.y - b.h, std::min(f.pos.y, b.y + b.h));
    float dx = f.pos.x - cx, dy = f.pos.y - cy;
    return dx * dx + dy * dy <= f.radius * f.radius;
}

// Draw a cloud
void drawCloud(const Cloud& c) {
    glColor4f(1.0f, 1.0f, 1.0f, 0.9f);
    glPushMatrix();
    glTranslatef(c.pos.x, c.pos.y, 0);
    glScalef(c.scale, c.scale, 1.0f);

    drawCircle(-0.05f, 0.0f, 0.06f, 20);
    drawCircle(0.05f, 0.0f, 0.06f, 20);
    drawCircle(0.0f, 0.03f, 0.07f, 20);
    drawCircle(0.0f, -0.02f, 0.05f, 20);

    glPopMatrix();
}

// Draw a flower, now using stored color data
void drawFlower(const Flower& f) {
    glPushMatrix();
    glTranslatef(f.x, f.y, 0);
    glScalef(f.scale, f.scale, 1.0f);

    // Stem
    glColor3f(0.2f, 0.6f, 0.1f);
    glBegin(GL_LINES);
    glVertex2f(0.0f, -0.03f);
    glVertex2f(0.0f, 0.0f);
    glEnd();

    // Petals (uses stored color)
    glColor3f(f.r, f.g, f.b);
    drawCircle(0.015f, 0.01f, 0.008f, 10);
    drawCircle(-0.015f, 0.01f, 0.008f, 10);
    drawCircle(0.0f, 0.02f, 0.008f, 10);
    drawCircle(0.0f, 0.0f, 0.008f, 10);

    // Center
    glColor3f(0.9f, 0.7f, 0.1f);
    drawCircle(0.0f, 0.01f, 0.004f, 8);

    glPopMatrix();
}

// New: Function to draw all static flowers
void drawStaticFlowers() {
    for (const auto& f : flowers) {
        drawFlower(f);
    }
}

// New: Function to draw dry grass stack (haystack)
void drawDryGrassStack(float x, float y, float scale) {
    glPushMatrix();
    glTranslatef(x, y, 0);
    glScalef(scale, scale, 1.0f);

    // Haystack color: dry yellow/brown
    glColor3f(0.8f, 0.65f, 0.35f);

    // Main body (simplified trapezoid)
    glBegin(GL_QUADS);
    glVertex2f(-0.08f, 0.0f);
    glVertex2f(0.08f, 0.0f);
    glVertex2f(0.1f, 0.15f);
    glVertex2f(-0.1f, 0.15f);
    glEnd();

    // Top cap (rough triangle for a stacked look)
    glBegin(GL_TRIANGLES);
    glVertex2f(-0.1f, 0.15f);
    glVertex2f(0.1f, 0.15f);
    glVertex2f(0.0f, 0.22f);
    glEnd();

    // Highlight / texture lines
    glColor3f(0.9f, 0.75f, 0.45f);
    glLineWidth(1.5f);
    glBegin(GL_LINES);
    glVertex2f(-0.06f, 0.08f);
    glVertex2f(0.0f, 0.18f);
    glVertex2f(0.05f, 0.03f);
    glVertex2f(0.07f, 0.14f);
    glEnd();

    glPopMatrix();
}

void drawGradientBG() {
    // Sky Gradient
    glBegin(GL_QUADS);
    // Changed top color to a lighter, softer blue
    glColor3f(0.5f, 0.7f, 0.9f);
    glVertex2f(worldL, 0.15f);
    glColor3f(0.5f, 0.7f, 0.9f);
    glVertex2f(worldR, 0.15f);
    // Changed bottom sky color
    glColor3f(0.7f, 0.85f, 0.95f);
    glVertex2f(worldR, worldT);
    glColor3f(0.7f, 0.85f, 0.95f);
    glVertex2f(worldL, worldT);
    glEnd();

    // Sun
    glColor4f(1.0f, 0.93f, 0.45f, 0.9f);
    drawCircle(-0.78f, 0.82f, 0.10f, 48);

    // Draw moving clouds
    for (const auto& c : clouds) {
        drawCloud(c);
    }

    // Hills/Distant Ground (Color change to a deeper green)
    glColor3f(0.45f, 0.7f, 0.45f); // Deeper green
    drawRect(-0.2f, 0.22f, 1.2f, 0.12f);
    glColor3f(0.35f, 0.6f, 0.35f); // Even deeper green
    drawRect(0.15f, 0.12f, 1.1f, 0.10f);

    // House 🏠 (Simplified shape
    float houseX = 0.65f, houseY = 0.2f;
    float houseW = 0.15f, houseH = 0.15f;
    // Walls (Color change: softer red/brown)
    glColor3f(0.8f, 0.45f, 0.25f);
    drawRect(houseX, houseY, houseW, houseH);
    // Roof (Changed shape to simple triangle, color change: rustic red)
    glColor3f(0.55f, 0.15f, 0.05f);
    glBegin(GL_TRIANGLES);
    glVertex2f(houseX - houseW, houseY + houseH);
    glVertex2f(houseX + houseW, houseY + houseH);
    glVertex2f(houseX, houseY + houseH * 1.8f);
    glEnd();
    // Door
    glColor3f(0.4f, 0.25f, 0.1f);
    drawRect(houseX + houseW * 0.4f, houseY - houseH * 0.5f, houseW * 0.25f,
             houseH * 0.5f);
    // Window
    glColor3f(0.6f, 0.8f, 1.0f);
    drawRect(houseX - houseW * 0.4f, houseY + houseH * 0.2f, houseW * 0.3f,
             houseH * 0.3f);

    // Draw Dry Grass Stacks (New Element)
    drawDryGrassStack(-0.7f, worldB + 0.3f,
                      1.0f); // Near left edge, on the distant ground
    drawDryGrassStack(0.3f, worldB + 0.3f, 0.8f); // Near center

    // Foreground Grass (Color change: vibrant green)
    glColor3f(0.2f, 0.65f, 0.3f);
    drawRect(0, -0.94f, 1.0f, 0.14f);

    // Grass Blades
    glLineWidth(2);
    glColor3f(0.15f, 0.5f, 0.2f);
    glBegin(GL_LINES);
    for (int i = 0; i < 50; i++) {
        float x = worldL + 0.04f * i + fmodf(i * 0.03f, 0.02f);
        glVertex2f(x, -0.92f);
        glVertex2f(x + 0.01f, -0.88f);
    }
    glEnd();

    // Top HUD Background Bar
    glColor3f(1, 1, 1);
    //    drawRect(0, 0.93f, 1.0f, 0.07f);
}

void eggShape(const Falling& o) {
    if (o.type == ObjType::GoldenEgg) {
        glColor3f(1.0f, 0.84f, 0.0f);
    } else if (o.type == ObjType::BlueEgg) {
        glColor3f(0.45f, 0.65f, 1.0f);
    } else if (o.type == ObjType::NormalEgg) {
        glColor3f(1.0f, 1.0f, 0.94f);
    }

    drawCircle(0, 0, o.radius, 40);
    glColor4f(1, 1, 1, 0.45f);
    drawCircle(-o.radius * 0.35f, o.radius * 0.25f, o.radius * 0.35f, 28);

    if (o.type == ObjType::GoldenEgg) {
        glColor4f(1.0f, 0.9f, 0.2f, 0.25f);
        drawRing(0, 0, o.radius * 1.35f, o.radius * 0.15f, 56);
    }
}

// Drawing the bomb
void bombShape(const Falling& o) {
    glColor3f(0.1f, 0.1f, 0.1f);
    drawCircle(0, 0, o.radius * 1.1f, 24);

    glColor3f(0.8f, 0.8f, 0.8f);
    drawRect(0.0f, o.radius * 1.1f + 0.01f, 0.005f, 0.01f);

    glColor3f(0.5f, 0.5f, 0.5f);
    drawCircle(o.radius * 0.3f, o.radius * 0.4f, o.radius * 0.15f, 10);
}

void drawChicken() {
    glPushMatrix();
    glTranslatef(chicken.x, chicken.y + chicken.bob, 0);

    // Chicken Color Changed: More traditional white/cream chicken
    glColor3f(0.95f, 0.95f, 0.95f);
    drawCircle(0, 0, 0.065f, 36); // Body
    glColor3f(1.0f, 1.0f, 1.0f);
    drawCircle(-0.015f, -0.01f, 0.045f, 28); // Lower body
    glColor3f(0.9f, 0.9f, 0.9f);
    drawCircle(0.055f, 0.05f, 0.037f, 28); // Head

    // Comb (Red)
    glColor3f(0.9f, 0.2f, 0.2f);
    drawCircle(0.04f, 0.08f, 0.012f, 16);
    drawCircle(0.055f, 0.09f, 0.012f, 16);
    drawCircle(0.07f, 0.08f, 0.012f, 16);

    // Beak (Orange)
    glColor3f(1.0f, 0.45f, 0.1f);
    glBegin(GL_TRIANGLES);
    glVertex2f(0.09f, 0.05f);
    glVertex2f(0.115f, 0.055f);
    glVertex2f(0.09f, 0.065f);
    glEnd();

    // Eye (Black)
    glColor3f(0, 0, 0);
    drawCircle(0.064f, 0.058f, 0.006f, 14);

    glPopMatrix();
}

void drawBasket() {
    glPushMatrix();
    glTranslatef(basket.x, basket.y, 0);
    glColor4f(0, 0, 0, 0.15f);
    drawRect(0, -0.02f, basket.halfW * 0.95f, 0.02f);

    // Basket color
    glColor3f(0.65f, 0.35f, 0.15f);
    drawRect(0, 0.0f, basket.halfW, basket.h * 0.85f);

    glColor3f(0.5f, 0.25f, 0.1f);
    glLineWidth(2); // Darker lines
    glBegin(GL_LINES);
    for (float y = -basket.h * 0.7f; y <= basket.h * 0.7f;
         y += basket.h * 0.28f) {
        glVertex2f(-basket.halfW, y);
        glVertex2f(basket.halfW, y);
    }
    glEnd();

    glColor3f(0.4f, 0.2f, 0.05f); // Even darker top rim
    glBegin(GL_QUADS);
    glVertex2f(-basket.halfW, basket.h * 0.85f);
    glVertex2f(basket.halfW, basket.h * 0.85f);
    glVertex2f(basket.halfW * 0.9f, basket.h * 1.05f);
    glVertex2f(-basket.halfW * 0.9f, basket.h * 1.05f);
    glEnd();

    glPopMatrix();
}

void drawObj(const Falling& o) {
    glPushMatrix();
    glTranslatef(o.pos.x, o.pos.y, 0);
    glRotatef(o.rot, 0, 0, 1);
    if (o.type == ObjType::Bomb)
        bombShape(o);
    else
        eggShape(o);
    glPopMatrix();
}

void drawParticles() {
    for (const auto& p : parts) {
        glColor4f(p.r, p.g, p.b, p.a);
        drawCircle(p.p.x, p.p.y, p.size, 10);
    }
}

void drawFloatTexts() {
    for (const auto& ft : floatTexts) {
        glColor3f(ft.r, ft.g, ft.b);
        drawText(ft.p.x - 0.02f, ft.p.y, ft.s);
    }
}

void drawHUD() {
    glColor3f(0, 0, 0);
    drawText(-0.95f, 0.92f, "Score: " + std::to_string(score));
    drawText(-0.22f, 0.92f, "Time: " + std::to_string(timeLeft));
    drawText(0.60f, 0.92f, "Lives: " + std::to_string(lives));

    // --- Wind Indicator ---
    float startX = 0.3f;
    float textX = startX - 0.2f; // Position of "Wind:" text

    // FIX: Increase this value to push the arrow start further right
    float arrowCenterX = startX + 0.1f;

    float arrowY = 0.92f;
    float arrowLength = std::abs(windForce) / maxWindForce * 0.2f;

    // Set color based on strength (lighter for weaker, darker for stronger)
    float strength = std::abs(windForce) / maxWindForce;
    glColor3f(0.0f, 0.0f, 0.0f);
    if (strength > 0.5f)
        glColor3f(1.0f, 0.0f, 0.1f); // Red for stronger wind

    // Draw Text
    drawText(textX, arrowY, "Wind:");

    if (arrowLength > 0.01f) {
        glLineWidth(3.0f * strength + 1.0f);

        // Calculate the arrow's start and end points
        float lineStart, lineEnd;
        int direction = (windForce > 0) ? 1 : -1;

        // The line starts at arrowCenterX and extends in the direction
        lineStart = arrowCenterX;
        lineEnd = arrowCenterX + arrowLength * direction;

        glBegin(GL_LINES);
        // Arrow line
        glVertex2f(lineStart, arrowY + 0.01f);
        glVertex2f(lineEnd, arrowY + 0.01f);
        glEnd();

        // Arrowhead (simple triangle)
        glBegin(GL_TRIANGLES);
        if (windForce > 0) { // Right arrow
            glVertex2f(lineEnd, arrowY + 0.01f);
            glVertex2f(lineEnd - 0.02f, arrowY + 0.025f);
            glVertex2f(lineEnd - 0.02f, arrowY - 0.005f);
        } else { // Left arrow
            glVertex2f(lineEnd, arrowY + 0.01f);
            glVertex2f(lineEnd + 0.02f, arrowY + 0.025f);
            glVertex2f(lineEnd + 0.02f, arrowY - 0.005f);
        }
        glEnd();
    } else {
        glColor3f(0.2f, 0.5f, 0.2f); // Gentle green for no wind
        // Draw "Calm" near the arrow's starting position
        drawText(arrowCenterX - 0.04f, arrowY, "Calm");
    }
    // ------------------------------
}

void drawMenu() {
    glColor3f(0.1f, 0.1f, 0.1f);
    drawText(-0.36f, 0.72f, "CATCH THE EGGS");

    drawGradientBG();
    drawStaticFlowers(); // Draw flowers in menu
    chicken.y = 0.70f;
    drawChicken();
    drawBasket();

    for (auto& b : menuButtons) {
        bool isSel = false;
        // Simple black drop shadow
        glColor4f(0, 0, 0, 0.2f);
        drawRect((b.x1 + b.x2) / 2, (b.y1 + b.y2) / 2 - 0.008f,
                 (b.x2 - b.x1) / 2, (b.y2 - b.y1) / 2);
        // Changed button color to a solid, friendly green
        glColor3f(0.3f, 0.7f, 0.3f);
        drawRect((b.x1 + b.x2) / 2, (b.y1 + b.y2) / 2, (b.x2 - b.x1) / 2,
                 (b.y2 - b.y1) / 2);
        glColor3f(0, 0, 0);
        drawText((b.x1 + b.x2) / 2 - 0.06f, (b.y1 + b.y2) / 2 - 0.01f, b.label);
    }

    glColor3f(0, 0, 0);
    drawText(-0.2f, -0.42f, "High Score: " + std::to_string(highScore));
}

void updateGame(float dt) {
    // Chicken movement
    chicken.x += chicken.vx * dt;
    if (chicken.x > 0.82f) {
        chicken.x = 0.82f;
        chicken.vx *= -1;
    }
    if (chicken.x < -0.82f) {
        chicken.x = -0.82f;
        chicken.vx *= -1;
    }
    chicken.bob = 0.01f * sinf(glutGet(GLUT_ELAPSED_TIME) * 0.008f);

    // Update clouds movement
    for (auto& c : clouds) {
        c.pos.x += c.speed * dt;
        if (c.pos.x > worldR + c.scale * 0.15f) {
            c.pos.x = worldL - c.scale * 0.15f;
            c.pos.y = frand(0.5f, worldT - 0.1f);
            c.scale = frand(0.5f, 1.2f);
            c.speed = frand(0.02f, 0.08f);
        }
    }

    // --- Wind Update ---
    windChangeTimer += dt;
    if (windChangeTimer >= windChangeEvery) {
        // Change wind force to a new random value between -maxWindForce and
        // maxWindForce
        windForce = frand(-maxWindForce, maxWindForce);
        // Optional: Change wind change frequency too
        windChangeEvery = frand(3.0f, 6.0f);
        windChangeTimer = 0.0f;
    }

    // Object spawning
    spawnTimer += dt;
    if (spawnTimer >= spawnEvery) {
        objs.push_back(makeObj(getRandomObjType()));
        spawnTimer = 0;
        spawnEvery = std::max(0.35f, spawnEvery - 0.0025f);
    }

    // Object movement and collision
    for (auto& o : objs) {
        if (!o.active)
            continue;

        // --- Apply Wind Force to Horizontal Velocity ---
        // Apply a small damping factor (0.8) to the wind effect so it's not
        // instant
        o.pos.x += windForce * 0.8f * dt;

        o.pos.y += o.vy * dt;
        o.rot += o.rotSpd * dt;

        if (aabbCircleCollide(basket, o)) {
            o.active = false;
            switch (o.type) {
            case ObjType::NormalEgg:
                score += 1;
                addParticles(o.pos, 12, 1, 1, 0.9f);
                addFloatText(o.pos, "+1", 0, 0.6f, 0);
                break;
            case ObjType::BlueEgg:
                score += 5;
                addParticles(o.pos, 16, 0.4f, 0.6f, 1);
                addFloatText(o.pos, "+5", 0.1f, 0.45f, 1);
                break;
            case ObjType::GoldenEgg:
                score += 10;
                addParticles(o.pos, 20, 1.0f, 0.84f, 0.0f);
                addFloatText(o.pos, "+10", 0.95f, 0.7f, 0);
                break;
            case ObjType::Bomb:
                bombsCaught++;
                lives--;
                addParticles(o.pos, 30, 1.0f, 0.0f, 0.0f);
                addFloatText(o.pos, "BOMB!", 0.8f, 0.2f, 0.1f);
                shake = 0.12f;

                if (bombsCaught >= MAX_BOMBS_ALLOWED) {
                    screenState = Screen::GameOver;
                    highScore = std::max(highScore, score);
                }
                break;
            }
        }
        // Object misses basket
        if (o.pos.y < worldB - 0.25f)
            o.active = false;
    }
    // Clean up inactive objects
    objs.erase(std::remove_if(objs.begin(), objs.end(),
                              [](const Falling& f) { return !f.active; }),
               objs.end());

    // Particle update
    for (auto& p : parts) {
        p.p.x += p.v.x * dt;
        p.p.y += p.v.y * dt;
        p.v.y -= 1.6f * dt;
        p.life -= dt;
        p.a = std::max(0.0f, p.life / p.maxLife);
    }
    parts.erase(std::remove_if(parts.begin(), parts.end(),
                               [](const Particle& p) { return p.life <= 0; }),
                parts.end());

    // FloatText update
    for (auto& ft : floatTexts) {
        ft.p.y += ft.vy * dt;
        ft.life -= dt;
    }
    floatTexts.erase(
        std::remove_if(floatTexts.begin(), floatTexts.end(),
                       [](const FloatText& t) { return t.life <= 0; }),
        floatTexts.end());

    // Screen shake update
    if (shake > 0) {
        shake = std::max(0.0f, shake - dt * 0.7f);
    }

    // Time update
    static float accu = 0;
    accu += dt;
    if (accu >= 1.0f) {
        timeLeft -= 1;
        accu = 0;
        if (timeLeft <= 0) {
            screenState = Screen::GameOver;
            highScore = std::max(highScore, score);
        }
    }
}

void display() {
    // Changed clear color to a very light, off-white/cream
    glClearColor(0.75f, 0.85f, 0.70f, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    glLoadIdentity();

    if (shake > 0) {
        float ox = frand(-shake, shake) * 0.015f;
        float oy = frand(-shake, shake) * 0.015f;
        glTranslatef(ox, oy, 0);
    }

    if (screenState == Screen::Menu) {
        drawMenu();
    } else if (screenState == Screen::Help) {
        drawGradientBG();
        drawStaticFlowers(); // Draw flowers
        glColor3f(0, 0, 0);
        drawText(-0.95f, 0.8f, "Help:");
        drawText(-0.95f, 0.68f, "- Move: Left/Right or A/D or Mouse");
        drawText(-0.95f, 0.58f,
                 "- Eggs: +1 (Normal), +5 (Blue), +10 (Golden).");
        drawText(-0.95f, 0.48f,
                 "- Bomb: Lose 1 Life (Game Over at " +
                     std::to_string(MAX_BOMBS_ALLOWED) + " Bombs)");
        drawText(-0.95f, 0.38f,
                 "- Space: Pause/Resume, Enter: Select, Esc: Back");
        drawText(-0.95f, -0.90f, "Click anywhere to return to Menu.");
    } else if (screenState == Screen::Playing ||
               screenState == Screen::Paused) {
        drawGradientBG();
        drawStaticFlowers(); // Draw flowers in game
        // Changed crossbar color to a darker brown
        glColor3f(0.5f, 0.35f, 0.15f);
        drawRect(0, 0.65f, 0.92f, 0.018f);
        chicken.y = 0.70f;
        drawChicken();
        for (const auto& o : objs)
            drawObj(o);
        drawBasket();
        drawParticles();
        drawHUD();
        drawFloatTexts();

        if (screenState == Screen::Paused) {
            glColor4f(0, 0, 0, 0.6f);
            drawRect(0, 0, 0.7f, 0.25f);
            glColor3f(1, 1, 1);
            drawText(-0.11f, 0.02f, "PAUSED");
            drawText(-0.38f, -0.06f, "Press Space to Resume, Esc for Menu");
        }
    } else if (screenState == Screen::GameOver) {
        drawGradientBG();
        drawStaticFlowers(); // Draw flowers in game over
        glColor4f(0, 0, 0, 0.7f);
        drawRect(0, 0, 0.82f, 0.35f);
        glColor3f(1, 1, 1);
        drawText(-0.16f, 0.10f, "GAME OVER");

        std::string gameOverMsg;
        if (timeLeft <= 0) {
            gameOverMsg = "Time is up!";
        } else if (bombsCaught >= MAX_BOMBS_ALLOWED) {
            gameOverMsg =
                "Caught " + std::to_string(MAX_BOMBS_ALLOWED) + " Bombs!";
        } else {
            gameOverMsg = "Game Over!";
        }
        drawText(-0.32f, 0.02f, gameOverMsg);
        drawText(-0.32f, -0.08f,
                 "Final Score: " + std::to_string(score) +
                     " Best: " + std::to_string(highScore));
        drawText(-0.44f, -0.16f, "Click left = Menu Click right = Restart");
    }

    glutSwapBuffers();
}

void reshape(int w, int h) {
    winW = w;
    winH = h;
    glViewport(0, 0, w, h);
    ortho();
}

void keyboard(unsigned char key, int, int) {
    if (key == 27) { // Esc
        if (screenState == Screen::Playing || screenState == Screen::Paused)
            screenState = Screen::Menu;
        else
            exit(0);
    } else if (key == ' ') { // Spacebar for Pause/Resume
        if (screenState == Screen::Playing)
            screenState = Screen::Paused;
        else if (screenState == Screen::Paused)
            screenState = Screen::Playing;
    } else if (key == 's' || key == 'S') {
        if (screenState == Screen::GameOver) {
            startGame();
        }
    } else if (key == 13) { // Enter
        if (screenState == Screen::Menu) {
            startGame();
        } else if (screenState == Screen::Help ||
                   screenState == Screen::GameOver) {
            screenState = Screen::Menu;
        }
    }
}

void special(int key, int, int) {
    if (screenState == Screen::Menu) {
        if (key == GLUT_KEY_UP)
            menuIndex = (menuIndex + 3) % 4;
        if (key == GLUT_KEY_DOWN)
            menuIndex = (menuIndex + 1) % 4;
    } else if (screenState == Screen::Playing) {
        if (key == GLUT_KEY_LEFT)
            basket.x -= 0.08f;
        if (key == GLUT_KEY_RIGHT)
            basket.x += 0.08f;
        basket.x = std::max(worldL + basket.halfW,
                            std::min(worldR - basket.halfW, basket.x));
    }
}

void passiveMotion(int mx, int) {
    if (screenState == Screen::Playing) {
        float norm = mx / (float)winW;
        float wx = worldL + norm * (worldR - worldL);
        basket.x = std::max(worldL + basket.halfW,
                            std::min(worldR - basket.halfW, wx));
    }
}

void timerFunc(int) {
    int now = glutGet(GLUT_ELAPSED_TIME);
    float dt = (now - lastTick) / 1000.0f;
    lastTick = now;
    dt = std::min(dt, 0.033f);

    if (screenState == Screen::Playing) {
        updateGame(dt);
    }
    glutPostRedisplay();
    glutTimerFunc(16, timerFunc, 0);
}

int main(int argc, char** argv) {
    srand((unsigned)time(nullptr));
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
    glutInitWindowSize(winW, winH);
    glutCreateWindow("Catch The Eggs - Static Flowers");
    ortho();
    enableSmooth();

    setupMenuButtons();
    // Initialize clouds
    for (int i = 0; i < 5; ++i) {
        clouds.push_back({{frand(worldL, worldR), frand(0.5f, worldT - 0.1f)},
                          frand(0.5f, 1.2f),
                          frand(0.02f, 0.08f)});
    }

    // Initialize flowers ONCE
    initializeFlowers();

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(special);
    glutPassiveMotionFunc(passiveMotion);
    glutMouseFunc(mouseClick);

    lastTick = glutGet(GLUT_ELAPSED_TIME);
    glutTimerFunc(16, timerFunc, 0);

    glutMainLoop();
    return 0;
}
