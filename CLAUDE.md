# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project

A 2D side-scrolling shooter ("survival") originally written in 2006 for **Windows / Visual Studio 2003 / DirectX 9**. The Phase 1–3 port (see `docs/PORT_AUDIT.md` and the `legacy-dx9` → `linux-mvp` → `phase-2-complete` → `phase-1-complete` tag arc) rewrote the rendering on **SDL2** (renderer + image + ttf + gfx) and modernised the codebase to **C++20** with `std::filesystem` / `std::ifstream` / `std::vector` / `enum class` / RAII. The game builds and runs on **Linux** (GCC / Clang) and **Windows** (MSVC).

The legacy DX9 source tree is reachable at the `legacy-dx9` tag for anyone who wants to diff the port against the original.

## Build

### Linux

```sh
sudo apt install libsdl2-dev libsdl2-image-dev libsdl2-ttf-dev libsdl2-gfx-dev libcatch2-dev
cmake -S . -B build
cmake --build build
ctest --test-dir build           # 14 unit tests for math2D / physics2D
./build/game                     # plays from any cwd; assets symlinked
```

`libcatch2-dev` is optional — if absent, `CMake` falls back to `FetchContent` against `Catch2 v3.5.4`. Internet required on the first configure when going that path.

### Windows (MSVC + vcpkg)

```cmd
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake
cmake --build build --config Release
ctest --test-dir build --build-config Release
build\Release\game.exe
```

`vcpkg.json` (manifest mode) drives dependency install on first configure: `sdl2`, `sdl2-image`, `sdl2-ttf`, `sdl2-gfx`, `catch2`. Visual Studio 2022 17.7+ recommended for full C++20 + `/std:c++20` support.

### CI

`.github/workflows/ci.yml` runs a three-job matrix on every push: **Linux GCC**, **Linux Clang**, **Windows MSVC**.

## Runtime layout

The executable expects the following relative to its working directory (the build target symlinks `data/` and `settings.ini` next to the binary so `./build/game` Just Works):

- `settings.ini` — INI parsed by `platform/read_ini.cpp` (replaces `GetPrivateProfileString`). Sections / keys: `[Game settings] Map=`, `[Display settings] Screen_width/Screen_height/Refresh_rate/Antialiasing_level/Run_fullscreen`.
- `data/maps/<Map>` — map file named by `settings.ini`. Loaded by `LoadMap()` in `game/world.cpp`.
- `data/meshes/ragdoll/*.dat` — 15 ragdoll body-part meshes (see `graphics/model.h`).
- `data/models/soldat.m2d` (+ numbered `_texture_N` files) — player model + embedded TGA textures.
- `data/textures/*.jpg|tga` — materials, indexed by `MATERIAL_TYPE` (`BRICKS`, `METAL`, `PLASTIC`, `WOOD`).
- `data/config/mat.cfg` — per-material tile sizes (`w h` per line, in `MATERIAL_TYPE` order).
- `data/sprites/`, `data/models/` — UI sprites, weapon sprites, particle textures.
- `data/fonts/DejaVuSans.ttf`, `data/fonts/DejaVuSansMono-Bold.ttf` — bundled fallbacks for SDL_ttf (the legacy build asked for `Arial` and `Lucida Console` via D3DX font init).

## Architecture

Subsystems are split across **`game/*.cpp`** (Phase 3h) on top of **`physics/`**, **`graphics/`**, **`platform/`**, **`compat/`**.

