#ifndef GAME_PLAYERS_H
#define GAME_PLAYERS_H

#include "gamecode.h"

// Player creation + respawn lifecycle. The PLAYER struct + Init/Update
// method declarations live in gamecode.h (alongside the extern arrays
// m_aPlayers / m_aFrags / m_aDeath / g_iNumPlayers).
HRESULT AddPlayer();
void RespawnPlayer(PLAYER* player);

// Authored animations referenced by AddPlayer + PLAYER::Update.
extern ANIMATION animations[static_cast<int>(ANIMATION_TYPE::NUMANIMATIONS)];

// Per-part ragdoll mesh paths.
extern const char* szMeshFile[];

#endif // GAME_PLAYERS_H
