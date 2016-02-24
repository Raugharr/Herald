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

#define ERRMSG_BHV "%s is not a valid behavior action."
#define ERRMSG_BHVDEC "%s is not a valid decorator."

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

void LuaAILibInit(lua_State* _State);

int LuaBhvCmp(const void* _One, const void* _Two);
int luaStrLuaBhvCmp(const void* _One, const void* _Two);

int LuaBhvSelector(lua_State* _State);
int LuaBhvSequence(lua_State* _State);
int LuaBhvInverter(lua_State* _State);
int LuaBhvNode(lua_State* _State);

int LuaBhvTree(lua_State* _State);
//int LuaAISetHook(lua_State* _State);

#endif
