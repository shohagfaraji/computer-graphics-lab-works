#include<bits/stdc++.h>
#include <GL/glut.h>
#include <cmath>
using namespace std;

const int inf = 1e9 + 7;
int windowSizeX = 760, windowSizeY = 500;

int segments = 9;
int midPointX = windowSizeX >> 1, midPointY = windowSizeY >> 1;
int baseHeight = windowSizeY / 6;
pair<int, int> baseStart = {0, baseHeight};
pair<int, int> baseEnd = {windowSizeX, baseHeight};
pair<int, int> topPoint = {midPointX, baseHeight * 5};
vector<int> basePointsL;
vector<int> basePointsR;
vector<pair<int, int>> barConnectPointsL;
vector<pair<int, int>> barConnectPointsR;
int segGapX = midPointX / segments;
int segGapY = ((topPoint.second - baseHeight) / 6) + 20;

pair<int, int> bottomTriTopPoint;
pair<int, int> middleBar[4] = {
    {-1, -1}, {-1, -1}, {-1, -1}, {-1, -1}
};

int firstLineSegLen;

void fixPoints() {
    int tempMidPointX = midPointX;
    for (int i = 0; i < segments - 2; i++) {
        tempMidPointX -= segGapX;
        basePointsL.emplace_back(tempMidPointX);
    }
    tempMidPointX = midPointX;
    for (int i = 0; i < segments - 2; i++) {
        tempMidPointX += segGapX;
        basePointsR.emplace_back(tempMidPointX);
    }
}

void putPixel(int x, int y) {
    glBegin(GL_POINTS);
    glVertex2i(x, y);
    glEnd();
}

void drawPixel(int x, int y, bool bold) {
    putPixel(x, y);
    if (bold) {
        putPixel(x + 1, y);
        putPixel(x - 1, y);
        putPixel(x, y + 1);
        putPixel(x, y - 1);
        putPixel(x + 1, y + 1);
        putPixel(x - 1, y - 1);
        putPixel(x + 1, y - 1);
        putPixel(x - 1, y + 1);
    }
}

void drawLine(int x1, int y1, int x2, int y2, int iden, bool bold) {
    int tx1 = x1, ty1 = y1;
    int dx = abs(x2 - x1);
    int dy = abs(y2 - y1);
    int sx = (x2 >= x1) ? 1 : -1;
    int sy = (y2 >= y1) ? 1 : -1;

    int err = dx - dy;

    int barLength;
    int nextBarGap = segGapY;
//    if (iden) {
//        barLength = sqrt ( (dx * dx) + (dy * dy) );
//        nextBarGap = (barLength / 5);
//    }
    pair<int, int> mxPointL[2];
    pair<int, int> mxPointR[2] = {{inf, inf}, {inf, inf}};

    while (true) {
        drawPixel(tx1, ty1, bold);

        if (iden == 1) {
            int tdx = abs(x2 - tx1);
            int tdy = abs(y2 - ty1);
            int curDistanceFromEndPoint = sqrt((tdx * tdx) + (tdy * tdy));
            if (abs(curDistanceFromEndPoint - nextBarGap) <= 10) {
                mxPointL[0] = max(mxPointL[0], {tx1, ty1});
            }
            tdx = abs(x1 - tx1);
            tdy = abs(y1 - ty1);
            curDistanceFromEndPoint = sqrt((tdx * tdx) + (tdy * tdy));
            if (abs(curDistanceFromEndPoint - nextBarGap) <= 10) {
                mxPointL[1] = max(mxPointL[1], {tx1, ty1});
            }
            if (abs(curDistanceFromEndPoint - (1.0 * firstLineSegLen * (6.5))) <= 10) {
                if (middleBar[3].first < 0) {
                    middleBar[3].first = tx1;
                    middleBar[3].second = ty1;
                }
            }
            if (abs(curDistanceFromEndPoint - (firstLineSegLen * 7)) <= 10) {
                if (middleBar[0].first < 0) {
                    middleBar[0].first = tx1;
                    middleBar[0].second = ty1;
                }
            }
        } else if (iden == 2) {
            int tdx = abs(x2 - tx1);
            int tdy = abs(y2 - ty1);
            int curDistanceFromEndPoint = sqrt((tdx * tdx) + (tdy * tdy));
            if (abs(curDistanceFromEndPoint - nextBarGap) <= 10) {
                mxPointR[0] = min(mxPointR[0], {tx1, ty1});
            }
            tdx = abs(x1 - tx1);
            tdy = abs(y1 - ty1);
            curDistanceFromEndPoint = sqrt((tdx * tdx) + (tdy * tdy));
            if (abs(curDistanceFromEndPoint - nextBarGap) <= 10) {
                mxPointR[1] = min(mxPointR[1], {tx1, ty1});
            }
            if (abs(curDistanceFromEndPoint - (1.0 * firstLineSegLen * (6.5))) <= 10) {
                if (middleBar[2].first < 0) {
                    middleBar[2].first = tx1;
                    middleBar[2].second = ty1;
                }
            }
            if (abs(curDistanceFromEndPoint - (firstLineSegLen * 7)) <= 10) {
                if (middleBar[1].first < 0) {
                    middleBar[1].first = tx1;
                    middleBar[1].second = ty1;
                }
            }
        }

        if (tx1 == x2 && ty1 == y2) break;

        int e2 = (err << 1);

        if (e2 > -dy) {
            err -= dy;
            tx1 += sx;
        }
        if (e2 < dx) {
            err += dx;
            ty1 += sy;
        }
    }
    if (iden == 1) {
        int distance1FromTop = topPoint.second - mxPointL[0].second;
        int distance2FromTop = topPoint.second - mxPointL[1].second;

        if (distance1FromTop < distance2FromTop) {
            barConnectPointsL.push_back(mxPointL[0]);
        } else {
            barConnectPointsL.push_back(mxPointL[1]);
        }
    } else if (iden == 2) {
        int distance1FromTop = topPoint.second - mxPointR[0].second;
        int distance2FromTop = topPoint.second - mxPointR[1].second;

        if (distance1FromTop < distance2FromTop) {
            barConnectPointsR.push_back(mxPointR[0]);
        } else {
            barConnectPointsR.push_back(mxPointR[1]);
        }
    }
}

