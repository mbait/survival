

#include "gamecode.h"
#include <SDL.h>
#include <SDL_image.h>

#include <fstream>

#include <vector>

#include "platform/font_cache.h"

//#define DEBUG
//#define GODMODE

const char *szMeshFile[] = {
	{"data/meshes/ragdoll/head_mesh_c.dat"},
	{"data/meshes/ragdoll/body_mesh_c.dat"},
	{"data/meshes/ragdoll/belt_mesh_c.dat"},
	{"data/meshes/ragdoll/shoulder_mesh_c.dat"},
	{"data/meshes/ragdoll/shoulder_mesh_c.dat"},
	{"data/meshes/ragdoll/arm_mesh_c.dat"},
	{"data/meshes/ragdoll/arm_mesh_c.dat"},
	{"data/meshes/ragdoll/hand_mesh_c.dat"},
	{"data/meshes/ragdoll/hand_mesh_c.dat"},
	{"data/meshes/ragdoll/thigh_mesh_c.dat"},
	{"data/meshes/ragdoll/thigh_mesh_c.dat"},
	{"data/meshes/ragdoll/leg_mesh_c.dat"},
	{"data/meshes/ragdoll/leg_mesh_c.dat"},
	{"data/meshes/ragdoll/foot_mesh_c.dat"},
	{"data/meshes/ragdoll/foot_mesh_c.dat"}
};

ANIMATION animations[static_cast<int>(ANIMATION_TYPE::NUMANIMATIONS)] = {
	{ 0,  0,  1.0f,     1, ANIMATION_SINGLE}, //ANIMATION_TYPE::ANIMATION_IDLE
	{ 6, 10, 0.01f,  500,  ANIMATION_LOOP},	  //ANIMATION_TYPE::ANIMATION_RUN
	{ 1,  5, 0.01f, 1000,  ANIMATION_RETURN}, //ANIMATION_TYPE::ANIMATION_JUMP
	{13, 15, 0.01f,  500,  ANIMATION_SINGLE}, //ANIMATION_TYPE::ANIMATION_WJUMP
	{11, 11,  1.0f,    1,  ANIMATION_SINGLE}, //ANIMATION_TYPE::ANIMATION_SLIDE
	{12, 12,  1.0f,    1,  ANIMATION_SINGLE}  //ANIMATION_TYPE::ANIMATION_FALL
};

// SDL render context — borrowed from the platform layer; we don't own these.
static SDL_Window*   g_window   = nullptr;
static SDL_Renderer* g_renderer = nullptr;

// Owned textures: loaded in LoadGameData, freed in Cleanup. Fonts and
// per-frame geometry batches come back in Phase 1k.2.
SDL_Texture* g_pFireTexture  = nullptr;
SDL_Texture* g_pSmokeTexture = nullptr;

// Input migrated to SDL — keyboard state is read directly from
// SDL_GetKeyboardState in UpdateScene, mouse state from
// SDL_GetRelativeMouseState. No persistent device handles needed.

MATERIAL g_aMaterials[MAX_MATERIALS];
const char *g_szMaterialFile[MAX_MATERIALS] = 
{
	"data/textures/Bricks.jpg\0",
	"data/textures/Metal.jpg\0",
	"data/textures/plastic.jpg\0",
	"data/textures/box.tga\0"
};

//game objects
PLAYER			m_aPlayers[MAX_PLAYERS];
RIGIDBODY		*aWalls;
RIGIDBODY		*aBodies;
VECTOR2D		*aRespawns;
PACK			*aPacks;
VECTOR2D		*aPackPlaces;
NODE			*aWayPoints;
int				**apPathParent;
float			**apPathDistance;
int				m_aFrags[MAX_PLAYERS];
int				m_aDeath[MAX_PLAYERS];
AI_STATE_TYPE	*m_AIStates;

//game sprites
SPRITE	rifle;
SPRITE	grenade;
SPRITE	fire;
SPRITE	cur_ptr;
SPRITE	ui_health;
SPRITE	ui_rifle;
SPRITE	ui_grenade;
SPRITE  pack_ammo;
SPRITE  pack_grenade;
SPRITE  pack_health;

//cursor coordinates
SDL_Point g_cursor;
VECTOR2D g_vCenter;

//particle objects
PARTICLE *g_pFire;
int g_FireParticleCnt = 0;
PARTICLE *g_pSmoke;
int g_SmokeParticleCnt = 0;
PARTICLE *g_pCustom;
int g_CustomParticleCnt = 0;

//world data
VECTOR2D **g_vWallTex = 0;
VECTOR2D **g_vBodyTex = 0;
int g_iNumPlayers = 0;
int g_iNumWalls = 0;
int g_iNumWallVertices = 0;
int g_iNumBodies = 0;
int g_iNumBodyVertices = 0;
int g_iNumRespawns = 0;
int g_iNumPackPlaces = 0;
int g_iNumWayPoints = 0;
bool g_bCollided = false;
DWORD g_last_coltime = 0;

//device settings

TIMER tmFPS;
int nFrameCount = 0;
int nFPS = 0;

bool g_bAddKeyOnce = true;
bool g_bRemoveKeyOnce = true;
bool g_bShowStat = false;

HRESULT	AddPlayer();
void	RespawnPlayer(PLAYER* player);
void	GetAIActions(int index, bool *actions);
int		ScoreCmp(const void* arg_1, const void* arg_2);

/////////////////////////////////////PARTICLE SYSTEM FUCTIONS//////////////////////////////////////
void AddFireParticles(VECTOR2D vPoint)
{
	PARTICLE p;
	
	for(int i=0; i<FIRE_NUMPARTICLES && g_FireParticleCnt<MAX_PARTICLES; i++)
	{
		float Mul = 30*RANDOM+100;
		float fTheta = 2*PI*RANDOM;
		float fRadius = Mul*RANDOM;

		p.Pos = vPoint;
		p.Velocity = VECTOR2D(fTheta)*fRadius;
		p.Acceleration = -p.Velocity/0.8;
		
		p.TTL = (int)(1000*((float)rand()/RAND_MAX));
		
		
		p.color_start = COLOR(128, 128, 128, 128);
		p.color_end   = COLOR();

		g_pFire->Add(&p);
		g_FireParticleCnt++;
	}
}

void AddSmokeParticles(VECTOR2D vPoint)
{
}

