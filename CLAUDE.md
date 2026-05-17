# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project

A 2D side-scrolling shooter ("survival") from ~2006, built for **Windows** with **Visual Studio 2003 (.NET)** and **DirectX 9** (Direct3D 9 + DirectInput 8). The source tree currently lives on Linux but the code is Win32-only (uses `windows.h`, `WinMain`, `GetPrivateProfileString`, etc.) and cannot be built or run on this host — treat work here as source editing only unless the user moves to a Windows box.

## Build

- Solution: `game.sln` (Format Version 8.00 / VS 2003); project: `game.vcproj` (`Version="7.10"`). Configurations: `Debug|Win32`, `Release|Win32`. Output: `Debug/game.exe` or `Release/game.exe`.
- Requires the **DirectX 9 SDK** on the include/lib path. Libs are pulled via `#pragma comment(lib, ...)` in `gamecode.h`: `d3d9`, `d3dx9`, `dxerr9`, `dinput8` (plus `winmm` in `main.cpp`).
- No CMake / no test suite / no lint config. There is no headless way to run the game — `WinMain` creates a window and enters a `PeekMessage` loop immediately.
- `game.vcproj` lists sources explicitly. When adding a `.cpp`/`.h` under `physics/` or `graphics/`, also add a `<File RelativePath="...">` entry under the matching `<Filter>`, otherwise VS 2003 will not compile it.

## Runtime layout

The exe expects to be launched from a directory that contains, as siblings:
- `settings.ini` — INI consumed via `GetPrivateProfileString/Int` in `main.cpp`. Sections/keys: `[Game settings] Map=`, `[Display settings] Screen_width/Screen_height/Refresh_rate/Antialiasing_level/Run_fullscreen`.
- `data/maps/<Map>` — map file named by `settings.ini`. Loaded by `LoadMap()` in `gamecode.cpp`.
- `data/meshes/ragdoll/*.dat` — 15 ragdoll body-part meshes (`HEAD`, `BODY`, `BELT`, `RSHOULDER`, ... see `graphics/model.h`).
- `data/models/soldat.m2d` (+ numbered `_texture_N` files) — player model.
- `data/textures/*.jpg|tga` — materials, indexed by `MATERIAL_TYPE` (`BRICKS`, `METAL`, `PLASTIC`, `WOOD`).
- `data/config/mat.cfg` — per-material tile sizes (one `w h` pair per line, in `MATERIAL_TYPE` order).
- `data/sprites/`, `data/models/` — UI sprites, weapon sprites, particle textures.

`d3dg/`, `d3dg2/`, `a/`, `Debug/`, `Release/`, `builds/`, the `*.rar` archives and the various standalone `.exe` files (`animator.exe`, `map_editor.exe`, `particlesystem.exe`, `UI.exe`) are historical snapshots / external tools, not part of the build. Do not edit them when changing game source.

## Architecture

Three roughly-layered modules, all linked into one executable:

1. **`physics/`** (`math2D`, `physics2D`, `polygon.h`) — 2D vector math + rigid-body simulator. `RIGIDBODY` carries polygon vertices, mass/inertia, velocity, force/torque, restitution/friction, and a `lMaterialID`. SAT-style `Collide(...)` returns an MTD and time-of-impact; `ResolveCollision`, `ApplyImpulse`, `Update(dt)` integrate motion. `JOINT` links two `RIGIDBODY`s by local points — used to assemble the 15-part ragdoll. `RayIntersect` / `CircleIntersect` back the hitscan/explosion code.
2. **`graphics/`** (`sprite2`, `model`, `particles`) — D3D9 drawing. `SPRITE` is the basic textured quad. `MODEL` is the 15-part articulated soldier driven by `KEYFRAME[NUM_PARTS]` arrays loaded from `.m2d`; animations are described by `ANIMATION { iStartFrame, iEndFrame, fRate, dwTime, iType }` with type `ANIMATION_SINGLE|LOOP|RETURN`. Particle systems back fire/smoke/impact effects.
3. **Game layer** — `main.cpp`, `gamecode.{h,cpp}`, `object.{h,cpp}`. `main.cpp` is pure Win32 boilerplate: read `settings.ini`, register window class, call `InitD3D` → `ShowSplash` → `InitDI` → `LoadMap` → `LoadGameData`, then loop `UpdateScene(Delta) / UpdateFrame()`. All game state and rendering logic live in `gamecode.cpp` as file-scope globals (`g_pD3D`, `g_pDevice`, `m_aPlayers[MAX_PLAYERS]`, `aWalls`, `aBodies`, `aRespawns`, `aPacks`, `aWayPoints`, `apPathParent`/`apPathDistance`, etc.). `object.{h,cpp}` is a tiny intrusive linked-list of (`SPRITE*`, `RIGIDBODY*`, TTL) for transient world objects.

### Cross-cutting things to know

- **Tuning lives in `gamecode.h`**, not in data files: physics constants (`RUN_ACC`, `JUMP_ACC`, `FRICTION`, `BOUNCE`, `SHOOT_FORCE`, `EXPLODE_FORCE`, gravity in `physics2D.h::g`), AI thresholds (`NEAR_DISTANCE`, `FAR_DISTANCE`), pack sizes, particle counts, and the gameplay enums (`ACTION`, `STATE`, `WEAPON`, `MATERIAL_TYPE`, `PACK_TYPE`, `AI_STATE_TYPE`, `ANIMATION_TYPE`). Touch these with care — changing an enum order will silently desync the parallel arrays indexed by it (e.g. `g_szMaterialFile`, `aPacks`, the `animations[]` table).
- **AI pathing** uses precomputed `apPathParent` / `apPathDistance` matrices over `aWayPoints` (Floyd-Warshall-style). `debug.txt` in the repo root is a dump of one such parent table — keep that in mind if you see it change.
- **PLAYER** (defined in `gamecode.h`) owns a `MODEL`, a body `RIGIDBODY`, a 15-element `ragdoll[NUM_PARTS]` plus 13 `joints[]`, a thrown grenade body, weapon ammo, and per-action timers. The same struct is used for AI- and human-controlled players; `GetAIActions(index, bool *actions)` fills the same `actions[NUMACTIONS]` array that the keyboard handler would.
- **DirectInput is exclusive-mode** and the message loop calls `SetCursorPos(hsx, hsy)` every frame and hides the OS cursor — debugging via remote desktop / VMs tends to fight this.
- `#define DEBUG` / `#define GODMODE` at the top of `gamecode.cpp` are commented out — flip them locally rather than wiring new ones in.

## Conventions in this codebase (be aware, don't "fix" wholesale)

- C-with-classes style: file-scope globals, raw `new`/`delete`, manual `FILE*` I/O, `memset`/`strcpy` on fixed `char[1024]` buffers, Hungarian-ish prefixes (`g_`, `m_`, `p`, `sz`, `i`, `f`, `b`). This is consistent throughout — match it when editing existing files rather than refactoring to modern C++.
- `using namespace std;` at the top of `gamecode.h` (which is included widely). Don't add `std::`-qualified names in a way that conflicts.
- Includes use **backslash** paths (`#include "graphics\model.h"`). These compile on MSVC; if you ever build on a case-sensitive/forward-slash toolchain you'll need to fix them.
- The project targets MSVC 7.1 — avoid C++11+ features (`auto`, range-for, `nullptr`, `<cstdint>`, lambdas, `std::unique_ptr`, etc.); they won't compile.
