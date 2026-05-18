#include "gamecode.h"

#include <SDL_image.h>

#include <algorithm>
#include <fstream>
#include <vector>

// ---- world-data globals (definitions own here) ---------------------------
MATERIAL    g_aMaterials[MAX_MATERIALS];
const char* g_szMaterialFile[MAX_MATERIALS] = {
	"data/textures/Bricks.jpg\0",
	"data/textures/Metal.jpg\0",
	"data/textures/plastic.jpg\0",
	"data/textures/box.tga\0",
};

std::vector<RIGIDBODY>              aWalls;
std::vector<RIGIDBODY>              aBodies;
std::vector<VECTOR2D>               aRespawns;
std::vector<PACK>                   aPacks;
std::vector<VECTOR2D>               aPackPlaces;
std::vector<NODE>                   aWayPoints;
std::vector<std::vector<int>>       apPathParent;
std::vector<std::vector<float>>     apPathDistance;
std::vector<std::vector<VECTOR2D>>  g_vWallTex;
std::vector<std::vector<VECTOR2D>>  g_vBodyTex;

int  g_iNumWalls         = 0;
int  g_iNumWallVertices  = 0;
int  g_iNumBodies        = 0;
int  g_iNumBodyVertices  = 0;
int  g_iNumRespawns      = 0;
int  g_iNumPackPlaces    = 0;
int  g_iNumWayPoints     = 0;
bool g_bCollided         = false;
DWORD g_last_coltime     = 0;

