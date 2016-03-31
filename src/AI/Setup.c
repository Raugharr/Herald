/*
 * File: Setup.c
 * Author: David Brotz
 */

#include "Setup.h"

#include "BehaviorTree.h"
#include "Behaviors.h"
#include "LuaLib.h"
#include "goap.h"

#include "Action.h"

#include "../Location.h"
#include "../Family.h"
#include "../BigGuy.h"
#include "../Person.h"
#include "../Government.h"
#include "../ArmyGoal.h"
#include "../Warband.h"

#include "../sys/Log.h"
#include "../sys/LuaCore.h"

#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <lua/lua.h>
#include <lua/lauxlib.h>

struct GOAPPlanner g_Goap;

void BGSetup(struct GOAPPlanner* _Planner, const char** _Atoms, int _AtomSz, AgentActions _Actions, AgentGoals _Goals) {
	GoapClear(_Planner);
	for(int i = 0; i < _AtomSz; ++i)
		GoapAddAtom(_Planner, _Atoms[i]);
	for(int i = 0; _Actions[i] != NULL; ++i) {
		++_Planner->ActionCt;
		_Actions[i](_Planner, &_Planner->Actions[i]);
	}
	for(int i = 0; i < GOAP_GOALSZ && _Goals[i] != NULL; ++i) {
		WorldStateClear(&_Planner->Goals[_Planner->GoalCt].GoalState);
		_Goals[i](_Planner, &_Planner->Goals[_Planner->GoalCt]);
		++_Planner->GoalCt;
	}
}

void AIInit(lua_State* _State) {
	if(BehaviorsInit(_State) == 0) {
		Log(ELOG_ERROR, "Behaviors have failed to load.");
		return;
	}
	LuaAILibInit(_State);
	GoapInit();
	BGSetup(&g_Goap, g_BGStateStr, BGBYTE_SIZE, g_GOAPActionList, g_GOAPGoalList);
}

void AIQuit() {
	GoapQuit();
}
