

#include "gamecode.h"
#include <SDL.h>

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

ANIMATION animations[NUMANIMATIONS] = {
	{ 0,  0,  1.0f,     1, ANIMATION_SINGLE}, //ANIMATION_IDLE
	{ 6, 10, 0.01f,  500,  ANIMATION_LOOP},	  //ANIMATION_RUN
	{ 1,  5, 0.01f, 1000,  ANIMATION_RETURN}, //ANIMATION_JUMP
	{13, 15, 0.01f,  500,  ANIMATION_SINGLE}, //ANIMATION_WJUMP
	{11, 11,  1.0f,    1,  ANIMATION_SINGLE}, //ANIMATION_SLIDE
	{12, 12,  1.0f,    1,  ANIMATION_SINGLE}  //ANIMATION_FALL
};

//DX interfaces
LPDIRECT3D9 g_pD3D;
LPDIRECT3DDEVICE9 g_pDevice;
LPDIRECT3DVERTEXBUFFER9 g_pTerrainVB;
LPDIRECT3DVERTEXBUFFER9 g_pParticleVB;
LPDIRECT3DTEXTURE9 g_pFireTexture;
LPDIRECT3DTEXTURE9 g_pSmokeTexture;

LPD3DXLINE g_pLine;
LPD3DXFONT g_pSysFont;
LPD3DXFONT g_pUIFont;

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
POINT g_cursor; 
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
HWND g_hwndParent;

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
	
	for(int i=0; i<CONCRETE_NUMPARTICLES && g_CustomParticleCnt<MAX_PARTICLES; i++)
	{
		fVel = CONCRETEPARTICLE_VEL*RANDOM;
		
		p.Pos = vPoint;
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

	hr = model.LoadFromFile(pDevice, szModelFileName);
	if(FAILED(hr))
		return hr;

	FILE *f = fopen(szBodyFileName, "r");
	if(!f)
		return E_FAIL;

	int iNumVertices = 0;
	fscanf(f, "%i", &iNumVertices);
	VECTOR2D *aVertices = new VECTOR2D[iNumVertices];
	float x, y;
	for(int i=0; i<iNumVertices; i++)
	{
		fscanf(f, "%f%f", &x, &y);
		aVertices[i] = VECTOR2D(x, y);
	}
	fclose(f);
	body = RIGIDBODY(aVertices, iNumVertices);
	delete [] aVertices;

	//////load ragdoll mesh///////
	float minx = _HUGE, miny = _HUGE;
	float maxx = -_HUGE, maxy = -_HUGE;
	for(int i=0; i<NUM_PARTS; i++)
	{
		f = fopen(szMeshFile[i], "r");
		if(!f)
			return E_FAIL;

		fscanf(f, "%d", &iNumVertices);
		for(int j=0; j<iNumVertices; j++)
		{
			fscanf(f, "%f%f", &x, &y);
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
		fclose(f);
		
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
	ammo[RIFLE]		= c_NumRifleAmmo;
	ammo[GRENADE]	= c_NumGrenades;

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
						if(m_aPlayers[i].health<=0 && i!=ID)
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
		coll = VERT;
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
			coll = HORZ;
		
		B = T*BOUNCE;
		F = VECTOR2D(N.x*(1-FRICTION), N.y);
		
		body.Move(Axis);
		body.Velocity = F+B;
	}
	else
	{
		coll = NONE;
		
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
				coll = HORZ;
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
			coll = HORZ;

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
				coll = HORZ;
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
	int index = nVertexIndex, v;

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
	state = IDLE;
	
	if(actions[MOVELEFT])
	{
		if(coll != NONE)
		{
			body.Velocity.x -= RUN_ACC*dwTime/1000.0f;
			if(body.Velocity.x < -RUN_MAXVEL)
				body.Velocity.x = -RUN_MAXVEL;

			state = RUN;
		}
			else
				if(body.Velocity.x>-FLY_MAXVEL)
					body.Velocity.x -= FLY_ACC*dwTime/1000.0f;
	}
	if(actions[MOVERIGHT])
	{
		if(coll != NONE)
		{
			body.Velocity.x += RUN_ACC*dwTime/1000.0f;
			if(body.Velocity.x > RUN_MAXVEL)
				body.Velocity.x = RUN_MAXVEL;

			state = RUN;
		}
		else
			if(body.Velocity.x<FLY_MAXVEL)
				body.Velocity.x += FLY_ACC*dwTime/1000.0f;
	}
	if(actions[JUMP])
	{
		if(bCanJump && coll == HORZ && bJumpKeyOnce)
		{
			body.Velocity.y -= JUMP_ACC;
			//body.Move(Axis*20);
			body.Move(VECTOR2D(0.0f, -20.0f));
	
			model.SetAnimation(&animations[ANIMATION_JUMP]);
			model.StartAnimation();

			state = FLY;
			bJumpKeyOnce = false;
		}
	}
	else
		bJumpKeyOnce = true;
	if(actions[WJUMP])
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

			model.SetAnimation(&animations[ANIMATION_WJUMP]);
			model.StartAnimation();

			state = FLY;
			bWJumpKeyOnce = false;
		}
	}
	else
		bWJumpKeyOnce = true;
    	
	//addition states
	if(state == IDLE && Length(body.Velocity)>40.0f)
		state = SLIDE;
	if(state == SLIDE && !bCollided)
		if(body.Velocity.y<0)
			state = FLY;
		else
			state = FALL;
		
	//resolve soldat state
	switch(state)
	{
	case IDLE:
		{
			model.SetAnimation(&animations[ANIMATION_IDLE]);
			model.StartAnimation();
		}break;
	case SLIDE:
		{
			model.SetAnimation(&animations[ANIMATION_SLIDE]);
			model.StartAnimation();
		}break;
	case RUN:
		{
			if(prev_state != RUN)
			{
				model.SetAnimation(&animations[ANIMATION_RUN]);
				model.StartAnimation();
			}
		}break;
	case FALL:
		{
			model.SetAnimation(&animations[ANIMATION_FALL]);
			model.StartAnimation();
		}break;
	case FLY:
		{
			if(!model.IsAnimated())
			{
				model.SetAnimation(&animations[ANIMATION_FALL]);
				model.StartAnimation();
			}
		}
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
		if(i!=ID && m_aPlayers[i].bAlive)
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
		
	VECTOR2D ptr_pos = g_vCenter+Normalize(cursor)*100;
	//cur_ptr.SetXYPos(ptr_pos.x, ptr_pos.y);
	//0.2f = atan2(100, 20) :-), precalculations rulezz!!!
	//cur_ptr.SetRotation(fTheta-0.2f*o); 

	model.GetPart(BODY)->SetRotation(o*fTheta+
		PI*model.GetOrientation());
	//resolve mouse state: shooting and etc.
	if(actions[SHOOT] && tmShoot.Delta()>50 && ammo[RIFLE]>0)
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
		if(mint>0.0f)
			if(bStaticObject)
				AddCustomParticles(view_pt, minNt, COLOR(128, 128, 128, 128));
			else
			{
				if(bViewSoldier)
					AddCustomParticles(view_pt, minNt, COLOR(255, 255, 155, 0));
				else
					AddCustomParticles(view_pt, minNt, COLOR(255, 190, 150, 10));

			}
				
		tmShoot.Reset();

		ammo[RIFLE]--;
	}
	else
	{
		bShooting = false;
		tmShoot.Update();
	}

	if(actions[ALTSHOOT] && tmAltShoot.Delta()>2500 && ammo[GRENADE]>0)
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

		ammo[GRENADE]--;
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
				case PACK_AMMO:
					{
						if(ammo[RIFLE] == c_NumRifleAmmo)
							continue;
						
						ammo[RIFLE] += PACK_AMMO_SIZE;
						if(ammo[RIFLE]>c_NumRifleAmmo)
							ammo[RIFLE] = c_NumRifleAmmo;
					}break;
				case PACK_GRENADE:
					{
						if(ammo[GRENADE] == c_NumGrenades)
							continue;
						
						ammo[GRENADE] += PACK_GRENADE_SIZE;
						if(ammo[GRENADE]>c_NumGrenades)
							ammo[GRENADE] = c_NumGrenades;
					}break;
				case PACK_HEALTH:
					{
						if(health == c_NumHealth)
							continue;
						
						health += PACK_HEALTH_SIZE;
						if(health>c_NumHealth)
							health = c_NumHealth;
					}break;
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
	HRESULT hr = S_OK;

	bool actions[NUMACTIONS];
	memset(actions, 0, sizeof(actions));

	//resolve keyboard state
	if(keystate[SDL_SCANCODE_A])
	{
		m_aPlayers[0].dir = LEFT;
		actions[MOVELEFT] = true;
	}
	if(keystate[SDL_SCANCODE_D])
	{
		m_aPlayers[0].dir = RIGHT;
		actions[MOVERIGHT] = true;
	}
	if(keystate[SDL_SCANCODE_W])
	{
		actions[JUMP] = true;
	}
	if(keystate[SDL_SCANCODE_Q])
		actions[WJUMP] = true;

	//none-control keystate
	if(keystate[SDL_SCANCODE_INSERT])
	{
		if(g_bAddKeyOnce && g_iNumPlayers<MAX_PLAYERS)
		{
			hr = AddPlayer();
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
		actions[SHOOT] = true;
	if(buttons & SDL_BUTTON(SDL_BUTTON_RIGHT))
		actions[ALTSHOOT] = true;

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

	for(int i=0; i<g_iNumPackPlaces; i++)
		if(!aPacks[i].bActive)
			if(aPacks[i].tmReset.Delta()>60000)
			{
				aPacks[i].type = (PACK_TYPE)(int)(RANDOM*NUM_PACKS);
				aPacks[i].bActive = true;
			}
			else
				aPacks[i].tmReset.Update();

	return S_OK;
}
///////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT UpdateFrame()
{
	HRESULT hr;
	hr = g_pDevice->Clear(0, 0, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0L);
	if(FAILED(hr))
		return hr;
	

	if(FAILED(hr = g_pDevice->BeginScene()))
		return hr;
	
	//======draw game elements======//
	
	//main player
	
	VECTOR2D player_pos;
	VECTOR2D vOffset;
	
	if(m_aPlayers[0].bAlive)
	{
		vOffset = m_aPlayers[0].body.Pos-g_vCenter;
	
		player_pos = g_vCenter+VECTOR2D(0.0f, 5.0f);
		m_aPlayers[0].model.SetXYPos(player_pos.x, player_pos.y);
		if(m_aPlayers[0].bShooting)
			m_aPlayers[0].model.Draw(&rifle, &fire);
		else
			m_aPlayers[0].model.Draw(&rifle);
	}
	else
	{
		vOffset = m_aPlayers[0].ragdoll[HEAD].Pos-g_vCenter;
		//vOffset = m_aPlayers[0].body.Pos-g_vCenter;

		for(int i=0; i<NUM_PARTS; i++)
		{
			VECTOR2D Pos = m_aPlayers[0].ragdoll[i].Pos-vOffset;
			float fRotation = m_aPlayers[0].ragdoll[i].fOrientation;
			
			/*int iNumVertices = m_aPlayers[0].ragdoll[i].iNumVertices;
			D3DXVECTOR2 *aVertices = new D3DXVECTOR2[iNumVertices];
			for(int j=0; j<iNumVertices; j++)
			{
				aVertices[j].x = m_aPlayers[0].ragdoll[i].lpVertices[j].x-
					vOffset.x;
				aVertices[j].y = m_aPlayers[0].ragdoll[i].lpVertices[j].y-
					vOffset.y;
			}
			aVertices[iNumVertices].x = m_aPlayers[0].ragdoll[i].lpVertices[0].x-
					vOffset.x;
			aVertices[iNumVertices].y = m_aPlayers[0].ragdoll[i].lpVertices[0].y-
					vOffset.y;
			g_pLine->Draw(aVertices, iNumVertices+1, D3DCOLOR_XRGB(255, 255, 255));
			*/

			float fRx = m_aPlayers[0].model.GetPart(i)->GetRotationX();
			float fRy = m_aPlayers[0].model.GetPart(i)->GetRotationY();
			m_aPlayers[0].model.GetPart(i)->Draw(Pos.x, Pos.y, -fRotation, 
				12, 12, 0.4f);
		}
	}

	if(m_aPlayers[0].bActiveGrenade)
		grenade.Draw(m_aPlayers[0].grenade_body.Pos.x-vOffset.x, m_aPlayers[0].grenade_body.Pos.y-
		vOffset.y, -m_aPlayers[0].grenade_body.fOrientation, grenade.GetRotationX(), 
		grenade.GetRotationY());

	for(int i=1; i<g_iNumPlayers; i++)
	{
		if(m_aPlayers[i].bAlive)
		{
			player_pos = m_aPlayers[i].body.Pos+
				VECTOR2D(0.0f, 5.0f)-vOffset;
			m_aPlayers[i].model.SetXYPos(player_pos.x, player_pos.y);
			if(m_aPlayers[i].bShooting)
				m_aPlayers[i].model.Draw(&rifle, &fire);
			else
				m_aPlayers[i].model.Draw(&rifle);
		}
		else
		{
			for(int j=0; j<NUM_PARTS; j++)
			{
				VECTOR2D Pos = m_aPlayers[i].ragdoll[j].Pos-vOffset;
				float fRotation = m_aPlayers[i].ragdoll[j].fOrientation;
				float fRx = m_aPlayers[i].model.GetPart(j)->GetRotationX();
				float fRy = m_aPlayers[i].model.GetPart(j)->GetRotationY();

				m_aPlayers[i].model.GetPart(j)->Draw(Pos.x, Pos.y, 
					 -fRotation, 12, 12, 0.4f);
			}
		}
		if(m_aPlayers[i].bActiveGrenade)
			grenade.Draw(m_aPlayers[i].grenade_body.Pos.x-vOffset.x, m_aPlayers[i].grenade_body.Pos.y-
			vOffset.y, -m_aPlayers[i].grenade_body.fOrientation, grenade.GetRotationX(), 
			grenade.GetRotationY());
	}

	//draw packs
	for(int i=0; i<g_iNumPackPlaces; i++)
		if(aPacks[i].bActive)
		{
			VECTOR2D pos;
			VECTOR2D c;
			switch(aPacks[i].type)
			{
			case PACK_AMMO:
				{
					pos = aPackPlaces[i]-vOffset;
					c.x = pack_ammo.GetRotationX();
					c.y = pack_ammo.GetRotationY();
					pack_ammo.Draw(pos.x, pos.y, 0,
						c.x, c.y, 0.8f);
				}break;
			case PACK_GRENADE:
				{
					pos = aPackPlaces[i]-vOffset;
					c.x = pack_grenade.GetRotationX();
					c.y = pack_grenade.GetRotationY();
					pack_grenade.Draw(pos.x, pos.y, 0,
						c.x, c.y, 0.8f);
					
				}break;
			case PACK_HEALTH:
				{
					pos = aPackPlaces[i]-vOffset;
					c.x = pack_health.GetRotationX();
					c.y = pack_health.GetRotationY();
					pack_health.Draw(pos.x, pos.y, 0,
						c.x, c.y, 0.8f);
					
				}break;
			}
		}
	
	TEXTUREVERTEX *pTexVertex;
	//draw static and dunamic objects
	g_pDevice->SetFVF(D3DFVF_TEXTUREVERTEX);
	g_pDevice->SetStreamSource(0, g_pTerrainVB, 0, sizeof(TEXTUREVERTEX));
	
	hr = g_pTerrainVB->Lock(0, (g_iNumWallVertices+g_iNumBodyVertices)
		*sizeof(COLORVERTEX), (void**)&pTexVertex, 0);
	if(FAILED(hr))
		return hr;
	
	//fill buffer with static vertices
	VECTOR2D v;
	for(int i=0; i<g_iNumWalls; i++)
		for(int j=0; j<aWalls[i].iNumVertices; j++)
		{
			v  = aWalls[i].lpVertices[j]-vOffset; 
			pTexVertex->x = v.x;
			pTexVertex->y = v.y;
			pTexVertex->z = 0.0f;	
			pTexVertex->rhw = 0.0f;
			pTexVertex->u = g_vWallTex[i][j].x;
			pTexVertex->v = g_vWallTex[i][j].y;
			pTexVertex++;
		}
	
	//draw static objects
	int iOffset = 0;
	for(int i=0; i<g_iNumWalls; i++)
	{
		g_pDevice->SetTexture(0, g_aMaterials[aWalls[i].lMaterialID].pTexture);
		hr = g_pDevice->DrawPrimitive(D3DPT_TRIANGLEFAN, iOffset, aWalls[i].iNumVertices-2);
		if(FAILED(hr))
			return hr;
		g_pDevice->SetTexture(0, 0);

		iOffset += aWalls[i].iNumVertices;
	}
	
	//fill buffer with dunamic vertices
	for(int i=0; i<g_iNumBodies; i++)
		for(int j=0; j<aBodies[i].iNumVertices; j++)
		{
			v  = aBodies[i].lpVertices[j]-vOffset;
			pTexVertex->x = v.x;
			pTexVertex->y = v.y;
			pTexVertex->z = 0.0f;	
			pTexVertex->rhw = 0.0f;
			pTexVertex->u = g_vBodyTex[i][j].x;
			pTexVertex->v = g_vBodyTex[i][j].y;
			pTexVertex++;
		}

	//draw dunamic objects
	for(int i=0; i<g_iNumBodies; i++)
	{
		g_pDevice->SetTexture(0, g_aMaterials[aBodies[i].lMaterialID].pTexture);
		hr = g_pDevice->DrawPrimitive(D3DPT_TRIANGLEFAN, iOffset, aBodies[i].iNumVertices-2);
		if(FAILED(hr))
			return hr;
		g_pDevice->SetTexture(0, 0);

		iOffset += aBodies[i].iNumVertices;
	}

	g_pTerrainVB->Unlock();

	//draw particles
	g_pDevice->SetFVF(D3DFVF_COLORVERTEX);
	g_pDevice->SetStreamSource(0, g_pParticleVB, 0, sizeof(COLORVERTEX));

	//enable effets
	g_pDevice->SetRenderState(D3DRS_POINTSPRITEENABLE, TRUE);
	g_pDevice->SetRenderState(D3DRS_POINTSCALEENABLE, TRUE);
	g_pDevice->SetRenderState(D3DRS_POINTSIZE, FtoDW(20.0f));
	
	g_pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
	g_pDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
	g_pDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);

	DWORD c1 = D3DCOLOR_XRGB(128, 128, 128);
	DWORD c2 = D3DCOLOR_ARGB(0, 128, 128, 128);
	
	int iSizeToLock = (g_FireParticleCnt+
		g_SmokeParticleCnt+g_CustomParticleCnt)*
		sizeof(COLORVERTEX);
	
	COLORVERTEX *pColVertex;
	hr = g_pParticleVB->Lock(0, iSizeToLock, (void**)&pColVertex, 0);
	if(FAILED(hr))
		return hr;

	DWORD color;
	PARTICLE *ptr = g_pFire->next; //fire particles
	while(ptr)
	{
		pColVertex->x = ptr->Pos.x-vOffset.x;
		pColVertex->y = ptr->Pos.y-vOffset.y;
		pColVertex->z = 0.0f;
		pColVertex->rhw = 0.0f;
		pColVertex->color = ptr->color_current.GetColor();
		
		pColVertex++;
		ptr = ptr->next;
	}
	if(g_FireParticleCnt)
	{
		g_pDevice->SetTexture(0, g_pFireTexture);
		hr = g_pDevice->DrawPrimitive(D3DPT_POINTLIST, 0, g_FireParticleCnt);
		if(FAILED(hr))
			return hr;
		g_pDevice->SetTexture(0, 0);
	}
	
	ptr = g_pSmoke->next; //smoke particles
	while(ptr)
	{
		pColVertex->x = ptr->Pos.x-vOffset.x;
		pColVertex->y = ptr->Pos.y-vOffset.y;
		pColVertex->z = 0.0f;
		pColVertex->rhw = 0.0f;
		pColVertex->color = ptr->color_current.GetColor();

		pColVertex++;
		ptr = ptr->next;
	}
	if(g_SmokeParticleCnt)
	{
		g_pDevice->SetTexture(0, g_pSmokeTexture);
		hr = g_pDevice->DrawPrimitive(D3DPT_POINTLIST, 
			g_FireParticleCnt, g_SmokeParticleCnt);
		if(FAILED(hr))
			return hr;
		g_pDevice->SetTexture(0, 0);
	}

	g_pDevice->SetRenderState(D3DRS_POINTSPRITEENABLE, FALSE);
	g_pDevice->SetRenderState(D3DRS_POINTSCALEENABLE, FALSE);
	g_pDevice->SetRenderState(D3DRS_POINTSIZE, FtoDW(1.5f));

	g_pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);

	ptr = g_pCustom->next; //concrete particles
	while(ptr)
	{
		pColVertex->x = ptr->Pos.x-vOffset.x;
		pColVertex->y = ptr->Pos.y-vOffset.y;
		pColVertex->z = 0.0f;
		pColVertex->rhw = 0.0f;
		pColVertex->color = ptr->color_current.GetColor();

		pColVertex++;
		ptr = ptr->next;
	}
	if(g_CustomParticleCnt)
	{
		hr = g_pDevice->DrawPrimitive(D3DPT_POINTLIST,
			g_FireParticleCnt+g_SmokeParticleCnt, g_CustomParticleCnt);
		if(FAILED(hr))
			return hr;
	}
	
	g_pParticleVB->Unlock();

	
	//draw FPS
	if(tmFPS.Delta()>100)
	{
		nFPS = (int)((float)nFrameCount/tmFPS.Delta()*1000.f);
		nFrameCount = 0;
		tmFPS.Reset();
	}
	else
	{
		tmFPS.Update();
		nFrameCount++;
	}

	char *szFPS = new char[256];
	memset(szFPS, 0, 255);
	itoa(nFPS, szFPS, 10);
	char *szText = new char[256];		
	memset(szText, 0, 255);
	strcpy(szText, "FPS: ");
	strcat(szText, szFPS);
	RECT rc;
	SetRect(&rc, 0, 0, 0, 0);
	g_pSysFont->DrawTextA(szText, strlen(szText), &rc, DT_NOCLIP, D3DCOLOR_XRGB(100, 100, 255));
	
#ifdef DEBUG
	g_pLine->Begin();
	//foot platform
	D3DXVECTOR2 *aVertices = new D3DXVECTOR2[m_aPlayers[0].body.iNumVertices+1];
	for(int i=0; i<m_aPlayers[0].body.iNumVertices; i++)
	{
		aVertices[i].x = m_aPlayers[0].body.lpVertices[i].x-
				m_aPlayers[0].body.Pos.x+(g_iScreenWidth>>1);
		aVertices[i].y = m_aPlayers[0].body.lpVertices[i].y-
				m_aPlayers[0].body.Pos.y+(g_iScreenHeight>>1);
	}
	aVertices[m_aPlayers[0].body.iNumVertices].x = m_aPlayers[0].body.lpVertices[0].x-
				m_aPlayers[0].body.Pos.x+(g_iScreenWidth>>1);;
	aVertices[m_aPlayers[0].body.iNumVertices].y = m_aPlayers[0].body.lpVertices[0].y-
				m_aPlayers[0].body.Pos.y+(g_iScreenHeight>>1);;
	g_pLine->Draw(aVertices, m_aPlayers[0].body.iNumVertices+1, D3DCOLOR_XRGB(0, 0, 255));
	delete [] aVertices;
	g_pLine->End();

	//game statistic
	if(g_bCollided)
	{
		strcpy(szText, "Collided\0");
		SetRect(&rc, 0, 15, 0, 0);
		g_pSysFont->DrawTextA(szText, strlen(szText), &rc, DT_NOCLIP, D3DCOLOR_XRGB(255, 255, 0));
	}
	//draw soldat action
	memset(szText, 0, 255);
	switch(m_aPlayers[0].state)
	{
	case IDLE:
		strcpy(szText, "IDLE\0"); break;
	case RUN:
		strcpy(szText, "RUN\0"); break;
	case SLIDE:
		strcpy(szText, "SLIDE\0"); break;
	case FLY:
		strcpy(szText, "FLY\0"); break;
	case FALL:
		strcpy(szText, "FALL\0"); break;
	}
	SetRect(&rc, 0, 30, 0, 0);
	g_pSysFont->DrawTextA(szText, strlen(szText), &rc, DT_NOCLIP, D3DCOLOR_XRGB(100, 100, 255));
	
	//draw body vel
	memset(szText, 0, 256);
	gcvt(m_aPlayers[0].body.Velocity.y, 3, szText);
	SetRect(&rc, 0, 45, 0, 0);
	g_pSysFont->DrawTextA(szText, strlen(szText), &rc, DT_NOCLIP, D3DCOLOR_XRGB(100, 100, 255));

	memset(szText, 0, 256);
	itoa(m_aPlayers[0].nVertexIndex, szText, 10);
	SetRect(&rc, 0, 60, 0, 0);
	g_pSysFont->DrawTextA(szText, strlen(szText), &rc, DT_NOCLIP, D3DCOLOR_XRGB(100, 100, 255));

	for(int i=1; i<g_iNumPlayers; i++)
	{
		memset(szText, 0, 256);
		itoa(m_aPlayers[i].nVertexIndex, szText, 10);
		SetRect(&rc, 0, 75+i*15, 0, 0);
		g_pSysFont->DrawTextA(szText, strlen(szText), &rc, 
			DT_NOCLIP, D3DCOLOR_XRGB(255, 50, 50));
	}

	for(int i=0; i<g_iNumPlayers-1; i++)
	{
		memset(szText, 0, 256);
		switch(m_AIStates[i])
		{
		case ATTACK:
			strcpy(szText, "attack");
			break;
		case PURSUIT:
			strcpy(szText, "pursuit");
			break;
		case RUNAWAY:
			strcpy(szText, "runaway");
			break;
		case SEARCH_PACK:
			strcpy(szText, "search_pack");
			break;
		}

		SetRect(&rc, 25, 90+i*15, 0, 0);
		g_pSysFont->DrawTextA(szText, strlen(szText), &rc, 
			DT_NOCLIP, D3DCOLOR_XRGB(200, 200, 0));
	}
	
	//draw gun ray
	VECTOR2D vec_start(0.0f, -25.0f);
	float fTheta = m_aPlayers[0].model.GetPart(BODY)->GetRotation();
	fTheta *= (1-2*m_aPlayers[0].model.GetOrientation());
	Rotate(&vec_start, 0, fTheta);
	vec_start += g_vCenter;
	aVertices = new D3DXVECTOR2[2];
	aVertices[0] = D3DXVECTOR2(vec_start.x, vec_start.y);
	aVertices[1] = D3DXVECTOR2(g_vCenter.x+m_aPlayers[0].cursor.x,
		g_vCenter.y+m_aPlayers[0].cursor.y); 
	g_pLine->Draw(aVertices, 2, D3DCOLOR_XRGB(255, 0, 0));
#endif
	
	if(m_aPlayers[0].bAlive)
	{
		//draw UI elements
		ui_health.Draw();
		ui_rifle.Draw();
		ui_grenade.Draw();

		//health level
		itoa(m_aPlayers[0].health, szText, 10);
		strcat(szText, "%");
		SetRect(&rc, (g_iScreenWidth>>1)+(ui_health.iWidth>>1), 
			g_iScreenHeight-ui_health.iHeight+(ui_health.iWidth>>2), 0, 0);
		g_pUIFont->DrawTextA(szText, strlen(szText), 
			&rc, DT_NOCLIP, D3DCOLOR_XRGB(255, 255, 0));

		//rifle ammo level
		itoa(m_aPlayers[0].ammo[RIFLE], szText, 10);
		SetRect(&rc, (ui_rifle.iWidth), g_iScreenHeight-
			ui_rifle.iHeight+(ui_rifle.iHeight>>2), 0, 0);
		g_pUIFont->DrawTextA(szText, strlen(szText),
			&rc, DT_NOCLIP, D3DCOLOR_XRGB(255, 255, 0));

		//grenade ammo level
		itoa(m_aPlayers[0].ammo[GRENADE], szText, 10);
		SetRect(&rc, g_iScreenWidth-(ui_grenade.iWidth>>1),
			g_iScreenHeight-ui_grenade.iHeight+(ui_grenade.iHeight>>2), 
			0, 0);
		g_pUIFont->DrawTextA(szText, strlen(szText),
			&rc, DT_NOCLIP, D3DCOLOR_XRGB(255, 255, 0));

		//draw cursor
		VECTOR2D vec = Normalize(m_aPlayers[0].cursor)*100;
		float fTheta = atan2(vec.y, vec.x);
		vec += g_vCenter;
		cur_ptr.Draw(vec.x, vec.y, -fTheta, 
			cur_ptr.GetRotationX(),
			cur_ptr.GetRotationY(), 0.6f);
	}

	if(g_bShowStat)
	{
		g_pParticleVB->Lock(0, 4*sizeof(COLORVERTEX), (void**)&pColVertex, 0);
		
		//left, top
		pColVertex->x = g_vCenter.x*0.5f;
		pColVertex->y = g_vCenter.y*0.5f;
		pColVertex->z = 0.0f;
		pColVertex->rhw = 0.0f;
		pColVertex->color = D3DCOLOR_ARGB(128, 128, 128, 128);
		pColVertex++;

		//right, top
		pColVertex->x = g_vCenter.x+g_vCenter.x*0.5f;
		pColVertex->y = g_vCenter.y*0.5f;
		pColVertex->z = 0.0f;
		pColVertex->rhw = 0.0f;
		pColVertex->color = D3DCOLOR_ARGB(128, 160, 160, 160);
		pColVertex++;


		//right, bottom
		pColVertex->x = g_vCenter.x+g_vCenter.x*0.5f;
		pColVertex->y = g_vCenter.y*0.5f+20*(g_iNumPlayers+2);
		pColVertex->z = 0.0f;
		pColVertex->rhw = 0.0f;
		pColVertex->color = D3DCOLOR_ARGB(128, 190, 190, 190);
		pColVertex++;

		//left, bottom
		pColVertex->x = g_vCenter.x*0.5f;
		pColVertex->y = g_vCenter.y*0.5f+20*(g_iNumPlayers+2);
		pColVertex->z = 0.0f;
		pColVertex->rhw = 0.0f;
		pColVertex->color = D3DCOLOR_ARGB(128, 160, 160, 160);

		g_pParticleVB->Unlock();

		g_pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
		g_pDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCCOLOR);
		g_pDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_DESTCOLOR);

		g_pDevice->DrawPrimitive(D3DPT_TRIANGLEFAN, 0, 2);

		g_pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);

		SetRect(&rc, (g_iScreenWidth>>2)+10, (g_iScreenHeight>>2)+10, 0, 0);
		g_pUIFont->DrawTextA("Players", 7, &rc, DT_NOCLIP, D3DCOLOR_ARGB(200, 255, 10, 10));

		SetRect(&rc, (g_iScreenWidth>>1)-20, (g_iScreenHeight>>2)+10, 0, 0);
		g_pUIFont->DrawTextA("Frags", 5, &rc, DT_NOCLIP, D3DCOLOR_ARGB(200, 255, 10, 10));

		SetRect(&rc, (g_iScreenWidth>>1)+(g_iScreenWidth>>2)-60, (g_iScreenHeight>>2)+10, 0, 0);
		g_pUIFont->DrawTextA("Deaths", 6, &rc, DT_NOCLIP, D3DCOLOR_ARGB(200, 255, 10, 10));

		int *ind = new int[g_iNumPlayers];
		for(int i=0; i<g_iNumPlayers; i++)
			ind[i] = i;
		qsort(ind, g_iNumPlayers, sizeof(int), &ScoreCmp);
		
		char szVal[256];
		DWORD color = D3DCOLOR_ARGB(200, 10, 10, 255);
		for(int i=0; i<g_iNumPlayers; i++)
		{
			//player name
			strset(szText, 0);
			strcpy(szText, "player_");
			itoa(ind[i]+1, szVal, 10);
			strcat(szText, szVal);
			SetRect(&rc, (g_iScreenWidth>>2)+10, (g_iScreenHeight>>2)+20*(i+2), 0, 0);
			g_pUIFont->DrawTextA(szText, strlen(szText), &rc, DT_NOCLIP, color);

			strset(szText, 0);
			itoa(m_aFrags[ind[i]], szText, 10);
			SetRect(&rc, (g_iScreenWidth>>1)-20, (g_iScreenHeight>>2)+20*(i+2), 0, 0);
			g_pUIFont->DrawTextA(szText, strlen(szText), &rc, DT_NOCLIP, color);

			strset(szText, 0);
			itoa(m_aDeath[ind[i]], szText, 10);
			SetRect(&rc, (g_iScreenWidth>>1)+(g_iScreenWidth>>2)-60, (g_iScreenHeight>>2)+20*(i+2), 0, 0);
			g_pUIFont->DrawTextA(szText, strlen(szText), &rc, DT_NOCLIP, color);
		}
	}
	//===========end drawing=======//

	g_pDevice->EndScene();
	return g_pDevice->Present( NULL, NULL, NULL, NULL );
}
///////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT AddPlayer()
{
	int index = g_iNumPlayers;
	m_aPlayers[index].ID = index;

	HRESULT hr;
	hr = m_aPlayers[index].Init(g_pDevice, "data/models/soldat.m2d",
					 "data/meshes/body.dat", "data/meshes/ragdoll/");
    if(FAILED(hr))
		return hr;

	m_aPlayers[index].model.SetScale(0.4f);
	m_aPlayers[index].model.SetAnimation(&animations[ANIMATION_IDLE]);
	m_aPlayers[index].model.StartAnimation();
	
	m_aPlayers[index].tmAltShoot.LastTickCount += 2200;

	VECTOR2D *aVertices = new VECTOR2D[6];
	FILE *f = fopen("data/meshes/grenade.dat", "r");
	if(!f)
		return E_FAIL;
	
	int iNumVerts;
	float x, y;

	fscanf(f, "%i", &iNumVerts);
	for(int i=0; i<iNumVerts; i++)
	{
		fscanf(f, "%f%f", &x, &y);
		aVertices[i] = VECTOR2D(x, y);
	}
	fclose(f);
	
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
		m_AIStates[player->ID-1] = PURSUIT;

	int index = (int)(RANDOM*g_iNumRespawns);

	
	player->body.Velocity = VECTOR2D();
	player->body.fAngVelocity = 0.0f;
	player->body.Force = VECTOR2D();
	player->body.fTorque = 0.0f;

	player->body.Move(-player->body.Pos+
		aRespawns[index]);
	player->body.Rotate(-player->body.fOrientation);

	player->ammo[RIFLE] = player->c_NumRifleAmmo;
	player->ammo[GRENADE] = 2;
	player->health = player->c_NumHealth;
	
	/*player->ammo[RIFLE] = 0;
	player->ammo[GRENADE] = 0;
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

	FILE *f;
	VECTOR2D *aVertices;
	int iNumVerts;
	float x, y;
	//==========load main player model===========//
	//m_aPlayers = new PLAYER[MAX_PLAYERS];
	
	hr = AddPlayer();
	if(FAILED(hr))
		return hr;
	//=============end soldat model========//
	//==========load enviroment objs=======//
	float fPack_scale = 0.8f;

	hr = pack_ammo.Init(g_pDevice, "data/sprites/ammo_pack.tga");
	if(FAILED(hr))
		return hr;
	pack_ammo.SetScale(fPack_scale);

	hr = pack_grenade.Init(g_pDevice, "data/sprites/grenade_pack.tga");
	if(FAILED(hr))
		return hr;
	pack_grenade.SetScale(fPack_scale);

	hr = pack_health.Init(g_pDevice, "data/sprites/health_pack.tga");
	if(FAILED(hr))
		return hr;
	pack_health.SetScale(fPack_scale);

	hr = D3DXCreateTextureFromFile(g_pDevice, 
		"data/sprites/fire.tga", &g_pFireTexture);
	if(FAILED(hr))
		return hr;
	
	g_pFire		= new PARTICLE();
	g_pSmoke	= new PARTICLE();
	g_pCustom	= new PARTICLE();
	//==========end enviroment objs=======//
	//===============load weapons=========//
	hr = rifle.Init(g_pDevice, "data/sprites/rifle.tga");
	if(FAILED(hr))
		return hr;
	rifle.SetXYPos(15, 25);
	rifle.SetRotation(PI/4.0f);
	
	hr = grenade.Init(g_pDevice, "data/sprites/grenade.tga");
	if(FAILED(hr))
		return hr;

	hr = fire.Init(g_pDevice, "data/sprites/shoot_fire.tga");
	if(FAILED(hr))
		return hr;
	//=================end weapons========//

	//============load UI elements========//
	hr = ui_health.Init(g_pDevice, "data/sprites/health_UI.tga");
	if(FAILED(hr))
		return hr;
	ui_health.SetXYPos(g_iScreenWidth>>1, 
		g_iScreenHeight-ui_health.iHeight+(ui_health.iHeight>>2));

	hr = ui_rifle.Init(g_pDevice, "data/sprites/rifle_UI.tga");
	if(FAILED(hr))
		return hr;
	ui_rifle.SetXYPos(ui_rifle.iWidth>>1,
		g_iScreenHeight-ui_rifle.iHeight+(ui_rifle.iHeight>>2));

	hr = ui_grenade.Init(g_pDevice, "data/sprites/grenade_UI.tga");
	if(FAILED(hr))
		return hr;
	ui_grenade.SetXYPos(g_iScreenWidth-ui_grenade.iWidth,
		g_iScreenHeight-ui_grenade.iHeight+(ui_grenade.iHeight>>2));
	
	hr = cur_ptr.Init(g_pDevice, "data/sprites/cursor.tga");
	if(FAILED(hr))
		return hr;
	cur_ptr.SetScale(0.6f);
	//==============end UI elements=======//

	return S_OK;
}
///////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT LoadMap(const char* szFileName)
{
		HRESULT hr;
	
	//===============load materials==============//
	FILE *f = fopen("data/config/mat.cfg", "r");
	if(!f)
		return E_FAIL;

	for(int i=0; i<NUM_MATERIALS; i++)
	{
		fscanf(f, "%f%f", &g_aMaterials[i].fWidth, &g_aMaterials[i].fHeight);
		hr = D3DXCreateTextureFromFile(g_pDevice, g_szMaterialFile[i],
			&g_aMaterials[i].pTexture);
		if(FAILED(hr))
			return hr;
	}

	fclose(f);
	//================end materials==============//

	//===========read walls data============//
	f = fopen(szFileName, "r");
	if(!f)
		return E_FAIL;

	LPVECTOR2D aVertices;
	int iNumVerts;

	int iNumWalls;
	fscanf(f, "%i", &iNumWalls);
	g_iNumWalls = iNumWalls;
	aWalls = new RIGIDBODY[iNumWalls];
	g_vWallTex = new LPVECTOR2D[iNumWalls];
	float x, y;
	int matID;
	for(int i=0; i<iNumWalls; i++)
	{
		fscanf(f, "%d", &iNumVerts);
		fscanf(f, "%d", &matID);
		g_iNumWallVertices += iNumVerts;
		aVertices = new VECTOR2D[iNumVerts];
		g_vWallTex[i] = new VECTOR2D[iNumVerts];
		//read vertices
		for(int j=0; j<iNumVerts; j++)
		{
			fscanf(f, "%f%f", &x, &y);
			fscanf(f, "%f%f", &g_vWallTex[i][j].x, 
				&g_vWallTex[i][j].y);
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
	fscanf(f, "%i", &iNumBodies);
	g_iNumBodies = iNumBodies;
	aBodies = new RIGIDBODY[iNumBodies];
	g_vBodyTex = new LPVECTOR2D[iNumBodies];
	for(int i=0; i<iNumBodies; i++)
	{
		fscanf(f, "%d", &iNumVerts);
		fscanf(f, "%d", &matID);
		g_iNumBodyVertices += iNumVerts;
		aVertices = new VECTOR2D[iNumVerts];
		g_vBodyTex[i] = new VECTOR2D[iNumVerts];
		for(int j=0; j<iNumVerts; j++)
		{
			fscanf(f, "%f%f", &x, &y);
			fscanf(f, "%f%f", &g_vBodyTex[i][j].x,
				&g_vBodyTex[i][j].y);
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
	fscanf(f, "%d", &g_iNumRespawns);
	aRespawns = (VECTOR2D*)malloc(sizeof(VECTOR2D)*g_iNumRespawns);


	for(int i=0; i<g_iNumRespawns; i++)
	{
		fscanf(f, "%f%f", &aRespawns[i].x, 
			&aRespawns[i].y);
	}

	//=============end respawn points=======//

	//=============read packplaces==========//
	fscanf(f, "%d", &g_iNumPackPlaces);
	aPackPlaces = (VECTOR2D*)malloc(sizeof(VECTOR2D)*g_iNumPackPlaces);
	aPacks      = (PACK*)malloc(sizeof(PACK)*g_iNumPackPlaces);

	for(int i=0; i<g_iNumPackPlaces; i++)
	{
		fscanf(f, "%f%f", &aPackPlaces[i].x, 
			&aPackPlaces[i].y);

		aPacks[i].type = (PACK_TYPE)(int)(RANDOM*NUM_PACKS);
		aPacks[i].bActive = true;
	}
	//===============end packplaces=========//

	//==============read waypoints==========//
	fscanf(f, "%d", &g_iNumWayPoints);
	aWayPoints = (NODE*)malloc(sizeof(NODE)*g_iNumWayPoints);

	for(int i=0; i<g_iNumWayPoints; i++)
	{
		fscanf(f, "%f%f",
			&aWayPoints[i].vPos.x, &aWayPoints[i].vPos.y);
		fscanf(f, "%d", &aWayPoints[i].iNumEdges);
		aWayPoints[i].aEdges = (int*)malloc(sizeof(int)*
			aWayPoints[i].iNumEdges);

		for(int j=0; j<aWayPoints[i].iNumEdges; j++)
			fscanf(f, "%d", &aWayPoints[i].aEdges[j]);
	}
	//===============end waypoints==========//
	fclose(f);

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
	f = fopen("debug.txt", "w");

	for(int i=0; i<g_iNumWayPoints; i++)
	{
		fprintf(f, "%d\n", i);
		for(int j=0; j<g_iNumWayPoints; j++)
		{
			fprintf(f, "%d ", apPathParent[i][j]);
		}
		fprintf(f, "\n\n");
	}

	fclose(f);
#endif

	return S_OK;
}
///////////////////////////////////////////////////////////////////////////////////////////////////
void Cleanup()
{
	// Input devices owned by the SDL platform layer — nothing to release here.

	if(g_pSysFont)
		g_pSysFont->Release();

	if(g_pUIFont)
		g_pUIFont->Release();

	if(g_pFireTexture)
		g_pFireTexture->Release();

	if(g_pSmokeTexture)
		g_pSmokeTexture->Release();
	
	if(g_pTerrainVB)
		g_pTerrainVB->Release();
	
	if(g_pParticleVB)
		g_pParticleVB->Release();
	
	if(g_pDevice)
        g_pDevice->Release();

    if(g_pD3D)
		g_pD3D->Release();
}
////////////////////////////////////////////ARTIFICIAL INTELEGENCE//////////////////////////////////
void RunToWayPoint(bool *actions, int index, int WPIndex)
{
	VECTOR2D dst_vec = aWayPoints[WPIndex].vPos-
		aWayPoints[m_aPlayers[index].nVertexIndex].vPos;
						
	if(dst_vec.x>0.0f)
		actions[MOVERIGHT] = true;
	else if(dst_vec.x<0.0f)
		actions[MOVELEFT]  = true;

	float fTheta = atan2(-dst_vec.y, dst_vec.x);
	if(fTheta>Pi)
		fTheta = -fTheta;
	fTheta -= Pi/2.0f;

	if(fabs(fTheta)<Pi/4.0f && m_aPlayers[index].prev_state != FLY)
		actions[JUMP] = true;
}

void GetAIActions(int index, bool *actions)
{
	memset(actions, false, NUMACTIONS);
	
	if(!g_iNumWayPoints)
		return;
	
	int src_vert = m_aPlayers[index].nVertexIndex;
	int dst_vert;
	int next_vert;

	bool bCanShoot = m_aPlayers[index].iViewSoldierID == 0;

	int ammo_level = m_aPlayers[index].ammo[RIFLE]*
		100/PLAYER::c_NumRifleAmmo;
	int grenade_level = m_aPlayers[index].ammo[GRENADE]*
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
	case ATTACK:
		{
			view_vec = obj_vec;
			if((ammo_level<20 && grenade_level==0) || health_level<15)
				m_AIStates[index-1] = RUNAWAY;
			else
			{
				if(bCanShoot)
				{
					actions[SHOOT] = true;

					if(DotProduct(obj_vec, obj_vec)>SQ_NEAR_DISTANCE)
						if((int)(RANDOM*10) == 5 && grenade_level>0)
							actions[ALTSHOOT] = true;
				}
				else
					m_AIStates[index-1] = PURSUIT;
			}
		}break;
	//look for player_0
	case PURSUIT:
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
						actions[SHOOT] = true;
				}
				else
					m_AIStates[index-1] = SEARCH_PACK;

			}
			else
			{
				if((1-2*m_aPlayers[index].model.GetOrientation()) != Sign(obj_vec.x))
					if(obj_vec.x>0.0f)
						actions[MOVERIGHT] = true;
					else
						actions[MOVELEFT]  = true;

				m_AIStates[index-1] = ATTACK;
			}
		}break;
	//runaway from player_0
	case RUNAWAY:
		{
			view_vec = VECTOR2D(10.0f, 0.0f);

			if(DotProduct(obj_vec, obj_vec)>SQ_FAR_DISTANCE)
				m_AIStates[index-1] = SEARCH_PACK;
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
					m_AIStates[index-1] = ATTACK;
			}
		}break;
	//search for packs to 
	//increase its resoruces
	case SEARCH_PACK:
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
						case PACK_AMMO:
							if(ammo_level>60)
								continue;
							break;
						case PACK_GRENADE:
							if(grenade_level==100)
								continue;
							break;
						case PACK_HEALTH:
							if(health_level>40)
								continue;
							break;
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
							actions[MOVERIGHT] = true;
						else if(v.x<-Epsilon)
							actions[MOVELEFT]  = true;
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
					m_AIStates[index-1] = RUNAWAY;
			}
			else
				m_AIStates[index-1] = PURSUIT;
		}break;
	//help to other bots
	//attack player_0
	case HELP:
		{
		}break;
	}

	if((actions[MOVELEFT] || actions[MOVERIGHT]) && 
		m_aPlayers[index].bBoxCollision)
	{
		actions[JUMP] = true;
	}

	view_vec.x = fabs(view_vec.x);
	m_aPlayers[index].cursor = view_vec;
}
///////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT InitD3D(HWND hwndParent)
{
	g_hwndParent = hwndParent;
	
	g_pD3D = 0;
	g_pDevice = 0;
	g_pLine = 0;
	g_pSysFont = 0;
	g_pUIFont = 0;
	g_pTerrainVB = 0;
	g_pParticleVB = 0;
	
	g_pD3D = Direct3DCreate9(D3D_SDK_VERSION);
	if(!g_pD3D)
		return E_FAIL;
	
	HRESULT hr;
	D3DPRESENT_PARAMETERS d3dpp;
	memset(&d3dpp, 0, sizeof(d3dpp));
	d3dpp.Windowed = (BOOL)!g_bFullScreen;
	d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	d3dpp.EnableAutoDepthStencil = TRUE;
	d3dpp.AutoDepthStencilFormat = D3DFMT_D16;
	d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
	d3dpp.BackBufferCount = 1;
	d3dpp.MultiSampleType = (D3DMULTISAMPLE_TYPE)g_iAALevel;
		
	if(g_bFullScreen)
	{
		d3dpp.BackBufferWidth = g_iScreenWidth;
		d3dpp.BackBufferHeight = g_iScreenHeight;
		d3dpp.FullScreen_RefreshRateInHz = g_iRefreshRate;
		for(int fmt = D3DFMT_A4R4G4B4; fmt>=D3DFMT_A8R8G8B8; fmt--)
			if(SUCCEEDED(g_pD3D->CheckDeviceType(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL,
									(D3DFORMAT)fmt, (D3DFORMAT)fmt, FALSE)))
				d3dpp.BackBufferFormat = (D3DFORMAT)fmt;
		if(d3dpp.BackBufferFormat == D3DFMT_UNKNOWN)
			return E_FAIL;
	}
	else
	{
		D3DDISPLAYMODE d3ddm;
		hr = g_pD3D->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &d3ddm);
		if(FAILED(hr))
			return hr;
		d3dpp.BackBufferFormat = d3ddm.Format;
	}

	hr = g_pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hwndParent,
							  D3DCREATE_SOFTWARE_VERTEXPROCESSING, 
							  &d3dpp, &g_pDevice);
	if(FAILED(hr))
		return hr;
	
	//create vertex buffers
	hr = g_pDevice->CreateVertexBuffer(BUFFERSIZE*sizeof(TEXTUREVERTEX), D3DUSAGE_WRITEONLY, 
		D3DFVF_COLORVERTEX, D3DPOOL_DEFAULT, &g_pTerrainVB, 0);
	if(FAILED(hr))
		return hr;

	hr = g_pDevice->CreateVertexBuffer(3*MAX_PARTICLES*sizeof(COLORVERTEX), D3DUSAGE_WRITEONLY,
		D3DFVF_COLORVERTEX, D3DPOOL_DEFAULT, &g_pParticleVB, 0);
	if(FAILED(hr))
		return hr;
	
	//create helper objects
	hr = D3DXCreateLine(g_pDevice, &g_pLine);
	if(FAILED(hr))
		return hr;

	//create fonts
	LOGFONT lf;
	memset(&lf, 0, sizeof(lf));
	lf.lfWidth = 8;
	lf.lfHeight = 12;
	lf.lfCharSet = RUSSIAN_CHARSET;
	lf.lfQuality = NONANTIALIASED_QUALITY;
	strcpy(lf.lfFaceName, "Arial");
    	
	hr = D3DXCreateFontIndirect(g_pDevice, &lf, &g_pSysFont);
	if(FAILED(hr))
		return hr;

	memset(&lf, 0, sizeof(lf));
	lf.lfWidth = 8;
	lf.lfHeight = 8;
	lf.lfWeight = 700;
	lf.lfCharSet = RUSSIAN_CHARSET;
	lf.lfQuality = ANTIALIASED_QUALITY;
	strcpy(lf.lfFaceName, "Lucida console");

	hr = D3DXCreateFontIndirect(g_pDevice, &lf, &g_pUIFont);
	if(FAILED(hr))
		return hr;

	//Disable lighting
	g_pDevice->SetRenderState(D3DRS_LIGHTING, FALSE);
    //Disable Zbuffer
	g_pDevice->SetRenderState(D3DRS_ZENABLE, FALSE);
	g_pDevice->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
	//Disable culling
	g_pDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);

	return S_OK;
}
///////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT RestoreD3D()
{
	HRESULT hr;
	
	while((hr = g_pDevice->TestCooperativeLevel()) == 
		D3DERR_DEVICELOST)
	{
	}

	/*D3DPRESENT_PARAMETERS d3dpp;
	memset(&d3dpp, 0, sizeof(d3dpp));
	d3dpp.Windowed = (BOOL)g_bWindowed;
	d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	d3dpp.EnableAutoDepthStencil = TRUE;
	d3dpp.AutoDepthStencilFormat = D3DFMT_D16;
	d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
	d3dpp.BackBufferCount = 1;
	d3dpp.hDeviceWindow = g_hwndParent;
	
	if(!g_bWindowed)
	{
		d3dpp.BackBufferWidth = g_iScreenWidth;
		d3dpp.BackBufferHeight = g_iScreenHeight;
		for(int fmt = D3DFMT_A4R4G4B4; fmt>=D3DFMT_A8R8G8B8; fmt--)
			if(SUCCEEDED(g_pD3D->CheckDeviceType(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL,
									(D3DFORMAT)fmt, (D3DFORMAT)fmt, FALSE)))
				d3dpp.BackBufferFormat = (D3DFORMAT)fmt;
		if(d3dpp.BackBufferFormat == D3DFMT_UNKNOWN)
			return E_FAIL;
	}
	else
	{
		D3DDISPLAYMODE d3ddm;
		hr = g_pD3D->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &d3ddm);
		if(FAILED(hr))
			return hr;
		d3dpp.BackBufferFormat = d3ddm.Format;
	}*/

	return InitD3D(g_hwndParent);	
}
///////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT ShowSplash()
{
	const float splash_width = 703.0f;
	const float splash_height = 251.0f;
	
	SPRITE splash;
	
	if(FAILED(splash.Init(g_pDevice, "data/sprites/splash.jpg")))
		return S_OK;

	HRESULT hr = g_pDevice->BeginScene();
	if(FAILED(hr))
		return hr;
	hr = g_pDevice->Clear(0, 0, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0L);

	VECTOR2D pos(g_iScreenWidth>>1, g_iScreenHeight>>1);
	float fScale = (float)g_iScreenWidth/1024;

	splash.SetXYPos(pos.x, pos.y);
	splash.SetScale(fScale);
	splash.Draw();
	g_pDevice->EndScene();
	g_pDevice->Present(0, 0, 0, 0);

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