- **`compat/`** — Win32 / D3DX shims (`HRESULT`, `DWORD`, `BYTE`, `S_OK` / `E_FAIL` / `FAILED` / `SUCCEEDED`, `GetTickCount`, `D3DX_PI`, `_HUGE`) for non-MSVC builds; pulls in `<windows.h>` on MSVC instead. Plus `affine2d.h` — minimal `{ angle, scale, tx, ty, flip_x }` 2D transform that replaced the legacy `D3DXMATRIX` chain (sprite2 + model use this).
- **`physics/`** (`math2D`, `physics2D`, `polygon.h`) — 2D vector math + SAT-based rigid-body simulator. `VECTOR2D` arithmetic and `MATRIX` ops are `constexpr noexcept` since Phase 3i. `RIGIDBODY::lpVertices` is a `std::vector<VECTOR2D>` (audit pre-Phase-3f noted this was a leak). `JOINT` links two `RIGIDBODY`s by local points — the 15-part ragdoll.
- **`graphics/`** (`sprite2`, `model`, `particles`) — `SPRITE` wraps `SDL_Texture*` + `SDL_Renderer*` and uses `SDL_RenderCopyEx` plus an `Affine2D` to draw, including the matrix-chain overload that the bone hierarchy in `MODEL::Draw` uses. `PARTICLE` is the basic particle (intrusive linked list, still uses `malloc` — slated for further cleanup).
- **`platform/`** — SDL entry point (`main_sdl.cpp`), INI loader (`read_ini.{h,cpp}`), and SDL_ttf wrapper with two named fonts (`font_cache.{h,cpp}`).
- **`game/`** — the gameplay layer (split from the old `gamecode.cpp` in Phase 3h):
    - `lifecycle.cpp` — `InitGfx`, `LoadGameData`, `ShowSplash`, `Cleanup`, `UpdateScene`, `CalcPhysics`.
    - `world.cpp` — `LoadMap` + the world-data globals (`aWalls`, `aBodies`, `aRespawns`, `aPacks`, `aPackPlaces`, `aWayPoints`, `apPathParent`/`apPathDistance`, materials).
    - `players.cpp` — `PLAYER::Init`, `PLAYER::Update`, `AddPlayer`, `RespawnPlayer`, animation key-frames, ragdoll mesh paths.
    - `render.cpp` — `UpdateFrame` and the file-static `render_textured_fan` / `render_particle` helpers; owns the HUD sprite globals and the FPS counter.
    - `ai.cpp` — `RunToWayPoint`, `GetAIActions`, `ScoreCmp`; owns `m_AIStates`.
    - `effects.cpp` — particle spawners (`AddFireParticles`, `AddSmokeParticles`, `AddCustomParticles`) and the three list heads.
- **`gamecode.h`** — common-types umbrella header (the gameplay enums, `MATERIAL` / `PACK` / `PLAYER` / `NODE` / `TIMER`, physics tuning macros, lifecycle prototypes, `extern` declarations of the cross-cutting globals defined in the subsystem `.cpp` files).
- **`tests/`** — Catch2 v3 unit tests for `math2D` and `physics2D`.

### Cross-cutting things to know

- **Tuning** lives in `gamecode.h` (physics: `RUN_ACC`, `JUMP_ACC`, `FRICTION`, `BOUNCE`, `SHOOT_FORCE`, `EXPLODE_FORCE`, gravity in `physics2D.h::g`; AI: `NEAR_DISTANCE`, `FAR_DISTANCE`; pack sizes; particle counts). The gameplay enums are `enum class` (since Phase 3c); changing their order silently desyncs the parallel arrays indexed by `static_cast<int>(ENUM::VALUE)` — pay attention to `g_szMaterialFile` and `animations[]`.
- **AI pathing** uses Dijkstra-precomputed `apPathParent` / `apPathDistance` matrices over `aWayPoints`. `debug.txt` in the repo root is a dump of one parent table from an old debugging session.
- **Mouse aim** is relative: `platform/main_sdl.cpp` calls `SDL_SetRelativeMouseMode(SDL_TRUE)` after window creation. The legacy `SetCursorPos`-every-frame hack is gone.
- **Soldier rendering** quirk that survived the port: when the player faces left (`iOrientation = 1`), `SPRITE::Draw` negates the rotation angle to compensate for SDL_RenderCopyEx applying flip *before* rotation (the original D3DX matrix chain applied flip *after*). `Affine2D::compose` matches the same convention — see the long comment there. Both directions had to land for `iOrientation = 1` to display correctly.
- **`#define DEBUG`** at the top of `game/lifecycle.cpp` (commented out) enables a `debug.txt` dump of the AI path-parent table and a few extra debug overlays — flip locally rather than wiring new ones in.

## Conventions

- C++20 with `<filesystem>`, `<vector>`, `<fstream>`, `<string_view>`. Use `std::` prefix; the header umbrella does not pull `using namespace std;` (Phase 3a).
- Header-only inline `constexpr noexcept` where the maths permit it (Phase 3i). Annotate pure helpers with `[[nodiscard]]`.
- Forward slashes in include paths; LF line endings (see `.gitattributes`).
- New files: clang-format-friendly mixed tab/space matches existing files; keep it consistent within a file.
