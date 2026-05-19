

#include <SDL.h>
#include <SDL_image.h>

#include <fstream>
#include <vector>

#include "game/ai.h"
#include "game/effects.h"
#include "game/players.h"
#include "platform/font_cache.h"

#include "gamecode.h"

// #define DEBUG
// #define GODMODE

// SDL render context — borrowed from the platform layer; we don't own these.
SDL_Window* g_window = nullptr;
SDL_Renderer* g_renderer = nullptr;

// Owned textures: loaded in LoadGameData, freed in Cleanup. Fonts and
// per-frame geometry batches come back in Phase 1k.2.
SDL_Texture* g_pFireTexture = nullptr;
SDL_Texture* g_pSmokeTexture = nullptr;

// Input migrated to SDL — keyboard state is read directly from
// SDL_GetKeyboardState in UpdateScene, mouse state from
// SDL_GetRelativeMouseState. No persistent device handles needed.

// game objects

// cursor coordinates
SDL_Point g_cursor;
VECTOR2D g_vCenter;

// world data

// device settings

bool g_bAddKeyOnce = true;
bool g_bRemoveKeyOnce = true;
bool g_bShowStat = false;

///////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////
void CalcPhysics(DWORD dwTime)
{
	VECTOR2D MTD;
	float t;

	// update world objects
	for (int i = 0; i < g_iNumBodies - 1; i++)
	{
		for (int j = i + 1; j < g_iNumBodies; j++)
			if (aBodies[i].Collide(aBodies[j], MTD, t))
				aBodies[i].ResolveCollision(aBodies[j], MTD, t);
	}

	for (int i = 0; i < g_iNumBodies; i++)
	{
		for (int j = 0; j < g_iNumWalls; j++)
			if (aBodies[i].Collide(aWalls[j], MTD, t))
				aBodies[i].ResolveCollision(aWalls[j], MTD, t);
	}

	for (int i = 0; i < g_iNumBodies; i++)
		aBodies[i].ApplyForce(VECTOR2D(0.0f, g * aBodies[i].fMass));

	for (int i = 0; i < g_iNumBodies; i++)
		aBodies[i].Update(dwTime / 1000.0f);
}
///////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT UpdateScene(DWORD dwTime)
{
	//===================read keyboard state====================//
	// SDL keeps a single keyboard state vector internally; SDL_PumpEvents
	// in the main loop refreshes it. SDL_GetKeyboardState returns a
	// pointer into that buffer indexed by SDL_SCANCODE_*.
	const Uint8* keystate = SDL_GetKeyboardState(nullptr);

	bool actions[static_cast<int>(ACTION::NUMACTIONS)];
	memset(actions, 0, sizeof(actions));

	// resolve keyboard state
	if (keystate[SDL_SCANCODE_A])
	{
		m_aPlayers[0].dir = DIRECTION::LEFT;
		actions[static_cast<int>(ACTION::MOVELEFT)] = true;
	}
	if (keystate[SDL_SCANCODE_D])
	{
		m_aPlayers[0].dir = DIRECTION::RIGHT;
		actions[static_cast<int>(ACTION::MOVERIGHT)] = true;
	}
	if (keystate[SDL_SCANCODE_W])
	{
		actions[static_cast<int>(ACTION::JUMP)] = true;
	}
	if (keystate[SDL_SCANCODE_Q])
		actions[static_cast<int>(ACTION::WJUMP)] = true;

	// none-control keystate
	if (keystate[SDL_SCANCODE_INSERT])
	{
		if (g_bAddKeyOnce && g_iNumPlayers < MAX_PLAYERS)
		{
			(void)AddPlayer();
			g_bAddKeyOnce = false;
		}
	}
	else
		g_bAddKeyOnce = true;

	if (keystate[SDL_SCANCODE_DELETE])
	{
		if (g_bRemoveKeyOnce && g_iNumPlayers > 1)
		{
			g_iNumPlayers--;
			g_bRemoveKeyOnce = false;
		}
	}
	else
		g_bRemoveKeyOnce = true;

	if (keystate[SDL_SCANCODE_TAB])
		g_bShowStat = true;
	else
		g_bShowStat = false;

	//=========================read mouse state=========================//
	// Relative-mouse mode is enabled by the platform layer at start-up;
	// each call returns motion accumulated since the last call.
	int mouse_dx = 0;
	int mouse_dy = 0;
	const Uint32 buttons = SDL_GetRelativeMouseState(&mouse_dx, &mouse_dy);

	if (buttons & SDL_BUTTON(SDL_BUTTON_LEFT))
		actions[static_cast<int>(ACTION::SHOOT)] = true;
	if (buttons & SDL_BUTTON(SDL_BUTTON_RIGHT))
		actions[static_cast<int>(ACTION::ALTSHOOT)] = true;

	// update cursor pos
	g_cursor.x = (g_cursor.x + mouse_dx);
	g_cursor.y = (g_cursor.y + mouse_dy);
	// clip x
	if (g_cursor.x > MOUSE_MAX_X)
		g_cursor.x = MOUSE_MAX_X;
	else if (g_cursor.x < MOUSE_MIN_X)
		g_cursor.x = MOUSE_MIN_X;
	// clip y
	if (g_cursor.y > MOUSE_MAX_Y)
		g_cursor.y = MOUSE_MAX_Y;
	else if (g_cursor.y < MOUSE_MIN_Y)
		g_cursor.y = MOUSE_MIN_Y;

	m_aPlayers[0].cursor.x = g_cursor.x;
	m_aPlayers[0].cursor.y = g_cursor.y;

	// update model
	m_aPlayers[0].Update(dwTime, actions);

	for (int i = 1; i < g_iNumPlayers; i++)
	{
		GetAIActions(i, actions);
		m_aPlayers[i].Update(dwTime, actions);
	}
	// update world
	int delta = dwTime;
	while (delta > 0)
	{
		CalcPhysics(5);
		delta -= 5;
	}

	//=========================update particle system====================//
	VECTOR2D vGravity(0, g);
	PARTICLE* ptr = g_pFire->next; // fire particles
	while (ptr)
	{
		if (!ptr->Update(dwTime, VECTOR2D()))
		{
			g_FireParticleCnt--;

			PARTICLE* next_ptr = ptr->next;
			ptr->Delete();
			ptr = next_ptr;
		}
		else
			ptr = ptr->next;
	}

	ptr = g_pSmoke->next; // smoke particles
	while (ptr)
	{
		if (!ptr->Update(dwTime, VECTOR2D()))
		{
			g_SmokeParticleCnt--;

			PARTICLE* next_ptr = ptr->next;
			ptr->Delete();
			ptr = next_ptr;
		}
		else
			ptr = ptr->next;
	}

	ptr = g_pCustom->next; // concrete particles
	while (ptr)
	{
		if (!(ptr->Update(dwTime, vGravity)))
		{
			g_CustomParticleCnt--;

			PARTICLE* next_ptr = ptr->next;
			ptr->Delete();
			ptr = next_ptr;
		}
		else
			ptr = ptr->next;
	}

	for (int i = 0; i < g_iNumPackPlaces; i++)
	{
		if (!aPacks[i].bActive)
		{
			if (aPacks[i].tmReset.Delta() > 60000)
			{
				aPacks[i].type = (PACK_TYPE)(int)(RANDOM * static_cast<int>(PACK_TYPE::NUM_PACKS));
				aPacks[i].bActive = true;
			}
			else
			{
				aPacks[i].tmReset.Update();
			}
		}
	}

	return S_OK;
}
///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT LoadGameData()
{
	g_vCenter = VECTOR2D(g_iScreenWidth >> 1, g_iScreenHeight >> 1);

	HRESULT hr;
	//==========load main player model===========//
	// m_aPlayers = new PLAYER[MAX_PLAYERS];

	hr = AddPlayer();
	if (FAILED(hr))
	{
		std::fprintf(stderr, "LoadGameData: AddPlayer failed\n");
		return hr;
	}
	//=============end soldat model========//
	//==========load enviroment objs=======//
	float fPack_scale = 0.8f;

	hr = pack_ammo.Init(g_renderer, "data/sprites/ammo_pack.tga");
	if (FAILED(hr))
	{
		std::fprintf(stderr, "LoadGameData: pack_ammo (%s) failed: %s\n",
		             "data/sprites/ammo_pack.tga", SDL_GetError());
		return hr;
	}
	pack_ammo.SetScale(fPack_scale);

	hr = pack_grenade.Init(g_renderer, "data/sprites/grenade_pack.tga");
	if (FAILED(hr))
	{
		std::fprintf(stderr, "LoadGameData: pack_grenade (%s) failed: %s\n",
		             "data/sprites/grenade_pack.tga", SDL_GetError());
		return hr;
	}
	pack_grenade.SetScale(fPack_scale);

	hr = pack_health.Init(g_renderer, "data/sprites/health_pack.tga");
	if (FAILED(hr))
	{
		std::fprintf(stderr, "LoadGameData: pack_health (%s) failed: %s\n",
		             "data/sprites/health_pack.tga", SDL_GetError());
		return hr;
	}
	pack_health.SetScale(fPack_scale);

	g_pFireTexture = IMG_LoadTexture(g_renderer, "data/sprites/fire.tga");
	if (!g_pFireTexture)
	{
		std::fprintf(stderr, "LoadGameData: fire texture failed: %s\n", SDL_GetError());
		return E_FAIL;
	}

	g_pFire = new PARTICLE();
	g_pSmoke = new PARTICLE();
	g_pCustom = new PARTICLE();
	//==========end enviroment objs=======//
	//===============load weapons=========//
	hr = rifle.Init(g_renderer, "data/sprites/rifle.tga");
	if (FAILED(hr))
	{
		std::fprintf(stderr, "LoadGameData: rifle (%s) failed: %s\n", "data/sprites/rifle.tga",
		             SDL_GetError());
		return hr;
	}
	rifle.SetXYPos(15, 25);
	rifle.SetRotation(PI / 4.0f);

	hr = grenade.Init(g_renderer, "data/sprites/grenade.tga");
	if (FAILED(hr))
	{
		std::fprintf(stderr, "LoadGameData: grenade (%s) failed: %s\n", "data/sprites/grenade.tga",
		             SDL_GetError());
		return hr;
	}

	hr = fire.Init(g_renderer, "data/sprites/shoot_fire.tga");
	if (FAILED(hr))
	{
		std::fprintf(stderr, "LoadGameData: fire (%s) failed: %s\n", "data/sprites/shoot_fire.tga",
		             SDL_GetError());
		return hr;
	}
	//=================end weapons========//

	//============load UI elements========//
	hr = ui_health.Init(g_renderer, "data/sprites/health_UI.tga");
	if (FAILED(hr))
	{
		std::fprintf(stderr, "LoadGameData: ui_health (%s) failed: %s\n",
		             "data/sprites/health_UI.tga", SDL_GetError());
		return hr;
	}
	ui_health.SetXYPos(g_iScreenWidth >> 1,
	                   g_iScreenHeight - ui_health.iHeight + (ui_health.iHeight >> 2));

	hr = ui_rifle.Init(g_renderer, "data/sprites/rifle_UI.tga");
	if (FAILED(hr))
	{
		std::fprintf(stderr, "LoadGameData: ui_rifle (%s) failed: %s\n",
		             "data/sprites/rifle_UI.tga", SDL_GetError());
		return hr;
	}
	ui_rifle.SetXYPos(ui_rifle.iWidth >> 1,
	                  g_iScreenHeight - ui_rifle.iHeight + (ui_rifle.iHeight >> 2));

	hr = ui_grenade.Init(g_renderer, "data/sprites/grenade_UI.tga");
	if (FAILED(hr))
	{
		std::fprintf(stderr, "LoadGameData: ui_grenade (%s) failed: %s\n",
		             "data/sprites/grenade_UI.tga", SDL_GetError());
		return hr;
	}
	ui_grenade.SetXYPos(g_iScreenWidth - ui_grenade.iWidth,
	                    g_iScreenHeight - ui_grenade.iHeight + (ui_grenade.iHeight >> 2));

	hr = cur_ptr.Init(g_renderer, "data/sprites/cursor.tga");
	if (FAILED(hr))
	{
		std::fprintf(stderr, "LoadGameData: cur_ptr (%s) failed: %s\n", "data/sprites/cursor.tga",
		             SDL_GetError());
		return hr;
	}
	cur_ptr.SetScale(0.6f);
	//==============end UI elements=======//

	return S_OK;
}
////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT InitGfx(SDL_Window* window, SDL_Renderer* renderer)
{
	if (!window || !renderer)
	{
		return E_FAIL;
	}
	g_window = window;
	g_renderer = renderer;
	if (FAILED(platform::fonts_init()))
	{
		return E_FAIL;
	}
	return S_OK;
}
void Cleanup()
{
	for (int i = 0; i < MAX_MATERIALS; i++)
	{
		if (g_aMaterials[i].pTexture)
		{
			SDL_DestroyTexture(g_aMaterials[i].pTexture);
			g_aMaterials[i].pTexture = nullptr;
		}
	}
	if (g_pFireTexture)
	{
		SDL_DestroyTexture(g_pFireTexture);
		g_pFireTexture = nullptr;
	}
	if (g_pSmokeTexture)
	{
		SDL_DestroyTexture(g_pSmokeTexture);
		g_pSmokeTexture = nullptr;
	}
	platform::fonts_shutdown();
	// Renderer / window are owned by the SDL platform layer.
	g_renderer = nullptr;
	g_window = nullptr;
}
HRESULT ShowSplash()
{
	if (!g_renderer)
	{
		return E_FAIL;
	}
	SPRITE splash;
	if (FAILED(splash.Init(g_renderer, "data/sprites/splash.jpg")))
	{
		return S_OK; // matches original: missing splash is non-fatal
	}
	SDL_SetRenderDrawColor(g_renderer, 0, 0, 0, 255);
	SDL_RenderClear(g_renderer);
	splash.SetXYPos(g_iScreenWidth >> 1, g_iScreenHeight >> 1);
	splash.SetScale(static_cast<float>(g_iScreenWidth) / 1024.0f);
	splash.Draw();
	SDL_RenderPresent(g_renderer);
	return S_OK;
}
///////////////////////////////////////////////////////////////////////////////////////////////////
