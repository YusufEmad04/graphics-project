// ============================================================
//  Catch the Falling Cubes - 3D OpenGL Mini Game
// ------------------------------------------------------------
//  Team members  : <Member 1 Name>, <Member 2 Name>
//  Reg. numbers  : <Reg 1>, <Reg 2>
//  TA            : <TA Name>
// ------------------------------------------------------------
//  Build (MSVC + freeglut):
//      cl /EHsc main.cpp /I lib\include /link /LIBPATH:lib\lib freeglut.lib opengl32.lib glu32.lib
//  Build (CodeBlocks / MinGW + classic GLUT):
//      Link with: -lopengl32 -lglu32 -lglut32   (or -lfreeglut on freeglut setups)
// ============================================================

#ifdef _MSC_VER
#define FREEGLUT_STATIC // we link the static freeglut on MSVC
#endif
#include <GL/glut.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
// Note: __iob_func shim for MSVC + static freeglut lives in msvc_shim.cpp

// --------- Window / world constants --------------------------
static int winW = 900, winH = 650;
static const float FLOOR_HALF = 8.0f; // half-size of square floor
static const float PLAYER_SPEED = 0.35f;
static const float CUBE_FALL_SPEED = 0.06f;
static const int NUM_CUBES = 3;
static const int GAME_SECONDS = 60;

// --------- Game state ----------------------------------------
enum GameState
{
    STATE_MENU,
    STATE_PLAY,
    STATE_OVER
};
static int gameState = STATE_MENU;
static int score = 0;
static int timeLeft = GAME_SECONDS;
static int menuSelection = 0; // 0 = START, 1 = EXIT

// Player paddle position (on the floor, y is fixed)
static float playerX = 0.0f;
static float playerZ = 4.0f;
static const float PLAYER_W = 1.6f;  // half-extent X
static const float PLAYER_D = 0.8f;  // half-extent Z
static const float PLAYER_Y = 0.25f; // base height above floor

// Falling cube
struct FallCube
{
    float x, y, z;
    float r, g, b;
};
static FallCube cubes[NUM_CUBES];

// Key state for smooth movement
static bool kLeft = false, kRight = false, kUp = false, kDown = false;

// ============================================================
//  Helpers
// ============================================================
static float randf(float a, float b)
{
    return a + (b - a) * ((float)rand() / (float)RAND_MAX);
}

static void resetCube(FallCube &c)
{
    c.x = randf(-FLOOR_HALF + 0.5f, FLOOR_HALF - 0.5f);
    c.z = randf(-FLOOR_HALF + 0.5f, FLOOR_HALF - 0.5f);
    c.y = randf(8.0f, 14.0f);
    // Random vivid color
    c.r = randf(0.3f, 1.0f);
    c.g = randf(0.3f, 1.0f);
    c.b = randf(0.3f, 1.0f);
}

static void resetGame()
{
    score = 0;
    timeLeft = GAME_SECONDS;
    playerX = 0.0f;
    playerZ = 4.0f;
    for (int i = 0; i < NUM_CUBES; ++i)
        resetCube(cubes[i]);
}

// Draw bitmap text in 2D screen coords
static void drawText2D(float x, float y, const char *s, void *font = GLUT_BITMAP_HELVETICA_18)
{
    glRasterPos2f(x, y);
    while (*s)
        glutBitmapCharacter(font, *s++);
}

// Switch to 2D ortho mode for HUD/menu text
static void begin2D()
{
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, winW, 0, winH);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
}
static void end2D()
{
    glEnable(GL_LIGHTING);
    glEnable(GL_DEPTH_TEST);
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
}

// ============================================================
//  Drawing: scene parts
// ============================================================
// Patterned (checkered) floor - procedural background
static void drawFloor()
{
    glDisable(GL_LIGHTING); // flat colored tiles
    const int N = 16;       // tiles per side
    const float step = (FLOOR_HALF * 2.0f) / N;
    for (int i = 0; i < N; ++i)
    {
        for (int j = 0; j < N; ++j)
        {
            if ((i + j) & 1)
                glColor3f(0.20f, 0.55f, 0.25f); // dark green
            else
                glColor3f(0.55f, 0.80f, 0.40f); // light green
            float x0 = -FLOOR_HALF + i * step;
            float z0 = -FLOOR_HALF + j * step;
            glBegin(GL_QUADS);
            glVertex3f(x0, 0.0f, z0);
            glVertex3f(x0 + step, 0.0f, z0);
            glVertex3f(x0 + step, 0.0f, z0 + step);
            glVertex3f(x0, 0.0f, z0 + step);
            glEnd();
        }
    }
    // Border frame
    glColor3f(0.15f, 0.15f, 0.15f);
    glLineWidth(2.0f);
    glBegin(GL_LINE_LOOP);
    glVertex3f(-FLOOR_HALF, 0.01f, -FLOOR_HALF);
    glVertex3f(FLOOR_HALF, 0.01f, -FLOOR_HALF);
    glVertex3f(FLOOR_HALF, 0.01f, FLOOR_HALF);
    glVertex3f(-FLOOR_HALF, 0.01f, FLOOR_HALF);
    glEnd();
    glEnable(GL_LIGHTING);
}

