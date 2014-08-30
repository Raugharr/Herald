/*
 * File: LuaLib.h
 * Author: David Brotz
 */
#ifndef __LUALIB_H
#define __LUALIB_H

#include <lua/lauxlib.h>

typedef struct lua_State lua_State;
struct Behavior;
struct Event;

struct LuaBehavior {
	char* Name;
	struct Behavior* Behavior;
};

struct AIHook {
	struct Event* Event;
	struct Behavior* Bhv;
	struct Behavior* Condition;
	struct Behavior** BhvPaths;
};

int LuaBhvCmp(const void* _One, const void* _Two);
int luaStrLuaBhvCmp(const void* _One, const void* _Two);

int LuaBhvComp(lua_State* _State);
int LuaBhvPrim(lua_State* _State);
int LuaBhvDec(lua_State* _State);

int LuaBhvTree(lua_State* _State);
int LuaAISetHook(lua_State* _State);

static const luaL_Reg g_LuaAIFuncs[] = {
	{"CompNode", LuaBhvComp},
	{"PrimNode", LuaBhvPrim},
	{"DecorateNode", LuaBhvDec},
	{"AddBhvTree", LuaBhvTree},
	{"Hook", LuaAISetHook},
	{NULL, NULL}
};

#endif
