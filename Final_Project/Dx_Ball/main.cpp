#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#endif
#ifdef __APPLE__
#include <GLUT/glut.h>
#include <OpenGL/glu.h>
#else
#include <GL/glu.h>
#include <GL/glut.h>
#endif

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// --- Custom Utility Functions ---
template <typename T> static inline T clampv(T v, T lo, T hi) {
    return (v < lo) ? lo : (v > hi) ? hi : v;
}
static inline int iround(float x) {
    return (int)((x >= 0.f) ? (x + 0.5f) : (x - 0.5f));
}

// --- Vector Math ---
struct Vec2 {
    float x = 0.f, y = 0.f;
};
static inline Vec2 operator+(Vec2 a, Vec2 b) { return {a.x + b.x, a.y + b.y}; }
// FIX APPLIED HERE: Changed b.b to b.y
static inline Vec2 operator-(Vec2 a, Vec2 b) { return {a.x - b.x, a.y - b.y}; }
static inline Vec2 operator*(Vec2 a, float s) { return {a.x * s, a.y * s}; }
static inline float dot(Vec2 a, Vec2 b) { return a.x * b.x + a.y * b.y; }
static inline float length(Vec2 a) { return std::sqrt(dot(a, a)); }
static inline Vec2 normalize(Vec2 a) {
    float L = length(a);
    return (L > 1e-6f) ? Vec2{a.x / L, a.y / L} : Vec2{1.f, 0.f};
}

// --- Globals ---
static int scrW = 900, scrH = 700;
static float nowSec() { return glutGet(GLUT_ELAPSED_TIME) / 1000.0f; }
static std::mt19937 rng(1234567u);
static std::uniform_real_distribution<float> u01(0.f, 1.f);
static int currentLevel = 1; // Tracks the selected level (1-5)
static const int MAX_LEVELS = 5;

// Placeholder for SFX/Music (Assume external implementation/library like OpenAL
// or SDL_mixer)
static void playSFX(const char*, bool = false, float = 1.0f) {}
static void playMusic(const char*) {}

