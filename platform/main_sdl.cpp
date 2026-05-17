// SDL2 platform entry point for the survival port.
//
// Phase 1e: this file is intentionally self-contained — it does NOT yet
// call into the game layer (LoadMap / InitD3D / UpdateScene / etc.).
// Those calls are wired up in Phase 1g (input), 1h (timing), 1k
// (rendering) as each subsystem gets ported. Today this opens a window,
// pumps the event loop, and exits cleanly on close / Esc.

#include <SDL.h>

#include <cstdio>

namespace {
constexpr int kDefaultWidth  = 640;
constexpr int kDefaultHeight = 480;

void die(const char* what)
{
    std::fprintf(stderr, "%s: %s\n", what, SDL_GetError());
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, what, SDL_GetError(), nullptr);
}
}  // namespace

int main(int /*argc*/, char* /*argv*/[])
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
        die("SDL_Init");
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow(
        "survival",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        kDefaultWidth, kDefaultHeight,
        SDL_WINDOW_SHOWN);
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

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        SDL_RenderPresent(renderer);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
