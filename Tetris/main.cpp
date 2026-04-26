#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <math.h>
#include <GL/freeglut.h>

#define PLAY_WIDTH    10
#define PLAY_HEIGHT   20
#define BLOCK_SIZE    30
#define SIDEBAR_WIDTH 150
#define WINDOW_WIDTH  450
#define WINDOW_HEIGHT 600

static const int BTN_WIDTH        = 120;
static const int BTN_HEIGHT       = 40;
static const int MENU_BTN_WIDTH   = 240;
static const int MENU_BTN_HEIGHT  = 80;
static const int MENU_BTN_SPACING = 60;

int btnX, btnY;

enum GameState { MENU, PLAYING, GAME_OVER };
enum GameState gameState = MENU;

static float getStrokeTextWidth(const char* text, void* font);
void loadHighScore(void);
void saveHighScore(void);
void restartGame(void);
int  checkCollision(int s, int r, int x, int y);
int  getGhostY(void);
void fixPiece(void);
void clearLines(void);
void spawnPiece(void);
void drawCell(int row, int col, int c, float alpha);
void display(void);
void timer(int v);
void specialKeys(int key, int x, int y);
void keyboardFunc(unsigned char key, int x, int y);
void mouseClick(int button, int state, int x, int y);

// все позиции каждой фигуры при 4 поворотах
static const int shapes[7][4][4][4] = {
    // I
    {{{0,0,0,0},{1,1,1,1},{0,0,0,0},{0,0,0,0}},
     {{0,0,1,0},{0,0,1,0},{0,0,1,0},{0,0,1,0}},
     {{0,0,0,0},{0,0,0,0},{1,1,1,1},{0,0,0,0}},
     {{0,1,0,0},{0,1,0,0},{0,1,0,0},{0,1,0,0}}},
    // J
    {{{1,0,0,0},{1,1,1,0},{0,0,0,0},{0,0,0,0}},
     {{0,1,1,0},{0,1,0,0},{0,1,0,0},{0,0,0,0}},
     {{0,0,0,0},{1,1,1,0},{0,0,1,0},{0,0,0,0}},
     {{0,1,0,0},{0,1,0,0},{1,1,0,0},{0,0,0,0}}},
    // L
    {{{0,0,1,0},{1,1,1,0},{0,0,0,0},{0,0,0,0}},
     {{0,1,0,0},{0,1,0,0},{0,1,1,0},{0,0,0,0}},
     {{0,0,0,0},{1,1,1,0},{1,0,0,0},{0,0,0,0}},
     {{1,1,0,0},{0,1,0,0},{0,1,0,0},{0,0,0,0}}},
    // O
    {{{0,1,1,0},{0,1,1,0},{0,0,0,0},{0,0,0,0}},
     {{0,1,1,0},{0,1,1,0},{0,0,0,0},{0,0,0,0}},
     {{0,1,1,0},{0,1,1,0},{0,0,0,0},{0,0,0,0}},
     {{0,1,1,0},{0,1,1,0},{0,0,0,0},{0,0,0,0}}},
    // S
    {{{0,1,1,0},{1,1,0,0},{0,0,0,0},{0,0,0,0}},
     {{0,1,0,0},{0,1,1,0},{0,0,1,0},{0,0,0,0}},
     {{0,0,0,0},{0,1,1,0},{1,1,0,0},{0,0,0,0}},
     {{1,0,0,0},{1,1,0,0},{0,1,0,0},{0,0,0,0}}},
    // T
    {{{0,1,0,0},{1,1,1,0},{0,0,0,0},{0,0,0,0}},
     {{0,1,0,0},{0,1,1,0},{0,1,0,0},{0,0,0,0}},
     {{0,0,0,0},{1,1,1,0},{0,1,0,0},{0,0,0,0}},
     {{0,1,0,0},{1,1,0,0},{0,1,0,0},{0,0,0,0}}},
    // Z
    {{{1,1,0,0},{0,1,1,0},{0,0,0,0},{0,0,0,0}},
     {{0,0,1,0},{0,1,1,0},{0,1,0,0},{0,0,0,0}},
     {{0,0,0,0},{1,1,0,0},{0,1,1,0},{0,0,0,0}},
     {{0,1,0,0},{1,1,0,0},{1,0,0,0},{0,0,0,0}}}
};

