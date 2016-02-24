/*
 * File: LuaAI.c
 * Author: David Brotz
 */

#include "LuaAI.h"
#include "Agent.h"

#include <lua/lauxlib.h>
#include <lua/lualib.h>

static const luaL_Reg g_LuaFuncsAgent[] = {
		{"GetAction", LuaAgentGetAction},
		{NULL, NULL},
};

const struct LuaObjectReg g_LuaAIObjects[] = {
		{"Agent", NULL, g_LuaFuncsAgent},
		{NULL, NULL, NULL}
};

int LuaAgentGetAction(lua_State* _State) {
	struct Agent* _Agent = LuaCheckClass(_State, 1, "Agent");

	lua_pushinteger(_State, AgentGetAction(_Agent));
	return 1;
}
