# Phase 1a — Win32 / DirectX 9 / DirectInput 8 audit

Inventory of every platform-specific symbol used by the game (excluding the standalone tool `.exe`s). Each row maps to a replacement strategy used in Phase 1b–1m. This document is temporary — once Phase 1 ships clean, it can be retired.

## 1. Windowing & message loop (`main.cpp`)

| Symbol | Replacement |
|---|---|
| `WinMain`, `HINSTANCE`, `LPSTR`, `nCmdShow` | `int main(int, char**)` (1e) |
| `WNDCLASSEX`, `RegisterClassEx`, `CreateWindowEx`, `WS_POPUP`, `WS_CAPTION`, `CS_HREDRAW`, `CS_VREDRAW` | `SDL_CreateWindow(..., SDL_WINDOW_RESIZABLE)` (1e) |
| `HWND`, `hwndWindow` | `SDL_Window*` (1e) |
| `PeekMessage` / `GetMessage` / `TranslateMessage` / `DispatchMessage` / `WindowProc` / `WM_ACTIVATE` / `WM_KEYDOWN` / `WM_DESTROY` / `PostQuitMessage` | `SDL_PollEvent` switch on `SDL_QUIT`, `SDL_WINDOWEVENT_FOCUS_GAINED/LOST`, `SDL_KEYDOWN` (1e/1g) |
| `GetSystemMetrics(SM_CXSCREEN/SM_CYSCREEN)` | `SDL_GetCurrentDisplayMode` (1e) |
| `SetCursorPos(hsx, hsy)` every frame (relative-mouse hack) | `SDL_SetRelativeMouseMode(SDL_TRUE)` + read `mouse.xrel/yrel` from `SDL_MOUSEMOTION` (1g) |
| `ShowCursor(SW_HIDE)`, `SetForegroundWindow`, `SetActiveWindow`, `ShowWindow`, `UpdateWindow` | Implicit in SDL window setup (1e) |
| `MessageBox(..., DXGetErrorDescription9(hr), ...)` (`ErrorMessage`) | `SDL_ShowSimpleMessageBox` + `SDL_GetError` (1e) |
| `GetTickCount()` (in main loop and `TIMER`), `timeGetTime()` (RNG seed) | `SDL_GetTicks64()` (1h) |
| `GetModuleFileName` + manual basename parsing | `SDL_GetBasePath()` or just relative paths (1e/1f) |
| `srand(timeGetTime())` | `srand(SDL_GetTicks())` (1h) |
| `byte`, `LRESULT`, `WPARAM`, `LPARAM`, `UINT`, `HDC`, `HBRUSH`, `PAINTSTRUCT` | Delete with WindowProc; shim `byte` → `uint8_t` (1c) |

## 2. INI loader (`main.cpp`)

| Symbol | Replacement |
|---|---|
| `GetPrivateProfileString`, `GetPrivateProfileInt` | Small in-repo `read_ini(path, section, key, default)` (1f) — file is 8 lines, two sections; no need for a real parser |

## 3. Input — DirectInput 8 (`gamecode.cpp` `InitDI`, `RestoreDI`, `UpdateScene`, `Cleanup`)

| Symbol | Replacement |
|---|---|
| `LPDIRECTINPUT8`, `LPDIRECTINPUTDEVICE8`, `g_pDI/g_pKeyboard/g_pMouse` | Delete entirely (1g) |
| `DirectInput8Create`, `IID_IDirectInput8`, `DIRECTINPUT_VERSION` | Delete (1g) |
| `GUID_SysKeyboard`, `GUID_SysMouse`, `c_dfDIKeyboard`, `c_dfDIMouse`, `SetDataFormat`, `SetCooperativeLevel`, `DISCL_FOREGROUND`, `DISCL_NONEXCLUSIVE`, `Acquire`/`Unacquire` | Delete (1g) |
| `GetDeviceState` into `unsigned char keystate[256]` | `SDL_KEYDOWN`/`SDL_KEYUP` → maintain `keys[SDL_NUM_SCANCODES]` ourselves (1g). Polled via `SDL_GetKeyboardState` is cleaner — switch to that. |
| `DIK_A`, `DIK_D`, `DIK_W`, `DIK_Q`, `DIK_INSERT`, `DIK_DELETE`, `DIK_TAB` | `SDL_SCANCODE_A`, `_D`, `_W`, `_Q`, `_INSERT`, `_DELETE`, `_TAB` (1g) |
| `DIMOUSESTATE { lX, lY, rgbButtons[] }` | `SDL_GetRelativeMouseState(&dx, &dy)` + `SDL_BUTTON(SDL_BUTTON_LEFT/RIGHT)` (1g) |
| `DIERR_INPUTLOST` handling in `main.cpp` | Delete — SDL doesn't have lost-input recovery, focus loss handled via window events (1g) |