// Player = base cube + head cube + cone nose  (>=3 shapes)
static void drawPlayer()
{
    glPushMatrix();
    glTranslatef(playerX, PLAYER_Y, playerZ);

    // 1) Base (wide flat cube)
    glColor3f(0.20f, 0.40f, 0.95f);
    glPushMatrix();
    glScalef(PLAYER_W * 2.0f, 0.5f, PLAYER_D * 2.0f);
    glutSolidCube(1.0);
    glPopMatrix();

    // 2) Head (small cube on top)
    glColor3f(1.0f, 0.85f, 0.4f);
    glPushMatrix();
    glTranslatef(0.0f, 0.55f, 0.0f);
    glutSolidCube(0.7);
    glPopMatrix();

    // 3) Nose (cone pointing forward, -Z)
    glColor3f(0.95f, 0.25f, 0.25f);
    glPushMatrix();
    glTranslatef(0.0f, 0.55f, -0.45f);
    glRotatef(-90.0f, 1.0f, 0.0f, 0.0f); // cone defaults +Z, rotate to -Z
    glutSolidCone(0.18, 0.6, 12, 12);
    glPopMatrix();

    // 4) Eye (sphere) - small extra detail (still simple)
    glColor3f(0.05f, 0.05f, 0.05f);
    glPushMatrix();
    glTranslatef(0.18f, 0.65f, -0.30f);
    glutSolidSphere(0.06, 8, 8);
    glPopMatrix();
    glPushMatrix();
    glTranslatef(-0.18f, 0.65f, -0.30f);
    glutSolidSphere(0.06, 8, 8);
    glPopMatrix();
    glPopMatrix();
}

static void drawFallingCubes()
{
    for (int i = 0; i < NUM_CUBES; ++i)
    {
        glPushMatrix();
        glTranslatef(cubes[i].x, cubes[i].y, cubes[i].z);
        glColor3f(cubes[i].r, cubes[i].g, cubes[i].b);
        glRotatef((float)(glutGet(GLUT_ELAPSED_TIME) % 3600) * 0.1f,
                  0.5f, 1.0f, 0.0f); // gentle spin
        glutSolidCube(0.7);
        glPopMatrix();
    }
}

// Drop-zone shadows: a flat dark disc on the floor directly under each cube.
// Helps the player see where the cube will land. Drawn with blending so the
// floor's checker pattern is still visible through it.
static void drawCubeShadows()
{
    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    // draw slightly above the floor to avoid z-fighting
    const float yShadow = 0.02f;
    const int SLICES = 24;

    for (int i = 0; i < NUM_CUBES; ++i)
    {
        // Shadow shrinks/darkens as the cube gets closer to the ground,
        // making the impact point obvious.
        float h = cubes[i].y; // height above floor
        float t = h / 12.0f;
        if (t > 1.0f)
            t = 1.0f;                              // 0 (low) .. 1 (high)
        float radius = 0.55f - 0.20f * (1.0f - t); // 0.35 .. 0.55
        float alpha = 0.55f - 0.30f * t;           // 0.25 .. 0.55 (darker when low)

        glColor4f(0.0f, 0.0f, 0.0f, alpha);
        glBegin(GL_TRIANGLE_FAN);
        glVertex3f(cubes[i].x, yShadow, cubes[i].z);
        for (int s = 0; s <= SLICES; ++s)
        {
            float a = (float)s * 2.0f * 3.14159265f / SLICES;
            glVertex3f(cubes[i].x + cosf(a) * radius,
                       yShadow,
                       cubes[i].z + sinf(a) * radius);
        }
        glEnd();
    }

    // Vertical drop-line marker (a thin dim line from cube down to floor)
    glLineWidth(1.0f);
    glBegin(GL_LINES);
    for (int i = 0; i < NUM_CUBES; ++i)
    {
        glColor4f(0.0f, 0.0f, 0.0f, 0.18f);
        glVertex3f(cubes[i].x, 0.03f, cubes[i].z);
        glVertex3f(cubes[i].x, cubes[i].y, cubes[i].z);
    }
    glEnd();

    glDisable(GL_BLEND);
    glEnable(GL_LIGHTING);
}

