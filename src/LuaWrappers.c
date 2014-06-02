/*
 * File: LuaWrappers.c
 * Author: David Brotz
 */

#include "LuaWrappers.h"

#include "sys/Constraint.h"
#include "sys/LuaHelper.h"

#include <lua/lauxlib.h>

#ifndef NULL
#define NULL ((void*)0)
#endif

struct Constraint* ConstraintFromLua(lua_State* _State, int _Index) {
	int _Min = 0;
	int _Max = 0;

	if(!lua_istable(_State, _Index))
		return NULL;
	lua_getmetatable(_State, _Index);
	lua_pushnil(_State);
	if(lua_next(_State, _Index - 1) == 0)
		return NULL;
	AddInteger(_State, _Index, &_Min);
	lua_pop(_State, 1);
	if(lua_next(_State, _Index - 1) == 0)
		return NULL;
	AddInteger(_State, _Index, &_Max);
	return CreateConstraint(_Min, _Max);
}