## 4. Direct3D 9 device & resources (`gamecode.cpp` `InitD3D`, `RestoreD3D`, `Cleanup`, `UpdateFrame`)

| Symbol | Replacement |
|---|---|
| `LPDIRECT3D9`, `Direct3DCreate9(D3D_SDK_VERSION)` | `SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC)` (1i) |
| `LPDIRECT3DDEVICE9`, `g_pDevice` | `SDL_Renderer*` (1i) — every API takes this instead of the device |
| `D3DPRESENT_PARAMETERS`, `D3DSWAPEFFECT_DISCARD`, `D3DPRESENT_INTERVAL_IMMEDIATE`, `D3DFMT_*`, `D3DMULTISAMPLE_TYPE`, `D3DADAPTER_DEFAULT`, `D3DDEVTYPE_HAL`, `D3DDISPLAYMODE`, `GetAdapterDisplayMode`, `CheckDeviceType`, `CreateDevice` | SDL hides all of this; `SDL_WINDOW_FULLSCREEN_DESKTOP` flag covers windowed/fullscreen toggle (1i) |
| `D3DCREATE_SOFTWARE_VERTEXPROCESSING` | n/a (1i) |
| `LPDIRECT3DTEXTURE9`, `D3DXCreateTextureFromFile`, `D3DXCreateTextureFromFileInMemory`, `D3DSURFACE_DESC`, `GetLevelDesc` | `SDL_Texture*` + `IMG_LoadTexture` / `IMG_LoadTexture_RW(SDL_RWFromMem(...))` + `SDL_QueryTexture` for size (1i) |
| `LPD3DXSPRITE`, `D3DXCreateSprite`, `pSprite->Begin/End/Draw/DrawTransform` | `SDL_RenderCopyEx(renderer, tex, src, dst, angle, center, flip)` (1i) |
| `D3DXMATRIX`, `D3DXMatrixScaling/RotationY/RotationZ/Translation/Multiply` | Either keep our `MATRIX` (math2D) and drop the matrix chain entirely (SDL doesn't take matrices for 2D), or build a tiny 3×3 `Mat3` for the model hierarchy (1j) |
| `D3DXVECTOR2`, `D3DCOLOR_XRGB`, `D3DCOLOR_ARGB` | `SDL_FPoint`, `SDL_Color { r, g, b, a }`, and a `static inline Uint32 argb(u8 a, u8 r, u8 g, u8 b)` helper (1i/1k) |
| `LPD3DXLINE`, `D3DXCreateLine`, `g_pLine->Draw(verts, n, color)` | `SDL2_gfxPrimitives` `aalineRGBA` in a loop, or `SDL_RenderDrawLinesF` (1k) |
| `LPD3DXFONT`, `D3DXCreateFontIndirect`, `LOGFONT`, `RUSSIAN_CHARSET`, `NONANTIALIASED_QUALITY`/`ANTIALIASED_QUALITY`, `DrawTextA`, `DT_NOCLIP`, `RECT` | `SDL_ttf`: open one `TTF_Font*` for sysfont (Arial 12), one for UI font (Lucida Console 8 bold); `TTF_RenderUTF8_Blended` → cache `SDL_Texture*` per string, blit via `SDL_RenderCopy` (1k). Cache by `(font, text, color)`. |
| `LPDIRECT3DVERTEXBUFFER9`, `CreateVertexBuffer`, `Lock`/`Unlock`, `SetFVF`, `SetStreamSource`, `DrawPrimitive(D3DPT_TRIANGLEFAN, ...)`, `DrawPrimitive(D3DPT_POINTLIST, ...)`, `D3DFVF_TEXTUREVERTEX`, `D3DFVF_COLORVERTEX`, `TEXTUREVERTEX`, `COLORVERTEX`, `D3DUSAGE_WRITEONLY`, `D3DPOOL_DEFAULT`, `D3DPOOL_MANAGED` | For terrain/body fans: build `SDL_Vertex` array + `SDL_RenderGeometry` (SDL 2.0.18+, available everywhere we'd target). For point-sprite particle stream: switch to drawing each particle as a small textured quad via `SDL_RenderCopyEx`, or use `SDL_RenderGeometry` with one triangle per particle (1i/1k). |
| `SetRenderState(D3DRS_LIGHTING/ZENABLE/ZWRITEENABLE/CULLMODE/...)` | n/a — SDL 2D has none of these (1i) |
| `SetRenderState(D3DRS_ALPHABLENDENABLE/SRCBLEND/DESTBLEND)`, `D3DBLEND_ONE/SRCCOLOR/DESTCOLOR` | `SDL_SetTextureBlendMode` + `SDL_SetRenderDrawBlendMode`; the additive fire pass needs `SDL_BLENDMODE_ADD`, the additive UI overlay needs a custom `SDL_ComposeCustomBlendMode` for `SRCCOLOR * DESTCOLOR` (1k) |
| `SetRenderState(D3DRS_POINTSPRITEENABLE/POINTSCALEENABLE/POINTSIZE)`, `FtoDW` | Delete — particles become quads (1i) |
| `BeginScene`/`EndScene`/`Present`/`Clear(D3DCLEAR_TARGET, D3DCOLOR_XRGB, 1.0f, 0L)` | `SDL_SetRenderDrawColor` + `SDL_RenderClear` / `SDL_RenderPresent` (1i) |
| `TestCooperativeLevel`, `D3DERR_DEVICELOST` in `RestoreD3D` | Delete — SDL handles lost devices internally (1i) |
| `S_OK`, `E_FAIL`, `HRESULT`, `FAILED(hr)`, `SUCCEEDED(hr)` | Shim in `compat/win32_compat.h`: `using HRESULT = int; constexpr int S_OK = 0, E_FAIL = -1; #define FAILED(hr) ((hr)<0); #define SUCCEEDED(hr) ((hr)>=0)` (1c). Internally `SPRITE::Init` etc. just return `0`/`-1` after the port. |
| `DXGetErrorDescription9(hr)` (dxerr9.lib) | Delete; pass `SDL_GetError()` to the SDL message box (1k) |

## 5. C runtime / MSVC-isms

| Symbol / construct | Action |
|---|---|
| `_HUGE` (MSVC <float.h>) used in physics2D.cpp | Replace with `std::numeric_limits<float>::infinity()` from `<limits>` (1l) |
| `byte` keyword (Win SDK typedef) — used in particles, model | `typedef uint8_t byte;` in `compat/win32_compat.h` (1c) |
| Backslash include paths (`#include "..\physics\math2D.h"`, `#include "graphics\model.h"`) | Forward slashes (1d) |
| Backslash string paths in `LoadMap`, `LoadGameData`, `ShowSplash` (`"data\\sprites\\splash.jpg"`, `"data\\maps\\..."`, `"data\\meshes\\ragdoll\\..."`, `"data\\textures\\..."`) | Forward slashes — works on both Linux and Windows (1d) |
| `gamecode.h:174-176` `static const c_NumHealth = 100;` etc. — implicit-int declaration | Add `int` (1l). Three lines. |
| `gamecode.h:14` `using namespace std;` in a widely-included header | Leave for Phase 1 (would cascade fixes); flag for Phase 3 (15) |
| `main.cpp:28` `int round(double v)` — collides with `std::round` once `<cmath>` is in scope (and `using namespace std`) | Rename to `iround` (1l) |
| `physics2D.cpp:418` `for(int j=body.iNumVertices-1, int i=0; ...)` — re-declares `int` mid-init; MSVC 7.1 silently accepts as comma expression, GCC/Clang reject | Split into two declarations or rewrite (1l) |
| `physics2D.cpp:754` `JOINT::JOINT()` calls `JOINT();` as a statement (creates and discards temporary, not a delegation) | Cosmetic/no-op on Linux too — leave (defer to Phase 3) |
| `physics2D.cpp:13-35` `RIGIDBODY()` ctor declares locals named `Pos`, `Velocity`, `Force`, `mOrientation` that shadow members — initialisation is a no-op. Latent bug, but reproduces original behaviour. | Leave (Phase 3 fix) |
| MSVC-only `#pragma comment(lib, "...")` in `gamecode.h` and `main.cpp` | Replace with CMake `target_link_libraries` (1b). The pragmas can stay (compilers ignore unknown pragmas with warnings) or be wrapped in `#ifdef _MSC_VER`. (1c) |
| `D3DX_PI` (`gamecode.h:23`) | `compat`: `#define D3DX_PI 3.14159265358979323846f` or replace usages with `M_PI` (1c). Used in `sprite2.cpp` matrix building (going away) and `model.cpp::Draw` (going away). |
| `fopen("rb"/"r")` + `fscanf`/`fread`/`fclose` everywhere | Leave for Phase 1; modernise to `std::ifstream` in Phase 3 (15) |
| `using namespace std;` in `gamecode.h` + a stray `LOGFONT`/`RECT` from `<windows.h>` | The `<windows.h>` symbols disappear with the DX port; only `LOGFONT`/`RECT` use is gone after 1k |
| `winmm.lib` (`timeGetTime`) | Delete with 1h |

## 6. Files to drop from the Linux build

These compile under Win32 only; CMake just doesn't list them in the Linux target:

- `main.cpp` → renamed/preserved as `platform/main_win32.cpp` (compiled only when `WIN32` is defined; Phase 1 builds use `platform/main_sdl.cpp` exclusively on both OSes for simplicity).
- `physics/physics2D.cpp.old` — already excluded by `.gitignore`'s `*.old` rule.
- Standalone tool exes (`animator.exe`, `map_editor.exe`, `particlesystem.exe`, `UI.exe`) — Phase 4.

## 7. Notes on subtle behaviours we must preserve

- The original calls `SetCursorPos(centerX, centerY)` **every frame** before reading mouse delta. We must emulate the same "infinite cursor" feel via `SDL_SetRelativeMouseMode(SDL_TRUE)`; otherwise aim will feel chunky.
- `g_iAALevel` from settings.ini becomes an `SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS,...)` call — but only if we switch to the GL renderer. Under pure SDL_Renderer there is no MSAA control; we silently ignore `Antialiasing_level` and document it.
- Fonts: original uses **Arial 12** (sysfont) and **Lucida Console 8 bold** (UI). Neither ships with most Linux distros under those exact names. We'll bundle two open fonts under `data/fonts/` (DejaVu Sans + DejaVu Sans Mono Bold are good substitutes) and hardcode their paths.
- Splash screen runs **synchronously between `InitD3D` and `LoadMap`** — must still appear before the long map-loading hitch in the SDL build.
- The `RUSSIAN_CHARSET` flag means the UI was authored expecting CP1251 strings; the source files contain no Russian literals, so we ignore this and use UTF-8 throughout SDL_ttf.

## 8. Per-step file ownership (cross-reference for 1b–1l)

| Step | Files touched |
|---|---|
| 1b | new `CMakeLists.txt`, new `compat/` |
| 1c | new `compat/win32_compat.h` |
| 1d | every `*.h`/`*.cpp` with `#include "...\..."`; `gamecode.cpp` strings |
| 1e | new `platform/main_sdl.cpp`; rename `main.cpp` → `platform/main_win32.cpp` |
| 1f | new `platform/read_ini.{h,cpp}` |
| 1g | `gamecode.cpp` (`InitDI`, `RestoreDI`, `UpdateScene`, `Cleanup`); drop DI parts of `main.h`/`main.cpp` |
| 1h | `gamecode.h::TIMER`, `platform/main_sdl.cpp` |
| 1i | `graphics/sprite2.{h,cpp}` |
| 1j | `graphics/model.{h,cpp}`, `graphics/particles.{h,cpp}` |
| 1k | `gamecode.cpp` `InitD3D`/`UpdateFrame`/`ShowSplash`/`RestoreD3D`/`Cleanup` (rendering); new `platform/font_cache.{h,cpp}` |
| 1l | `gamecode.h`, `main.cpp`(→retired), `physics/physics2D.cpp` |