void AddCustomParticles(VECTOR2D vPoint, VECTOR2D vNormal, COLOR color)
{
	float fTheta = atan2(vNormal.y, vNormal.x)+PI/2.0f;
	float fVel;
	PARTICLE p;

	// Offset spawn slightly along the surface normal so static-wall
	// particles don't get re-absorbed by gravity before they're visible.
	// Dynamic bodies move away on impact and don't need this; static
	// walls do.
	const VECTOR2D vSpawn = vPoint + Normalize(vNormal) * 4.0f;

	for(int i=0; i<CONCRETE_NUMPARTICLES && g_CustomParticleCnt<MAX_PARTICLES; i++)
	{
		fVel = CONCRETEPARTICLE_VEL*RANDOM;

		p.Pos = vSpawn;
		p.Velocity = VECTOR2D(fTheta-PI*RANDOM)*fVel;
		p.Acceleration = VECTOR2D(0, 0);
		
		p.color_start = color;
		p.color_end   = COLOR();
		
		p.TTL = 500;

		g_pCustom->Add(&p);
		g_CustomParticleCnt++;
	}
}
///////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT PLAYER::Init(SDL_Renderer* pRenderer,
					 const char *szModelFileName,
					 const char *szBodyFileName,
					 const char *szRagDollDir)
{
	bAlive = true;
	
	bJumpKeyOnce = true;
	bWJumpKeyOnce = true;

	pViewObject = 0;
	
	HRESULT hr;

	hr = model.LoadFromFile(pRenderer, szModelFileName);
	if(FAILED(hr))
		return hr;

	std::ifstream f(szBodyFileName);
	if(!f)
		return E_FAIL;

	int iNumVertices = 0;
	f >> iNumVertices;
	VECTOR2D *aVertices = new VECTOR2D[iNumVertices];
	float x, y;
	for(int i=0; i<iNumVertices; i++)
	{
		f >> x >> y;
		aVertices[i] = VECTOR2D(x, y);
	}
	f.close();
	body = RIGIDBODY(aVertices, iNumVertices);
	delete [] aVertices;

	//////load ragdoll mesh///////
	float minx = _HUGE, miny = _HUGE;
	float maxx = -_HUGE, maxy = -_HUGE;
	for(int i=0; i<NUM_PARTS; i++)
	{
		f.open(szMeshFile[i]);
		if(!f)
			return E_FAIL;

		f >> iNumVertices;
		for(int j=0; j<iNumVertices; j++)
		{
			f >> x >> y;
			x *= 0.6f;
			y *= 0.6f;

			if(x>maxx)
				maxx = x;
			else if(x<minx)
				minx = x;
			
			if(y>maxy)
				maxy = y;
			else if(y<miny)
				miny = y;
		}
		f.close();
		
		iNumVertices = 4;
		aVertices = new VECTOR2D[iNumVertices];
		aVertices[0] = VECTOR2D();
		aVertices[1] = VECTOR2D(maxx-minx, 0.0f);
		aVertices[2] = VECTOR2D(maxx-minx, maxy-miny);
		aVertices[3] = VECTOR2D(0.0f, maxy-miny);
		ragdoll[i] = RIGIDBODY(aVertices, iNumVertices);
		
		float dx = (model.GetPart(i)->GetRotationX()-
			(model.GetPart(i)->iWidth>>1))*0.4f;
		float dy = (model.GetPart(i)->GetRotationY()-
			(model.GetPart(i)->iHeight>>1))*0.4f;
		VECTOR2D vOffset(dx, dy);
		
		ragdoll[i].Pos += vOffset;
		ragdoll[i].fRestitution = 0.6f;
		ragdoll[i].fFriction = 0.01f;
		ragdoll[i] = RIGIDBODY(aVertices, iNumVertices);
		ragdoll[i].fRestitution = 0.6f;
		ragdoll[i].fFriction = 0.01f;
		delete [] aVertices;
	}
	
	//head -> body
	/*VECTOR2D pos = VECTOR2D(model.GetPart(HEAD)->GetXPos(),
		model.GetPart(HEAD)->GetYPos()); 
	joints[0] = JOINT(&ragdoll[BODY], &ragdoll[HEAD], ragdoll[BODY].lpVertices[1],
		ragdoll[HEAD].lpVertices[0]);

	//body -> belt
	pos = VECTOR2D(model.GetPart(BODY)->GetXPos(),
		model.GetPart(BODY)->GetYPos()); 
	joints[1] = JOINT(&ragdoll[BELT], &ragdoll[BODY], ragdoll[BELT].lpVertices[1],
		ragdoll[BODY].lpVertices[0]);

	//lthigh -> belt
	pos = VECTOR2D(model.GetPart(LTHIGH)->GetXPos(),
		model.GetPart(LTHIGH)->GetYPos()); 
	joints[2] = JOINT(&ragdoll[BELT], &ragdoll[LTHIGH], ragdoll[BELT].lpVertices[1],
		ragdoll[LTHIGH].lpVertices[0]);


	//rthigh -> belt
	pos = VECTOR2D(model.GetPart(RTHIGH)->GetXPos(),
		model.GetPart(RTHIGH)->GetYPos()); 
	joints[3] = JOINT(&ragdoll[BELT], &ragdoll[RTHIGH], ragdoll[BELT].lpVertices[1],
		ragdoll[RTHIGH].lpVertices[0]);

	//lshoulder -> body
	joints[4] = JOINT(&ragdoll[BODY], &ragdoll[LSHOULDER], ragdoll[BODY].lpVertices[1],
		ragdoll[LSHOULDER].lpVertices[0]);

	//rshoulder -> body
	joints[5] = JOINT(&ragdoll[BODY], &ragdoll[RSHOULDER], ragdoll[BODY].lpVertices[1],
		ragdoll[RSHOULDER].lpVertices[0]);

	//larm -> lshoulder
	joints[6] = JOINT(&ragdoll[LARM], &ragdoll[LSHOULDER], ragdoll[LARM].lpVertices[1],
		ragdoll[LSHOULDER].lpVertices[0]);

	//rarm -> rshoulder
	joints[7] = JOINT(&ragdoll[RARM], &ragdoll[RSHOULDER], ragdoll[RARM].lpVertices[1],
		ragdoll[RSHOULDER].lpVertices[0]);

	//lhand -> larm
	joints[8] = JOINT(&ragdoll[LHAND], &ragdoll[LARM], ragdoll[LHAND].lpVertices[1],
		ragdoll[LARM].lpVertices[0]);

	//rhand -> rarm
	joints[9] = JOINT(&ragdoll[RHAND], &ragdoll[RARM], ragdoll[RHAND].lpVertices[1],
		ragdoll[RARM].lpVertices[0]);

	//lleg -> lthigh
	joints[10] = JOINT(&ragdoll[LLEG], &ragdoll[LTHIGH], ragdoll[LLEG].lpVertices[1],
		ragdoll[LTHIGH].lpVertices[0]);

	//rleg -> rthigh
	joints[11] = JOINT(&ragdoll[RLEG], &ragdoll[RTHIGH], ragdoll[RLEG].lpVertices[1],
		ragdoll[RTHIGH].lpVertices[0]);

	//lfoot -> lleg
	joints[12] = JOINT(&ragdoll[LFOOT], &ragdoll[LLEG], ragdoll[LFOOT].lpVertices[1],
		ragdoll[LLEG].lpVertices[0]);

	//rfoot -> rleg
	joints[13] = JOINT(&ragdoll[RFOOT], &ragdoll[RLEG], ragdoll[RFOOT].lpVertices[1],
		ragdoll[RLEG].lpVertices[0]);
	*/
	//////////////////////////////

	health	= c_NumHealth;
	ammo[static_cast<int>(WEAPON::RIFLE)]		= c_NumRifleAmmo;
	ammo[static_cast<int>(WEAPON::GRENADE)]	= c_NumGrenades;

    return S_OK;
}

