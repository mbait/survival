#include "players.h"

#include <SDL.h>

#include <cmath>
#include <cstdio>
#include <fstream>
#include <vector>

#include "game/ai.h"        // m_AIStates, RunToWayPoint, GetAIActions
#include "game/effects.h"   // AddCustomParticles

// ---- player-state globals (definitions own here) ------------------
PLAYER m_aPlayers[MAX_PLAYERS];
int    g_iNumPlayers = 0;
int    m_aFrags[MAX_PLAYERS];
int    m_aDeath[MAX_PLAYERS];

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
	std::vector<VECTOR2D> aVertices(iNumVertices);
	float x, y;
	for (int i = 0; i < iNumVertices; i++) {
		f >> x >> y;
		aVertices[i] = VECTOR2D(x, y);
	}
	f.close();
	body = RIGIDBODY(aVertices.data(), iNumVertices);

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
		aVertices.assign({
			VECTOR2D(),
			VECTOR2D(maxx - minx, 0.0f),
			VECTOR2D(maxx - minx, maxy - miny),
			VECTOR2D(0.0f, maxy - miny),
		});
		ragdoll[i] = RIGIDBODY(aVertices.data(), iNumVertices);
		
		float dx = (model.GetPart(i)->GetRotationX()-
			(model.GetPart(i)->iWidth>>1))*0.4f;
		float dy = (model.GetPart(i)->GetRotationY()-
			(model.GetPart(i)->iHeight>>1))*0.4f;
		VECTOR2D vOffset(dx, dy);
		
		ragdoll[i].Pos += vOffset;
		ragdoll[i].fRestitution = 0.6f;
		ragdoll[i].fFriction = 0.01f;
		ragdoll[i] = RIGIDBODY(aVertices.data(), iNumVertices);
		ragdoll[i].fRestitution = 0.6f;
		ragdoll[i].fFriction = 0.01f;
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

	std::vector<VECTOR2D> aVertices(6);
	std::ifstream f("data/meshes/grenade.dat");
	if (!f)
		return E_FAIL;

	int iNumVerts;
	float x, y;

	f >> iNumVerts;
	for (int i = 0; i < iNumVerts; i++) {
		f >> x >> y;
		aVertices[i] = VECTOR2D(x, y);
	}
	f.close();

	m_aPlayers[index].grenade_body = RIGIDBODY(aVertices.data(), iNumVerts);
	m_aPlayers[index].grenade_body.fRestitution = 0.6f;

	m_aFrags[index] = 0;
	m_aDeath[index] = 0;

	if (g_iNumPlayers > 0) {
		m_AIStates.resize(g_iNumPlayers);
	}

	g_iNumPlayers++;

	RespawnPlayer(&m_aPlayers[index]);

	return S_OK;
}

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
