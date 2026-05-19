#include <SDL.h>

#include <vector>

#include "game/effects.h" // g_pFire / g_pSmoke / g_pCustom + counters
#include "platform/font_cache.h"

#include "gamecode.h"

// ---- sprites & per-frame UI state (owned by this TU) ---------------------
SPRITE rifle;
SPRITE grenade;
SPRITE fire;
SPRITE cur_ptr;
SPRITE ui_health;
SPRITE ui_rifle;
SPRITE ui_grenade;
SPRITE pack_ammo;
SPRITE pack_grenade;
SPRITE pack_health;

TIMER tmFPS;
int nFrameCount = 0;
int nFPS = 0;

namespace
{

// Render a single textured triangle fan via SDL_RenderGeometry.
void render_textured_fan(SDL_Renderer* renderer, SDL_Texture* tex, const LPVECTOR2D pts,
                         const LPVECTOR2D uvs, int n, VECTOR2D vOffset)
{
	if (n < 3)
		return;
	std::vector<SDL_Vertex> verts(n);
	for (int i = 0; i < n; i++)
	{
		verts[i].position.x = pts[i].x - vOffset.x;
		verts[i].position.y = pts[i].y - vOffset.y;
		verts[i].color = SDL_Color {255, 255, 255, 255};
		verts[i].tex_coord.x = uvs[i].x;
		verts[i].tex_coord.y = uvs[i].y;
	}
	std::vector<int> idx;
	idx.reserve(3 * (n - 2));
	for (int i = 1; i + 1 < n; i++)
	{
		idx.push_back(0);
		idx.push_back(i);
		idx.push_back(i + 1);
	}
	SDL_RenderGeometry(renderer, tex, verts.data(), static_cast<int>(verts.size()), idx.data(),
	                   static_cast<int>(idx.size()));
}

// Draw a particle as a small textured quad.
void render_particle(SDL_Renderer* renderer, SDL_Texture* tex, const PARTICLE* p, VECTOR2D vOffset,
                     int size, SDL_BlendMode blend)
{
	if (!tex)
		return;
	SDL_SetTextureBlendMode(tex, blend);
	SDL_SetTextureColorMod(tex, p->color_current.R, p->color_current.G, p->color_current.B);
	SDL_SetTextureAlphaMod(tex, p->color_current.Alpha);
	SDL_Rect dst {static_cast<int>(p->Pos.x - vOffset.x - size / 2.0f),
	              static_cast<int>(p->Pos.y - vOffset.y - size / 2.0f), size, size};
	SDL_RenderCopy(renderer, tex, nullptr, &dst);
}

} // namespace