// ============================================================
//  Camera
// ============================================================
static void setupCamera()
{
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(55.0, (double)winW / (double)winH, 0.1, 100.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(0.0, 9.0, 12.0, // eye
              0.0, 0.5, 0.0,  // center
              0.0, 1.0, 0.0); // up
}

// ============================================================
//  Game logic
// ============================================================
static bool aabbHit(const FallCube &c)
{
    // Cube half size = 0.35; player paddle half extents PLAYER_W/PLAYER_D, Y range [PLAYER_Y-0.25, PLAYER_Y+0.25]
    const float ch = 0.35f;
    bool ix = (c.x + ch >= playerX - PLAYER_W) && (c.x - ch <= playerX + PLAYER_W);
    bool iz = (c.z + ch >= playerZ - PLAYER_D) && (c.z - ch <= playerZ + PLAYER_D);
    bool iy = (c.y - ch <= PLAYER_Y + 0.5f);
    return ix && iz && iy;
}

static void updatePlay()
{
    // Movement
    if (kLeft)
        playerX -= PLAYER_SPEED;
    if (kRight)
        playerX += PLAYER_SPEED;
    if (kUp)
        playerZ -= PLAYER_SPEED;
    if (kDown)
        playerZ += PLAYER_SPEED;
    // Clamp inside floor
    if (playerX < -FLOOR_HALF + PLAYER_W)
        playerX = -FLOOR_HALF + PLAYER_W;
    if (playerX > FLOOR_HALF - PLAYER_W)
        playerX = FLOOR_HALF - PLAYER_W;
    if (playerZ < -FLOOR_HALF + PLAYER_D)
        playerZ = -FLOOR_HALF + PLAYER_D;
    if (playerZ > FLOOR_HALF - PLAYER_D)
        playerZ = FLOOR_HALF - PLAYER_D;

    // Falling cubes
    for (int i = 0; i < NUM_CUBES; ++i)
    {
        cubes[i].y -= CUBE_FALL_SPEED;
        if (aabbHit(cubes[i]))
        {
            score += 10;
            resetCube(cubes[i]);
        }
        else if (cubes[i].y <= 0.0f)
        {
            score -= 5;
            resetCube(cubes[i]);
        }
    }
}

// ============================================================
//  Draw screens
// ============================================================
static void drawHUD()
{
    char buf[64];
    begin2D();
    glColor3f(1.0f, 1.0f, 1.0f);
    snprintf(buf, sizeof(buf), "Score: %d", score);
    drawText2D(15.0f, (float)winH - 25.0f, buf);

    snprintf(buf, sizeof(buf), "Time: %d", timeLeft);
    drawText2D((float)winW - 110.0f, (float)winH - 25.0f, buf);

    drawText2D(15.0f, 20.0f, "Move: Arrows / WASD    ESC: Menu", GLUT_BITMAP_HELVETICA_12);
    end2D();
}

static void drawMenu()
{
    // 3D backdrop: same floor for atmosphere
    drawFloor();

    // Two 3D button cubes
    const char *labels[2] = {"START", "EXIT"};
    for (int i = 0; i < 2; ++i)
    {
        glPushMatrix();
        glTranslatef(-2.5f + 5.0f * i, 1.0f, 0.0f);
        if (i == menuSelection)
        {
            glColor3f(1.0f, 0.85f, 0.2f);
            glScalef(1.15f, 1.15f, 1.15f);
        }
        else
        {
            glColor3f(0.3f, 0.5f, 0.9f);
        }
        glutSolidCube(1.6);
        glPopMatrix();
    }

    // 2D overlay text
    begin2D();
    glColor3f(1.0f, 1.0f, 1.0f);
    drawText2D(winW * 0.5f - 120.0f, winH - 60.0f,
               "CATCH THE FALLING CUBES", GLUT_BITMAP_TIMES_ROMAN_24);
    drawText2D(winW * 0.5f - 110.0f, winH - 90.0f,
               "Use Up/Down to choose, Enter to confirm", GLUT_BITMAP_HELVETICA_12);

    // Button labels
    drawText2D(winW * 0.5f - 150.0f, winH * 0.5f - 10.0f, labels[0]);
    drawText2D(winW * 0.5f + 110.0f, winH * 0.5f - 10.0f, labels[1]);
    end2D();
}

static void drawGame()
{
    drawFloor();
    drawCubeShadows();
    drawPlayer();
    drawFallingCubes();
    drawHUD();
}

static void drawGameOver()
{
    drawFloor();
    drawPlayer();

    begin2D();
    glColor3f(1.0f, 0.3f, 0.3f);
    drawText2D(winW * 0.5f - 70.0f, winH * 0.5f + 30.0f,
               "GAME OVER", GLUT_BITMAP_TIMES_ROMAN_24);

    char buf[64];
    snprintf(buf, sizeof(buf), "Final Score: %d", score);
    glColor3f(1.0f, 1.0f, 1.0f);
    drawText2D(winW * 0.5f - 70.0f, winH * 0.5f, buf);
    drawText2D(winW * 0.5f - 110.0f, winH * 0.5f - 30.0f,
               "Press R to restart   ESC to menu", GLUT_BITMAP_HELVETICA_12);
    end2D();
}

// ============================================================
//  GLUT callbacks
// ============================================================
static void display()
{
    glClearColor(0.55f, 0.78f, 0.95f, 1.0f); // sky
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    setupCamera();

    switch (gameState)
    {
    case STATE_MENU:
        drawMenu();
        break;
    case STATE_PLAY:
        drawGame();
        break;
    case STATE_OVER:
        drawGameOver();
        break;
    }
    glutSwapBuffers();
}

static void reshape(int w, int h)
{
    winW = w;
    winH = (h > 0) ? h : 1;
    glViewport(0, 0, winW, winH);
}

// ~60 FPS update loop
static void frameTimer(int)
{
    if (gameState == STATE_PLAY)
        updatePlay();
    glutPostRedisplay();
    glutTimerFunc(16, frameTimer, 0);
}

// 1-second tick for countdown
static void secondTimer(int)
{
    if (gameState == STATE_PLAY)
    {
        timeLeft--;
        if (timeLeft <= 0)
        {
            timeLeft = 0;
            gameState = STATE_OVER;
        }
    }
    glutTimerFunc(1000, secondTimer, 0);
}

static void keyDown(unsigned char key, int, int)
{
    if (key == 27)
    { // ESC
        if (gameState == STATE_MENU)
            exit(0);
        gameState = STATE_MENU;
        menuSelection = 0;
        return;
    }
    if (gameState == STATE_MENU)
    {
        if (key == 13)
        { // Enter
            if (menuSelection == 0)
            {
                resetGame();
                gameState = STATE_PLAY;
            }
            else
                exit(0);
        }
        return;
    }
    if (gameState == STATE_OVER && (key == 'r' || key == 'R'))
    {
        resetGame();
        gameState = STATE_PLAY;
        return;
    }
    // WASD during play
    if (gameState == STATE_PLAY)
    {
        if (key == 'a' || key == 'A')
            kLeft = true;
        if (key == 'd' || key == 'D')
            kRight = true;
        if (key == 'w' || key == 'W')
            kUp = true;
        if (key == 's' || key == 'S')
            kDown = true;
    }
}

static void keyUpFn(unsigned char key, int, int)
{
    if (key == 'a' || key == 'A')
        kLeft = false;
    if (key == 'd' || key == 'D')
        kRight = false;
    if (key == 'w' || key == 'W')
        kUp = false;
    if (key == 's' || key == 'S')
        kDown = false;
}

static void specialDown(int key, int, int)
{
    if (gameState == STATE_MENU)
    {
        if (key == GLUT_KEY_LEFT || key == GLUT_KEY_UP)
            menuSelection = 0;
        if (key == GLUT_KEY_RIGHT || key == GLUT_KEY_DOWN)
            menuSelection = 1;
        return;
    }
    if (gameState == STATE_PLAY)
    {
        if (key == GLUT_KEY_LEFT)
            kLeft = true;
        if (key == GLUT_KEY_RIGHT)
            kRight = true;
        if (key == GLUT_KEY_UP)
            kUp = true;
        if (key == GLUT_KEY_DOWN)
            kDown = true;
    }
}

static void specialUp(int key, int, int)
{
    if (key == GLUT_KEY_LEFT)
        kLeft = false;
    if (key == GLUT_KEY_RIGHT)
        kRight = false;
    if (key == GLUT_KEY_UP)
        kUp = false;
    if (key == GLUT_KEY_DOWN)
        kDown = false;
}

// ============================================================
//  Init
// ============================================================
static void initGL()
{
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

    GLfloat lightPos[] = {4.0f, 10.0f, 6.0f, 1.0f};
    GLfloat lightAmb[] = {0.35f, 0.35f, 0.35f, 1.0f};
    GLfloat lightDiff[] = {0.9f, 0.9f, 0.9f, 1.0f};
    glLightfv(GL_LIGHT0, GL_POSITION, lightPos);
    glLightfv(GL_LIGHT0, GL_AMBIENT, lightAmb);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, lightDiff);

    glShadeModel(GL_SMOOTH);
}

int main(int argc, char **argv)
{
    srand((unsigned)time(NULL));
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(winW, winH);
    glutCreateWindow("Catch the Falling Cubes - 3D");

    initGL();
    resetGame();

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyDown);
    glutKeyboardUpFunc(keyUpFn);
    glutSpecialFunc(specialDown);
    glutSpecialUpFunc(specialUp);
    glutTimerFunc(16, frameTimer, 0);
    glutTimerFunc(1000, secondTimer, 0);

    glutMainLoop();
    return 0;
}
