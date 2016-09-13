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
		{LOBJ_AGENT, "Agent", LUA_REFNIL, g_LuaFuncsAgent},
		{LUA_REFNIL, NULL, LUA_REFNIL, NULL}
};

int LuaAgentGetAction(lua_State* _State) {
	struct Agent* _Agent = LuaCheckClass(_State, 1, LOBJ_AGENT);

	lua_pushinteger(_State, 0);//AgentGetAction(_Agent));
	return 1;
}
