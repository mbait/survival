#ifndef GAMECODE_H
#define GAMECODE_H

#define INITGUID

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>

#include "compat/win32_compat.h"
#include "graphics/model.h"
#include "physics/physics2D.h"
#include "graphics/particles.h"
#include "main.h"

// Linker dependencies are handled by CMake on Linux; the #pragma comment
// directives are MSVC-only and silently ignored elsewhere. Kept gated so
// a future Windows MSVC build still picks them up.
#ifdef _MSC_VER
#pragma comment(lib, "d3d9.lib")
#pragma comment(lib, "d3dx9.lib")
#endif

#define D3DFVF_TEXTUREVERTEX (D3DFVF_XYZRHW|D3DFVF_TEX1)
#define D3DFVF_COLORVERTEX   (D3DFVF_XYZRHW|D3DFVF_DIFFUSE)
#define PI D3DX_PI
//#define MIN_VELX 60.5f
//#define MIN_VELY 60.5f

//game options////////
#define MAX_PLAYERS 15
//////////////////////

//world physic constants//////
#define RUN_ACC			800.0f
#define RUN_MINVEL		5.0E-1f
#define RUN_MAXVEL		600.0f
#define FALL_MINVEL		0.54E3f
#define FALL_MAXVEL		3.0E3f
#define FALL_MAXVEL2	4.0E3f
#define JUMP_ACC		300.0f
#define WJUMP_ACC		150.0f
#define FLY_MAXVEL		200.0f
#define FLY_ACC			100.0f
#define FRICTION		0.01f
#define BOUNCE			0.1f
#define SHOOT_FORCE		0.5E3f
#define SHOOT_DAMAGE	4
#define EXPLODE_FORCE	4.0E5f
#define EXPLODE_RANGE	100.0f
//////////////////////////////

#define DEPTH_EPSILON 2.0f

//mouse settings////////
#define MOUSE_MIN_X    0
#define MOUSE_MIN_Y -100
#define MOUSE_MAX_X	 100
#define MOUSE_MAX_Y  100
////////////////////////

#define BUFFERSIZE		10000
#define MAX_PARTICLES	20000

//enviroment options////////////////
#define FIRE_NUMPARTICLES		1000
#define SMOKE_NUMPARTICLES		1000
#define CONCRETE_NUMPARTICLES	3

#define FIREPARTICLE_VEL		50
#define SMOKEPARTICLE_VEL		50
#define CONCRETEPARTICLE_VEL	100

#define MAX_MATERIALS			32
//player options////////////////////
#define PACK_AMMO_SIZE			20
#define PACK_GRENADE_SIZE		2
#define PACK_HEALTH_SIZE		50
////////////////////////////////////

//AI options////////////////////////
#define NEAR_DISTANCE			150.0f
#define SQ_NEAR_DISTANCE		22500
#define FAR_DISTANCE			300.0f
#define SQ_FAR_DISTANCE			90000
////////////////////////////////////

//macroses
#define RANDOM ((float)rand()/RAND_MAX)

//enumerations
enum class ANIMATION_TYPE { ANIMATION_IDLE, ANIMATION_RUN, ANIMATION_JUMP, ANIMATION_WJUMP, ANIMATION_SLIDE, ANIMATION_FALL, ANIMATION_CROUCHIDLE, ANIMATION_CROUCHRUN, NUMANIMATIONS };
enum class WEAPON { RIFLE, GRENADE, NUMWEAPONS };
enum class ACTION { MOVEUP, MOVEDOWN, MOVELEFT, MOVERIGHT, JUMP, WJUMP, SHOOT, ALTSHOOT, CROUCH, NUMACTIONS };
enum class STATE { IDLE, RUN, SLIDE, FLY, FALL, NUMSTATES };
enum class DIRECTION { LEFT = -1, RIGHT = 1 };
enum class COLLISION { NONE, VERT, HORZ };
enum class MATERIAL_TYPE { BRICKS, METAL, PLASTIC, WOOD, NUM_MATERIALS };
enum class PACK_TYPE { PACK_AMMO, PACK_GRENADE, PACK_HEALTH, NUM_PACKS };
enum class AI_STATE_TYPE { ATTACK, RUNAWAY, PURSUIT, SEARCH_PACK, HELP };

//screen initialization struct
/*struct SCREENSETTINGS
{
	HWND hwndParent;
	HINSTANCE hInstance;

	int iWidth;
	int iHeight;
	
	bool bWindowed;

	DWORD dwAAFlag;
};
*/
//D3D structures
struct COLORVERTEX
{
	float x, y, z, rhw;
	DWORD color;
};
struct TEXTUREVERTEX
{
	float x, y, z, rhw;
	float u, v;
};

//game structures
struct TIMER
{
	DWORD LastTickCount;
	DWORD ThisTickCount;
	