void PLAYER::Update(DWORD dwTime, bool *actions)
{
#ifdef GODMODE
	if(!ID)
		health = 100;
#endif

	VECTOR2D MTD(0.0f, 0.0f);
	VECTOR2D Axis(0.0f, 0.0f);
	float t = 0.0f;

	//update grenade state
	int num = dwTime/5;
	for(int i=0; i<num; i++)
	{
		DWORD dwTime = 5;
		if(bActiveGrenade)
		{
			for(int i=0; i<g_iNumWalls; i++)
				if(grenade_body.Collide(
					aWalls[i], MTD, t))
					grenade_body.ResolveCollision(
						aWalls[i], MTD, t);
			
			for(int i=0; i<g_iNumBodies; i++)
				if(grenade_body.Collide(
					aBodies[i], MTD, t))
					grenade_body.ResolveCollision(
						aBodies[i], MTD, t);

			for(int i=0; i<g_iNumPlayers; i++)
			{
				if(m_aPlayers[i].bAlive)	
					if(grenade_body.Collide(
						m_aPlayers[i].body, MTD, t))
					{
						DWORD d = tmGrenade.Delta();
						if(d<2000)
						{
							tmGrenade.LastTickCount -=
								2000-d;
						}
					}
			}
		
			grenade_body.ApplyForce(VECTOR2D(0.0f, g*
				m_aPlayers[0].grenade_body.fMass*0.5f));
			grenade_body.Update(dwTime/1000.0f);

			if(tmGrenade.Delta()>2000)
			{
				AddFireParticles(grenade_body.Pos);
				bActiveGrenade = false;

				//add explode force
				for(int i=0; i<g_iNumBodies; i++)
				{
					if(CircleIntersect(aBodies[i], 
						grenade_body.Pos, 
						EXPLODE_RANGE, t, MTD))
					{
						float fForce = EXPLODE_FORCE*(t/EXPLODE_RANGE);
						VECTOR2D force_point = MTD*(EXPLODE_RANGE-
							t)+grenade_body.Pos;
						aBodies[i].ApplyImpulse(fForce,
							MTD, force_point);
					}
				}

				for(int i=0; i<g_iNumPlayers; i++)
				{
					if(CircleIntersect(m_aPlayers[i].body, 
						grenade_body.Pos, 
						EXPLODE_RANGE, t, MTD))
					{
						float fForce = EXPLODE_FORCE*(t/EXPLODE_RANGE);
						VECTOR2D force_point = MTD*(EXPLODE_RANGE-
							t)+grenade_body.Pos;
						m_aPlayers[i].body.ApplyImpulse(fForce,
							MTD, force_point);

						m_aPlayers[i].health -= (int)((fForce/EXPLODE_FORCE)*100);
						if(m_aPlayers[i].health<=0 && i!=static_cast<int>(ID))
							m_aFrags[ID]++;

					}
				}
			}
			else
				tmGrenade.Update();
		}
	}

	//update life state
	if(body.Velocity.y>FALL_MAXVEL)
		health = 0;
	
	if(health<=0 && bAlive)
	{
		m_aDeath[ID]++;
		
		for(int i=0; i<NUM_PARTS; i++)
		{
			ragdoll[i].Move(body.Pos-ragdoll[i].Pos);
			ragdoll[i].Rotate(-ragdoll[i].fOrientation);

			float fTheta = PI*(2.0f*RANDOM-1.0f);
			ragdoll[i].Velocity = VECTOR2D(fTheta)*(RANDOM*500-250);
			ragdoll[i].fAngVelocity = RANDOM*100-50;
		}

		health = 0;
		bAlive = false;
		tmDeath.Reset();
	}
	
	if(!bAlive)
	{
		if(tmDeath.Delta()>4000)
		{
			RespawnPlayer(this);
			return;
		}
		
		for(int i=0; i<NUM_PARTS; i++)
		{
			/*for(int j=0; j<10; j++)
				joints[j].CalcForce();
			*/

			for(int j=0; j<g_iNumBodies; j++)
				if(ragdoll[i].Collide(aBodies[j],
					MTD, t))
				{
					ragdoll[i].ResolveCollision(aBodies[j],
						MTD, t);
				}

			for(int j=0; j<g_iNumWalls; j++)
				if(ragdoll[i].Collide(aWalls[j],
					MTD, t))
				{
					ragdoll[i].ResolveCollision(aWalls[j],
						MTD, t);
				}

			ragdoll[i].ApplyForce(VECTOR2D(0.0f, g*
				ragdoll[i].fMass));
			ragdoll[i].Update(dwTime/1000.0f);
		}

		tmDeath.Update();

		return;
	}

	//update animation
	model.Tick(dwTime);
		
	for(int i=0; i<NUM_PARTS; i++)
		model.GetPart(i)->SetRotation(0.0f);
	
	body.fAngVelocity = 0.0f;
	//check body for collisions
	int j = 0;
	bool bCollided = false;
	while(j<g_iNumWalls)
	{
		if(body.Collide(aWalls[j], MTD, t))
		{
			Axis += MTD*t;
			bCollided = true;
		}
		j++;
	}

	bool bCanJump = false;
	if(bCollided)
	{
		coll = COLLISION::VERT;
		bCanJump = true;

		if(body.Velocity.y>FALL_MINVEL)
				health -= (int)((body.Velocity.y/FALL_MAXVEL)*
					(float)c_NumHealth);
	}
	
	//resolve collision
	if(bCollided)
	{
		VECTOR2D D = Perp(Normalize(Axis));
		float dp = DotProduct(body.Velocity, D);
		VECTOR2D N = D*dp;
		VECTOR2D T = body.Velocity-N;

		VECTOR2D B, F;
		if(DotProduct(Axis, VECTOR2D(0.0f, -1.0f)))
			coll = COLLISION::HORZ;
		
		B = T*BOUNCE;
		F = VECTOR2D(N.x*(1-FRICTION), N.y);
		
		body.Move(Axis);
		body.Velocity = F+B;
	}
	else
	{
		coll = COLLISION::NONE;
		
		body.Move(VECTOR2D(0.0f, DEPTH_EPSILON));
		
		VECTOR2D MTD;
		float t;
		int j=0;
		while(j<g_iNumWalls)
		{	
			if(body.Collide(aWalls[j], MTD, t))
			{
				bCollided = true;
				bCanJump = true;
				coll = COLLISION::HORZ;
				Axis = MTD;
				break;
			}
			j++;
		}
		
		body.Move(VECTOR2D(0.0f, -DEPTH_EPSILON));
	}
	
	//check collision with boxes
	bBoxCollision = false;

	j = 0;
	while(j<g_iNumBodies)
	{
		if(body.Collide(aBodies[j], MTD, t))
		{
			body.ResolveCollision(aBodies[j],
				MTD, t);
			body.fAngVelocity = 0.0f;
			Axis += MTD*t;
			bCollided = true;
			coll = COLLISION::HORZ;

			bBoxCollision = true;
		}
		j++;
	}

	j = 0;
	if(!bCollided)
	{
		body.Move(VECTOR2D(0.0f, DEPTH_EPSILON));
		
		while(j<g_iNumBodies)
		{	
			if(body.Collide(aBodies[j], MTD, t))
			{
				bCollided = true;
				bCanJump = true;
				coll = COLLISION::HORZ;
				Axis = MTD;
				break;

				bBoxCollision = true;
			}
			j++;
		}

		body.Move(VECTOR2D(0.0f, -DEPTH_EPSILON));
	}

	if(fabs(body.Velocity.x)<RUN_MINVEL)
		body.Velocity.x = 0.0f;

	g_bCollided = bCollided;
	///////////////////////////////////////////////////////////////////////////////////////////////

	VECTOR2D d = aWayPoints[nVertexIndex].vPos-body.Pos;
	float dp = DotProduct(d, d);
	float mindp = dp;
	int index = nVertexIndex;

	//absolute points
	for(int i=0; i<g_iNumWayPoints; i++)
	{
		d = aWayPoints[i].vPos-body.Pos;
		dp = DotProduct(d, d);
		if(dp<mindp)
		{
			index = i;
			mindp = dp;
		}
	}
	
	
	//local points
	/*for(int i=0; i<aWayPoints[nVertexIndex].iNumEdges; i++)
	{
		v = aWayPoints[nVertexIndex].aEdges[i];
		d = aWayPoints[v].vPos-body.Pos;
		dp = DotProduct(d, d);
		if(dp<mindp)
		{
			index = v;
			mindp = dp;
		}
	}
	*/
	nVertexIndex = index;

	//resolve soldat actions (aka keyboard state :-))
	state = STATE::IDLE;
	
	if(actions[static_cast<int>(ACTION::MOVELEFT)])
	{
		if(coll != COLLISION::NONE)
		{
			body.Velocity.x -= RUN_ACC*dwTime/1000.0f;
			if(body.Velocity.x < -RUN_MAXVEL)
				body.Velocity.x = -RUN_MAXVEL;

			state = STATE::RUN;
		}
			else
				if(body.Velocity.x>-FLY_MAXVEL)
					body.Velocity.x -= FLY_ACC*dwTime/1000.0f;
	}
	if(actions[static_cast<int>(ACTION::MOVERIGHT)])
	{
		if(coll != COLLISION::NONE)
		{
			body.Velocity.x += RUN_ACC*dwTime/1000.0f;
			if(body.Velocity.x > RUN_MAXVEL)
				body.Velocity.x = RUN_MAXVEL;

			state = STATE::RUN;
		}
		else
			if(body.Velocity.x<FLY_MAXVEL)
				body.Velocity.x += FLY_ACC*dwTime/1000.0f;
	}
	if(actions[static_cast<int>(ACTION::JUMP)])
	{
		if(bCanJump && coll == COLLISION::HORZ && bJumpKeyOnce)
		{
			body.Velocity.y -= JUMP_ACC;
			//body.Move(Axis*20);
			body.Move(VECTOR2D(0.0f, -20.0f));
	
			model.SetAnimation(&animations[static_cast<int>(ANIMATION_TYPE::ANIMATION_JUMP)]);
			model.StartAnimation();

			state = STATE::FLY;
			bJumpKeyOnce = false;
		}
	}
	else
		bJumpKeyOnce = true;
	if(actions[static_cast<int>(ACTION::WJUMP)])
	{
		if(bCanJump && bWJumpKeyOnce)
		{
			VECTOR2D vJump = Axis;
			float vsign;
			if(PerpDotProduct(vJump, VECTOR2D(0, -1.0f))>0)
				vsign = 1.0f;
			else
				vsign = -1.0f;
			Rotate(&vJump, 0, PI/4*vsign);
			if(!DotProduct(Axis, VECTOR2D(1.0f, 0.0f)))
			{
				float fSign = 1-2*model.GetOrientation();
				body.Velocity += VECTOR2D(fSign, -1.0f)*WJUMP_ACC;
			}
			else	
				body.Velocity += VECTOR2D(-vsign, -1.0f)*WJUMP_ACC;
			body.Move(Axis*10);

			model.SetAnimation(&animations[static_cast<int>(ANIMATION_TYPE::ANIMATION_WJUMP)]);
			model.StartAnimation();

			state = STATE::FLY;
			bWJumpKeyOnce = false;
		}
	}
	else
		bWJumpKeyOnce = true;
    	
	//addition states
	if(state == STATE::IDLE && Length(body.Velocity)>40.0f)
		state = STATE::SLIDE;
	if(state == STATE::SLIDE && !bCollided) {
		if(body.Velocity.y<0)
			state = STATE::FLY;
		else
			state = STATE::FALL;
	}
		
	//resolve soldat state
	switch(state)
	{
	case STATE::IDLE:
		{
			model.SetAnimation(&animations[static_cast<int>(ANIMATION_TYPE::ANIMATION_IDLE)]);
			model.StartAnimation();
		}break;
	case STATE::SLIDE:
		{
			model.SetAnimation(&animations[static_cast<int>(ANIMATION_TYPE::ANIMATION_SLIDE)]);
			model.StartAnimation();
		}break;
	case STATE::RUN:
		{
			if(prev_state != STATE::RUN)
			{
				model.SetAnimation(&animations[static_cast<int>(ANIMATION_TYPE::ANIMATION_RUN)]);
				model.StartAnimation();
			}
		}break;
	case STATE::FALL:
		{
			model.SetAnimation(&animations[static_cast<int>(ANIMATION_TYPE::ANIMATION_FALL)]);
			model.StartAnimation();
		}break;
	case STATE::FLY:
		{
			if(!model.IsAnimated())
			{
				model.SetAnimation(&animations[static_cast<int>(ANIMATION_TYPE::ANIMATION_FALL)]);
				model.StartAnimation();
			}
		}
	case STATE::NUMSTATES: break;
	}
	prev_state = state;
	
	//apply forces
	body.ApplyForce(VECTOR2D(0, g*body.fMass));
	body.Update(dwTime/1000.0f);
	
	//update model orientation
	int s = Sign(body.Velocity.x);
	if(s == 1)
		model.SetOrientation(0);
	else if(s == -1)
		model.SetOrientation(1);
		
	//update model pos and rotation
	int o = 1-model.GetOrientation()*2;
	VECTOR2D Pos(body.Pos.x, body.Pos.y+5.0f);
	Rotate(&Pos, &body.Pos, body.fOrientation);
	//model.SetXYPos(g_iScreenWidth>>1, (g_iScreenHeight>>1)+5);
	if(DotProduct(body.Velocity, body.Velocity)>40000.0f)
		model.SetRotation(o*body.fOrientation);
	
	//==============update model view direction================//
	float fTheta = atan2(cursor.y, 50*o);
	
	//Orient cursor in space
	VECTOR2D cursor_vec(100.0f, 15.0f*o);
	Rotate(&cursor_vec, 0, fTheta);
		
	//calc ray
	VECTOR2D RayStart = body.Pos;
	VECTOR2D RayEnd	  = RayStart+cursor_vec;
	float mint = _HUGE;
	VECTOR2D Nt, minNt;
	
	for(int i=0; i<g_iNumWalls; i++)
		if(RayIntersect(aWalls[i], RayStart, RayEnd, t, Nt))
			if(t<mint)
			{
				mint = t;
				minNt = Nt;
				pViewObject = &aWalls[i];
			}
	for(int i=0; i<g_iNumBodies; i++)
		if(RayIntersect(aBodies[i], RayStart, RayEnd, t, Nt))
			if(t<mint)
			{
				mint = t;
				minNt = Nt;
				pViewObject = &aBodies[i];
			}

	bool bViewSoldier = false;
	int  iSoldierInd = -1;
	VECTOR2D vView;
	for(int i=0; i<g_iNumPlayers; i++)
		if(i!=static_cast<int>(ID) && m_aPlayers[i].bAlive)
			if(RayIntersect(m_aPlayers[i].body, RayStart, RayEnd, t, Nt))
			{
				vView = Nt*t;
				if(t<mint && DotProduct(vView, vView)>500)
				{
					mint = t;
					minNt = Nt;
					pViewObject = &m_aPlayers[i].body;

					bViewSoldier = true;
					iSoldierInd = i;
				}
			}

	iViewSoldierID = iSoldierInd;

	if(mint == _HUGE)
	{
		mint = g_iScreenWidth*g_iScreenWidth+
				g_iScreenHeight*g_iScreenHeight;

		pViewObject = 0;
	}
		
	cursor = cursor_vec*mint;
	
	//cur_ptr.SetXYPos(g_vCenter.x + Normalize(cursor).x * 100, ...);
	//0.2f = atan2(100, 20) :-), precalculations rulezz!!!
	//cur_ptr.SetRotation(fTheta-0.2f*o); 

	model.GetPart(BODY)->SetRotation(o*fTheta+
		PI*model.GetOrientation());
	//resolve mouse state: shooting and etc.
	if(actions[static_cast<int>(ACTION::SHOOT)] && tmShoot.Delta()>50 && ammo[static_cast<int>(WEAPON::RIFLE)]>0)
	{
		bShooting = true;
		
		VECTOR2D view_pt = RayStart+cursor;

		//apply shoot impulse
		bool bStaticObject = true;
		if(pViewObject)
			if(!pViewObject->IsStatic())
			{
				pViewObject->ApplyImpulse(-SHOOT_FORCE,
					minNt, view_pt);
				
				if(bViewSoldier)
				{
					m_aPlayers[iSoldierInd].health -= SHOOT_DAMAGE;
					if(m_aPlayers[iSoldierInd].health<=0)
						m_aFrags[ID]++;

				}

				bStaticObject = false;
			}

		//add particles
		if(mint>0.0f) {
			if(bStaticObject) {
				AddCustomParticles(view_pt, minNt, COLOR(128, 128, 128, 128));
			} else {
				if(bViewSoldier)
					AddCustomParticles(view_pt, minNt, COLOR(255, 255, 155, 0));
				else
					AddCustomParticles(view_pt, minNt, COLOR(255, 190, 150, 10));
			}
		}
				
		tmShoot.Reset();

		ammo[static_cast<int>(WEAPON::RIFLE)]--;
	}
	else
	{
		bShooting = false;
		tmShoot.Update();
	}

	if(actions[static_cast<int>(ACTION::ALTSHOOT)] && tmAltShoot.Delta()>2500 && ammo[static_cast<int>(WEAPON::GRENADE)]>0)
	{
		//init grenade
		if(DotProduct(cursor, cursor)>2500)
			grenade_body.Move(-grenade_body.Pos+
				body.Pos+Normalize(cursor)*50);
		else
			grenade_body.Move(-grenade_body.Pos+
				body.Pos);
		grenade_body.Rotate(-grenade_body.fOrientation);
		grenade_body.Velocity = Normalize(cursor_vec)*400;
		grenade_body.fAngVelocity = 50*(1-2*RANDOM);
		bActiveGrenade = true;

		tmGrenade.Reset();
		tmAltShoot.Reset();

		ammo[static_cast<int>(WEAPON::GRENADE)]--;
	}
	else
		tmAltShoot.Update();

	//check for collisions with packs
	for(int i=0; i<g_iNumPackPlaces; i++)
		if(aPacks[i].bActive)
		{
			VECTOR2D v = aPackPlaces[i]-body.Pos;
			if(DotProduct(v, v)<=1500.0f)
			{
				switch(aPacks[i].type)
				{
				case PACK_TYPE::PACK_AMMO:
					{
						if(ammo[static_cast<int>(WEAPON::RIFLE)] == c_NumRifleAmmo)
							continue;
						
						ammo[static_cast<int>(WEAPON::RIFLE)] += PACK_AMMO_SIZE;
						if(ammo[static_cast<int>(WEAPON::RIFLE)]>c_NumRifleAmmo)
							ammo[static_cast<int>(WEAPON::RIFLE)] = c_NumRifleAmmo;
					}break;
				case PACK_TYPE::PACK_GRENADE:
					{
						if(ammo[static_cast<int>(WEAPON::GRENADE)] == c_NumGrenades)
							continue;
						
						ammo[static_cast<int>(WEAPON::GRENADE)] += PACK_GRENADE_SIZE;
						if(ammo[static_cast<int>(WEAPON::GRENADE)]>c_NumGrenades)
							ammo[static_cast<int>(WEAPON::GRENADE)] = c_NumGrenades;
					}break;
				case PACK_TYPE::PACK_HEALTH:
					{
						if(health == c_NumHealth)
							continue;
						
						health += PACK_HEALTH_SIZE;
						if(health>c_NumHealth)
							health = c_NumHealth;
					}break;
				case PACK_TYPE::NUM_PACKS: break;
				}

				aPacks[i].bActive = false;
				aPacks[i].tmReset.Reset();
			}
		}
}
///////////////////////////////////////////////////////////////////////////////////////////////////
void CalcPhysics(DWORD dwTime)
{
	VECTOR2D MTD;
	float t;

	//update world objects
	for(int i=0; i<g_iNumBodies-1; i++)
	{
		for(int j=i+1; j<g_iNumBodies; j++)
			if(aBodies[i].Collide(aBodies[j],
				MTD, t))
				aBodies[i].ResolveCollision(
					aBodies[j], MTD, t);
	}

	for(int i=0; i<g_iNumBodies; i++)
	{
		for(int j=0; j<g_iNumWalls; j++)
			if(aBodies[i].Collide(aWalls[j],
				MTD, t))
				aBodies[i].ResolveCollision(
					aWalls[j], MTD, t);
	}

	for(int i=0; i<g_iNumBodies; i++)
		aBodies[i].ApplyForce(VECTOR2D(0.0f, g*
			aBodies[i].fMass));

	for(int i=0; i<g_iNumBodies; i++)
		aBodies[i].Update(dwTime/1000.0f);
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

	//resolve keyboard state
	if(keystate[SDL_SCANCODE_A])
	{
		m_aPlayers[0].dir = DIRECTION::LEFT;
		actions[static_cast<int>(ACTION::MOVELEFT)] = true;
	}
	if(keystate[SDL_SCANCODE_D])
	{
		m_aPlayers[0].dir = DIRECTION::RIGHT;
		actions[static_cast<int>(ACTION::MOVERIGHT)] = true;
	}
	if(keystate[SDL_SCANCODE_W])
	{
		actions[static_cast<int>(ACTION::JUMP)] = true;
	}
	if(keystate[SDL_SCANCODE_Q])
		actions[static_cast<int>(ACTION::WJUMP)] = true;

	//none-control keystate
	if(keystate[SDL_SCANCODE_INSERT])
	{
		if(g_bAddKeyOnce && g_iNumPlayers<MAX_PLAYERS)
		{
			(void)AddPlayer();
			g_bAddKeyOnce = false;
		}
	}
	else
		g_bAddKeyOnce = true;

	if(keystate[SDL_SCANCODE_DELETE])
	{
		if(g_bRemoveKeyOnce && g_iNumPlayers>1)
		{
			g_iNumPlayers--;
			g_bRemoveKeyOnce = false;
		}
	}
	else
		g_bRemoveKeyOnce = true;

	if(keystate[SDL_SCANCODE_TAB])
		g_bShowStat = true;
	else
		g_bShowStat = false;

	//=========================read mouse state=========================//
	// Relative-mouse mode is enabled by the platform layer at start-up;
	// each call returns motion accumulated since the last call.
	int mouse_dx = 0;
	int mouse_dy = 0;
	const Uint32 buttons = SDL_GetRelativeMouseState(&mouse_dx, &mouse_dy);

	if(buttons & SDL_BUTTON(SDL_BUTTON_LEFT))
		actions[static_cast<int>(ACTION::SHOOT)] = true;
	if(buttons & SDL_BUTTON(SDL_BUTTON_RIGHT))
		actions[static_cast<int>(ACTION::ALTSHOOT)] = true;

	//update cursor pos
	g_cursor.x = (g_cursor.x+mouse_dx);
	g_cursor.y = (g_cursor.y+mouse_dy);
	//clip x
	if(g_cursor.x>MOUSE_MAX_X)
		g_cursor.x = MOUSE_MAX_X;
	else if(g_cursor.x<MOUSE_MIN_X)
		g_cursor.x = MOUSE_MIN_X;
	//clip y
	if(g_cursor.y>MOUSE_MAX_Y)
		g_cursor.y = MOUSE_MAX_Y;
	else if(g_cursor.y<MOUSE_MIN_Y)
		g_cursor.y = MOUSE_MIN_Y;

	m_aPlayers[0].cursor.x = g_cursor.x;
	m_aPlayers[0].cursor.y = g_cursor.y;
	
	//update model
	m_aPlayers[0].Update(dwTime, actions);

	for(int i=1; i<g_iNumPlayers; i++)
	{
		GetAIActions(i, actions);	
		m_aPlayers[i].Update(dwTime, actions);
	}	
	//update world
	int delta = dwTime;
	while(delta>0)
	{
		CalcPhysics(5);
		delta -= 5;
	}

	//=========================update particle system====================//
	VECTOR2D vGravity(0, g);
	PARTICLE *ptr = g_pFire->next; //fire particles
	while(ptr)
	{
		if(!ptr->Update(dwTime, VECTOR2D()))
		{
			g_FireParticleCnt--;
			
			PARTICLE* next_ptr = ptr->next;
			ptr->Delete();
			ptr = next_ptr;
		}
		else
			ptr = ptr->next;
	}
	
	ptr = g_pSmoke->next; //smoke particles
	while(ptr)
	{
		if(!ptr->Update(dwTime, VECTOR2D()))
		{
			g_SmokeParticleCnt--;
			
			PARTICLE* next_ptr = ptr->next;
			ptr->Delete();
			ptr = next_ptr;
		}
		else
			ptr = ptr->next;
	}
	
	ptr = g_pCustom->next; //concrete particles
	while(ptr)
	{
		if(!(ptr->Update(dwTime, vGravity)))
		{
			g_CustomParticleCnt--;
			
			PARTICLE* next_ptr = ptr->next;
			ptr->Delete();
			ptr = next_ptr;
		}
		else
			ptr = ptr->next;
	}

	for(int i=0; i<g_iNumPackPlaces; i++) {
		if(!aPacks[i].bActive) {
			if(aPacks[i].tmReset.Delta()>60000) {
				aPacks[i].type = (PACK_TYPE)(int)(RANDOM*static_cast<int>(PACK_TYPE::NUM_PACKS));
				aPacks[i].bActive = true;
			} else {
				aPacks[i].tmReset.Update();
			}
		}
	}

	return S_OK;
}
///////////////////////////////////////////////////////////////////////////////////////////////////
namespace {

// Render a single textured triangle fan via SDL_RenderGeometry. The
// fan vertices share a single material texture; UVs come from the
// game's pre-baked g_vWallTex / g_vBodyTex arrays.
void render_textured_fan(SDL_Renderer* renderer, SDL_Texture* tex,
                         const LPVECTOR2D pts, const LPVECTOR2D uvs,
                         int n, VECTOR2D vOffset)
{
	if (n < 3) return;
	std::vector<SDL_Vertex> verts(n);
	for (int i = 0; i < n; i++) {
		verts[i].position.x = pts[i].x - vOffset.x;
		verts[i].position.y = pts[i].y - vOffset.y;
		verts[i].color      = SDL_Color{ 255, 255, 255, 255 };
		verts[i].tex_coord.x = uvs[i].x;
		verts[i].tex_coord.y = uvs[i].y;
	}
	std::vector<int> idx;
	idx.reserve(3 * (n - 2));
	for (int i = 1; i + 1 < n; i++) {
		idx.push_back(0);
		idx.push_back(i);
		idx.push_back(i + 1);
	}
	SDL_RenderGeometry(renderer, tex,
	                   verts.data(), static_cast<int>(verts.size()),
	                   idx.data(),   static_cast<int>(idx.size()));
}

// Draw a particle as a small textured quad, alpha+colour modulated and
// using `blend` for the texture (additive for fire/smoke, alpha for
// concrete debris).
void render_particle(SDL_Renderer* renderer, SDL_Texture* tex,
                     const PARTICLE* p, VECTOR2D vOffset,
                     int size, SDL_BlendMode blend)
{
	if (!tex) return;
	SDL_SetTextureBlendMode(tex, blend);
	SDL_SetTextureColorMod(tex, p->color_current.R, p->color_current.G, p->color_current.B);
	SDL_SetTextureAlphaMod(tex, p->color_current.Alpha);
	SDL_Rect dst{
		static_cast<int>(p->Pos.x - vOffset.x - size / 2),
		static_cast<int>(p->Pos.y - vOffset.y - size / 2),
		size, size
	};
	SDL_RenderCopy(renderer, tex, nullptr, &dst);
}

}  // namespace