static const float colors[7][3] = {
    {0.0f, 1.0f, 1.0f},
    {0.0f, 0.0f, 1.0f},
    {1.0f, 0.5f, 0.0f},
    {1.0f, 1.0f, 0.0f},
    {0.0f, 1.0f, 0.0f},
    {0.5f, 0.0f, 0.5f},
    {1.0f, 0.0f, 0.0f}
};

// 1 линия = 100, 2 = 300, 3 = 500, 4 = 800 (умножается на уровень)
static const int LINE_SCORES[5] = { 0, 100, 300, 500, 800 };

int  grid[PLAY_HEIGHT][PLAY_WIDTH];
int  currentPiece, nextPiece;
int  currentRot;
int  currentX, currentY;
int  score, record, totalLines, level;
bool gameOver;
time_t startTime, lastSpeedUpdate;
int  speed; // мс между тиками

static float getStrokeTextWidth(const char* text, void* font) {
    float w = 0;
    while (*text) w += glutStrokeWidth(font, *text++);
    return w;
}

void loadHighScore(void) {
    FILE* f = NULL;
    fopen_s(&f, "Record.txt", "r");
    if (f) { fscanf_s(f, "%d", &record); fclose(f); }
    else    { record = 0; }
}

void saveHighScore(void) {
    FILE* f = NULL;
    fopen_s(&f, "Record.txt", "w");
    if (f) { fprintf(f, "%d", record); fclose(f); }
}

int checkCollision(int s, int r, int x, int y) {
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++) {
            if (!shapes[s][r][i][j]) continue;
            int gx = x + j, gy = y + i;
            if (gx < 0 || gx >= PLAY_WIDTH || gy >= PLAY_HEIGHT) return 1;
            if (gy >= 0 && grid[gy][gx]) return 1;
        }
    return 0;
}

int getGhostY(void) {
    int gy = currentY;
    while (!checkCollision(currentPiece, currentRot, currentX, gy + 1)) gy++;
    return gy;
}

void spawnPiece(void) {
    currentPiece = nextPiece;
    currentRot   = 0;
    currentX     = PLAY_WIDTH / 2 - 2;
    currentY     = -2;
    nextPiece    = rand() % 7;
    if (checkCollision(currentPiece, currentRot, currentX, currentY)) {
        gameOver  = true;
        gameState = GAME_OVER;
    }
}

void restartGame(void) {
    memset(grid, 0, sizeof(grid));
    score = totalLines = 0;
    level           = 1;
    gameOver        = false;
    startTime       = lastSpeedUpdate = time(NULL);
    speed           = 1000;
    nextPiece       = rand() % 7;
    spawnPiece();
    glutPostRedisplay();
    glutTimerFunc(speed, timer, 0);
}

void fixPiece(void) {
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
            if (shapes[currentPiece][currentRot][i][j]) {
                int gy = currentY + i;
                if (gy >= 0 && gy < PLAY_HEIGHT)
                    grid[gy][currentX + j] = currentPiece + 1;
            }
    for (int j = 0; j < PLAY_WIDTH; j++)
        if (grid[0][j]) { gameOver = true; gameState = GAME_OVER; break; }
}

void clearLines(void) {
    int cleared = 0;
    for (int i = PLAY_HEIGHT - 1; i >= 0; i--) {
        bool full = true;
        for (int j = 0; j < PLAY_WIDTH; j++) if (!grid[i][j]) { full = false; break; }
        if (full) {
            for (int k = i; k > 0; k--) memcpy(grid[k], grid[k-1], sizeof(grid[k]));
            memset(grid[0], 0, sizeof(grid[0]));
            cleared++; i++;
        }
    }
    if (cleared > 0) {
        score      += LINE_SCORES[cleared] * level;
        totalLines += cleared;
        level       = totalLines / 10 + 1;
    }
}

