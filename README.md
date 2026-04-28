# Catch the Falling Cubes — 3D OpenGL Game
## Code Walkthrough

This document walks through every part of `main.cpp` from top to bottom, explaining what each section does and why in simple language.

---

## How to Run

**On Windows with Visual Studio 2022:**
1. Double-click `build.bat` — it compiles everything automatically.
2. Double-click `game.exe` — the game window opens.

**On college Code::Blocks (MinGW + GLUT):**
1. Create a new Empty Project, add `main.cpp`.
2. In *Project → Build options → Linker settings → Other linker options* add:
   ```
   -lopengl32 -lglu32 -lfreeglut
   ```
   (use `-lglut32` if the lab uses the older GLUT)
3. Press **F9**.

---

## Controls

| Action | Key |
|---|---|
| Move paddle | Arrow keys or `W A S D` |
| Navigate menu | Arrow keys or `W S` |
| Confirm menu choice | `Enter` |
| Restart after game over | `R` |
| Back to menu / quit | `Esc` |

---

## Game Rules
- 3 coloured cubes fall from the sky at random positions.
- Move your paddle under them to **catch** a cube → **+10 score**.
- If a cube hits the floor → **−5 score**.
- You have **60 seconds**. When the timer hits zero → Game Over.
- Press `R` to restart, `Esc` to return to the menu.

---

## Part 1 — Includes & MSVC compatibility (lines 1–22)

```cpp
#ifdef _MSC_VER
#define FREEGLUT_STATIC
#endif
#include <GL/glut.h>
```

`glut.h` is the main OpenGL/GLUT header — it gives us every drawing function we use (`glutSolidCube`, `glutBitmapCharacter`, etc.) and manages the window.

`FREEGLUT_STATIC` tells freeglut we are linking it as a static library (no separate `.dll` needed at runtime). This define is only active on MSVC, not on CodeBlocks/MinGW.

`msvc_shim.cpp` (compiled separately) fixes a missing symbol (`__iob_func`) that the old prebuilt freeglut library expects but modern Visual Studio removed. You don't need to touch it — it just has to be compiled together with `main.cpp`.

---

## Part 2 — Constants (lines 24–30)

```cpp
static int   winW = 900, winH = 650;
static const float FLOOR_HALF      = 8.0f;
static const float PLAYER_SPEED    = 0.35f;
static const float CUBE_FALL_SPEED = 0.06f;
static const int   NUM_CUBES       = 3;
static const int   GAME_SECONDS    = 60;
```

These are the tuning knobs for the whole game:

- **`FLOOR_HALF = 8`** — the play area runs from −8 to +8 on both X and Z axes, making a 16×16 unit square floor.
- **`PLAYER_SPEED = 0.35`** — how many units the paddle moves each frame when you hold a key.
- **`CUBE_FALL_SPEED = 0.06`** — how many units a falling cube drops each frame.
- **`NUM_CUBES = 3`** — how many cubes are falling at the same time.
- **`GAME_SECONDS = 60`** — the countdown starts here.

---

## Part 3 — Game state (lines 32–57)

```cpp
enum GameState { STATE_MENU, STATE_PLAY, STATE_OVER };
static int gameState = STATE_MENU;
```

The game has three screens. `gameState` stores which one is currently shown.
The `display()` function switches on this value to decide what to draw.

```cpp
static int score = 0;
static int timeLeft = GAME_SECONDS;
static int menuSelection = 0; // 0 = START, 1 = EXIT
```

`menuSelection` remembers which menu button is highlighted so the player can switch between them with the arrow keys.

```cpp
static float playerX = 0.0f, playerZ = 4.0f;
static const float PLAYER_W = 1.6f;   // half-width
static const float PLAYER_D = 0.8f;   // half-depth
static const float PLAYER_Y = 0.25f;  // height above floor
```

The paddle lives on the floor. Its position is stored as `(playerX, playerZ)` — Y is always fixed. `PLAYER_W` and `PLAYER_D` are the *half-extents* used for collision checking.

