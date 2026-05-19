#ifndef GAME_EFFECTS_H
#define GAME_EFFECTS_H

// Particle-spawn helpers and the three PARTICLE list heads they push to.
// The actual particle list machinery (PARTICLE::Add / Update / Delete)
// lives in graphics/particles.{h,cpp}; this header is the game-side
// glue that decides when and what to spawn.

#include "graphics/particles.h"
#include "physics/math2D.h"

extern PARTICLE* g_pFire;
extern PARTICLE* g_pSmoke;
extern PARTICLE* g_pCustom;

extern int g_FireParticleCnt;
extern int g_SmokeParticleCnt;
extern int g_CustomParticleCnt;

void AddFireParticles(VECTOR2D vPoint);
void AddSmokeParticles(VECTOR2D vPoint);
void AddCustomParticles(VECTOR2D vPoint, VECTOR2D vNormal, COLOR color);

#endif // GAME_EFFECTS_H
