/*
 * File: Setup.c
 * Author: David Brotz
 */

#include "Setup.h"

#include "BehaviorTree.h"
#include "Behaviors.h"
#include "LuaLib.h"
#include "goap.h"

#include "actions/Action.h"

#include "../Location.h"
#include "../Family.h"
#include "../BigGuy.h"
#include "../Person.h"
#include "../Government.h"
#include "../ArmyGoal.h"
#include "../Warband.h"

#include "../sys/LinkedList.h"
#include "../sys/HashTable.h"
#include "../sys/Log.h"
#include "../sys/Stack.h"
#include "../sys/LuaCore.h"
#include "../sys/Math.h"

#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <lua/lua.h>
#include <lua/lauxlib.h>

struct GOAPPlanner g_Goap;

void BGSetup(struct GOAPPlanner* _Planner, const char** _Atoms, int _AtomSz, AgentActions _Actions) {
	GoapClear(_Planner);
	for(int i = 0; i < _AtomSz; ++i)
		GoapAddAtom(_Planner, _Atoms[i]);
	for(int i = 0; _Actions[i] != NULL; ++i)
		_Actions[i](_Planner);
}

void AIInit(lua_State* _State) {
	if(BehaviorsInit(_State) == 0) {
		Log(ELOG_ERROR, "Behaviors have failed to load.");
		return;
	}
	LuaAILibInit(_State);
	GoapInit();
	BGSetup(&g_Goap, g_BGStateStr, BGBYTE_SIZE, g_GOAPActionList);
}

void AIQuit() {
	GoapQuit();
}