HRESULT UpdateFrame()
{
	if (!g_renderer) {
		return E_FAIL;
	}

	SDL_SetRenderDrawColor(g_renderer, 0, 0, 0, 255);
	SDL_RenderClear(g_renderer);

	// ===== camera offset: world -> screen ===============================
	VECTOR2D vOffset;
	VECTOR2D player_pos;
	if (m_aPlayers[0].bAlive) {
		vOffset    = m_aPlayers[0].body.Pos - g_vCenter;
		player_pos = g_vCenter + VECTOR2D(0.0f, 5.0f);
		m_aPlayers[0].model.SetXYPos(player_pos.x, player_pos.y);
	} else {
		vOffset = m_aPlayers[0].ragdoll[HEAD].Pos - g_vCenter;
	}

	// ===== terrain (static walls) + dynamic bodies as triangle fans =====
	for (int i = 0; i < g_iNumWalls; i++) {
		render_textured_fan(g_renderer,
		                    g_aMaterials[aWalls[i].lMaterialID].pTexture,
		                    aWalls[i].lpVertices, g_vWallTex[i],
		                    aWalls[i].iNumVertices, vOffset);
	}
	for (int i = 0; i < g_iNumBodies; i++) {
		render_textured_fan(g_renderer,
		                    g_aMaterials[aBodies[i].lMaterialID].pTexture,
		                    aBodies[i].lpVertices, g_vBodyTex[i],
		                    aBodies[i].iNumVertices, vOffset);
	}

	// ===== packs ========================================================
	for (int i = 0; i < g_iNumPackPlaces; i++) {
		if (!aPacks[i].bActive) continue;
		const VECTOR2D pos = aPackPlaces[i] - vOffset;
		SPRITE* spr = nullptr;
		switch (aPacks[i].type) {
			case PACK_TYPE::PACK_AMMO:    spr = &pack_ammo;    break;
			case PACK_TYPE::PACK_GRENADE: spr = &pack_grenade; break;
			case PACK_TYPE::PACK_HEALTH:  spr = &pack_health;  break;
			default: continue;
		}
		spr->Draw(pos.x, pos.y, 0.0f, spr->GetRotationX(), spr->GetRotationY(), 0.8f);
	}

	// ===== main player (or ragdoll if dead) ============================
	if (m_aPlayers[0].bAlive) {
		if (m_aPlayers[0].bShooting) {
			m_aPlayers[0].model.Draw(&rifle, &fire);
		} else {
			m_aPlayers[0].model.Draw(&rifle);
		}
	} else {
		for (int i = 0; i < NUM_PARTS; i++) {
			VECTOR2D Pos     = m_aPlayers[0].ragdoll[i].Pos - vOffset;
			float    fRot    = m_aPlayers[0].ragdoll[i].fOrientation;
			m_aPlayers[0].model.GetPart(i)->Draw(Pos.x, Pos.y, -fRot, 12.0f, 12.0f, 0.4f);
		}
	}
	if (m_aPlayers[0].bActiveGrenade) {
		grenade.Draw(m_aPlayers[0].grenade_body.Pos.x - vOffset.x,
		             m_aPlayers[0].grenade_body.Pos.y - vOffset.y,
		             -m_aPlayers[0].grenade_body.fOrientation,
		             grenade.GetRotationX(), grenade.GetRotationY());
	}

	// ===== AI players ==================================================
	for (int i = 1; i < g_iNumPlayers; i++) {
		if (m_aPlayers[i].bAlive) {
			VECTOR2D pp = m_aPlayers[i].body.Pos + VECTOR2D(0.0f, 5.0f) - vOffset;
			m_aPlayers[i].model.SetXYPos(pp.x, pp.y);
			if (m_aPlayers[i].bShooting) {
				m_aPlayers[i].model.Draw(&rifle, &fire);
			} else {
				m_aPlayers[i].model.Draw(&rifle);
			}
		} else {
			for (int j = 0; j < NUM_PARTS; j++) {
				VECTOR2D Pos  = m_aPlayers[i].ragdoll[j].Pos - vOffset;
				float    fRot = m_aPlayers[i].ragdoll[j].fOrientation;
				m_aPlayers[i].model.GetPart(j)->Draw(Pos.x, Pos.y, -fRot, 12.0f, 12.0f, 0.4f);
			}
		}
		if (m_aPlayers[i].bActiveGrenade) {
			grenade.Draw(m_aPlayers[i].grenade_body.Pos.x - vOffset.x,
			             m_aPlayers[i].grenade_body.Pos.y - vOffset.y,
			             -m_aPlayers[i].grenade_body.fOrientation,
			             grenade.GetRotationX(), grenade.GetRotationY());
		}
	}

	// ===== particles: fire (additive) ==================================
	constexpr int kParticleSize = 20;
	for (PARTICLE* p = g_pFire ? g_pFire->next : nullptr; p; p = p->next) {
		render_particle(g_renderer, g_pFireTexture, p, vOffset,
		                kParticleSize, SDL_BLENDMODE_ADD);
	}
	// smoke uses the same texture (original g_pSmokeTexture is never
	// initialised; AddSmokeParticles is a no-op in the legacy code too).
	for (PARTICLE* p = g_pSmoke ? g_pSmoke->next : nullptr; p; p = p->next) {
		render_particle(g_renderer, g_pFireTexture, p, vOffset,
		                kParticleSize, SDL_BLENDMODE_ADD);
	}
	// concrete debris: regular alpha blend, smaller quads
	for (PARTICLE* p = g_pCustom ? g_pCustom->next : nullptr; p; p = p->next) {
		render_particle(g_renderer, g_pFireTexture, p, vOffset,
		                4, SDL_BLENDMODE_BLEND);
	}

	// ===== HUD: FPS + alive UI ========================================
	if (tmFPS.Delta() > 100) {
		nFPS = static_cast<int>(static_cast<float>(nFrameCount) / tmFPS.Delta() * 1000.0f);
		nFrameCount = 0;
		tmFPS.Reset();
	} else {
		tmFPS.Update();
		nFrameCount++;
	}
	{
		char buf[32];
		std::snprintf(buf, sizeof(buf), "FPS: %d", nFPS);
		platform::draw_text(g_renderer, platform::FONT_SYS, buf, 4, 2,
		                    100, 100, 255);
	}

	if (m_aPlayers[0].bAlive) {
		ui_health.Draw();
		ui_rifle.Draw();
		ui_grenade.Draw();

		char buf[32];
		std::snprintf(buf, sizeof(buf), "%d%%", m_aPlayers[0].health);
		platform::draw_text(g_renderer, platform::FONT_UI, buf,
		                    (g_iScreenWidth >> 1) + (ui_health.iWidth >> 1),
		                    g_iScreenHeight - ui_health.iHeight + (ui_health.iWidth >> 2),
		                    255, 255, 0);

		std::snprintf(buf, sizeof(buf), "%d", m_aPlayers[0].ammo[static_cast<int>(WEAPON::RIFLE)]);
		platform::draw_text(g_renderer, platform::FONT_UI, buf,
		                    ui_rifle.iWidth,
		                    g_iScreenHeight - ui_rifle.iHeight + (ui_rifle.iHeight >> 2),
		                    255, 255, 0);

		std::snprintf(buf, sizeof(buf), "%d", m_aPlayers[0].ammo[static_cast<int>(WEAPON::GRENADE)]);
		platform::draw_text(g_renderer, platform::FONT_UI, buf,
		                    g_iScreenWidth - (ui_grenade.iWidth >> 1),
		                    g_iScreenHeight - ui_grenade.iHeight + (ui_grenade.iHeight >> 2),
		                    255, 255, 0);

		// cursor / crosshair
		VECTOR2D vec = Normalize(m_aPlayers[0].cursor) * 100;
		float fTheta = atan2(vec.y, vec.x);
		vec += g_vCenter;
		// Cursor sprite's "green line" graphic rotates around its center.
		// Original D3D9 build used -fTheta here; under SDL_RenderCopyEx
		// the equivalent visual is +fTheta.
		cur_ptr.Draw(vec.x, vec.y, fTheta,
		             cur_ptr.GetRotationX(), cur_ptr.GetRotationY(), 0.6f);
	}

	// (Scoreboard via g_bShowStat: deferred — needs a custom modulate-blend
	// mode for the translucent backdrop. Hooked up in Phase 2 follow-up.)

	SDL_RenderPresent(g_renderer);
	return S_OK;
}
///////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT AddPlayer()
{
	int index = g_iNumPlayers;
	m_aPlayers[index].ID = index;

	HRESULT hr;
	hr = m_aPlayers[index].Init(g_renderer, "data/models/soldat.m2d",
					 "data/meshes/body.dat", "data/meshes/ragdoll/");
	if (FAILED(hr)) {
		std::fprintf(stderr, "AddPlayer: PLAYER::Init failed (model/body/ragdoll). SDL: %s\n", SDL_GetError());
		return hr;
	}

	m_aPlayers[index].model.SetScale(0.4f);
	m_aPlayers[index].model.SetAnimation(&animations[static_cast<int>(ANIMATION_TYPE::ANIMATION_IDLE)]);
	m_aPlayers[index].model.StartAnimation();

	m_aPlayers[index].tmAltShoot.LastTickCount += 2200;

	VECTOR2D *aVertices = new VECTOR2D[6];
	std::ifstream f("data/meshes/grenade.dat");
	if(!f)
		return E_FAIL;

	int iNumVerts;
	float x, y;

	f >> iNumVerts;
	for(int i=0; i<iNumVerts; i++)
	{
		f >> x >> y;
		aVertices[i] = VECTOR2D(x, y);
	}
	f.close();

	m_aPlayers[index].grenade_body = RIGIDBODY(aVertices,
		iNumVerts);
	m_aPlayers[index].grenade_body.fRestitution = 0.6f;
	delete [] aVertices;

	m_aFrags[index] = 0;
	m_aDeath[index] = 0;

	if(g_iNumPlayers>0)
	{
		if(!m_AIStates)
			m_AIStates = (AI_STATE_TYPE*)malloc(sizeof(AI_STATE_TYPE));
		else
			m_AIStates = (AI_STATE_TYPE*)realloc(m_AIStates, sizeof(AI_STATE_TYPE)*g_iNumPlayers);
	}

	g_iNumPlayers++;

	RespawnPlayer(&m_aPlayers[index]);

	return S_OK;
}
///////////////////////////////////////////////////////////////////////////////////////////////////
void RespawnPlayer(PLAYER* player)
{
	if(!g_iNumRespawns)
		return;

	if(player->ID>0)
		m_AIStates[player->ID-1] = AI_STATE_TYPE::PURSUIT;

	int index = (int)(RANDOM*g_iNumRespawns);

	
	player->body.Velocity = VECTOR2D();
	player->body.fAngVelocity = 0.0f;
	player->body.Force = VECTOR2D();
	player->body.fTorque = 0.0f;

	player->body.Move(-player->body.Pos+
		aRespawns[index]);
	player->body.Rotate(-player->body.fOrientation);

	player->ammo[static_cast<int>(WEAPON::RIFLE)] = player->c_NumRifleAmmo;
	player->ammo[static_cast<int>(WEAPON::GRENADE)] = 2;
	player->health = player->c_NumHealth;
	
	/*player->ammo[static_cast<int>(WEAPON::RIFLE)] = 0;
	player->ammo[static_cast<int>(WEAPON::GRENADE)] = 0;
	player->health = 10;
	*/

	player->bAlive = true;
	player->bActiveGrenade = false;

	//set the graph vertex index for this player
	VECTOR2D d;
	float min = _HUGE, dp;
	for(int i=0; i<g_iNumWayPoints; i++)
	{
		d = aWayPoints[i].vPos-player->body.Pos;
		dp = DotProduct(d, d);
		if(dp<min)
		{
			min = dp;
			index = i;
		}
	}

	player->nVertexIndex = index;
}
///////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT LoadGameData()
{
	g_vCenter = VECTOR2D(g_iScreenWidth>>1, g_iScreenHeight>>1);

	HRESULT hr;
	//==========load main player model===========//
	//m_aPlayers = new PLAYER[MAX_PLAYERS];
	
	hr = AddPlayer();
	if (FAILED(hr)) {
		std::fprintf(stderr, "LoadGameData: AddPlayer failed\n");
		return hr;
	}
	//=============end soldat model========//
	//==========load enviroment objs=======//
	float fPack_scale = 0.8f;

	hr = pack_ammo.Init(g_renderer, "data/sprites/ammo_pack.tga");
	if (FAILED(hr)) {
		std::fprintf(stderr, "LoadGameData: pack_ammo (%s) failed: %s\n", "data/sprites/ammo_pack.tga", SDL_GetError());
		return hr;
	}
	pack_ammo.SetScale(fPack_scale);

	hr = pack_grenade.Init(g_renderer, "data/sprites/grenade_pack.tga");
	if (FAILED(hr)) {
		std::fprintf(stderr, "LoadGameData: pack_grenade (%s) failed: %s\n", "data/sprites/grenade_pack.tga", SDL_GetError());
		return hr;
	}
	pack_grenade.SetScale(fPack_scale);

	hr = pack_health.Init(g_renderer, "data/sprites/health_pack.tga");
	if (FAILED(hr)) {
		std::fprintf(stderr, "LoadGameData: pack_health (%s) failed: %s\n", "data/sprites/health_pack.tga", SDL_GetError());
		return hr;
	}
	pack_health.SetScale(fPack_scale);

	g_pFireTexture = IMG_LoadTexture(g_renderer, "data/sprites/fire.tga");
	if (!g_pFireTexture) {
		std::fprintf(stderr, "LoadGameData: fire texture failed: %s\n", SDL_GetError());
		return E_FAIL;
	}
	
	g_pFire		= new PARTICLE();
	g_pSmoke	= new PARTICLE();
	g_pCustom	= new PARTICLE();
	//==========end enviroment objs=======//
	//===============load weapons=========//
	hr = rifle.Init(g_renderer, "data/sprites/rifle.tga");
	if (FAILED(hr)) {
		std::fprintf(stderr, "LoadGameData: rifle (%s) failed: %s\n", "data/sprites/rifle.tga", SDL_GetError());
		return hr;
	}
	rifle.SetXYPos(15, 25);
	rifle.SetRotation(PI/4.0f);
	
	hr = grenade.Init(g_renderer, "data/sprites/grenade.tga");
	if (FAILED(hr)) {
		std::fprintf(stderr, "LoadGameData: grenade (%s) failed: %s\n", "data/sprites/grenade.tga", SDL_GetError());
		return hr;
	}

	hr = fire.Init(g_renderer, "data/sprites/shoot_fire.tga");
	if (FAILED(hr)) {
		std::fprintf(stderr, "LoadGameData: fire (%s) failed: %s\n", "data/sprites/shoot_fire.tga", SDL_GetError());
		return hr;
	}
	//=================end weapons========//

	//============load UI elements========//
	hr = ui_health.Init(g_renderer, "data/sprites/health_UI.tga");
	if (FAILED(hr)) {
		std::fprintf(stderr, "LoadGameData: ui_health (%s) failed: %s\n", "data/sprites/health_UI.tga", SDL_GetError());
		return hr;
	}
	ui_health.SetXYPos(g_iScreenWidth>>1, 
		g_iScreenHeight-ui_health.iHeight+(ui_health.iHeight>>2));

	hr = ui_rifle.Init(g_renderer, "data/sprites/rifle_UI.tga");
	if (FAILED(hr)) {
		std::fprintf(stderr, "LoadGameData: ui_rifle (%s) failed: %s\n", "data/sprites/rifle_UI.tga", SDL_GetError());
		return hr;
	}
	ui_rifle.SetXYPos(ui_rifle.iWidth>>1,
		g_iScreenHeight-ui_rifle.iHeight+(ui_rifle.iHeight>>2));

	hr = ui_grenade.Init(g_renderer, "data/sprites/grenade_UI.tga");
	if (FAILED(hr)) {
		std::fprintf(stderr, "LoadGameData: ui_grenade (%s) failed: %s\n", "data/sprites/grenade_UI.tga", SDL_GetError());
		return hr;
	}
	ui_grenade.SetXYPos(g_iScreenWidth-ui_grenade.iWidth,
		g_iScreenHeight-ui_grenade.iHeight+(ui_grenade.iHeight>>2));
	
	hr = cur_ptr.Init(g_renderer, "data/sprites/cursor.tga");
	if (FAILED(hr)) {
		std::fprintf(stderr, "LoadGameData: cur_ptr (%s) failed: %s\n", "data/sprites/cursor.tga", SDL_GetError());
		return hr;
	}
	cur_ptr.SetScale(0.6f);
	//==============end UI elements=======//

	return S_OK;
}
///////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT LoadMap(const char* szFileName)
{
	//===============load materials==============//
	std::ifstream f("data/config/mat.cfg");
	if(!f)
		return E_FAIL;

	for (int i = 0; i < static_cast<int>(MATERIAL_TYPE::NUM_MATERIALS); i++) {
		f >> g_aMaterials[i].fWidth >> g_aMaterials[i].fHeight;
		if (g_szMaterialFile[i] == nullptr) {
			g_aMaterials[i].pTexture = nullptr;
			continue;
		}
		g_aMaterials[i].pTexture = IMG_LoadTexture(g_renderer, g_szMaterialFile[i]);
		if (!g_aMaterials[i].pTexture) {
			f.close();
			return E_FAIL;
		}
	}

	f.close();
	//================end materials==============//

	//===========read walls data============//
	f.open(szFileName);
	if(!f)
		return E_FAIL;

	LPVECTOR2D aVertices;
	int iNumVerts;

	int iNumWalls;
	f >> iNumWalls;
	g_iNumWalls = iNumWalls;
	aWalls = new RIGIDBODY[iNumWalls];
	g_vWallTex = new LPVECTOR2D[iNumWalls];
	float x, y;
	int matID;
	for(int i=0; i<iNumWalls; i++)
	{
		f >> iNumVerts;
		f >> matID;
		g_iNumWallVertices += iNumVerts;
		aVertices = new VECTOR2D[iNumVerts];
		g_vWallTex[i] = new VECTOR2D[iNumVerts];
		//read vertices
		for(int j=0; j<iNumVerts; j++)
		{
			f >> x >> y;
			f >> g_vWallTex[i][j].x >> g_vWallTex[i][j].y;
			aVertices[j].x = x;
			aVertices[j].y = y;
			g_vWallTex[i][j] = VECTOR2D(x/g_aMaterials[matID].fWidth,
				y/g_aMaterials[matID].fHeight);
		}
		aWalls[i] = RIGIDBODY(aVertices, iNumVerts, true);
		aWalls[i].fRestitution = 0.5f;
		aWalls[i].fFriction	   = 0.1f;
		aWalls[i].lMaterialID  = matID;
		delete [] aVertices;
	}
	
	//=============end walls data===========//
	//////////////////////////////////////////
	//============load map objects==========//
	int iNumBodies;
	f >> iNumBodies;
	g_iNumBodies = iNumBodies;
	aBodies = new RIGIDBODY[iNumBodies];
	g_vBodyTex = new LPVECTOR2D[iNumBodies];
	for(int i=0; i<iNumBodies; i++)
	{
		f >> iNumVerts;
		f >> matID;
		g_iNumBodyVertices += iNumVerts;
		aVertices = new VECTOR2D[iNumVerts];
		g_vBodyTex[i] = new VECTOR2D[iNumVerts];
		for(int j=0; j<iNumVerts; j++)
		{
			f >> x >> y;
			f >> g_vBodyTex[i][j].x >> g_vBodyTex[i][j].y;
			aVertices[j] = VECTOR2D(x, y);
		}
		aBodies[i] = RIGIDBODY(aVertices, iNumVerts);
		aBodies[i].fRestitution = 0.55f;
		aBodies[i].fFriction	= 0.03f;
		aBodies[i].lMaterialID	= matID;
		delete [] aVertices;
	}
	//============end map object============//	

	//===========load respawn points========//
	f >> g_iNumRespawns;
	aRespawns = (VECTOR2D*)malloc(sizeof(VECTOR2D)*g_iNumRespawns);


	for(int i=0; i<g_iNumRespawns; i++)
	{
		f >> aRespawns[i].x >> aRespawns[i].y;
	}

	//=============end respawn points=======//

	//=============read packplaces==========//
	f >> g_iNumPackPlaces;
	aPackPlaces = (VECTOR2D*)malloc(sizeof(VECTOR2D)*g_iNumPackPlaces);
	aPacks      = (PACK*)malloc(sizeof(PACK)*g_iNumPackPlaces);

	for(int i=0; i<g_iNumPackPlaces; i++)
	{
		f >> aPackPlaces[i].x >> aPackPlaces[i].y;

		aPacks[i].type = (PACK_TYPE)(int)(RANDOM*static_cast<int>(PACK_TYPE::NUM_PACKS));
		aPacks[i].bActive = true;
	}
	//===============end packplaces=========//

	//==============read waypoints==========//
	f >> g_iNumWayPoints;
	aWayPoints = (NODE*)malloc(sizeof(NODE)*g_iNumWayPoints);

	for(int i=0; i<g_iNumWayPoints; i++)
	{
		f >> aWayPoints[i].vPos.x >> aWayPoints[i].vPos.y;
		f >> aWayPoints[i].iNumEdges;
		aWayPoints[i].aEdges = (int*)malloc(sizeof(int)*
			aWayPoints[i].iNumEdges);

		for(int j=0; j<aWayPoints[i].iNumEdges; j++)
			f >> aWayPoints[i].aEdges[j];
	}
	//===============end waypoints==========//
	f.close();

	//assign packplace to waypoint
	for(int i=0; i<g_iNumPackPlaces; i++)
	{
		float min = _HUGE, dp;
		int	av = -1;

		for(int j=0; j<g_iNumWayPoints; j++)
		{
			VECTOR2D v = aPackPlaces[i]-
				aWayPoints[j].vPos;
			if((dp = DotProduct(v, v))<min)
			{
				min = dp;
				av = j;
			}
		}

		aPacks[i].nVertexIndex = av;
	}

	//find the shortest way  for each vertex//
	apPathParent = new int*[g_iNumWayPoints];
	apPathDistance = new float*[g_iNumWayPoints];

	bool *vis = new bool[g_iNumWayPoints];

	for(int s=0; s<g_iNumWayPoints; s++)
	{
		apPathParent[s] = new int[g_iNumWayPoints];
		apPathDistance[s] = new float[g_iNumWayPoints];
		
		for(int i=0; i<g_iNumWayPoints; i++)
		{
			apPathParent[s][i] = -1;
			apPathDistance[s][i] = 0x7fffffff;
			
			vis[i] = false;
		}

		apPathDistance[s][s] = 0.0f;

		int u, v, min;
		float l;

		for(int i=0; i<g_iNumWayPoints; i++)
		{
			min = 0x7fffffff; 
			for(int j=0; j<g_iNumWayPoints; j++)
				if(apPathDistance[s][j]<min && !vis[j])
				{
					u = j;
					min = apPathDistance[s][j];
				}
			vis[u] = true;

			for(int j=0; j<aWayPoints[u].iNumEdges; j++)
			{
				v = aWayPoints[u].aEdges[j];
				l = Length(aWayPoints[v].vPos-aWayPoints[u].vPos);
				
				if(apPathDistance[s][u]+l<apPathDistance[s][v])
				{
					apPathDistance[s][v] = apPathDistance[s][u]+l;
					apPathParent[s][v] = u;
				}
			}
		}
	}
	delete [] vis;

#ifdef DEBUG
	{
		std::ofstream dbg("debug.txt");
		for (int i = 0; i < g_iNumWayPoints; i++) {
			dbg << i << '\n';
			for (int j = 0; j < g_iNumWayPoints; j++) {
				dbg << apPathParent[i][j] << ' ';
			}
			dbg << "\n\n";
		}
	}
#endif

	return S_OK;
}
///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT InitGfx(SDL_Window* window, SDL_Renderer* renderer)
{
	if (!window || !renderer) {
		return E_FAIL;
	}
	g_window   = window;
	g_renderer = renderer;
	if (FAILED(platform::fonts_init())) {
		return E_FAIL;
	}
	return S_OK;
}
void Cleanup()
{
	for (int i = 0; i < MAX_MATERIALS; i++) {
		if (g_aMaterials[i].pTexture) {
			SDL_DestroyTexture(g_aMaterials[i].pTexture);
			g_aMaterials[i].pTexture = nullptr;
		}
	}
	if (g_pFireTexture) {
		SDL_DestroyTexture(g_pFireTexture);
		g_pFireTexture = nullptr;
	}
	if (g_pSmokeTexture) {
		SDL_DestroyTexture(g_pSmokeTexture);
		g_pSmokeTexture = nullptr;
	}
	platform::fonts_shutdown();
	// Renderer / window are owned by the SDL platform layer.
	g_renderer = nullptr;
	g_window   = nullptr;
}
////////////////////////////////////////////ARTIFICIAL INTELEGENCE//////////////////////////////////
void RunToWayPoint(bool *actions, int index, int WPIndex)
{
	VECTOR2D dst_vec = aWayPoints[WPIndex].vPos-
		aWayPoints[m_aPlayers[index].nVertexIndex].vPos;
						
	if(dst_vec.x>0.0f)
		actions[static_cast<int>(ACTION::MOVERIGHT)] = true;
	else if(dst_vec.x<0.0f)
		actions[static_cast<int>(ACTION::MOVELEFT)]  = true;

	float fTheta = atan2(-dst_vec.y, dst_vec.x);
	if(fTheta>Pi)
		fTheta = -fTheta;
	fTheta -= Pi/2.0f;

	if(fabs(fTheta)<Pi/4.0f && m_aPlayers[index].prev_state != STATE::FLY)
		actions[static_cast<int>(ACTION::JUMP)] = true;
}