```cpp
struct FallCube { float x, y, z, r, g, b; };
static FallCube cubes[NUM_CUBES];
```

Each falling cube stores its 3D position and its colour (r, g, b).

```cpp
static bool kLeft, kRight, kUp, kDown;
```

Instead of moving the paddle only on the frame when a key is pressed (which feels choppy), we track *whether each key is currently held down*. Every frame we check these flags and move the paddle if they're true, giving smooth continuous movement.

---

## Part 4 — Helper functions (lines 59–120)

### `randf(a, b)`
Returns a random float between `a` and `b`. Used to spawn cubes at random positions and with random colours.

### `resetCube(c)`
Teleports a cube to a random X/Z position inside the floor boundary and sends it to a random height between 8 and 14 units (so they don't all start at exactly the same point). Also assigns it a new random colour.

### `resetGame()`
Called when starting or restarting. Sets score to 0, timer to 60, moves the paddle to the centre, and calls `resetCube` on every cube.

### `drawText2D(x, y, s)`
Places 2D text at a pixel position on screen. Internally it calls `glutBitmapCharacter` in a loop — one call per character.

### `begin2D()` and `end2D()`
The game normally runs in 3D perspective mode. To draw the HUD (score, timer, hints) we need to switch to 2D mode temporarily:

- `begin2D()` saves the current projection matrix, replaces it with `gluOrtho2D` (a flat 2D view where coordinates match pixel positions), and disables depth testing and lighting so text is drawn flat on top of everything.
- `end2D()` restores the saved matrix and turns depth test and lighting back on.

---

## Part 5 — Drawing the floor (lines 122–158) — `drawFloor()`

```cpp
const int N = 16;
const float step = (FLOOR_HALF * 2.0f) / N;
for (int i = 0; i < N; ++i)
    for (int j = 0; j < N; ++j) {
        if ((i + j) & 1) glColor3f(dark green);
        else             glColor3f(light green);
        // draw one GL_QUADS tile
    }
```

This nested loop draws a **16×16 checker pattern** on the floor. The trick `(i + j) & 1` is just checking whether the sum of column + row is odd or even — that naturally alternates colours in a checkerboard. Each tile is a flat quad (`GL_QUADS`) with Y = 0 (ground level).

This satisfies the *"patterned background"* project requirement.

A `GL_LINE_LOOP` border is drawn around the whole floor as a dark frame.

---

## Part 6 — Drawing the player (lines 160–202) — `drawPlayer()`

```cpp
glPushMatrix();
glTranslatef(playerX, PLAYER_Y, playerZ);
```

`glPushMatrix` saves the current coordinate system. `glTranslatef` moves the origin to where the paddle is — every subsequent draw call is now relative to that position.

The paddle is made of **5 shapes**:

| # | Shape | Function | Colour |
|---|---|---|---|
| 1 | Wide flat cube | base / paddle | blue |
| 2 | Smaller cube | head | yellow |
| 3 | Cone | nose, pointing forward | red |
| 4 | Sphere | left eye | black |
| 5 | Sphere | right eye | black |

Each sub-shape uses its own `glPushMatrix / glPopMatrix` pair so its translation or scale doesn't affect the others.

```cpp
glScalef(PLAYER_W * 2.0f, 0.5f, PLAYER_D * 2.0f);
glutSolidCube(1.0);
```

`glScalef` stretches the cube before drawing it — makes it wide and flat.

```cpp
glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
glutSolidCone(0.18, 0.6, 12, 12);
```

`glutSolidCone` points upward by default. Rotating −90° around the X axis makes it point in the −Z direction (forward).

This satisfies the *"player from at least 3 shapes"* project requirement.

---

## Part 7 — Drawing the falling cubes (lines 204–217) — `drawFallingCubes()`

```cpp
glRotatef((float)(glutGet(GLUT_ELAPSED_TIME) % 3600) * 0.1f, 0.5f, 1.0f, 0.0f);
glutSolidCube(0.7);
```

`glutGet(GLUT_ELAPSED_TIME)` returns the number of milliseconds since the program started. Multiplied by 0.1 and used as a rotation angle — this makes each cube slowly spin in place as it falls, which makes the 3D depth much more visible.

---

## Part 8 — Drop-zone shadows (lines 219–268) — `drawCubeShadows()`

```cpp
glEnable(GL_BLEND);
glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
```

Blending makes it possible to draw semi-transparent things. `GL_SRC_ALPHA / GL_ONE_MINUS_SRC_ALPHA` is the standard formula for "paint this colour on top with transparency controlled by the alpha value".

For each cube, we compute:
- **`t`** — how high the cube is (0 = at the floor, 1 = at the top).
- **`radius`** — the shadow disc grows larger when the cube is higher (easier to see the landing zone), shrinks as it gets close.
- **`alpha`** — the shadow gets darker (more opaque) as the cube gets close to the floor, giving a clear warning.

```cpp
glBegin(GL_TRIANGLE_FAN);
glVertex3f(cubes[i].x, yShadow, cubes[i].z);  // centre
for (int s = 0; s <= SLICES; ++s) { ... }      // ring of points
```

`GL_TRIANGLE_FAN` draws a filled circle by connecting a centre point to a ring of outer points with triangles.

A faint vertical line (`GL_LINES`) is also drawn straight down from each cube to the floor — a crosshair marker so you can see exactly which column the cube is in.

---

## Part 9 — Camera (lines 270–282) — `setupCamera()`

```cpp
gluPerspective(55.0, aspect, 0.1, 100.0);
gluLookAt(0.0, 9.0, 12.0,   // camera is here
          0.0, 0.5,  0.0,   // looking at this point
          0.0, 1.0,  0.0);  // Y is "up"
```

`gluPerspective` sets up a 3D perspective view with a 55° field of view. Objects farther away appear smaller — this is what gives the scene its 3D look. This satisfies the *3D bonus* requirement.

`gluLookAt` positions the camera above and behind the play area, looking down at the centre of the floor. The result is an angled view that makes depth obvious.

This is called at the start of every `display()` call, after clearing the screen.

---

## Part 10 — Collision detection (lines 284–292) — `aabbHit()`

```cpp
bool ix = (c.x + ch >= playerX - PLAYER_W) && (c.x - ch <= playerX + PLAYER_W);
bool iz = (c.z + ch >= playerZ - PLAYER_D) && (c.z - ch <= playerZ + PLAYER_D);
bool iy = (c.y - ch <= PLAYER_Y + 0.5f);
return ix && iz && iy;
```

This is an **AABB (Axis-Aligned Bounding Box)** check. The idea: two boxes overlap if and only if they overlap on *all three axes* at the same time.

- `ch = 0.35` is the half-size of the falling cube.
- The paddle's extents are `PLAYER_W` (X) and `PLAYER_D` (Z).
- `iy` checks that the cube has come down close enough to the paddle height.

If all three are true at the same time, the cube touched the paddle.

---

## Part 11 — Game logic per frame (lines 294–330) — `updatePlay()`

This runs ~60 times per second (driven by `frameTimer`).

**Movement:**
```cpp
if (kLeft)  playerX -= PLAYER_SPEED;
if (kRight) playerX += PLAYER_SPEED;
```
Because `kLeft`/`kRight`/`kUp`/`kDown` are held-key booleans, the paddle moves smoothly every frame while the key is held.

```cpp
if (playerX < -FLOOR_HALF + PLAYER_W) playerX = -FLOOR_HALF + PLAYER_W;
```
These four lines clamp the paddle so it can never slide off the edge of the floor.

**Falling & scoring:**
```cpp
cubes[i].y -= CUBE_FALL_SPEED;
if (aabbHit(cubes[i])) { score += 10; resetCube(cubes[i]); }
else if (cubes[i].y <= 0.0f) { score -= 5; resetCube(cubes[i]); }
```
Every frame each cube drops a tiny amount. If it now overlaps the paddle → caught (+10). If it hit Y=0 (floor level) without being caught → missed (−5). Either way the cube is immediately respawned at a new random position.

---

## Part 12 — HUD (lines 332–349) — `drawHUD()`

Calls `begin2D()` to switch to pixel coordinates, then draws three text strings:
- **Score** — top left.
- **Time** — top right.
- **Controls hint** — bottom left.

Uses `snprintf` to format numbers into strings before drawing them.

This satisfies the *"text timer"* and *"text score"* project requirements.

---

## Part 13 — Menu screen (lines 351–380) — `drawMenu()`

Draws the same 3D floor as a backdrop for atmosphere, then two 3D cubes as buttons:

```cpp
glTranslatef(-2.5f + 5.0f * i, 1.0f, 0.0f);
if (i == menuSelection) {
    glColor3f(1.0f, 0.85f, 0.2f); // yellow = selected
    glScalef(1.15f, 1.15f, 1.15f); // slightly bigger
}
glutSolidCube(1.6);
```

The selected button is yellow and 15% larger. The player toggles with arrow keys; pressing Enter confirms.

Text labels ("START", "EXIT", the title, the hint) are drawn on top in 2D mode.

This satisfies the *"main menu with Start and Exit"* project requirement.

---

## Part 14 — Game over screen (lines 382–400) — `drawGameOver()`

Draws the floor and the paddle still visible in the background, then switches to 2D and writes "GAME OVER" in red, the final score in white, and a restart hint. No special overlay — keeping it simple.

---

## Part 15 — Display callback (lines 402–420) — `display()`

```cpp
glClearColor(0.55f, 0.78f, 0.95f, 1.0f); // sky blue
glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
setupCamera();
switch (gameState) { ... }
glutSwapBuffers();
```

This is called every time OpenGL needs to redraw the window.

- **`glClear`** wipes the screen and the depth buffer (depth buffer tracks which object is in front at each pixel — must be cleared each frame or old data bleeds through).
- **`setupCamera()`** sets the 3D view.
- The `switch` calls the right draw functions for the current game state.
- **`glutSwapBuffers()`** flips the hidden back buffer onto the screen (double buffering — prevents flickering).

---

## Part 16 — Reshape callback (lines 422–426) — `reshape()`

```cpp
winW = w; winH = h;
glViewport(0, 0, winW, winH);
```

Called whenever the user resizes the window. Updates the stored size (used for text positioning and aspect ratio) and adjusts the OpenGL viewport.

---

## Part 17 — Two timer callbacks (lines 428–452)

### `frameTimer` — runs every 16 ms (~60 FPS)
```cpp
if (gameState == STATE_PLAY) updatePlay();
glutPostRedisplay();
glutTimerFunc(16, frameTimer, 0); // schedule itself again
```
This is the game loop. It calls `updatePlay()` to advance physics and scoring, then tells OpenGL to redraw. At the end it schedules itself again — so it keeps looping forever at ~60 Hz.

### `secondTimer` — runs every 1000 ms (1 second)
```cpp
timeLeft--;
if (timeLeft <= 0) gameState = STATE_OVER;
glutTimerFunc(1000, secondTimer, 0);
```
Completely separate from the frame timer. Every real second it decrements the countdown. When it hits zero the game state switches to Game Over. This satisfies the *"interactive timer"* requirement.

---

## Part 18 — Keyboard input (lines 454–556)

There are four callback functions:

| Function | Triggered by |
|---|---|
| `keyDown(key)` | Any regular key pressed |
| `keyUpFn(key)` | Any regular key released |
| `specialDown(key)` | Arrow key pressed |
| `specialUp(key)` | Arrow key released |

**Why four functions?**
Regular letters (`W A S D`) and arrow keys are handled by different GLUT callbacks. We need both *press* and *release* callbacks because we track keys as held-down booleans.

**State-aware input:**
```cpp
if (key == 27) { // ESC
    if (gameState == STATE_MENU) exit(0);
    gameState = STATE_MENU;
}
```
The same key does different things depending on the current screen — ESC exits from the menu, but returns to the menu during play or game over.

```cpp
if (gameState == STATE_PLAY) {
    if (key == 'a') kLeft = true;
    ...
}
```
During play, WASD set the movement flags. On `keyUpFn`, those same flags are cleared. Arrow keys do the same thing in `specialDown`/`specialUp`.

In the menu, arrow keys change `menuSelection` instead.

---

## Part 19 — OpenGL initialisation (lines 558–582) — `initGL()`

```cpp
glEnable(GL_DEPTH_TEST);
```
Depth testing: when two objects overlap on screen, only the one closer to the camera is drawn. Without this, objects would paint on top of each other in draw order, making the scene look broken.

```cpp
glEnable(GL_LIGHTING);
glEnable(GL_LIGHT0);
glEnable(GL_COLOR_MATERIAL);
glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
```
Lighting makes 3D objects look solid — faces pointing toward the light are bright, faces pointing away are darker. `GL_LIGHT0` is one directional light source placed at position `(4, 10, 6)`.

`GL_COLOR_MATERIAL` means we can set the material colour simply with `glColor3f` instead of needing separate `glMaterialfv` calls — keeps the code simple.

```cpp
glShadeModel(GL_SMOOTH);
```
Smooth shading blends colours across the surface of each polygon, so curved shapes (like the cone and spheres) look smooth instead of faceted.

---

## Part 20 — `main()` (lines 584–608)

```cpp
srand((unsigned)time(NULL));
```
Seeds the random number generator with the current time, so food and cube positions are different every run.

```cpp
glutInit(&argc, argv);
glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
glutInitWindowSize(winW, winH);
glutCreateWindow("Catch the Falling Cubes - 3D");
```
Creates the OpenGL window. `GLUT_DOUBLE` = double buffering (prevents flicker), `GLUT_DEPTH` = enables the depth buffer.

```cpp
initGL();
resetGame();
```
Set up OpenGL state and place everything in its starting position.

```cpp
glutDisplayFunc(display);
glutReshapeFunc(reshape);
glutKeyboardFunc(keyDown);
glutKeyboardUpFunc(keyUpFn);
glutSpecialFunc(specialDown);
glutSpecialUpFunc(specialUp);
glutTimerFunc(16,   frameTimer,  0);
glutTimerFunc(1000, secondTimer, 0);
glutMainLoop();
```
Register the callbacks — this tells GLUT which function to call for each event. Start both timers. Then call `glutMainLoop()`, which hands control to OpenGL. From this point on GLUT manages everything: it calls `display()` when needed, calls the keyboard functions when keys are pressed, and calls the timer functions on schedule.

---

## Summary — How everything connects

```
glutMainLoop()
  │
  ├── every 16 ms  → frameTimer()  → updatePlay()  (moves paddle, drops cubes, checks score)
  │                               → glutPostRedisplay()
  │                                         │
  │                                         └── display()
  │                                               ├── setupCamera()
  │                                               ├── drawMenu()      (if MENU)
  │                                               ├── drawGame()      (if PLAY)
  │                                               │     ├── drawFloor()
  │                                               │     ├── drawCubeShadows()
  │                                               │     ├── drawPlayer()
  │                                               │     ├── drawFallingCubes()
  │                                               │     └── drawHUD()
  │                                               └── drawGameOver()  (if OVER)
  │
  ├── every 1000 ms → secondTimer() → timeLeft--  (triggers game over at 0)
  │
  └── on key events → keyDown / keyUpFn / specialDown / specialUp
                          └── update kLeft/kRight/kUp/kDown flags
                               or change gameState (menu navigation, restart, ESC)
```

---

## Team Info
Fill in the header at the top of `main.cpp` before submitting:
```
Team members : <Name 1>, <Name 2>
Reg. numbers : <Reg 1>, <Reg 2>
TA           : <TA Name>
```
