#include "effects.h"

#include <cmath>
#include <cstdlib>
#include <numbers>

#include "gamecode.h" // FIRE_NUMPARTICLES, CONCRETE_NUMPARTICLES,
                      // MAX_PARTICLES, CONCRETEPARTICLE_VEL, PI, RANDOM

PARTICLE* g_pFire = nullptr;
PARTICLE* g_pSmoke = nullptr;
PARTICLE* g_pCustom = nullptr;

int g_FireParticleCnt = 0;
int g_SmokeParticleCnt = 0;
int g_CustomParticleCnt = 0;

void AddFireParticles(VECTOR2D vPoint)
{
	PARTICLE p;
	for (int i = 0; i < FIRE_NUMPARTICLES && g_FireParticleCnt < MAX_PARTICLES; i++)
	{
		const float Mul = 30 * RANDOM + 100;
		const float fTheta = 2 * std::numbers::pi_v<float> * RANDOM;
		const float fRadius = Mul * RANDOM;

		p.Pos = vPoint;
		p.Velocity = VECTOR2D(fTheta) * fRadius;
		p.Acceleration = -p.Velocity / 0.8f;
		p.TTL = (int)(1000 * ((float)rand() / RAND_MAX));
		p.color_start = COLOR(128, 128, 128, 128);
		p.color_end = COLOR();

		g_pFire->Add(&p);
		g_FireParticleCnt++;
	}
}

void AddSmokeParticles(VECTOR2D /*vPoint*/)
{
	// Empty in the legacy build (the smoke texture is never loaded
	// either). Kept as a hook so future smoke effects don't need to
	// touch the call sites.
}

void AddCustomParticles(VECTOR2D vPoint, VECTOR2D vNormal, COLOR color)
{
	const float fTheta = atan2(vNormal.y, vNormal.x) + std::numbers::pi_v<float> / 2.0f;

	// Offset spawn slightly along the surface normal so static-wall
	// particles don't get re-absorbed by gravity before they're visible.
	// Dynamic bodies move away on impact and don't need this; static
	// walls do.
	const VECTOR2D vSpawn = vPoint + Normalize(vNormal) * 4.0f;

	PARTICLE p;
	for (int i = 0; i < CONCRETE_NUMPARTICLES && g_CustomParticleCnt < MAX_PARTICLES; i++)
	{
		const float fVel = CONCRETEPARTICLE_VEL * RANDOM;
		p.Pos = vSpawn;
		p.Velocity = VECTOR2D(fTheta - std::numbers::pi_v<float> * RANDOM) * fVel;
		p.Acceleration = VECTOR2D(0, 0);
		p.color_start = color;
		p.color_end = COLOR();
		p.TTL = 500;

		g_pCustom->Add(&p);
		g_CustomParticleCnt++;
	}
}