void GetAIActions(int index, bool *actions)
{
	memset(actions, false, static_cast<size_t>(ACTION::NUMACTIONS));
	
	if(!g_iNumWayPoints)
		return;
	
	int src_vert = m_aPlayers[index].nVertexIndex;
	int dst_vert;
	int next_vert;

	bool bCanShoot = m_aPlayers[index].iViewSoldierID == 0;

	int ammo_level = m_aPlayers[index].ammo[static_cast<int>(WEAPON::RIFLE)]*
		100/PLAYER::c_NumRifleAmmo;
	int grenade_level = m_aPlayers[index].ammo[static_cast<int>(WEAPON::GRENADE)]*
		100/PLAYER::c_NumGrenades;
	int health_level = m_aPlayers[index].health*
		100/PLAYER::c_NumHealth;
	
	int o = 1-2*m_aPlayers[index].model.GetOrientation();
	VECTOR2D obj_vec = m_aPlayers[0].body.Pos-
		m_aPlayers[index].body.Pos-VECTOR2D(0.0f, 10.0f*o);

	VECTOR2D view_vec;
	
	switch(m_AIStates[index-1])
	{
	//attack the player_0
	case AI_STATE_TYPE::ATTACK:
		{
			view_vec = obj_vec;
			if((ammo_level<20 && grenade_level==0) || health_level<15)
				m_AIStates[index-1] = AI_STATE_TYPE::RUNAWAY;
			else
			{
				if(bCanShoot)
				{
					actions[static_cast<int>(ACTION::SHOOT)] = true;

					if(DotProduct(obj_vec, obj_vec)>SQ_NEAR_DISTANCE)
						if((int)(RANDOM*10) == 5 && grenade_level>0)
							actions[static_cast<int>(ACTION::ALTSHOOT)] = true;
				}
				else
					m_AIStates[index-1] = AI_STATE_TYPE::PURSUIT;
			}
		}break;
	//look for player_0
	case AI_STATE_TYPE::PURSUIT:
		{
			view_vec = obj_vec;
			dst_vert = m_aPlayers[0].nVertexIndex;
			
			if(apPathDistance[src_vert][dst_vert]>NEAR_DISTANCE)
			{
				if(ammo_level>=50 && health_level>=40)
				{
					next_vert = apPathParent[src_vert][dst_vert];
	
					while(next_vert != -1 && 
						apPathParent[src_vert][next_vert] != src_vert)
					{
						next_vert = apPathParent[src_vert][next_vert];
					}

					bool bCanReach = next_vert != -1;

					//can we reach the enemy?
					//yeah, run, Lola, run...
					if(bCanReach)
						RunToWayPoint(actions, index, next_vert);

					//do we see the enemy?
					//yeah, let's shoot him down :-)
					if(bCanShoot)
						actions[static_cast<int>(ACTION::SHOOT)] = true;
				}
				else
					m_AIStates[index-1] = AI_STATE_TYPE::SEARCH_PACK;

			}
			else
			{
				if((1-2*m_aPlayers[index].model.GetOrientation()) != Sign(obj_vec.x)) {
					if(obj_vec.x>0.0f)
						actions[static_cast<int>(ACTION::MOVERIGHT)] = true;
					else
						actions[static_cast<int>(ACTION::MOVELEFT)]  = true;
				}

				m_AIStates[index-1] = AI_STATE_TYPE::ATTACK;
			}
		}break;
	//runaway from player_0
	case AI_STATE_TYPE::RUNAWAY:
		{
			view_vec = VECTOR2D(10.0f, 0.0f);

			if(DotProduct(obj_vec, obj_vec)>SQ_FAR_DISTANCE)
				m_AIStates[index-1] = AI_STATE_TYPE::SEARCH_PACK;
			else
			{
				int dir = Sign(obj_vec.x);

				int u = m_aPlayers[index].nVertexIndex;
				int iNum = aWayPoints[u].iNumEdges;
				
				next_vert = -1;
				float min = _HUGE;

				for(int i=0; i<iNum; i++)
					if(apPathDistance[src_vert][aWayPoints[u].aEdges[i]]<min)
						if(Sign(aWayPoints[src_vert].vPos.x-
							aWayPoints[aWayPoints[u].aEdges[i]].vPos.x)==dir)
						{
							min = apPathDistance[src_vert][aWayPoints[u].aEdges[i]];
							next_vert = aWayPoints[u].aEdges[i];
						}
				
				if(next_vert != -1)
					RunToWayPoint(actions, index, next_vert);
				//we can't run away???
				//then we die in a fight!!!
				else
					m_AIStates[index-1] = AI_STATE_TYPE::ATTACK;
			}
		}break;
	//search for packs to 
	//increase its resoruces
	case AI_STATE_TYPE::SEARCH_PACK:
		{
			view_vec = VECTOR2D(10.0f, 0.0f);
			
			if(ammo_level<50 || health_level<40)
			{
				float min = _HUGE;
				next_vert = -1;

				for(int i=0; i<g_iNumPackPlaces; i++)
					if(aPacks[i].bActive && 
						apPathDistance[src_vert][aPacks[i].nVertexIndex]<min)
					{
						switch(aPacks[i].type)
						{
						case PACK_TYPE::PACK_AMMO:
							if(ammo_level>60)
								continue;
							break;
						case PACK_TYPE::PACK_GRENADE:
							if(grenade_level==100)
								continue;
							break;
						case PACK_TYPE::PACK_HEALTH:
							if(health_level>40)
								continue;
							break;
				case PACK_TYPE::NUM_PACKS: break;
				}
						
						min = apPathDistance[src_vert][aPacks[i].nVertexIndex];
						next_vert = aPacks[i].nVertexIndex;
					}
				
				if(next_vert != -1)
				{
					if(next_vert == src_vert)
					{
						VECTOR2D v = aPackPlaces[src_vert]-
							m_aPlayers[index].body.Pos;

						if(v.x>Epsilon)
							actions[static_cast<int>(ACTION::MOVERIGHT)] = true;
						else if(v.x<-Epsilon)
							actions[static_cast<int>(ACTION::MOVELEFT)]  = true;
					}
					else
					{
						next_vert = apPathParent[src_vert][next_vert];
					
						while(next_vert!=-1
							&& apPathParent[src_vert][next_vert]!=src_vert)
						{
							next_vert = apPathParent[src_vert][next_vert];
						}
						
						if(next_vert != -1)
							RunToWayPoint(actions, index, next_vert);
					}
				}
				else
					m_AIStates[index-1] = AI_STATE_TYPE::RUNAWAY;
			}
			else
				m_AIStates[index-1] = AI_STATE_TYPE::PURSUIT;
		}break;
	//help to other bots
	//attack player_0
	case AI_STATE_TYPE::HELP:
		{
		}break;
	}

	if((actions[static_cast<int>(ACTION::MOVELEFT)] || actions[static_cast<int>(ACTION::MOVERIGHT)]) && 
		m_aPlayers[index].bBoxCollision)
	{
		actions[static_cast<int>(ACTION::JUMP)] = true;
	}

	view_vec.x = fabs(view_vec.x);
	m_aPlayers[index].cursor = view_vec;
}
///////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT ShowSplash()
{
	if (!g_renderer) {
		return E_FAIL;
	}
	SPRITE splash;
	if (FAILED(splash.Init(g_renderer, "data/sprites/splash.jpg"))) {
		return S_OK;  // matches original: missing splash is non-fatal
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
int ScoreCmp(const void* arg_1, const void* arg_2)
{
	int a = *(int*)arg_1;
	int b = *(int*)arg_2;

	if(m_aDeath[a]<m_aDeath[b])
		return -1;
	else if(m_aDeath[a]>m_aDeath[b])
		return 1;
	else
	{
		if(m_aFrags[a]>m_aFrags[b])
			return -1;
		else if(m_aFrags[a]<m_aFrags[b])
			return 1;
		else 
			return 0;
	}
}