HRESULT UpdateFrame()
{
	if (!g_renderer)
	{
		return E_FAIL;
	}

	SDL_SetRenderDrawColor(g_renderer, 0, 0, 0, 255);
	SDL_RenderClear(g_renderer);

	// camera offset: world -> screen
	VECTOR2D vOffset;
	VECTOR2D player_pos;
	if (m_aPlayers[0].bAlive)
	{
		vOffset = m_aPlayers[0].body.Pos - g_vCenter;
		player_pos = g_vCenter + VECTOR2D(0.0f, 5.0f);
		m_aPlayers[0].model.SetXYPos(player_pos.x, player_pos.y);
	}
	else
	{
		vOffset = m_aPlayers[0].ragdoll[HEAD].Pos - g_vCenter;
	}

	// terrain + dynamic bodies as triangle fans
	for (int i = 0; i < g_iNumWalls; i++)
	{
		render_textured_fan(g_renderer, g_aMaterials[aWalls[i].lMaterialID].pTexture,
		                    aWalls[i].lpVertices.data(), g_vWallTex[i].data(),
		                    aWalls[i].iNumVertices, vOffset);
	}
	for (int i = 0; i < g_iNumBodies; i++)
	{
		render_textured_fan(g_renderer, g_aMaterials[aBodies[i].lMaterialID].pTexture,
		                    aBodies[i].lpVertices.data(), g_vBodyTex[i].data(),
		                    aBodies[i].iNumVertices, vOffset);
	}

	// packs
	for (int i = 0; i < g_iNumPackPlaces; i++)
	{
		if (!aPacks[i].bActive)
			continue;
		const VECTOR2D pos = aPackPlaces[i] - vOffset;
		SPRITE* spr = nullptr;
		switch (aPacks[i].type)
		{
		case PACK_TYPE::PACK_AMMO:
			spr = &pack_ammo;
			break;
		case PACK_TYPE::PACK_GRENADE:
			spr = &pack_grenade;
			break;
		case PACK_TYPE::PACK_HEALTH:
			spr = &pack_health;
			break;
		default:
			continue;
		}
		spr->Draw(pos.x, pos.y, 0.0f, spr->GetRotationX(), spr->GetRotationY(), 0.8f);
	}

	// main player (or ragdoll if dead) + grenade
	if (m_aPlayers[0].bAlive)
	{
		if (m_aPlayers[0].bShooting)
			m_aPlayers[0].model.Draw(&rifle, &fire);
		else
			m_aPlayers[0].model.Draw(&rifle);
	}
	else
	{
		for (int i = 0; i < NUM_PARTS; i++)
		{
			VECTOR2D Pos = m_aPlayers[0].ragdoll[i].Pos - vOffset;
			float fRot = m_aPlayers[0].ragdoll[i].fOrientation;
			m_aPlayers[0].model.GetPart(i)->Draw(Pos.x, Pos.y, -fRot, 12.0f, 12.0f, 0.4f);
		}
	}
	if (m_aPlayers[0].bActiveGrenade)
	{
		grenade.Draw(m_aPlayers[0].grenade_body.Pos.x - vOffset.x,
		             m_aPlayers[0].grenade_body.Pos.y - vOffset.y,
		             -m_aPlayers[0].grenade_body.fOrientation, grenade.GetRotationX(),
		             grenade.GetRotationY());
	}

	// AI players
	for (int i = 1; i < g_iNumPlayers; i++)
	{
		if (m_aPlayers[i].bAlive)
		{
			VECTOR2D pp = m_aPlayers[i].body.Pos + VECTOR2D(0.0f, 5.0f) - vOffset;
			m_aPlayers[i].model.SetXYPos(pp.x, pp.y);
			if (m_aPlayers[i].bShooting)
				m_aPlayers[i].model.Draw(&rifle, &fire);
			else
				m_aPlayers[i].model.Draw(&rifle);
		}
		else
		{
			for (int j = 0; j < NUM_PARTS; j++)
			{
				VECTOR2D Pos = m_aPlayers[i].ragdoll[j].Pos - vOffset;
				float fRot = m_aPlayers[i].ragdoll[j].fOrientation;
				m_aPlayers[i].model.GetPart(j)->Draw(Pos.x, Pos.y, -fRot, 12.0f, 12.0f, 0.4f);
			}
		}
		if (m_aPlayers[i].bActiveGrenade)
		{
			grenade.Draw(m_aPlayers[i].grenade_body.Pos.x - vOffset.x,
			             m_aPlayers[i].grenade_body.Pos.y - vOffset.y,
			             -m_aPlayers[i].grenade_body.fOrientation, grenade.GetRotationX(),
			             grenade.GetRotationY());
		}
	}

	// particles: fire (additive), smoke (additive — same texture; the
	// legacy g_pSmokeTexture is never loaded), concrete debris (alpha)
	constexpr int kParticleSize = 20;
	for (PARTICLE* p = g_pFire ? g_pFire->next : nullptr; p; p = p->next)
		render_particle(g_renderer, g_pFireTexture, p, vOffset, kParticleSize, SDL_BLENDMODE_ADD);
	for (PARTICLE* p = g_pSmoke ? g_pSmoke->next : nullptr; p; p = p->next)
		render_particle(g_renderer, g_pFireTexture, p, vOffset, kParticleSize, SDL_BLENDMODE_ADD);
	for (PARTICLE* p = g_pCustom ? g_pCustom->next : nullptr; p; p = p->next)
		render_particle(g_renderer, g_pFireTexture, p, vOffset, 4, SDL_BLENDMODE_BLEND);

	// HUD: FPS + alive UI
	if (tmFPS.Delta() > 100)
	{
		nFPS = static_cast<int>(static_cast<float>(nFrameCount) / tmFPS.Delta() * 1000.0f);
		nFrameCount = 0;
		tmFPS.Reset();
	}
	else
	{
		tmFPS.Update();
		nFrameCount++;
	}
	{
		char buf[32];
		std::snprintf(buf, sizeof(buf), "FPS: %d", nFPS);
		platform::draw_text(g_renderer, platform::FONT_SYS, buf, 4, 2, 100, 100, 255);
	}

	if (m_aPlayers[0].bAlive)
	{
		ui_health.Draw();
		ui_rifle.Draw();
		ui_grenade.Draw();

		char buf[32];
		std::snprintf(buf, sizeof(buf), "%d%%", m_aPlayers[0].health);
		platform::draw_text(
		    g_renderer, platform::FONT_UI, buf, (g_iScreenWidth >> 1) + (ui_health.iWidth >> 1),
		    g_iScreenHeight - ui_health.iHeight + (ui_health.iWidth >> 2), 255, 255, 0);

		std::snprintf(buf, sizeof(buf), "%d", m_aPlayers[0].ammo[static_cast<int>(WEAPON::RIFLE)]);
		platform::draw_text(g_renderer, platform::FONT_UI, buf, ui_rifle.iWidth,
		                    g_iScreenHeight - ui_rifle.iHeight + (ui_rifle.iHeight >> 2), 255, 255,
		                    0);

		std::snprintf(buf, sizeof(buf), "%d",
		              m_aPlayers[0].ammo[static_cast<int>(WEAPON::GRENADE)]);
		platform::draw_text(
		    g_renderer, platform::FONT_UI, buf, g_iScreenWidth - (ui_grenade.iWidth >> 1),
		    g_iScreenHeight - ui_grenade.iHeight + (ui_grenade.iHeight >> 2), 255, 255, 0);

		// cursor / crosshair — original D3D9 used -fTheta; SDL_RenderCopyEx
		// equivalent visual is +fTheta.
		VECTOR2D vec = Normalize(m_aPlayers[0].cursor) * 100;
		float fTheta = atan2(vec.y, vec.x);
		vec += g_vCenter;
		cur_ptr.Draw(vec.x, vec.y, fTheta, cur_ptr.GetRotationX(), cur_ptr.GetRotationY(), 0.6f);
	}

	// scoreboard (g_bShowStat) deferred — needs custom modulate-blend.

	SDL_RenderPresent(g_renderer);
	return S_OK;
}
