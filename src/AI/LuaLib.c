/*
 * File: LuaLib.c
 * Author: David Brotz
 */

#include "LuaLib.h"

#include "BehaviorTree.h"
#include "Setup.h"
#include "../sys/LuaHelper.h"
#include "../sys/Log.h"

#include <stdlib.h>
#include <string.h>
#include <lua/lua.h>
#include <lua/lauxlib.h>

int LuaBhvCmp(const void* _One, const void* _Two) {
	return strcmp(((const struct LuaBehavior*)_One)->Name, ((const struct LuaBehavior*)_Two)->Name);
}

int luaStrLuaBhvCmp(const void* _One, const void* _Two) {
	return strcmp(((const char*)_One), ((const struct LuaBehavior*)_Two)->Name);
}

int LuaBhvComp(lua_State* _State) {
	int _Len = 0;
	int i = 0;
	const char* _Key = NULL;
	struct Behavior* _Bhv = NULL;
	struct Behavior* _Child = NULL;

	if(!lua_isstring(_State, -2)) {
		LUA_BADARG(1, "Must be a string.");
		return 0;
	}
	if(!lua_istable(_State, -1)) {
		LUA_BADARG(2, "Must be a table.");
		return 0;
	}
	if((_Key = lua_tostring(_State, -2)) == NULL) {
		LogLua(_State, ELOG_WARNING, "Name not found.");
		return 0;
	}
	_Len = lua_rawlen(_State, -2);
	if(!strcmp(_Key, "Sequence")) {
		_Bhv = CreateBehavior(NULL, NULL, _Len, BhvSelector);
	} else if(!strcmp(_Key, "Selector")) {
		_Bhv = CreateBehavior(NULL, NULL, _Len, BhvSequence);
	} else {
		LUA_BADARG(1, "Must be either \"Sequence\" or \"Selector\".");
		return 0;
	}
	lua_pushnil(_State);
	while(lua_next(_State, -2) != 0) {
		if(!lua_islightuserdata(_State, -1)) {
			LUA_BADARG(2, "The %sth element is not a user data.");
			goto loopend;
		}
		_Child = lua_touserdata(_State, -1);
		_Bhv->Children[i++] = _Child;
		loopend:
		lua_pop(_State, 1);
	}
	lua_pushlightuserdata(_State, _Bhv);
	return 1;
}

int LuaBhvPrim(lua_State* _State) {
	struct LuaBhvAction _Temp;
	struct LuaBhvAction* _Action = NULL;

	if(!lua_isstring(_State, -1)) {
		LUA_BADARG(1, "Must be a string.");
		return 0;
	}
	_Temp.Name = lua_tostring(_State, -1);
	if((_Action = bsearch(&_Temp, g_BhvActions, g_BhvActionsSz, sizeof(struct LuaBhvAction), LuaBaCmp)) == NULL) {
		LUA_BADARG_V(1, "%s is not a valid behavior action.", _Temp.Name);
		return 0;
	}
	lua_pushlightuserdata(_State, CreateBehavior(NULL, _Action->Action, 0, NULL));
	return 1;
}

int LuaBhvDec(lua_State* _State) {
	struct LuaBhvAction _Temp;
	struct LuaBhvAction* _Action = NULL;
	const char* _Name = NULL;

	if(!lua_isstring(_State, -2)) {
		LUA_BADARG(1, "Must be a string.");
		return 0;
	}
	_Name = lua_tostring(_State, -2);
	if(!strcmp(_Name, "Not")) {
		_Temp.Name = lua_tostring(_State, -1);
		if((_Action = bsearch(&_Temp, g_BhvActions, g_BhvActionsSz, sizeof(struct LuaBhvAction), LuaBaCmp)) == NULL) {
			LUA_BADARG_V(2, "%s is not a valid behavior action.", _Temp.Name);
			return 0;
		}
		lua_pushlightuserdata(_State, CreateBehavior(NULL, _Action, 0, BhvNot));
	} else {
		LUA_BADARG_V(2, "%s is not a valid decorator.", _Action->Name);
		return 0;
	}
	return 1;
}

int LuaBhvTree(lua_State* _State) {
	const char* _Arg = NULL;

	if(!lua_islightuserdata(_State, -2)) {
		LUA_BADARG(1, "Must be a behavior tree.");
		return 0;
	}
	if(!lua_isstring(_State, -1)) {
		LUA_BADARG(2, "Must be a string.");
		return 0;
	}
	_Arg = lua_tostring(_State, -1);
	if(BinarySearch(_Arg, g_BhvList.Table, g_BhvList.Size, LuaBhvCmp) == NULL) {
		struct LuaBehavior* _Bhv = (struct LuaBehavior*) malloc(sizeof(struct LuaBehavior));

		_Bhv->Name = calloc(strlen(_Arg) + 1, sizeof(char));
		_Bhv->Behavior = lua_touserdata(_State, -2);
		ArrayInsert_S(&g_BhvList, _Bhv);
		return 0;
	}
	LUA_BADARG_V(2, "%s is already a behavior tree.", _Arg);
	return 0;
}

int LuaAISetHook(lua_State* _State) {
	struct AIHook* _Hook = NULL;
	int i = 0;
	int _Size = 0;

	if(!lua_islightuserdata(_State, -4)) {
		LUA_BADARG(1, "Must be a behavior tree.");
		return 0;
	}
	if(!lua_islightuserdata(_State, -3)) {
		LUA_BADARG(2, "Must be a hook.");
		return 0;
	}
	if(!lua_istable(_State, -2)) {
		LUA_BADARG(3, "Must be a table.");
		return 0;
	}
	if(!lua_istable(_State, -1)) {
		LUA_BADARG(4, "Must be a table.");
		return 0;
	}
	if(lua_rawlen(_State, -2) != lua_rawlen(_State, -1))
		LogLua(_State, ELOG_WARNING, "Argument 3 and 4 do not have the same length.");
	_Size = lua_rawlen(_State, -2);
	_Hook = (struct AIHook*) malloc(sizeof(struct AIHook));
	_Hook->Bhv = (struct Behavior*) lua_touserdata(_State, -4);
	_Hook->Event = (struct Event*) lua_touserdata(_State, -3);
	_Hook->Condition = CreateBehavior(NULL, NULL, _Size, BhvSelector);
	_Hook->BhvPaths = calloc(_Size + 1, sizeof(struct Behavior*));
	lua_pushvalue(_State, -2);
	lua_pushnil(_State);
	while(lua_next(_State, -2) != 0) {
		if(lua_islightuserdata(_State, -1) == 0) {
			LUA_BADARG(3, "Argument #3 has an element which is not a behavior.");
			goto argthree;
		}
		_Hook->Condition->Children[i++] = lua_touserdata(_State, -1);
		argthree:
		lua_pop(_State, 1);
	}
	lua_pop(_State, 1);
	i = 0;
	lua_pushvalue(_State, -1);
	lua_pushnil(_State);
	while(lua_next(_State, -2) != 0) {
		if(lua_islightuserdata(_State, -1) == 0) {
			LUA_BADARG(3, "Argument #4 has an element which is not a behavior tree.");
			goto argfour;
		}
		_Hook->BhvPaths[i++] = lua_touserdata(_State, -1);
		argfour:
		lua_pop(_State, 1);
	}
	_Hook->BhvPaths[i] = NULL;
	lua_pop(_State, 1);
	return 0;
}