void drawCell(int row, int col, int c, float alpha) {
    if (c < 1) return;
    const float* colr = colors[c - 1];
    int x = col * BLOCK_SIZE + SIDEBAR_WIDTH;
    int y = WINDOW_HEIGHT - (row + 1) * BLOCK_SIZE;

    glColor4f(colr[0], colr[1], colr[2], alpha);
    glBegin(GL_QUADS);
    glVertex2i(x, y); glVertex2i(x + BLOCK_SIZE, y);
    glVertex2i(x + BLOCK_SIZE, y + BLOCK_SIZE); glVertex2i(x, y + BLOCK_SIZE);
    glEnd();

    glColor4f(1, 1, 1, alpha * 0.30f);
    glBegin(GL_QUADS);
    glVertex2i(x, y + BLOCK_SIZE - 3); glVertex2i(x + BLOCK_SIZE - 3, y + BLOCK_SIZE - 3);
    glVertex2i(x + BLOCK_SIZE - 3, y + BLOCK_SIZE); glVertex2i(x, y + BLOCK_SIZE);
    glEnd();
    glBegin(GL_QUADS);
    glVertex2i(x, y); glVertex2i(x + 3, y);
    glVertex2i(x + 3, y + BLOCK_SIZE); glVertex2i(x, y + BLOCK_SIZE);
    glEnd();

    glColor4f(0, 0, 0, alpha);
    glBegin(GL_LINE_LOOP);
    glVertex2i(x, y); glVertex2i(x + BLOCK_SIZE, y);
    glVertex2i(x + BLOCK_SIZE, y + BLOCK_SIZE); glVertex2i(x, y + BLOCK_SIZE);
    glEnd();
}

static void drawBitmapText(const char* text, void* font, int x, int y) {
    glRasterPos2i(x, y);
    for (const char* c = text; *c; c++) glutBitmapCharacter(font, *c);
}

static void drawStrokeText(const char* text, void* font,
                           float x, float y, float scale, float lw) {
    glLineWidth(lw);
    glPushMatrix();
    glTranslatef(x, y, 0); glScalef(scale, scale, 1);
    for (const char* c = text; *c; c++) glutStrokeCharacter(font, *c);
    glPopMatrix();
    glLineWidth(1.0f);
}

static void drawButton(int x, int y, int w, int h,
                       float r1, float g1, float b1,
                       float r2, float g2, float b2,
                       const char* label) {
    glBegin(GL_QUADS);
    glColor3f(r1, g1, b1); glVertex2i(x, y);   glVertex2i(x + w, y);
    glColor3f(r2, g2, b2); glVertex2i(x + w, y + h); glVertex2i(x, y + h);
    glEnd();
    int tw = glutBitmapLength(GLUT_BITMAP_HELVETICA_18, (const unsigned char*)label);
    glColor3f(1, 1, 1);
    drawBitmapText(label, GLUT_BITMAP_HELVETICA_18,
                   x + (w - tw) / 2, y + (h - 18) / 2 + 3);
}