// --- Low-Level Drawing Primitives ---
static inline void lab_draw_pixel(int x, int y) {
    glBegin(GL_POINTS);
    glVertex2i(x, y);
    glEnd();
}
static void lab_draw_line(int x1, int y1, int x2, int y2) {
    int dx = std::abs(x2 - x1), dy = std::abs(y2 - y1);
    int sx = (x1 < x2) ? 1 : -1;
    int sy = (y1 < y2) ? 1 : -1;
    int err = dx - dy;
    int x = x1, y = y1;
    for (;;) {
        lab_draw_pixel(x, y);
        if (x == x2 && y == y2)
            break;
        int e2 = err << 1;
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
static void lab_midpoint_circle(int cx, int cy, int r) {
    int x = 0, y = r;
    int d = 1 - r;
    auto oct = [&](int x, int y) {
        lab_draw_pixel(cx + x, cy + y);
        lab_draw_pixel(cx - x, cy + y);
        lab_draw_pixel(cx + x, cy - y);
        lab_draw_pixel(cx - x, cy - y);
        lab_draw_pixel(cx + y, cy + x);
        lab_draw_pixel(cx - y, cy + x);
        lab_draw_pixel(cx + y, cy - x);
        lab_draw_pixel(cx - y, cy - x);
    };
    oct(x, y);
    while (y > x) {
        if (d < 0) {
            d += (x << 1) + 3;
        } else {
            d += ((x - y) << 1) + 5;
            --y;
        }
        ++x;
        oct(x, y);
    }
}

// --- Enums ---
enum Screen {
    MENU,
    LEVEL_SELECT,
    PLAY,
    PAUSE,
    HELP,
    HIGHSCORES,
    WIN,
    GAMEOVER
};
enum PerkType {
    EXTRA_LIFE,
    SPEED_UP,
    WIDE_PADDLE,
    SHRINK_PADDLE,
    THROUGH_BALL,
    FIREBALL,
    INSTANT_DEATH,
    SHOOTING_PADDLE
};

// --- Structures ---
struct Brick {
    float x, y, w, h;
    bool alive;
    int hp;
    float r, g, b;
    int score;
};
struct Perk {
    Vec2 pos, vel;
    float size;
    PerkType type;
    bool alive;
};
struct Bullet {
    Vec2 pos, vel;
    float w, h;
    bool alive;
};

struct Ball {
    Vec2 pos, vel;
    float speed, radius;
    bool stuck;
    bool through;
    float throughTimer;
    bool fireball;
    float fireballTimer;
};
struct Paddle {
    Vec2 pos;
    float w, h;
    float speed;
    float widthTimer;
    bool shooting;
    float shootingTimer;
};
struct Run {
    float t;
    int s;
}; // For High Scores

// --- Game State ---
static Screen current = MENU;
static std::vector<Brick> bricks;
static std::vector<Perk> perks;
static std::vector<Bullet> bullets;
static Ball ball;
static Paddle paddle;
static int lives = 3, score = 0;
static float startTime = 0.f, playTime = 0.f, lastTick = 0.f;
static bool leftHeld = false, rightHeld = false, hasLaunched = false;
static bool canResume = false;
static int menuIndex = 0;
static float globalSpeedGain = 0.f;

static bool haveBest = false;
static int bestScore = 0;
static float bestTime = 0.f;
static std::vector<Run> history;

static const int MAX_LIVES = 5;

// Stores the last few ball positions for the trail effect
static std::vector<Vec2> ballTrail;
static const int TRAIL_LENGTH = 12; // Number of trail segments

// --- Basic Drawing Utilities ---
static void drawRect(float cx, float cy, float w, float h) {
    float x0 = cx - w / 2.f, x1 = cx + w / 2.f, y0 = cy - h / 2.f,
          y1 = cy + h / 2.f;
    glBegin(GL_QUADS);
    glVertex2f(x0, y0);
    glVertex2f(x1, y0);
    glVertex2f(x1, y1);
    glVertex2f(x0, y1);
    glEnd();
}
static void drawCircleFilled(float cx, float cy, float r, int seg = 28) {
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(cx, cy);
    for (int i = 0; i <= seg; i++) {
        float th = (float)i * (float)(2.0 * M_PI) / seg;
        glVertex2f(cx + cosf(th) * r, cy + sinf(th) * r);
    }
    glEnd();
}
static void drawText(float x, float y, const std::string& s,
                     void* font = GLUT_BITMAP_HELVETICA_18) {
    glRasterPos2f(x, y);
    for (size_t i = 0; i < s.size(); ++i)
        glutBitmapCharacter(font, s[i]);
}

// --- Score/History Utilities ---
static void saveHighScore() { history.push_back({playTime, score}); }
static void loadBest() {
    haveBest = false;
    bestScore = 0;
    bestTime = 0.f;
    for (const auto& r : history) {
        if (!haveBest || r.s > bestScore ||
            (r.s == bestScore && r.t < bestTime)) {
            haveBest = true;
            bestScore = r.s;
            bestTime = r.t;
        }
    }
}
static void resetBallOnPaddle() {
    ball.stuck = true;
    hasLaunched = false;
    ball.through = false;
    ball.throughTimer = 0.f;
    ball.fireball = false;
    ball.fireballTimer = 0.f;
    ball.speed = 320.f + globalSpeedGain;
    ball.pos = {paddle.pos.x,
                paddle.pos.y + paddle.h / 2.f + ball.radius + 1.f};
    ball.vel = {0.f, 1.f};
}

// --- Level Layouts (The core of the "Feature Game") ---
static void
buildBricks(int level, int rows = 8,
            int cols = 14) { // Increased rows/cols for better shapes
    bricks.clear();
    float marginX = 70.f, marginY = 100.f, gap = 4.f; // Reduced gap
    float areaW = scrW - 2 * marginX;
    float bw = (areaW - (cols - 1) * gap) / cols;
    float bh = 22.f;

    // Difficulty scaling: speed, health, and complexity
    globalSpeedGain = 0.f + (level - 1) * 35.f;
    ball.speed = 320.f + globalSpeedGain;

    for (int r = 0; r < rows; r++) {
        for (int c = 0; c < cols; c++) {
            Brick b;
            b.x = marginX + c * (bw + gap) + bw / 2.f;
            b.y = scrH - marginY - r * (bh + gap) - bh / 2.f;
            b.w = bw;
            b.h = bh;
            b.alive = true;
            b.score = 50 + 10 * r;
            b.hp = 1; // Default

            // --- LEVEL DESIGN LOGIC ---
            bool skip = false;

            if (level == 1) { // LVL 1 (Easy): Simple, high chance of power-ups
                b.hp = 1;
            } else if (level == 2) { // LVL 2 (Checkerboard): Introduction to
                                     // 2HP bricks and gaps
                b.hp = (r < 3) ? 2 : 1;
                if ((r + c) % 2 != 0)
                    skip = true;
            } else if (level == 3) { // LVL 3 (The Heart)
                b.hp = 2;            // All 2-hit bricks
                if (r < 2 && (c < 3 || c > 10))
                    skip = true;
                if (r == 2 && (c == 0 || c == 13))
                    skip = true;
                if (r >= 5 && (c < r - 5 || c > 18 - r))
                    skip = true;
                if (r == 7 && (c < 3 || c > 10))
                    skip = true;
                if (r > 7)
                    skip = true; // Only use 8 rows (0-7) for the heart shape

                if (!skip) {
                    b.r = 0.9f;
                    b.g = 0.2f;
                    b.b = 0.4f;
                } // Pink/Red heart color
            } else if (level == 4) { // LVL 4 (The Star)
                b.hp = 3;            // All 3-hit bricks
                int centerC = cols / 2;
                int distC = std::abs(c - centerC);
                int distR = std::abs(r - 4);

                if (distC + distR > 6 || distC < 1 || distR < 1)
                    skip = true; // Creates a general star shape
                if (distC > 4 && distR > 2)
                    skip = true;

                if (!skip) {
                    b.hp = (distC + distR < 3) ? 999 : 3; // Indestructible core
                    b.r = (b.hp == 999) ? 0.9f : 0.8f;
                    b.g = (b.hp == 999) ? 0.9f : 0.6f;
                    b.b = (b.hp == 999) ? 0.1f : 0.2f; // Yellow/Gold star color
                    b.score = (b.hp == 999) ? 0 : 150;
                }
            } else if (level == 5) { // LVL 5 (The Happy Emoji)
                b.hp = 1;
                if (r < 1 || r > 6 || c < 1 || c > 12)
                    skip = true; // Circular Bounding Box

                // Base face color: 1HP
                if (!skip) {
                    b.r = 1.0f;
                    b.g = 0.8f;
                    b.b = 0.2f;
                }

                // Outline and features: Indestructible (999 HP)
                bool outline = (r == 1 || r == 6 || c == 1 || c == 12);
                bool eye1 = (r == 2 && c >= 4 && c <= 5);
                bool eye2 = (r == 2 && c >= 8 && c <= 9);
                bool mouth = (r == 4 && c >= 5 && c <= 8);

                if (outline || eye1 || eye2) {
                    b.hp = 999;
                    b.r = 0.1f;
                    b.g = 0.1f;
                    b.b = 0.1f; // Black/Dark Grey
                    b.score = 0;
                }
                // The Target/Hole: Make the middle of the mouth the only
                // breakable part
                if (mouth) {
                    b.hp = 1;
                    b.r = 0.9f;
                    b.g = 0.0f;
                    b.b = 0.0f; // Red target
                    b.score = 500;
                }
                if (r == 4 && (c == 6 || c == 7)) {
                    skip = false;
                    b.hp = 1;
                    b.r = 0.9f;
                    b.g = 0.0f;
                    b.b = 0.0f;
                    b.score = 500;
                }
            }

            if (skip)
                continue;

            // --- Final Color Adjustment based on HP if not set above ---
            if (b.hp == 999 && level < 5) {
                b.r = 0.1f;
                b.g = 0.1f;
                b.b = 0.1f;
                b.score = 0;
            } // Indestructible (Black/Dark Grey)
            else if (b.hp == 3 && level < 5) {
                b.r = 0.4f;
                b.g = 0.4f;
                b.b = 0.4f;
            } // Metal/Dark Grey
            else if (b.hp == 2 && level < 3) {
                b.r = 0.8f;
                b.g = 0.2f;
                b.b = 0.2f;
            } // Red/Strong
            else if (b.hp == 1 && level < 3) {
                b.r = 0.2f + 0.13f * r;
                b.g = 0.4f + 0.05f * c;
                b.b = 0.8f - 0.08f * r;
            } // Normal Blue/Colored

            bricks.push_back(b);
        }
    }
}

// --- New Game Initialization ---
static void newGame() {
    score = 0;
    lives = 3;
    globalSpeedGain = 0.f;
    perks.clear();
    bullets.clear();
    paddle.pos = {scrW / 2.f, 48.f};
    paddle.w = 120.f;
    paddle.h = 16.f;
    paddle.speed = 630.f;
    paddle.widthTimer = 0.f;
    paddle.shooting = false;
    paddle.shootingTimer = 0.f;
    ball.radius = 9.f;
    ball.speed = 320.f;
    ball.stuck = true;
    ball.through = false;
    ball.fireball = false;
    resetBallOnPaddle();

    // START HERE: Go to Level Selection
    current = LEVEL_SELECT;
    canResume = false;
    playMusic("assets/music_loop.ogg");
}

// --- Collision Logic ---
static void fireBullet() {
    if (!paddle.shooting)
        return;
    Bullet b;
    b.pos = {paddle.pos.x, paddle.pos.y + paddle.h / 2.f + 8.f};
    b.vel = {0, 640.f};
    b.w = 4.f;
    b.h = 10.f;
    b.alive = true;
    bullets.push_back(b);
    playSFX("pew");
}

static bool aabbCircleCollision(float rx, float ry, float rw, float rh, Vec2 c,
                                float r, Vec2* nrm, float* pen) {
    float cx = clampv(c.x, rx - rw / 2.f, rx + rw / 2.f);
    float cy = clampv(c.y, ry - rh / 2.f, ry + rh / 2.f);
    float dx = c.x - cx, dy = c.y - cy;
    float d2 = dx * dx + dy * dy;
    if (d2 > r * r)
        return false;
    float d = std::sqrt(d2 < 1e-6f ? 1e-6f : d2);
    if (nrm) {
        if (d > 1e-4f)
            *nrm = {dx / d, dy / d};
        else
            *nrm = {0.f, 1.f};
    }
    if (pen)
        *pen = r - d;
    return true;
}

static void reflectBall(Vec2 n) {
    Vec2 v = ball.vel;
    float sp = length(v);
    if (sp < 1e-6f)
        return;
    Vec2 dir = v * (1.f / sp);
    Vec2 r = dir - n * (2.f * dot(dir, n));
    ball.vel = normalize(r) * ball.speed;
}

static void loseLife() {
    if (lives > 0)
        lives--;
    playSFX("lose");
    if (lives <= 0) {
        lives = 0;
        current = GAMEOVER;
        saveHighScore();
        canResume = false;
    } else {
        paddle.pos.x = scrW / 2.f;
        paddle.w = 120.f;
        paddle.widthTimer = 0.f;
        paddle.shooting = false;
        paddle.shootingTimer = 0.f;
        resetBallOnPaddle();
    }
}

static void maybeSpawnPerk(const Brick& b) {
    float p = 0.22f; // Base chance
    if (b.hp == 999)
        return; // Never spawn on indestructible
    if (u01(rng) < p) {
        Perk pk;
        pk.pos = {b.x, b.y};
        pk.vel = {0, -150.f};
        pk.size = 18.f;
        pk.alive = true;
        float r = u01(rng);
        if (r < 0.18f)
            pk.type = EXTRA_LIFE;
        else if (r < 0.36f)
            pk.type = SPEED_UP;
        else if (r < 0.52f)
            pk.type = WIDE_PADDLE;
        else if (r < 0.66f)
            pk.type = SHRINK_PADDLE;
        else if (r < 0.78f)
            pk.type = THROUGH_BALL;
        else if (r < 0.90f)
            pk.type = FIREBALL;
        else if (r < 0.96f)
            pk.type = SHOOTING_PADDLE;
        else
            pk.type = INSTANT_DEATH;
        perks.push_back(pk);
    }
}

static void applyPerk(PerkType t) {
    switch (t) {
    case EXTRA_LIFE:
        lives = (lives < MAX_LIVES ? lives + 1 : MAX_LIVES);
        playSFX("extra_life");
        break;
    case SPEED_UP:
        ball.speed *= 1.18f;
        playSFX("speed");
        break;
    case WIDE_PADDLE:
        paddle.w = (paddle.w * 1.35f < 320.f ? paddle.w * 1.35f : 320.f);
        paddle.widthTimer = 14.f;
        playSFX("wide");
        break;
    case SHRINK_PADDLE:
        paddle.w = (paddle.w * 0.7f > 60.f ? paddle.w * 0.7f : 60.f);
        paddle.widthTimer = 12.f;
        playSFX("shrink");
        break;
    case THROUGH_BALL:
        ball.through = true;
        ball.throughTimer = 10.f;
        playSFX("through");
        break;
    case FIREBALL:
        ball.fireball = true;
        ball.fireballTimer = 8.f;
        ball.through = true;
        if (ball.throughTimer < 8.f)
            ball.throughTimer = 8.f;
        playSFX("fireball");
        break;
    case INSTANT_DEATH:
        lives = 0;
        current = GAMEOVER;
        saveHighScore();
        canResume = false;
        break;
    case SHOOTING_PADDLE:
        paddle.shooting = true;
        paddle.shootingTimer = 12.f;
        playSFX("shoot");
        break;
    }
}

static void updateGame(float dt) {
    globalSpeedGain += dt * 2.f;
    ball.speed += dt * 4.f;
    if (ball.through) {
        ball.throughTimer -= dt;
        if (ball.throughTimer <= 0) {
            ball.through = false;
        }
    }
    if (ball.fireball) {
        ball.fireballTimer -= dt;
        if (ball.fireballTimer <= 0) {
            ball.fireball = false;
        }
    }
    if (paddle.widthTimer > 0) {
        paddle.widthTimer -= dt;
        if (paddle.widthTimer <= 0) {
            paddle.widthTimer = 0;
            paddle.w = 120.f;
        }
    }
    if (paddle.shooting) {
        paddle.shootingTimer -= dt;
        if (paddle.shootingTimer <= 0) {
            paddle.shooting = false;
        }
    }

    float vx = 0.f;
    if (leftHeld)
        vx -= paddle.speed;
    if (rightHeld)
        vx += paddle.speed;
    paddle.pos.x += vx * dt;
    paddle.pos.x =
        clampv(paddle.pos.x, paddle.w / 2.f + 6.f, scrW - paddle.w / 2.f - 6.f);

    if (ball.stuck) {
        ball.pos.x = paddle.pos.x;
        ball.pos.y = paddle.pos.y + paddle.h / 2.f + ball.radius + 1.f;
    } else {
        ball.pos = ball.pos + ball.vel * dt;

        // Wall Reflection
        if (ball.pos.x - ball.radius < 0) {
            ball.pos.x = ball.radius;
            ball.vel.x = std::fabs(ball.vel.x);
            playSFX("wall");
        }
        if (ball.pos.x + ball.radius > scrW) {
            ball.pos.x = scrW - ball.radius;
            ball.vel.x = -std::fabs(ball.vel.x);
            playSFX("wall");
        }
        if (ball.pos.y + ball.radius > scrH) {
            ball.pos.y = scrH - ball.radius;
            ball.vel.y = -std::fabs(ball.vel.y);
            playSFX("wall");
        }

        // Game Over Check
        if (ball.pos.y - ball.radius < 0) {
            loseLife();
            return;
        }

        // Paddle Collision
        Vec2 n;
        float pen;
        if (aabbCircleCollision(paddle.pos.x, paddle.pos.y, paddle.w, paddle.h,
                                ball.pos, ball.radius, &n, &pen)) {
            ball.pos = ball.pos + n * pen;
            float rel = (ball.pos.x - paddle.pos.x) / (paddle.w / 2.f);
            rel = clampv(rel, -1.f, 1.f);
            Vec2 dir = normalize(Vec2{rel, 1.2f});
            ball.vel = dir * ball.speed;
            ball.vel.y = std::fabs(ball.vel.y);
            playSFX("paddle");
        }

        // Brick Collision
        for (size_t i = 0; i < bricks.size(); ++i) {
            Brick& b = bricks[i];
            if (!b.alive)
                continue;
            Vec2 bn;
            float bpen;
            if (aabbCircleCollision(b.x, b.y, b.w, b.h, ball.pos, ball.radius,
                                    &bn, &bpen)) {
                if (b.hp == 999) { // Indestructible brick (only reflect)
                    ball.pos = ball.pos + bn * bpen;
                    reflectBall(bn);
                    playSFX("wall");
                    continue;
                }

                int before = b.hp;
                b.hp -= 1;
                score += b.score;
                playSFX("brick");

                if (before > 0 && b.hp <= 0) {
                    b.alive = false;
                    maybeSpawnPerk(b);
                }

                // Ball reflection only if NOT in through/fireball mode
                if (!(ball.through || ball.fireball)) {
                    ball.pos = ball.pos + bn * bpen;
                    reflectBall(bn);
                }
            }
        }
    }

    // Perk Movement and Collection
    for (size_t i = 0; i < perks.size(); ++i) {
        Perk& p = perks[i];
        if (!p.alive)
            continue;
        p.pos = p.pos + p.vel * dt;
        if (p.pos.y < -30.f) {
            p.alive = false;
            continue;
        }

        // Collision check with paddle (AABB vs AABB approximation)
        if (std::fabs(p.pos.x - paddle.pos.x) <=
                (paddle.w / 2.f + p.size / 2.f) &&
            std::fabs(p.pos.y - paddle.pos.y) <=
                (paddle.h / 2.f + p.size / 2.f)) {
            p.alive = false;
            applyPerk(p.type);
            if (lives <= 0) {
                return;
            }
        }
    }

    // Bullet Movement and Collision
    for (size_t i = 0; i < bullets.size(); ++i) {
        Bullet& bu = bullets[i];
        if (!bu.alive)
            continue;
        bu.pos = bu.pos + bu.vel * dt;
        if (bu.pos.y > scrH + 20.f) {
            bu.alive = false;
            continue;
        }

        for (size_t j = 0; j < bricks.size(); ++j) {
            Brick& br = bricks[j];
            if (!br.alive)
                continue;
            // Collision check with brick (AABB vs AABB)
            if (std::fabs(bu.pos.x - br.x) <= (br.w / 2.f) &&
                std::fabs(bu.pos.y - br.y) <= (br.h / 2.f)) {
                if (br.hp == 999) { // Indestructible brick (bullet breaks but
                                    // brick does not)
                    bu.alive = false;
                    playSFX("wall");
                    break;
                }

                bu.alive = false;
                int before = br.hp;
                br.hp -= 1;
                score += br.score;
                playSFX("brick");
                if (before > 0 && br.hp <= 0) {
                    br.alive = false;
                    maybeSpawnPerk(br);
                }
                break;
            }
        }
    }

    // Level Win Check (MODIFIED for multi-level)
    bool any = false;
    for (size_t i = 0; i < bricks.size(); ++i) {
        // Only check for breakable bricks (hp < 999)
        if (bricks[i].alive && bricks[i].hp < 999) {
            any = true;
            break;
        }
    }
    if (!any) {
        if (currentLevel < MAX_LEVELS) {
            currentLevel++;
            buildBricks(currentLevel); // Load the next level
            resetBallOnPaddle();
            // TODO: Add a brief 'Level Up' message overlay here
        } else {
            current = WIN;
            saveHighScore();
            canResume = false;
        }
    }
}

// --- Drawing and Rendering ---
static void updateBallTrail() {
    if (ball.fireball || ball.through) {
        ballTrail.insert(ballTrail.begin(), ball.pos);
        if (ballTrail.size() > TRAIL_LENGTH) {
            ballTrail.pop_back();
        }
    } else {
        // Clear trail quickly when effect ends
        if (ballTrail.size() > 0)
            ballTrail.pop_back();
    }
}

static void drawBallTrail() {
    if (ballTrail.empty())
        return;

    for (size_t i = 0; i < ballTrail.size(); ++i) {
        const Vec2& p = ballTrail[i];

        // Calculate size and alpha based on position in trail (fading out)
        float alpha = 1.0f - (float)i / TRAIL_LENGTH;
        float radius = ball.radius * (1.0f - (float)i / TRAIL_LENGTH) * 0.7f;

        // Lava/Fireball mode effect color
        glColor4f(1.0f, 0.45f + 0.55f * alpha, 0.15f, alpha);

        drawCircleFilled(p.x, p.y, radius, 12);
    }
}

static void drawPerkIcon(PerkType t, float x, float y, float s) {
    glPushMatrix();
    glTranslatef(x, y, 0);
    glScalef(s, s, 1);
    switch (t) {
    case EXTRA_LIFE:
        glColor3f(1, 0.3f, 0.3f);
        glBegin(GL_TRIANGLES);
        glVertex2f(0, 0.9f);
        glVertex2f(-0.9f, 0);
        glVertex2f(0.9f, 0);
        glEnd();
        break;
    case SPEED_UP:
        glColor3f(1, 1, 0.2f);
        glBegin(GL_TRIANGLES);
        glVertex2f(-0.3f, 0.9f);
        glVertex2f(0.1f, 0.1f);
        glVertex2f(-0.1f, 0.1f);
        glVertex2f(0.3f, -0.9f);
        glVertex2f(-0.1f, -0.1f);
        glVertex2f(0.1f, -0.1f);
        glEnd();
        break;
    case WIDE_PADDLE:
        glColor3f(0.3f, 1, 0.3f);
        drawRect(0, 0, 1.6f, 0.35f);
        break;
    case SHRINK_PADDLE:
        glColor3f(1, 0.5f, 0.1f);
        drawRect(0, 0, 0.8f, 0.35f);
        break;
    case THROUGH_BALL:
        glColor3f(0.4f, 0.8f, 1.0f);
        drawCircleFilled(0, 0, 0.8f, 26);
        glColor3f(0, 0, 0);
        drawCircleFilled(0, 0, 0.55f, 26);
        break;
    case FIREBALL:
        glColor3f(1.0f, 0.5f, 0.0f);
        glBegin(GL_TRIANGLE_FAN);
        for (int i = 0; i < 16; i++) {
            float th = i * (float)(2.0 * M_PI / 16);
            glVertex2f(cosf(th) * 0.8f, sinf(th) * 0.8f);
        }
        glEnd();
        break;
    case INSTANT_DEATH:
        glColor3f(0.8f, 0.0f, 0.0f);
        drawRect(0, 0, 0.7f, 1.2f);
        break;
    case SHOOTING_PADDLE:
        glColor3f(0.9f, 0.9f, 0.2f);
        glBegin(GL_TRIANGLES);
        glVertex2f(0, -0.9f);
        glVertex2f(-0.5f, 0.2f);
        glVertex2f(0.5f, 0.2f);
        glEnd();
        break;
    }
    glPopMatrix();
}

static void renderHUD() {
    glColor3f(1, 1, 1);
    drawText(10, scrH - 24, std::string("Score: ") + std::to_string(score));
    drawText(10, scrH - 48, std::string("Lives: ") + std::to_string(lives));

    // Display Current Level
    drawText(10, scrH - 72,
             std::string("Level: ") + std::to_string(currentLevel));

    float tNow = nowSec();
    if (current == PLAY)
        playTime += (tNow - lastTick);
    lastTick = tNow;
    char buf[64];
    std::snprintf(buf, sizeof(buf), "Time: %.1fs", playTime);
    drawText(scrW - 160, scrH - 24, buf);

    int y = scrH - 72;
    char pbuf[64];
    if (ball.through) {
        std::snprintf(pbuf, sizeof(pbuf), "Through: %ds",
                      (int)std::ceil(ball.throughTimer));
        drawText(scrW - 200, y, pbuf);
        y -= 22;
    }
    if (ball.fireball) {
        std::snprintf(pbuf, sizeof(pbuf), "Fireball: %ds",
                      (int)std::ceil(ball.fireballTimer));
        drawText(scrW - 200, y, pbuf);
        y -= 22;
    }
    if (paddle.shooting) {
        std::snprintf(pbuf, sizeof(pbuf), "Shooting: %ds",
                      (int)std::ceil(paddle.shootingTimer));
        drawText(scrW - 200, y, pbuf);
        y -= 22;
    }
}

static void renderLevelSelect() {
    glColor3f(1.f, 1.f, 1.f);
    drawText(scrW / 2.f - 140, scrH - 120, "SELECT LEVEL - START YOUR GAME",
             GLUT_BITMAP_TIMES_ROMAN_24);

    float centerX = scrW / 2.f;
    float startY = scrH / 2.f + 50;
    float buttonW = 150.f;
    float buttonH = 40.f;
    float spacing = 20.f;

    // Draw 5 Level Buttons
    for (int i = 0; i < MAX_LEVELS; ++i) {
        float x = centerX + (i - 2.f) * (buttonW + spacing);
        float y = startY;

        // Highlight selected level
        if (i + 1 == currentLevel) {
            glColor3f(0.8f, 1.0f, 0.2f); // Bright yellow/green highlight
            drawRect(x, y, buttonW + 10.f, buttonH + 10.f); // Bigger background
        }

        // Draw the button body
        glColor3f(0.2f, 0.4f, 0.8f); // Blue
        drawRect(x, y, buttonW, buttonH);

        // Draw text
        glColor3f(1.f, 1.f, 1.f);
        std::string levelText = "LVL " + std::to_string(i + 1);
        drawText(x - 30, y - 5, levelText, GLUT_BITMAP_HELVETICA_18);
    }

    // Draw Difficulty Key
    glColor3f(1.f, 1.f, 1.f);
    drawText(40, 60, "Press Left/Right/Click to choose, ENTER/Click to start.",
             GLUT_BITMAP_HELVETICA_18);

    // Show current level description
    int level = currentLevel;
    float descY = startY - 80;
    glColor3f(0.9f, 0.9f, 0.9f);
    std::string diff;
    std::string desc;

    if (level == 1) {
        diff = "EASY";
        desc = "Standard layout, low speed gain.";
        glColor3f(0.2f, 1.0f, 0.2f);
    } else if (level == 2) {
        diff = "NORMAL";
        desc = "Checkerboard gaps, 2HP bricks introduced.";
        glColor3f(1.0f, 1.0f, 0.2f);
    } else if (level == 3) {
        diff = "HEART SHAPE";
        desc = "Solid Heart of 2HP bricks. Break it fast!";
        glColor3f(1.0f, 0.5f, 0.2f);
    } else if (level == 4) {
        diff = "STAR CORE";
        desc = "A huge Star with an indestructible core (999HP).";
        glColor3f(1.0f, 0.2f, 0.2f);
    } else {
        diff = "HAPPY EMOJI WALL";
        desc = "Indestructible face with a tiny breakable target. Precision is "
               "key!";
        glColor3f(1.0f, 0.0f, 0.0f);
    }

    drawText(centerX - 100, descY, "Difficulty: " + diff,
             GLUT_BITMAP_HELVETICA_18);
    descY -= 25;
    glColor3f(0.8f, 0.8f, 0.8f);
    drawText(centerX - 100, descY, desc, GLUT_BITMAP_HELVETICA_18);
}

static void renderScene() {
    // Animated gradient background
    glClear(GL_COLOR_BUFFER_BIT);
    float tbg = nowSec() * 0.2f;
    float r1 = 0.5f + 0.5f * sinf(tbg * 2.1f);
    float g1 = 0.5f + 0.5f * sinf(tbg * 2.6f + 2.0f);
    float b1 = 0.5f + 0.5f * sinf(tbg * 2.3f + 4.0f);
    float r2 = 0.5f + 0.5f * sinf(tbg * 2.0f + 1.0f);
    float g2 = 0.5f + 0.5f * sinf(tbg * 2.7f + 3.0f);
    float b2 = 0.5f + 0.5f * sinf(tbg * 2.4f + 5.0f);
    glBegin(GL_QUADS);
    glColor3f(r1, g1, b1);
    glVertex2f(0, 0);
    glColor3f(r2, g2, b2);
    glVertex2f((float)scrW, 0);
    glColor3f(r1, g1, b1);
    glVertex2f((float)scrW, (float)scrH);
    glColor3f(r2, g2, b2);
    glVertex2f(0, (float)scrH);
    glEnd();

    // --- UI/Screen Logic ---
    if (current == MENU) {
        glColor3f(1, 1, 1);
        drawText(scrW / 2.f - 90, scrH - 120, "DX-Ball (OpenGL)");
        const char* itemsResume[] = {"Resume", "Start", "High Scores", "Help",
                                     "Exit"};
        const char* itemsFresh[] = {"Start", "High Scores", "Help", "Exit"};
        const char** items = canResume ? itemsResume : itemsFresh;
        int itemCount = canResume ? 5 : 4;
        for (int i = 0; i < itemCount; i++) {
            float y = scrH / 2.f + 60 - i * 40.f;
            if (i == menuIndex) {
                glColor3f(0.9f, 0.9f, 0.2f);
                drawText(scrW / 2.f - 60, y, std::string("> ") + items[i]);
            } else {
                glColor3f(1, 1, 1);
                drawText(scrW / 2.f - 40, y, items[i]);
            }
        }
        loadBest();
        if (haveBest) {
            char b[96];
            std::snprintf(b, sizeof(b), "Best: %d pts in %.1fs", bestScore,
                          bestTime);
            glColor3f(0.8f, 0.9f, 1.0f);
            drawText(scrW / 2.f - 95, scrH / 2.f - 140, b);
        }
    } else if (current == LEVEL_SELECT) {
        renderLevelSelect();
    } else if (current == HELP) {
        glColor3f(1, 1, 1);
        drawText(40, scrH - 100, "Help / Controls:");
        drawText(40, scrH - 130, "Mouse or Left/Right to move paddle");
        drawText(40, scrH - 155, "Space / Left Click: Launch ball");
        drawText(40, scrH - 180, "P or Esc: Pause/Resume");
        drawText(40, scrH - 205,
                 "F or Right Click: Fire bullet (when Shooting perk active)");
        drawText(
            40, scrH - 235,
            "Perks: Heart(+1), Bolt(Speed), Wide/Small Paddle, Ring(Through),");
        drawText(40, scrH - 255,
                 "        Flame(Fireball), Skull(Death), Ship(Shooting)");
        drawText(40, scrH - 285,
                 "Goal: Clear all BREAKABLE bricks as fast as possible.");
        drawText(40, scrH - 315, "Press Enter to return to Menu.");
    } else if (current == HIGHSCORES) {
        glColor3f(1, 1, 1);
        drawText(40, scrH - 90, "High Scores (Score, Time)");

        std::vector<Run> rows = history;
        std::sort(rows.begin(), rows.end(), [](const Run& a, const Run& b) {
            if (a.s != b.s)
                return a.s > b.s;
            return a.t < b.t;
        });

        int y = scrH - 130;
        int shown = 0;
        if (rows.empty()) {
            drawText(60, y, "No scores yet");
        } else {
            for (size_t i = 0; i < rows.size() && shown < 15; i++) {
                char row[96];
                std::snprintf(row, sizeof(row), "%2d) %6d pts    %6.1fs",
                              (int)i + 1, rows[i].s, rows[i].t);
                drawText(60, y, row);
                y -= 24;
                ++shown;
            }
        }

        loadBest();
        if (haveBest) {
            char b[96];
            std::snprintf(b, sizeof(b), "Best: %d pts in %.1fs", bestScore,
                          bestTime);
            glColor3f(0.8f, 0.9f, 1.0f);
            drawText(40, y - 20, b);
            glColor3f(1, 1, 1);
        }

        drawText(40, 60, "Press Enter for Menu");
    }

    // If not a menu screen, render the game elements
    if (current == PLAY || current == PAUSE || current == WIN ||
        current == GAMEOVER) {
        // Bricks (fill)
        for (size_t i = 0; i < bricks.size(); ++i) {
            const Brick& b = bricks[i];
            if (!b.alive)
                continue;
            glColor3f(b.r, b.g, b.b);
            drawRect(b.x, b.y, b.w, b.h);
        }

        // Brick outlines (To emphasize separation, but no internal lines)
        for (size_t i = 0; i < bricks.size(); ++i) {
            const Brick& b = bricks[i];
            if (!b.alive)
                continue;
            // Change outline color for indestructible bricks to emphasize them
            if (b.hp == 999)
                glColor3f(0.8f, 0.8f, 0.8f);
            else
                glColor3f(0, 0, 0);

            int x0 = iround(b.x - b.w / 2.f), x1 = iround(b.x + b.w / 2.f);
            int y0 = iround(b.y - b.h / 2.f), y1 = iround(b.y + b.h / 2.f);
            lab_draw_line(x0, y0, x1, y0);
            lab_draw_line(x1, y0, x1, y1);
            lab_draw_line(x1, y1, x0, y1);
            lab_draw_line(x0, y1, x0, y0);
        }

        // Paddle
        glColor3f(0.9f, 0.9f, 0.9f);
        drawRect(paddle.pos.x, paddle.pos.y, paddle.w, paddle.h);

        // Draw Ball Trail (Lava/Fireball Effect)
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        drawBallTrail();
        glDisable(GL_BLEND);

        // Ball
        if (ball.fireball)
            glColor3f(1.0f, 0.45f, 0.15f);
        else if (ball.through)
            glColor3f(1.0f, 0.3f, 0.3f);
        else
            glColor3f(1, 1, 1);
        drawCircleFilled(ball.pos.x, ball.pos.y, ball.radius, 24);
        glPointSize(2.0f);
        glColor3f(0, 0, 0);
        lab_midpoint_circle(iround(ball.pos.x), iround(ball.pos.y),
                            iround(ball.radius));
        glPointSize(1.0f);

        // Perks
        for (size_t i = 0; i < perks.size(); ++i) {
            const Perk& p = perks[i];
            if (!p.alive)
                continue;

            // Simple Glow Effect for Perks
            glColor4f(1.0f, 1.0f, 0.0f, 0.4f);
            drawCircleFilled(p.pos.x, p.pos.y, p.size * 1.2f, 12);

            // Draw actual perk icon
            switch (p.type) {
            case EXTRA_LIFE:
                glColor3f(1, 0.3f, 0.3f);
                break;
            case SPEED_UP:
                glColor3f(1, 1, 0.2f);
                break;
            case WIDE_PADDLE:
                glColor3f(0.3f, 1, 0.3f);
                break;
            case SHRINK_PADDLE:
                glColor3f(1, 0.5f, 0.1f);
                break;
            case THROUGH_BALL:
                glColor3f(0.4f, 0.8f, 1.0f);
                break;
            case FIREBALL:
                glColor3f(1.0f, 0.5f, 0.0f);
                break;
            case INSTANT_DEATH:
                glColor3f(0.8f, 0.0f, 0.0f);
                break;
            case SHOOTING_PADDLE:
                glColor3f(0.9f, 0.9f, 0.2f);
                break;
            }
            drawRect(p.pos.x, p.pos.y, p.size, p.size);
            glColor3f(0, 0, 0);
            drawPerkIcon(p.type, p.pos.x, p.pos.y, 8.f);
        }

        // Bullets
        for (size_t i = 0; i < bullets.size(); ++i) {
            const Bullet& bu = bullets[i];
            if (!bu.alive)
                continue;
            glColor3f(1, 1, 1);
            drawRect(bu.pos.x, bu.pos.y, bu.w, bu.h);
        }

        renderHUD();

        if (current == PAUSE) {
            glColor3f(1, 1, 1);
            drawText(scrW / 2.f - 40, scrH / 2.f, "PAUSED");
        }
        if (current == WIN) {
            glColor3f(0.8f, 1, 0.8f);
            drawText(scrW / 2.f - 40, scrH / 2.f, "YOU WIN!");
            drawText(scrW / 2.f - 120, scrH / 2.f - 30, "Press Enter for Menu");
        }
        if (current == GAMEOVER) {
            glColor3f(1, 0.8f, 0.8f);
            drawText(scrW / 2.f - 40, scrH / 2.f, "GAME OVER");
            drawText(scrW / 2.f - 120, scrH / 2.f - 30, "Press Enter for Menu");
        }
    }

    glutSwapBuffers();
}

// --- GLUT Callback Functions ---
static void onDisplay() { renderScene(); }

static void onIdle() {
    if (current == PLAY) {
        static float prev = nowSec();
        float t = nowSec();
        float dt = t - prev;
        prev = t;
        if (dt < 0.f)
            dt = 0.f; // FIX: Ensures this is guarded by the if statement above,
                      // fixing the misleading indentation warning.
        if (dt > 0.03f)
            dt = 0.03f;
        updateGame(dt);
        updateBallTrail();
    }
    glutPostRedisplay();
}

static void onReshape(int w, int h) {
    scrW = w;
    scrH = h;
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, (GLdouble)w, 0, (GLdouble)h);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

static void goToMenuOption(int index, bool isResume) {
    const char* itemsResume[] = {"Resume", "Start", "High Scores", "Help",
                                 "Exit"};
    const char* itemsFresh[] = {"Start", "High Scores", "Help", "Exit"};
    const char** items = isResume ? itemsResume : itemsFresh;
    int itemCount = isResume ? 5 : 4;

    if (index >= 0 && index < itemCount) {
        std::string it = items[index];
        if (it == "Resume" && isResume)
            current = PLAY;
        else if (it == "Start")
            newGame();
        else if (it == "High Scores")
            current = HIGHSCORES;
        else if (it == "Help")
            current = HELP;
        else if (it == "Exit")
            std::exit(0);
    }
}

static void onKey(unsigned char key, int, int) {
    if (current == MENU) {
        if (key == '\r' || key == '\n') {
            goToMenuOption(menuIndex, canResume);
        }
        if (key == 27)
            std::exit(0);
        return;
    }

    if (current == LEVEL_SELECT) {
        if (key == '\r' || key == '\n') {
            buildBricks(currentLevel); // Load the selected level
            current = PLAY;            // Start the game
            playTime = 0.f;            // Reset game timer
            lastTick = nowSec();
        }
        if (key == 27)
            current = MENU;
        return;
    }

    if (current == HELP || current == HIGHSCORES) {
        if (key == '\r' || key == '\n' || key == 27)
            current = MENU;
        return;
    }
    if (current == WIN || current == GAMEOVER) {
        if (key == '\r' || key == '\n')
            current = MENU;
        return;
    }

    if (key == 27 || key == 'p' || key == 'P') {
        if (current == PLAY) {
            current = PAUSE;
            canResume = true;
        } else if (current == PAUSE) {
            current = PLAY;
        }
        return;
    }

    if (current != PLAY)
        return;
    if (key == ' ' && ball.stuck) {
        ball.stuck = false;
        ball.vel = normalize(Vec2{0.2f, 1.f}) * ball.speed;
        hasLaunched = true;
    }
    if (key == 'f' || key == 'F')
        fireBullet();
}

static void onSpKey(int key, int, int) {
    if (current == MENU) {
        int itemCount = canResume ? 5 : 4;
        if (key == GLUT_KEY_UP) {
            menuIndex = (menuIndex - 1 + itemCount) % itemCount;
        }
        if (key == GLUT_KEY_DOWN) {
            menuIndex = (menuIndex + 1) % itemCount;
        }
        return;
    }

    if (current == LEVEL_SELECT) {
        if (key == GLUT_KEY_LEFT) {
            currentLevel = clampv(currentLevel - 1, 1, MAX_LEVELS);
        }
        if (key == GLUT_KEY_RIGHT) {
            currentLevel = clampv(currentLevel + 1, 1, MAX_LEVELS);
        }
        glutPostRedisplay();
        return;
    }

    if (current != PLAY)
        return;
    if (key == GLUT_KEY_LEFT)
        leftHeld = true;
    if (key == GLUT_KEY_RIGHT)
        rightHeld = true;
}

static void onSpKeyUp(int key, int, int) {
    if (key == GLUT_KEY_LEFT)
        leftHeld = false;
    if (key == GLUT_KEY_RIGHT)
        rightHeld = false;
}

static void onMouse(int button, int state, int x, int y) {
    int clickY = scrH - y; // Convert mouse y to OpenGL y (0 is bottom)

    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
        if (current == MENU) {
            float centerX = scrW / 2.f;
            const char* itemsResume[] = {"Resume", "Start", "High Scores",
                                         "Help", "Exit"};
            const char* itemsFresh[] = {"Start", "High Scores", "Help", "Exit"};
            const char** items = canResume ? itemsResume : itemsFresh;
            int itemCount = canResume ? 5 : 4;

            for (int i = 0; i < itemCount; i++) {
                float itemY = scrH / 2.f + 60 - i * 40.f;
                // Simple hit test based on text coordinates (x-90 to x+90, y to
                // y+25)
                if (x > centerX - 100 && x < centerX + 100 && clickY > itemY &&
                    clickY < itemY + 25) {
                    menuIndex = i;                // Highlight the clicked item
                    goToMenuOption(i, canResume); // Execute the option
                    return;
                }
            }
        }

        if (current == LEVEL_SELECT) {
            float centerX = scrW / 2.f;
            float startY = scrH / 2.f + 50;
            float buttonW = 150.f;
            float buttonH = 40.f;
            float spacing = 20.f;

            for (int i = 0; i < MAX_LEVELS; ++i) {
                float bx = centerX + (i - 2.f) * (buttonW + spacing);
                float by = startY;

                // Check if click is inside the button (x,y are center of
                // button)
                if (x > bx - buttonW / 2.f && x < bx + buttonW / 2.f &&
                    clickY > by - buttonH / 2.f &&
                    clickY < by + buttonH / 2.f) {
                    currentLevel = i + 1;      // Select level
                    buildBricks(currentLevel); // Load the selected level
                    current = PLAY;            // Start the game
                    playTime = 0.f;
                    lastTick = nowSec();
                    return;
                }
            }
        }

        if (current == PLAY && ball.stuck) {
            ball.stuck = false;
            ball.vel = normalize(Vec2{0, 1}) * ball.speed;
        }
    }
    if (current == PLAY && button == GLUT_RIGHT_BUTTON && state == GLUT_DOWN) {
        fireBullet();
    }
}

static void onMotion(int x, int y) {
    (void)y;
    if (current == PLAY) {
        float minX = paddle.w / 2.f + 6.f, maxX = scrW - paddle.w / 2.f - 6.f;
        float nx = (float)x;
        if (nx < minX)
            nx = minX;
        if (nx > maxX)
            nx = maxX;
        paddle.pos.x = nx;
    }
}

static void onPassiveMotion(int x, int y) { onMotion(x, y); }

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_ALPHA);
    glutInitWindowSize(scrW, scrH);
    glutCreateWindow("DX-Ball - Expert Project (Final - Symbolic)");
    glDisable(GL_DEPTH_TEST);
    glClearColor(0, 0, 0, 1);
    current = MENU;

    glutDisplayFunc(onDisplay);
    glutIdleFunc(onIdle);
    glutReshapeFunc(onReshape);
    glutKeyboardFunc(onKey);
    glutSpecialFunc(onSpKey);
    glutSpecialUpFunc(onSpKeyUp);
    glutMouseFunc(onMouse);
    glutMotionFunc(onMotion);
    glutPassiveMotionFunc(onPassiveMotion);

    glutMainLoop();
    return 0;
}
