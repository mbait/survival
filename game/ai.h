#ifndef GAME_AI_H
#define GAME_AI_H

#include <vector>

#include "gamecode.h"

// Per-AI state vector (one entry per AI player; index 0 is the human).
extern std::vector<AI_STATE_TYPE> m_AIStates;

// Drive an AI player toward a waypoint by setting walk/jump actions.
void RunToWayPoint(bool* actions, int index, int WPIndex);

// Decide what the AI player at `index` does this frame; fills the
// `actions` array indexed by static_cast<int>(ACTION::...).
void GetAIActions(int index, bool* actions);

// qsort comparator for player scoreboard sort: lower death count wins,
// ties broken by higher frag count.
int  ScoreCmp(const void* arg_1, const void* arg_2);

#endif  // GAME_AI_H