void display(void) {
    glClear(GL_COLOR_BUFFER_BIT);
    char buf[64];

    if (gameState == MENU) {
        glColor3f(0, 0, 0);
        glRecti(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);

        const char* title  = "TETRIS";
        float        tScale = 0.8f;
        float        tWidth = getStrokeTextWidth(title, GLUT_STROKE_ROMAN) * tScale;
        static const float titleColors[6][3] = {
            {1,0,0},{1,0.5f,0},{1,1,0},{0,1,0},{0,0.5f,1},{0.7f,0,1}
        };
        glPushMatrix();
        glTranslatef((WINDOW_WIDTH - tWidth) / 2.0f, (float)(WINDOW_HEIGHT / 2 + 150), 0);
        glScalef(tScale, tScale, 1);
        glLineWidth(12);
        for (int i = 0; title[i]; i++) {
            const float* tc = titleColors[i % 6];
            glColor3f(tc[0], tc[1], tc[2]);
            glutStrokeCharacter(GLUT_STROKE_ROMAN, title[i]);
        }
        glPopMatrix();
        glLineWidth(1);

        int playX = (WINDOW_WIDTH - MENU_BTN_WIDTH) / 2;
        int playY = WINDOW_HEIGHT / 2 + MENU_BTN_SPACING / 2;
        drawButton(playX, playY, MENU_BTN_WIDTH, MENU_BTN_HEIGHT,
                   0.2f, 0.7f, 0.3f, 0.1f, 0.4f, 0.15f, "Play");

        int exitX = (WINDOW_WIDTH - MENU_BTN_WIDTH) / 2;
        int exitY = WINDOW_HEIGHT / 2 - MENU_BTN_HEIGHT - MENU_BTN_SPACING / 2;
        drawButton(exitX, exitY, MENU_BTN_WIDTH, MENU_BTN_HEIGHT,
                   0.6f, 0.2f, 0.8f, 0.4f, 0.1f, 0.5f, "Exit");
    }
    else if (gameState == PLAYING) {
        glColor3f(0.15f, 0.15f, 0.15f);
        glRecti(0, 0, SIDEBAR_WIDTH, WINDOW_HEIGHT);
        glColor3f(0.05f, 0.05f, 0.05f);
        glRecti(SIDEBAR_WIDTH, 0, WINDOW_WIDTH, WINDOW_HEIGHT);

        glColor3f(0.12f, 0.12f, 0.12f);
        for (int i = 0; i <= PLAY_WIDTH; i++) {
            int x = SIDEBAR_WIDTH + i * BLOCK_SIZE;
            glBegin(GL_LINES); glVertex2i(x, 0); glVertex2i(x, WINDOW_HEIGHT); glEnd();
        }
        for (int i = 0; i <= PLAY_HEIGHT; i++) {
            int y = i * BLOCK_SIZE;
            glBegin(GL_LINES); glVertex2i(SIDEBAR_WIDTH, y); glVertex2i(WINDOW_WIDTH, y); glEnd();
        }

        glColor3f(0.6f, 0.6f, 0.6f);
        drawBitmapText("NEXT", GLUT_BITMAP_HELVETICA_12, 47, WINDOW_HEIGHT - 18);
        glColor3f(0.45f, 0.45f, 0.45f);
        glLineWidth(2);
        glBegin(GL_LINE_LOOP);
        glVertex2i(15, WINDOW_HEIGHT - 130); glVertex2i(125, WINDOW_HEIGHT - 130);
        glVertex2i(125, WINDOW_HEIGHT - 30); glVertex2i(15, WINDOW_HEIGHT - 30);
        glEnd();
        glLineWidth(1);
        for (int i = 0; i < 4; i++)
            for (int j = 0; j < 4; j++)
                if (shapes[nextPiece][0][i][j]) {
                    int nx = 30 + j * 22, ny = WINDOW_HEIGHT - 78 - i * 22;
                    const float* colr = colors[nextPiece];
                    glColor4f(colr[0], colr[1], colr[2], 1);
                    glBegin(GL_QUADS);
                    glVertex2i(nx, ny); glVertex2i(nx+22, ny);
                    glVertex2i(nx+22, ny+22); glVertex2i(nx, ny+22);
                    glEnd();
                    glColor4f(0,0,0,1);
                    glBegin(GL_LINE_LOOP);
                    glVertex2i(nx, ny); glVertex2i(nx+22, ny);
                    glVertex2i(nx+22, ny+22); glVertex2i(nx, ny+22);
                    glEnd();
                }

        time_t now = time(NULL);
        int elapsed = (int)(now - startTime);
        glColor3f(0.8f, 0.8f, 0.8f);
        sprintf_s(buf, "Time:  %02d:%02d", elapsed/60, elapsed%60);
        drawBitmapText(buf, GLUT_BITMAP_HELVETICA_12, 8, WINDOW_HEIGHT - 155);
        sprintf_s(buf, "Score: %d", score);
        drawBitmapText(buf, GLUT_BITMAP_HELVETICA_12, 8, WINDOW_HEIGHT - 185);
        sprintf_s(buf, "Lines: %d", totalLines);
        drawBitmapText(buf, GLUT_BITMAP_HELVETICA_12, 8, WINDOW_HEIGHT - 215);
        sprintf_s(buf, "Level: %d", level);
        drawBitmapText(buf, GLUT_BITMAP_HELVETICA_12, 8, WINDOW_HEIGHT - 245);
        glColor3f(1.0f, 0.85f, 0.0f);
        sprintf_s(buf, "Best:  %d", record);
        drawBitmapText(buf, GLUT_BITMAP_HELVETICA_12, 8, WINDOW_HEIGHT - 275);

        glColor3f(0.4f, 0.4f, 0.4f);
        drawBitmapText("Arrows: move",   GLUT_BITMAP_HELVETICA_10, 5, 105);
        drawBitmapText("Up:     rotate", GLUT_BITMAP_HELVETICA_10, 5, 90);
        drawBitmapText("Space:  drop",   GLUT_BITMAP_HELVETICA_10, 5, 75);
        drawBitmapText("Home:   slam",   GLUT_BITMAP_HELVETICA_10, 5, 60);

        for (int i = 0; i < PLAY_HEIGHT; i++)
            for (int j = 0; j < PLAY_WIDTH; j++)
                drawCell(i, j, grid[i][j], 1.0f);

        if (!gameOver) {
            // тень показывает куда упадёт фигура
            int ghostY = getGhostY();
            if (ghostY != currentY) {
                for (int i = 0; i < 4; i++)
                    for (int j = 0; j < 4; j++)
                        if (shapes[currentPiece][currentRot][i][j]) {
                            int row = ghostY + i, col = currentX + j;
                            if (row >= 0) drawCell(row, col, currentPiece + 1, 0.2f);
                        }
            }
            for (int i = 0; i < 4; i++)
                for (int j = 0; j < 4; j++)
                    if (shapes[currentPiece][currentRot][i][j]) {
                        int row = currentY + i, col = currentX + j;
                        if (row >= 0) drawCell(row, col, currentPiece + 1, 1.0f);
                    }
        }

        glColor3f(0.4f, 0.4f, 0.4f);
        glLineWidth(2);
        glBegin(GL_LINE_LOOP);
        glVertex2i(SIDEBAR_WIDTH, 0); glVertex2i(WINDOW_WIDTH, 0);
        glVertex2i(WINDOW_WIDTH, WINDOW_HEIGHT); glVertex2i(SIDEBAR_WIDTH, WINDOW_HEIGHT);
        glEnd();
        glLineWidth(1);
    }
    else if (gameState == GAME_OVER) {
        if (score > record) { record = score; saveHighScore(); }

        glColor4f(0, 0, 0, 0.78f);
        glRecti(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);

        const char* goText = "GAME OVER";
        float goScale = 0.5f;
        float goW = getStrokeTextWidth(goText, GLUT_STROKE_ROMAN) * goScale;
        glColor3f(1, 0.2f, 0.2f);
        drawStrokeText(goText, GLUT_STROKE_ROMAN,
                       (WINDOW_WIDTH - goW) / 2.0f, WINDOW_HEIGHT / 2.0f + 120,
                       goScale, 6.0f);

        sprintf_s(buf, "Score: %d", score);
        float sScale = 0.3f;
        float sW = getStrokeTextWidth(buf, GLUT_STROKE_ROMAN) * sScale;
        glColor3f(1, 1, 1);
        drawStrokeText(buf, GLUT_STROKE_ROMAN,
                       (WINDOW_WIDTH - sW) / 2.0f, WINDOW_HEIGHT / 2.0f + 60,
                       sScale, 4.0f);

        if (score > 0 && score == record) {
            const char* nr = "NEW RECORD!";
            float nrW = getStrokeTextWidth(nr, GLUT_STROKE_ROMAN) * 0.22f;
            glColor3f(1.0f, 0.85f, 0.0f);
            drawStrokeText(nr, GLUT_STROKE_ROMAN,
                           (WINDOW_WIDTH - nrW) / 2.0f, WINDOW_HEIGHT / 2.0f + 18,
                           0.22f, 3.0f);
        }

        btnX = (WINDOW_WIDTH - BTN_WIDTH) / 2;
        btnY = WINDOW_HEIGHT / 2 - BTN_HEIGHT - 20;
        drawButton(btnX, btnY, BTN_WIDTH, BTN_HEIGHT,
                   0.2f, 0.6f, 1.0f, 0.1f, 0.3f, 0.8f, "Retry");

        int mmY = btnY - BTN_HEIGHT - 12;
        drawButton(btnX, mmY, BTN_WIDTH, BTN_HEIGHT,
                   0.6f, 0.2f, 0.8f, 0.4f, 0.1f, 0.5f, "Main Menu");
    }

    glutSwapBuffers();
}