HRESULT LoadMap(const char* szFileName)
{
	//===============load materials==============//
	std::ifstream f("data/config/mat.cfg");
	if (!f)
		return E_FAIL;

	for (int i = 0; i < static_cast<int>(MATERIAL_TYPE::NUM_MATERIALS); i++) {
		f >> g_aMaterials[i].fWidth >> g_aMaterials[i].fHeight;
		if (g_szMaterialFile[i] == nullptr) {
			g_aMaterials[i].pTexture = nullptr;
			continue;
		}
		g_aMaterials[i].pTexture = IMG_LoadTexture(g_renderer, g_szMaterialFile[i]);
		if (!g_aMaterials[i].pTexture)
			return E_FAIL;
	}
	f.close();
	//================end materials==============//

	//===========read walls data============//
	f.open(szFileName);
	if (!f)
		return E_FAIL;

	int iNumVerts;

	int iNumWalls;
	f >> iNumWalls;
	g_iNumWalls = iNumWalls;
	aWalls.assign(iNumWalls, RIGIDBODY{});
	g_vWallTex.assign(iNumWalls, std::vector<VECTOR2D>{});
	float x, y;
	int matID;
	for (int i = 0; i < iNumWalls; i++) {
		f >> iNumVerts;
		f >> matID;
		g_iNumWallVertices += iNumVerts;
		std::vector<VECTOR2D> aVertices(iNumVerts);
		g_vWallTex[i].resize(iNumVerts);
		for (int j = 0; j < iNumVerts; j++) {
			f >> x >> y;
			f >> g_vWallTex[i][j].x >> g_vWallTex[i][j].y;
			aVertices[j].x = x;
			aVertices[j].y = y;
			g_vWallTex[i][j] = VECTOR2D(x / g_aMaterials[matID].fWidth,
			                            y / g_aMaterials[matID].fHeight);
		}
		aWalls[i] = RIGIDBODY(aVertices.data(), iNumVerts, true);
		aWalls[i].fRestitution = 0.5f;
		aWalls[i].fFriction	   = 0.1f;
		aWalls[i].lMaterialID  = matID;
	}

	//============load map objects==========//
	int iNumBodies;
	f >> iNumBodies;
	g_iNumBodies = iNumBodies;
	aBodies.assign(iNumBodies, RIGIDBODY{});
	g_vBodyTex.assign(iNumBodies, std::vector<VECTOR2D>{});
	for (int i = 0; i < iNumBodies; i++) {
		f >> iNumVerts;
		f >> matID;
		g_iNumBodyVertices += iNumVerts;
		std::vector<VECTOR2D> aVertices(iNumVerts);
		g_vBodyTex[i].resize(iNumVerts);
		for (int j = 0; j < iNumVerts; j++) {
			f >> x >> y;
			f >> g_vBodyTex[i][j].x >> g_vBodyTex[i][j].y;
			aVertices[j] = VECTOR2D(x, y);
		}
		aBodies[i] = RIGIDBODY(aVertices.data(), iNumVerts);
		aBodies[i].fRestitution = 0.55f;
		aBodies[i].fFriction	= 0.03f;
		aBodies[i].lMaterialID	= matID;
	}

	//===========load respawn points========//
	f >> g_iNumRespawns;
	aRespawns.resize(g_iNumRespawns);
	for (int i = 0; i < g_iNumRespawns; i++) {
		f >> aRespawns[i].x >> aRespawns[i].y;
	}

	//=============read packplaces==========//
	f >> g_iNumPackPlaces;
	aPackPlaces.resize(g_iNumPackPlaces);
	aPacks.assign(g_iNumPackPlaces, PACK{});
	for (int i = 0; i < g_iNumPackPlaces; i++) {
		f >> aPackPlaces[i].x >> aPackPlaces[i].y;
		aPacks[i].type    = (PACK_TYPE)(int)(RANDOM*static_cast<int>(PACK_TYPE::NUM_PACKS));
		aPacks[i].bActive = true;
	}

	//==============read waypoints==========//
	f >> g_iNumWayPoints;
	aWayPoints.assign(g_iNumWayPoints, NODE{});
	for (int i = 0; i < g_iNumWayPoints; i++) {
		f >> aWayPoints[i].vPos.x >> aWayPoints[i].vPos.y;
		f >> aWayPoints[i].iNumEdges;
		aWayPoints[i].aEdges.resize(aWayPoints[i].iNumEdges);
		for (int j = 0; j < aWayPoints[i].iNumEdges; j++) {
			f >> aWayPoints[i].aEdges[j];
		}
	}
	f.close();

	//assign packplace to nearest waypoint
	for (int i = 0; i < g_iNumPackPlaces; i++) {
		float min = _HUGE, dp;
		int   av  = -1;
		for (int j = 0; j < g_iNumWayPoints; j++) {
			VECTOR2D v = aPackPlaces[i] - aWayPoints[j].vPos;
			if ((dp = DotProduct(v, v)) < min) {
				min = dp;
				av  = j;
			}
		}
		aPacks[i].nVertexIndex = av;
	}

	//find shortest path for each vertex (Dijkstra over the waypoint graph)
	apPathParent.assign(g_iNumWayPoints, std::vector<int>(g_iNumWayPoints, -1));
	apPathDistance.assign(g_iNumWayPoints,
	                     std::vector<float>(g_iNumWayPoints, 0x7fffffff));

	std::vector<bool> vis(g_iNumWayPoints);
	for (int s = 0; s < g_iNumWayPoints; s++) {
		std::fill(vis.begin(), vis.end(), false);
		apPathDistance[s][s] = 0.0f;
		int u = 0;
		for (int i = 0; i < g_iNumWayPoints; i++) {
			int min = 0x7fffffff;
			for (int j = 0; j < g_iNumWayPoints; j++) {
				if (apPathDistance[s][j] < min && !vis[j]) {
					u   = j;
					min = apPathDistance[s][j];
				}
			}
			vis[u] = true;
			for (int j = 0; j < aWayPoints[u].iNumEdges; j++) {
				const int   v = aWayPoints[u].aEdges[j];
				const float l = Length(aWayPoints[v].vPos - aWayPoints[u].vPos);
				if (apPathDistance[s][u] + l < apPathDistance[s][v]) {
					apPathDistance[s][v] = apPathDistance[s][u] + l;
					apPathParent[s][v]   = u;
				}
			}
		}
	}

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
