#include "ai.h"

#include <cmath>
#include <cstring>

std::vector<AI_STATE_TYPE> m_AIStates;

void RunToWayPoint(bool* actions, int index, int WPIndex)
{
	VECTOR2D dst_vec = aWayPoints[WPIndex].vPos - aWayPoints[m_aPlayers[index].nVertexIndex].vPos;

	if (dst_vec.x > 0.0f)
		actions[static_cast<int>(ACTION::MOVERIGHT)] = true;
	else if (dst_vec.x < 0.0f)
		actions[static_cast<int>(ACTION::MOVELEFT)] = true;

	float fTheta = atan2(-dst_vec.y, dst_vec.x);
	if (fTheta > Pi)
		fTheta = -fTheta;
	fTheta -= Pi / 2.0f;

	if (fabs(fTheta) < Pi / 4.0f && m_aPlayers[index].prev_state != STATE::FLY)
		actions[static_cast<int>(ACTION::JUMP)] = true;
}

void GetAIActions(int index, bool* actions)
{
	memset(actions, false, static_cast<size_t>(ACTION::NUMACTIONS));

	if (!g_iNumWayPoints)
		return;

	int src_vert = m_aPlayers[index].nVertexIndex;
	int dst_vert;
	int next_vert;

	bool bCanShoot = m_aPlayers[index].iViewSoldierID == 0;

	int ammo_level =
	    m_aPlayers[index].ammo[static_cast<int>(WEAPON::RIFLE)] * 100 / PLAYER::c_NumRifleAmmo;
	int grenade_level =
	    m_aPlayers[index].ammo[static_cast<int>(WEAPON::GRENADE)] * 100 / PLAYER::c_NumGrenades;
	int health_level = m_aPlayers[index].health * 100 / PLAYER::c_NumHealth;

	int o = 1 - 2 * m_aPlayers[index].model.GetOrientation();
	VECTOR2D obj_vec =
	    m_aPlayers[0].body.Pos - m_aPlayers[index].body.Pos - VECTOR2D(0.0f, 10.0f * o);

	VECTOR2D view_vec;

	switch (m_AIStates[index - 1])
	{
	// attack the player_0
	case AI_STATE_TYPE::ATTACK:
	{
		view_vec = obj_vec;
		if ((ammo_level < 20 && grenade_level == 0) || health_level < 15)
		{
			m_AIStates[index - 1] = AI_STATE_TYPE::RUNAWAY;
		}
		else
		{
			if (bCanShoot)
			{
				actions[static_cast<int>(ACTION::SHOOT)] = true;
				if (DotProduct(obj_vec, obj_vec) > SQ_NEAR_DISTANCE)
				{
					if ((int)(RANDOM * 10) == 5 && grenade_level > 0)
					{
						actions[static_cast<int>(ACTION::ALTSHOOT)] = true;
					}
				}
			}
			else
			{
				m_AIStates[index - 1] = AI_STATE_TYPE::PURSUIT;
			}
		}
	}
	break;

	// look for player_0
	case AI_STATE_TYPE::PURSUIT:
	{
		view_vec = obj_vec;
		dst_vert = m_aPlayers[0].nVertexIndex;

		if (apPathDistance[src_vert][dst_vert] > NEAR_DISTANCE)
		{
			if (ammo_level >= 50 && health_level >= 40)
			{
				next_vert = apPathParent[src_vert][dst_vert];
				while (next_vert != -1 && apPathParent[src_vert][next_vert] != src_vert)
				{
					next_vert = apPathParent[src_vert][next_vert];
				}
				bool bCanReach = next_vert != -1;
				if (bCanReach)
					RunToWayPoint(actions, index, next_vert);
				if (bCanShoot)
					actions[static_cast<int>(ACTION::SHOOT)] = true;
			}
			else
			{
				m_AIStates[index - 1] = AI_STATE_TYPE::SEARCH_PACK;
			}
		}
		else
		{
			if ((1 - 2 * m_aPlayers[index].model.GetOrientation()) != Sign(obj_vec.x))
			{
				if (obj_vec.x > 0.0f)
					actions[static_cast<int>(ACTION::MOVERIGHT)] = true;
				else
					actions[static_cast<int>(ACTION::MOVELEFT)] = true;
			}
			m_AIStates[index - 1] = AI_STATE_TYPE::ATTACK;
		}
	}
	break;

	// runaway from player_0
	case AI_STATE_TYPE::RUNAWAY:
	{
		view_vec = VECTOR2D(10.0f, 0.0f);
		if (DotProduct(obj_vec, obj_vec) > SQ_FAR_DISTANCE)
		{
			m_AIStates[index - 1] = AI_STATE_TYPE::SEARCH_PACK;
		}
		else
		{
			int dir = Sign(obj_vec.x);
			int u = m_aPlayers[index].nVertexIndex;
			int iNum = aWayPoints[u].iNumEdges;
			next_vert = -1;
			float min = _HUGE;
			for (int i = 0; i < iNum; i++)
			{
				if (apPathDistance[src_vert][aWayPoints[u].aEdges[i]] < min)
				{
					if (Sign(aWayPoints[src_vert].vPos.x -
					         aWayPoints[aWayPoints[u].aEdges[i]].vPos.x) == dir)
					{
						min = apPathDistance[src_vert][aWayPoints[u].aEdges[i]];
						next_vert = aWayPoints[u].aEdges[i];
					}
				}
			}
			if (next_vert != -1)
				RunToWayPoint(actions, index, next_vert);
			else
				m_AIStates[index - 1] = AI_STATE_TYPE::ATTACK;
		}
	}
	break;

	// search for packs to increase its resources
	case AI_STATE_TYPE::SEARCH_PACK:
	{
		view_vec = VECTOR2D(10.0f, 0.0f);
		if (ammo_level < 50 || health_level < 40)
		{
			float min = _HUGE;
			next_vert = -1;
			for (int i = 0; i < g_iNumPackPlaces; i++)
			{
				if (aPacks[i].bActive && apPathDistance[src_vert][aPacks[i].nVertexIndex] < min)
				{
					switch (aPacks[i].type)
					{
					case PACK_TYPE::PACK_AMMO:
						if (ammo_level > 60)
							continue;
						break;
					case PACK_TYPE::PACK_GRENADE:
						if (grenade_level == 100)
							continue;
						break;
					case PACK_TYPE::PACK_HEALTH:
						if (health_level > 40)
							continue;
						break;
					case PACK_TYPE::NUM_PACKS:
						break;
					}
					min = apPathDistance[src_vert][aPacks[i].nVertexIndex];
					next_vert = aPacks[i].nVertexIndex;
				}
			}
			if (next_vert != -1)
			{
				if (next_vert == src_vert)
				{
					VECTOR2D v = aPackPlaces[src_vert] - m_aPlayers[index].body.Pos;
					if (v.x > Epsilon)
						actions[static_cast<int>(ACTION::MOVERIGHT)] = true;
					else if (v.x < -Epsilon)
						actions[static_cast<int>(ACTION::MOVELEFT)] = true;
				}
				else
				{
					next_vert = apPathParent[src_vert][next_vert];
					while (next_vert != -1 && apPathParent[src_vert][next_vert] != src_vert)
					{
						next_vert = apPathParent[src_vert][next_vert];
					}
					if (next_vert != -1)
						RunToWayPoint(actions, index, next_vert);
				}
			}
			else
			{
				m_AIStates[index - 1] = AI_STATE_TYPE::RUNAWAY;
			}
		}
		else
		{
			m_AIStates[index - 1] = AI_STATE_TYPE::PURSUIT;
		}
	}
	break;

	// help other bots attack player_0 (stub)
	case AI_STATE_TYPE::HELP:
		break;
	}

	if ((actions[static_cast<int>(ACTION::MOVELEFT)] ||
	     actions[static_cast<int>(ACTION::MOVERIGHT)]) &&
	    m_aPlayers[index].bBoxCollision)
	{
		actions[static_cast<int>(ACTION::JUMP)] = true;
	}

	view_vec.x = fabs(view_vec.x);
	m_aPlayers[index].cursor = view_vec;
}

int ScoreCmp(const void* arg_1, const void* arg_2)
{
	int a = *(int*)arg_1;
	int b = *(int*)arg_2;
	if (m_aDeath[a] < m_aDeath[b])
		return -1;
	if (m_aDeath[a] > m_aDeath[b])
		return 1;
	if (m_aFrags[a] > m_aFrags[b])
		return -1;
	if (m_aFrags[a] < m_aFrags[b])
		return 1;
	return 0;
}