void timer(int v) {
    if (gameState != PLAYING || gameOver) return;

    time_t now = time(NULL);
    if (now - lastSpeedUpdate >= 30) {
        speed = speed > 100 ? speed - 100 : 100;
        lastSpeedUpdate = now;
    }

    if (!checkCollision(currentPiece, currentRot, currentX, currentY + 1)) {
        currentY++;
    } else {
        fixPiece();
        clearLines();
        spawnPiece();
        if (gameOver) { glutPostRedisplay(); return; }
    }

    glutPostRedisplay();
    glutTimerFunc(speed, timer, 0);
}

void specialKeys(int key, int x, int y) {
    if (gameState != PLAYING || gameOver) return;
    switch (key) {
    case GLUT_KEY_LEFT:
        if (!checkCollision(currentPiece, currentRot, currentX - 1, currentY)) currentX--;
        break;
    case GLUT_KEY_RIGHT:
        if (!checkCollision(currentPiece, currentRot, currentX + 1, currentY)) currentX++;
        break;
    case GLUT_KEY_DOWN:
        if (!checkCollision(currentPiece, currentRot, currentX, currentY + 1)) currentY++;
        break;
    case GLUT_KEY_UP: {
        int nr = (currentRot + 1) % 4;
        if (!checkCollision(currentPiece, nr, currentX, currentY)) currentRot = nr;
        break;
    }
    case GLUT_KEY_HOME:
        while (!checkCollision(currentPiece, currentRot, currentX, currentY + 1)) currentY++;
        break;
    }
    glutPostRedisplay();
}

