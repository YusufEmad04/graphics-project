# Catch the Falling Cubes — 3D OpenGL Mini Game

A minimal 3D game written in C++ with **OpenGL + GLUT**, satisfying every requirement
in `project.md` (and the **3D bonus**) with the smallest reasonable code base.

---

## Team submission info
Edit the header comment at the top of [main.cpp](main.cpp) and fill in:

```text
Team members  : <Member 1 Name>, <Member 2 Name>
Reg. numbers  : <Reg 1>, <Reg 2>
TA            : <TA Name>
```

---

## 1. The game in one paragraph
A 3D paddle stands on a checkered floor under a blue sky. Colored cubes spawn high
in the air and fall straight down. You move the paddle around the floor with the
**arrow keys** or **WASD**. Catch a cube → **+10**. Let it hit the floor → **−5**.
You have **60 seconds**. When the timer hits zero you see your final score and can
restart with **R** or return to the menu with **ESC**.

### Controls

| Action | Keys |
|---|---|
| Move paddle | `←` `→` `↑` `↓`  or  `A` `D` `W` `S` |
| Choose menu item | `←` `→` `↑` `↓` |
| Confirm menu choice | `Enter` |
| Restart after game over | `R` |
| Back to menu / Exit (from menu) | `Esc` |

### Game states
1. **MENU** — title text + two 3D buttons (`START`, `EXIT`). The selected button is
   yellow and slightly larger.
2. **PLAYING** — the live game with HUD showing **Score** (top-left) and **Time**
   (top-right).
3. **GAME OVER** — final score, hint to restart.

---

## 2. How the requirements are met

| # | Requirement | Where in code |
|---|---|---|
| 1 | Text timer | `drawHUD()` → `glutBitmapCharacter`, decremented in `secondTimer()` |
| 2 | Text score | `drawHUD()` shows `score`; updated in `updatePlay()` |
| 3 | Patterned background | `drawFloor()` — procedural checker pattern (16×16 tiles) |
| 4 | Player from ≥3 shapes, keyboard-movable | `drawPlayer()` (cube + cube + cone + 2 spheres); movement in `updatePlay()` driven by `kLeft/kRight/kUp/kDown` |
| 5 | Main menu with Start / Exit | `drawMenu()` + `keyDown()` switching `gameState` |
| 6 | Interactive score & countdown timer | `updatePlay()` adjusts score on hit/miss; `secondTimer()` triggers `STATE_OVER` at 0 |
| Bonus | 3D | `gluPerspective` + `gluLookAt` in `setupCamera()`, depth test on, lighting on |

---

## 3. Project layout

```
graphics-project/
├── main.cpp                 ← the entire game (single file)
├── build.bat                ← MSVC build script (calls vcvars64 + cl.exe)
├── game.exe                 ← produced after build
├── project.md               ← original assignment brief
├── README.md                ← this file
├── .vscode/
│   ├── tasks.json           ← Ctrl+Shift+B = Build, "Run game" task = run
│   └── settings.json        ← IntelliSense include path
├── build/                   ← *.obj output (auto-created)
└── lib/
    ├── include/GL/*.h       ← freeglut headers
    └── lib/freeglut.lib     ← static freeglut (MSVC)
```

---

## 4. Building & running

### A) On your PC (Windows + Visual Studio 2022)
Two options — pick whichever you prefer.

**Option 1 — VS Code (recommended)**
1. Open the folder in VS Code.
2. Press **Ctrl + Shift + B** → runs the *Build game* task (calls `build.bat`).
3. Open the integrated terminal and run `./game.exe`, **or** run the *Run game* task
   from `Terminal → Run Task…`.

**Option 2 — Plain command line**
Open *“x64 Native Tools Command Prompt for VS 2022”* (or any shell), then:
```bat
cd C:\Users\Acer\ma3bad\graphics-project
build.bat
game.exe
```

`build.bat` automatically loads `vcvars64.bat`, so you don’t need a special shell.

### B) On a college machine with Code::Blocks + GLUT (MinGW)
The source is **classic-GLUT compatible** — only standard `glut.h` calls
(`glutSolidCube`, `glutSolidCone`, `glutSolidSphere`, `glutBitmapCharacter`, etc.).

1. Open Code::Blocks → **File → New → Project → Empty Project**, name it anything.
2. Drag `main.cpp` into the project (or *Project → Add files…*).
3. **Project → Build options… → Linker settings → Other linker options**, add:
   ```
   -lopengl32 -lglu32 -lfreeglut
   ```
   (On the older lab template the lib may be called `glut32` — try `-lglut32`
   if `-lfreeglut` doesn’t resolve.)
4. Make sure the GLUT setup the lab uses is on the include / lib path
   (`glut.h` reachable, `freeglut.dll` / `glut32.dll` next to the produced `.exe`).
5. Press **F9** (Build & Run).

> The `#define FREEGLUT_STATIC` block at the top of `main.cpp` is wrapped in
> `#ifdef _MSC_VER`, so it only kicks in on MSVC. On MinGW / Code::Blocks the file
> compiles as plain GLUT.

### C) Any other compiler quickly (MinGW shell)
```bash
g++ main.cpp -o game.exe -lopengl32 -lglu32 -lfreeglut
./game.exe
```

---

## 5. Walk-through of `main.cpp`

The file is a single ~370-line C++ program organised top-to-bottom.

### 5.1 Configuration & globals
* `winW`, `winH` — current window size.
* `FLOOR_HALF` — half-extent of the play floor (so the floor spans `[-8, +8]`).
* `PLAYER_SPEED`, `CUBE_FALL_SPEED`, `NUM_CUBES`, `GAME_SECONDS` — gameplay tuning.
* `enum GameState { STATE_MENU, STATE_PLAY, STATE_OVER }` — the three screens.
* `score`, `timeLeft`, `playerX/Z`, plus a `FallCube` struct holding `(x, y, z, r, g, b)`
  for each falling cube.
