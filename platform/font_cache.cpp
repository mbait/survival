#include "font_cache.h"

#include <SDL_ttf.h>

#include <cstdio>

namespace platform
{
namespace
{

constexpr const char* kSysFontPath = "data/fonts/DejaVuSans.ttf";
constexpr const char* kUIFontPath = "data/fonts/DejaVuSansMono-Bold.ttf";
constexpr int kSysFontPt = 12;
constexpr int kUIFontPt = 12;

TTF_Font* g_fonts[2] = {nullptr, nullptr};
bool g_ttf_inited = false;

} // namespace

HRESULT fonts_init()
{
	if (!g_ttf_inited)
	{
		if (TTF_Init() != 0)
		{
			std::fprintf(stderr, "TTF_Init failed: %s\n", TTF_GetError());
			return E_FAIL;
		}
		g_ttf_inited = true;
	}

	g_fonts[FONT_SYS] = TTF_OpenFont(kSysFontPath, kSysFontPt);
	if (!g_fonts[FONT_SYS])
	{
		std::fprintf(stderr, "TTF_OpenFont(%s): %s\n", kSysFontPath, TTF_GetError());
		return E_FAIL;
	}
	g_fonts[FONT_UI] = TTF_OpenFont(kUIFontPath, kUIFontPt);
	if (!g_fonts[FONT_UI])
	{
		std::fprintf(stderr, "TTF_OpenFont(%s): %s\n", kUIFontPath, TTF_GetError());
		return E_FAIL;
	}
	return S_OK;
}

void fonts_shutdown()
{
	for (auto& f : g_fonts)
	{
		if (f)
		{
			TTF_CloseFont(f);
			f = nullptr;
		}
	}
	if (g_ttf_inited)
	{
		TTF_Quit();
		g_ttf_inited = false;
	}
}

void draw_text(SDL_Renderer* renderer, FontId font, const char* text, int x, int y, Uint8 r,
               Uint8 g, Uint8 b, Uint8 a)
{
	if (!renderer || !text || !*text || !g_fonts[font])
	{
		return;
	}
	const SDL_Color color {r, g, b, a};
	SDL_Surface* surf = TTF_RenderUTF8_Blended(g_fonts[font], text, color);
	if (!surf)
	{
		return;
	}
	SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, surf);
	if (tex)
	{
		SDL_Rect dst {x, y, surf->w, surf->h};
		SDL_RenderCopy(renderer, tex, nullptr, &dst);
		SDL_DestroyTexture(tex);
	}
	SDL_FreeSurface(surf);
}

} // namespace platform
