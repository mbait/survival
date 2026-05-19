// SDL2 platform entry point for the survival port.
//
// Phase 1e: open a window, pump the event loop.
// Phase 1f: read settings.ini and apply Screen_width / Screen_height /
//           Run_fullscreen to the SDL window. The map name is read into
//           g_szMapName for later phases that actually load it.
//
// The game layer (gamecode.cpp etc.) is still gated out of the build —
// this file deliberately does NOT call InitD3D / LoadMap / UpdateScene
// yet. Those land in Phase 1g/1h/1k.

#include <SDL.h>

#include <cstdio>
#include <cstring>
#include <filesystem>

#include "gamecode.h"
#include "main.h"
#include "read_ini.h"

// Definitions for the globals declared in main.h. Both the SDL entry
// point and (eventually) gamecode.cpp will reference these.
int         g_iScreenWidth  = 0;
int         g_iScreenHeight = 0;
int         g_iRefreshRate  = 0;
int         g_iAALevel      = 0;
bool        g_bFullScreen   = false;
std::string g_szMapName;

namespace {

void die(const char* what)
{
    std::fprintf(stderr, "%s: %s\n", what, SDL_GetError());
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, what, SDL_GetError(), nullptr);
}

// Resolve settings.ini relative to argv[0] so the binary can be run
// from anywhere as long as the working dir or the exe dir holds it.
std::filesystem::path resolve_ini(const char* argv0)
{
    namespace fs = std::filesystem;
    fs::path cwd_ini = fs::current_path() / "settings.ini";
    if (fs::exists(cwd_ini)) {
        return cwd_ini;
    }
    if (argv0) {
        fs::path exe_ini = fs::weakly_canonical(fs::path(argv0)).parent_path() / "settings.ini";
        if (fs::exists(exe_ini)) {
            return exe_ini;
        }
    }
    return cwd_ini;  // best effort; the loader will fall back to defaults
}

void load_settings(const std::filesystem::path& ini)
{
    using platform::ini_get_int;
    using platform::ini_get_string;

    g_szMapName = ini_get_string(ini, "Game settings", "Map", "default.map");

    g_iScreenWidth  = ini_get_int(ini, "Display settings", "Screen_width",       800);
    g_iScreenHeight = ini_get_int(ini, "Display settings", "Screen_height",      600);
    g_iRefreshRate  = ini_get_int(ini, "Display settings", "Refresh_rate",        75);
    g_iAALevel      = ini_get_int(ini, "Display settings", "Antialiasing_level",   0);
    g_bFullScreen   = ini_get_int(ini, "Display settings", "Run_fullscreen",       1) != 0;
}

}  // namespace

int main(int /*argc*/, char* argv[])
{
    const std::filesystem::path ini = resolve_ini(argv ? argv[0] : nullptr);
    load_settings(ini);

    std::fprintf(stderr,
                 "settings: %dx%d %s, refresh=%d, AA=%d, map=%s (from %s)\n",
                 g_iScreenWidth, g_iScreenHeight,
                 g_bFullScreen ? "fullscreen" : "windowed",
                 g_iRefreshRate, g_iAALevel, g_szMapName.c_str(), ini.c_str());

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
        die("SDL_Init");
        return 1;
    }

    const Uint32 window_flags =
        SDL_WINDOW_SHOWN |
        (g_bFullScreen ? Uint32{SDL_WINDOW_FULLSCREEN_DESKTOP} : Uint32{0});

    SDL_Window* window = SDL_CreateWindow(
        "survival",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        g_iScreenWidth, g_iScreenHeight,
        window_flags);
    if (!window) {
        die("SDL_CreateWindow");
        SDL_Quit();
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(
        window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        die("SDL_CreateRenderer");
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // Initialise the game's render layer and load assets. ShowSplash blits
    // the splash texture once and presents; we hold it on screen briefly so
    // the user can actually see it before LoadGameData takes over.
    if (FAILED(InitGfx(window, renderer))) {
        die("InitGfx");
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    ShowSplash();
    SDL_Delay(800);

    {
        const std::filesystem::path map_path =
            std::filesystem::path("data/maps") / g_szMapName;
        const std::string map_path_str = map_path.string();
        if (FAILED(LoadMap(map_path_str.c_str()))) {
            std::fprintf(stderr, "LoadMap failed: %s\n", map_path_str.c_str());
            Cleanup();
            SDL_DestroyRenderer(renderer);
            SDL_DestroyWindow(window);
            SDL_Quit();
            return 1;
        }
    }
    if (FAILED(LoadGameData())) {
        std::fprintf(stderr, "LoadGameData failed\n");
        Cleanup();
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // Capture the mouse for relative-motion aim, matching the legacy
    // SetCursorPos-every-frame hack.
    SDL_SetRelativeMouseMode(SDL_TRUE);

    Uint64 last_ticks = SDL_GetTicks64();
    bool running = true;
    while (running) {
        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            switch (ev.type) {
                case SDL_QUIT:
                    running = false;
                    break;
                case SDL_KEYDOWN:
                    if (ev.key.keysym.scancode == SDL_SCANCODE_ESCAPE) {
                        running = false;
                    }
                    break;
                default:
                    break;
            }
        }

        const Uint64 now = SDL_GetTicks64();
        const DWORD  dt  = static_cast<DWORD>(now - last_ticks);
        last_ticks = now;

        if (FAILED(UpdateScene(dt))) {
            running = false;
        }
        if (FAILED(UpdateFrame())) {
            running = false;
        }
    }

    Cleanup();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