void display() {
    glClear(GL_COLOR_BUFFER_BIT);
    glColor3f(0.0, 0.0, 0.0);

    int dy = topPoint.second - baseHeight;
    int middleLineSegLen = dy / 12;
    bottomTriTopPoint = {midPointX, baseHeight + middleLineSegLen};

    int dx = topPoint.first - basePointsL[0];
    firstLineSegLen = sqrt((dx * dx) + (dy * dy)) / 20;

    drawLine(baseStart.first, baseStart.second, baseEnd.first, baseEnd.second, 0, true);

    drawLine(basePointsL[0], baseHeight, topPoint.first, topPoint.second, 1, true);
    drawLine(basePointsR[0], baseHeight, topPoint.first, topPoint.second, 2, true);

    for (int i = 1 ; i < segments - 2 ; i++) {
        drawLine(barConnectPointsL.back().first, barConnectPointsL.back().second, basePointsL[i], baseHeight, 1, true);
        drawLine(barConnectPointsR.back().first, barConnectPointsR.back().second, basePointsR[i], baseHeight, 2, true);
    }

    drawLine(basePointsL[0], baseHeight, bottomTriTopPoint.first, bottomTriTopPoint.second, 0, false);
    drawLine(basePointsR[0], baseHeight, bottomTriTopPoint.first, bottomTriTopPoint.second, 0, false);

    drawLine(middleBar[0].first, middleBar[0].second, middleBar[1].first, middleBar[1].second, 0, false);
    drawLine(middleBar[2].first, middleBar[2].second, middleBar[3].first, middleBar[3].second, 0, false);

    drawLine(middleBar[2].first, middleBar[2].second, bottomTriTopPoint.first, bottomTriTopPoint.second, 0, false);
    drawLine(middleBar[3].first, middleBar[3].second, bottomTriTopPoint.first, bottomTriTopPoint.second, 0, false);

    int lowerBarLength = middleBar[2].first - middleBar[3].first;
    int lowerBarHalfLength = lowerBarLength >> 1;
    int lowerBarMidX = middleBar[3].first + lowerBarHalfLength;
    int lowerTriHeight = middleBar[3].second - bottomTriTopPoint.second;

    int upperBarLength = middleBar[1].first - middleBar[0].first;
    int upperBarHalfLength = upperBarLength >> 1;
    int upperBarMidX = middleBar[0].first + upperBarHalfLength;

    pair<int, int> upperTriPoint = {upperBarMidX, middleBar[0].second + lowerTriHeight};
    drawLine(middleBar[0].first, middleBar[0].second, upperTriPoint.first, upperTriPoint.second, 0, false);
    drawLine(middleBar[1].first, middleBar[1].second, upperTriPoint.first, upperTriPoint.second, 0, false);

    drawLine(topPoint.first, topPoint.second, upperTriPoint.first, upperTriPoint.second, 0, false);

    glFlush();
}

void init() {
    glClearColor(1.0, 1.0, 1.0, 1.0);
    glColor3f(0.0, 0.0, 0.0);
    gluOrtho2D(0, windowSizeX, 0, windowSizeY);
    glPointSize(1.0);
}

int main(int argc, char** argv) {
    fixPoints();
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);
    glutInitWindowSize(windowSizeX, windowSizeY);
    glutInitWindowPosition(100, 100);
    glutCreateWindow("National Martyrs' Monument");
    init();
    glutDisplayFunc(display);
    glutMainLoop();
    return 0;
}