void keyboardFunc(unsigned char key, int x, int y) {
    if (gameState != PLAYING || gameOver) return;
    if (key == ' ') {
        while (!checkCollision(currentPiece, currentRot, currentX, currentY + 1)) currentY++;
        fixPiece();
        clearLines();
        spawnPiece();
        glutPostRedisplay();
    }
}

void mouseClick(int button, int state, int x, int y) {
    if (button != GLUT_LEFT_BUTTON || state != GLUT_DOWN) return;
    int my = WINDOW_HEIGHT - y;

    if (gameState == MENU) {
        int playX = (WINDOW_WIDTH - MENU_BTN_WIDTH) / 2;
        int playY = WINDOW_HEIGHT / 2 + MENU_BTN_SPACING / 2;
        if (x >= playX && x <= playX + MENU_BTN_WIDTH &&
            my >= playY && my <= playY + MENU_BTN_HEIGHT) {
            gameState = PLAYING; restartGame();
        }
        int exitX = (WINDOW_WIDTH - MENU_BTN_WIDTH) / 2;
        int exitY = WINDOW_HEIGHT / 2 - MENU_BTN_HEIGHT - MENU_BTN_SPACING / 2;
        if (x >= exitX && x <= exitX + MENU_BTN_WIDTH &&
            my >= exitY && my <= exitY + MENU_BTN_HEIGHT) exit(0);
    }
    else if (gameState == GAME_OVER) {
        if (x >= btnX && x <= btnX + BTN_WIDTH &&
            my >= btnY && my <= btnY + BTN_HEIGHT) {
            gameState = PLAYING; gameOver = false; restartGame();
        }
        int mmY = btnY - BTN_HEIGHT - 12;
        if (x >= btnX && x <= btnX + BTN_WIDTH &&
            my >= mmY && my <= mmY + BTN_HEIGHT) {
            gameState = MENU; gameOver = false; glutPostRedisplay();
        }
    }
}

int main(int argc, char** argv) {
    srand((unsigned)time(NULL));
    loadHighScore();

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
    glutInitWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);
    glutCreateWindow("Tetris");

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glOrtho(0, WINDOW_WIDTH, 0, WINDOW_HEIGHT, -1, 1);

    glutDisplayFunc(display);
    glutSpecialFunc(specialKeys);
    glutKeyboardFunc(keyboardFunc);
    glutMouseFunc(mouseClick);
    glutMainLoop();
    return 0;
}
