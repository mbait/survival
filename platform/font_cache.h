#ifndef PLATFORM_FONT_CACHE_H
#define PLATFORM_FONT_CACHE_H

#include <SDL.h>

#include "compat/win32_compat.h"

// Tiny TTF wrapper used by UpdateFrame for FPS / UI / scoreboard text.
// Two named fonts are loaded at init: SYS (DejaVuSans 12) for diagnostic
// strings, UI (DejaVuSansMono-Bold 12) for the on-screen UI numbers.
// Both substitute for the legacy Arial 12 / Lucida Console 8 Bold that
// the original D3DXCreateFontIndirect call asked for.
//
// draw_text creates a single-line texture per call, blits it, destroys
// it. Good enough for the handful of strings per frame the game draws;
// can be cached if Phase 2 testing shows it costing real frame time.

namespace platform {

enum FontId { FONT_SYS = 0, FONT_UI = 1 };

HRESULT fonts_init();
void    fonts_shutdown();

void draw_text(SDL_Renderer* renderer, FontId font,
               const char* text,
               int x, int y,
               Uint8 r, Uint8 g, Uint8 b, Uint8 a = 255);

}  // namespace platform

#endif  // PLATFORM_FONT_CACHE_H