	TIMER() {
		LastTickCount = GetTickCount();
		ThisTickCount = GetTickCount();
	}
	inline DWORD Delta()
	{
		return (ThisTickCount-LastTickCount);
	}
	inline DWORD Update() {
		return ThisTickCount = GetTickCount();
	}
	inline void Reset()
	{
		LastTickCount = ThisTickCount =
		GetTickCount();
	}
};

struct MATERIAL
{
	SDL_Texture* pTexture;

	float fWidth;
	float fHeight;
};

struct PACK
{
	PACK_TYPE	type;
	TIMER		tmReset;
	bool		bActive;
	int			nVertexIndex;
};

struct PLAYER
{
	unsigned int ID;
	
	static constexpr int c_NumHealth    = 100;
	static constexpr int c_NumRifleAmmo = 200;
	static constexpr int c_NumGrenades  = 10;
	
	bool bAlive;
	bool bShooting;
	bool bBoxCollision;
	
	bool bJumpKeyOnce;
	bool bWJumpKeyOnce;

	RIGIDBODY *pViewObject;
	int	iViewSoldierID;

	//object sprites and bodies
	MODEL		model;
	RIGIDBODY	body;
    
	RIGIDBODY	ragdoll[NUM_PARTS];
	JOINT		joints[13];

	RIGIDBODY	grenade_body;
	TIMER		tmGrenade;
	bool		bActiveGrenade;
	
	//object states
	STATE		state;
	STATE		prev_state;
	DIRECTION	dir;
	COLLISION	coll;
	int			nVertexIndex;

	VECTOR2D	cursor;

	//object timers
	TIMER		tmDeath;
	TIMER		tmShoot;
	TIMER		tmAltShoot;

	//object parameters
	int		health;
	int		ammo[static_cast<int>(WEAPON::NUMWEAPONS)];

	HRESULT Init(SDL_Renderer* pRenderer,
				 const char* szModelFileName,
				 const char* szBodyFilename,
				 const char* szRagDollDir);
	void	Update(DWORD dwTime, bool *actions);
};

struct NODE
{
	VECTOR2D vPos;

	std::vector<int> aEdges;
	int iNumEdges;
};

inline DWORD FtoDW(float f) { return *((DWORD*)&f); }

// Forward declarations of the game's lifecycle entry points. After the
// SDL port (Phase 1g/1h/1k) the renderer/window come from the platform
// layer instead of HWND/HINSTANCE.
struct SDL_Window;
struct SDL_Renderer;

HRESULT InitGfx(SDL_Window* window, SDL_Renderer* renderer);
HRESULT ShowSplash();
HRESULT LoadGameData();
HRESULT LoadMap(const char* szFileName);
HRESULT UpdateScene(DWORD dwTime);
HRESULT UpdateFrame();
void    Cleanup();

// ----- shared game-state globals (extern; definitions live in their
// owning subsystem .cpp during the Phase 3h split). -----
extern PLAYER         m_aPlayers[MAX_PLAYERS];
extern int            g_iNumPlayers;
extern int            m_aFrags[MAX_PLAYERS];
extern int            m_aDeath[MAX_PLAYERS];
extern ANIMATION      animations[static_cast<int>(ANIMATION_TYPE::NUMANIMATIONS)];
extern const char*    szMeshFile[];

extern std::vector<RIGIDBODY>              aWalls;
extern std::vector<RIGIDBODY>              aBodies;
extern std::vector<VECTOR2D>               aRespawns;
extern std::vector<PACK>                   aPacks;
extern std::vector<VECTOR2D>               aPackPlaces;
extern std::vector<NODE>                   aWayPoints;
extern std::vector<std::vector<int>>       apPathParent;
extern std::vector<std::vector<float>>     apPathDistance;
extern std::vector<std::vector<VECTOR2D>>  g_vWallTex;
extern std::vector<std::vector<VECTOR2D>>  g_vBodyTex;

extern int  g_iNumWalls;
extern int  g_iNumWallVertices;
extern int  g_iNumBodies;
extern int  g_iNumBodyVertices;
extern int  g_iNumRespawns;
extern int  g_iNumPackPlaces;
extern int  g_iNumWayPoints;
extern bool g_bCollided;
extern DWORD g_last_coltime;

extern MATERIAL    g_aMaterials[MAX_MATERIALS];
extern const char* g_szMaterialFile[MAX_MATERIALS];

extern SDL_Window*   g_window;
extern SDL_Renderer* g_renderer;
extern SDL_Texture*  g_pFireTexture;
extern SDL_Texture*  g_pSmokeTexture;

extern SPRITE rifle, grenade, fire, cur_ptr;
extern SPRITE ui_health, ui_rifle, ui_grenade;
extern SPRITE pack_ammo, pack_grenade, pack_health;

extern SDL_Point g_cursor;
extern VECTOR2D  g_vCenter;

extern TIMER tmFPS;
extern int   nFrameCount;
extern int   nFPS;
extern bool  g_bAddKeyOnce;
extern bool  g_bRemoveKeyOnce;
extern bool  g_bShowStat;
#endif  // GAMECODE_H