* `kLeft / kRight / kUp / kDown` — boolean key-state flags so movement is smooth
  (held key → continuous motion) instead of the choppy autorepeat behaviour.

### 5.2 Helpers
* `randf(a, b)` — uniform random float in `[a, b]`.
* `resetCube(c)` — places a cube at random `x, z` with `y` somewhere above the world
  and a fresh random vivid colour.
* `resetGame()` — score 0, timer 60, paddle centred, all cubes respawned.
* `drawText2D(x, y, s)` — uses `glutBitmapCharacter` to draw text in pixel
  coordinates.
* `begin2D() / end2D()` — push perspective matrix, switch to `gluOrtho2D` for HUD,
  disable depth & lighting, then restore on `end2D()`.

### 5.3 Drawing the world
* **`drawFloor()`** — nested loops over a 16×16 grid; each tile is a `GL_QUADS`
  with a colour that depends on `(i + j) & 1`, producing the checker pattern (the
  required “patterned background”). A black `GL_LINE_LOOP` border frames it.
* **`drawPlayer()`** — translates to `(playerX, PLAYER_Y, playerZ)` and draws:
  1. a wide flat **cube** (the paddle base — scaled `glutSolidCube`),
  2. a small **cube** on top (the “head”),
  3. a red **cone** for the nose pointing in `-Z`,
  4. two tiny black **spheres** for eyes (extra detail, still trivial).
  This satisfies the “three shapes minimum” requirement.
* **`drawFallingCubes()`** — for each `FallCube` translates to its position, sets
  its colour, applies a tiny rotation animation, and calls `glutSolidCube`.

### 5.4 Camera
`setupCamera()` sets a perspective projection (`gluPerspective(55°, …)`) and a
fixed `gluLookAt(eye=(0, 9, 12), center=(0, 0.5, 0), up=Y)`. The result is an
angled overhead view that clearly shows depth — this is what makes the game *3D*
rather than just orthographic 2D.

### 5.5 Game logic
* `aabbHit(c)` — axis-aligned bounding-box test between the falling cube
  (half-size 0.35) and the paddle (half extents `PLAYER_W` × `PLAYER_D`, plus a
  vertical tolerance). Cheap and good enough for a minimal game.
* `updatePlay()` —
  * Apply movement based on the key flags.
  * Clamp the paddle inside the floor.
  * For each cube: drop it by `CUBE_FALL_SPEED`, then  
    – if it hits the paddle → `score += 10`, respawn,  
    – else if `y <= 0` → `score -= 5`, respawn.

### 5.6 Screens (`display()` switches on `gameState`)
* `drawMenu()` — checker floor as backdrop + two coloured cubes (`START`, `EXIT`);
  the selected one is bigger and yellow. 2D overlay shows the title and the
  selected button’s label.
* `drawGame()` — floor + player + falling cubes + HUD.
* `drawGameOver()` — floor + paddle + “GAME OVER” + final score + restart hint.

### 5.7 Timing — two `glutTimerFunc` chains
* **Frame timer** (`16 ms` ≈ 60 FPS) calls `updatePlay()` and `glutPostRedisplay()`
  to keep the visuals smooth.
* **Second timer** (`1000 ms`) decrements `timeLeft`. When it reaches zero the
  state flips to `STATE_OVER`. (A second source of truth is fine here because
  the second timer only edits one variable that the frame loop reads.)

### 5.8 Input
* `keyDown` / `keyUpFn` (regular keys) — toggle the WASD flags, and handle
  ESC / Enter / R for state changes.
* `specialDown` / `specialUp` — toggle arrow-key flags; on the menu, arrow keys
  move the menu selection.

### 5.9 Init & main
`initGL()` enables depth test, one directional light (`GL_LIGHT0`),
`glColorMaterial` (so `glColor3f` controls material colour), and smooth shading.

`main()`:
1. seeds `rand`,
2. creates a double-buffered RGB+depth window,
3. registers all the GLUT callbacks above,
4. starts the two timers,
5. enters `glutMainLoop()`.

---

## 6. The MSVC quirks (why the build script looks slightly weird)

The freeglut binary used here is the **NuGet `freeglut 2.8.1.15` package**, which
ships only static `.lib` files compiled with **VS 2012 (v110)**. On modern MSVC
(2015+) two compatibility shims are needed — both already handled:

1. **`legacy_stdio_definitions.lib`** is added to the linker line because the
   old lib calls `printf`, `sprintf`, etc. as exported functions. UCRT made them
   inline, so we link the legacy stubs back in.
2. **`__iob_func` shim** — UCRT removed this internal helper, so `main.cpp`
   defines a tiny replacement that returns an array of `stdin / stdout / stderr`.
   This is wrapped in `#if defined(_MSC_VER) && _MSC_VER >= 1900` so it only
   compiles on MSVC ≥ 2015.
3. **`/MT`** (static CRT) is used to match how the freeglut lib was built and
   avoid CRT mismatch warnings.

None of these matter on Code::Blocks/MinGW — the `#ifdef _MSC_VER` guards skip
them automatically.

---

## 7. Submission checklist
- [x] Game source: [main.cpp](main.cpp)
- [x] 3D (bonus) — perspective camera + depth + lighting
- [x] Timer (text), Score (text), patterned background, 3-shape player,
      main menu (Start/Exit), interactive score & timer
- [ ] **TODO before turning in:** fill the team / TA names in the comment header
      of `main.cpp`
- [ ] **TODO before turning in:** include a screenshot of the running game
