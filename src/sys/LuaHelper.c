/*
 * File: LuaHelper.c
 * Author: David Brotz
 */

#include "LuaHelper.h"

#include <stdlib.h>
#include <string.h>
#include <lua/lauxlib.h>

int AddInteger(lua_State* _State, int _Index, int* _Number) {
	if(lua_isnumber(_State, _Index)) {
		*_Number = lua_tointeger(_State, _Index);
		return 1;
	}
	luaL_error(_State, "metafield is not a integer");
	return -1;
}

int AddString(lua_State* _State, int _Index, const char** _String) {
	if(lua_isstring(_State, _Index)) {
		*_String = lua_tostring(_State, _Index);
		return 1;
	}
	luaL_error(_State, "metafield is not a string");
	return -1;
}

int AddNumber(lua_State* _State, int _Index, double* _Number) {
	if(lua_isnumber(_State, _Index)) {
		*_Number = lua_tonumber(_State, _Index);
		return 1;
	}
	luaL_error(_State, "metafield is not a number");
	return -1;
}

void LuaStackToTable(lua_State* _State, int* _Table) {
	int _Top = lua_gettop(_State);
	int i;

	for(i = 0; i < _Top; ++i)
		_Table[i] = lua_type(_State, i);
}

int LuaIntPair(lua_State* _State, int _Index, int* _One, int* _Two) {
	lua_pushnil(_State);
	if(lua_next(_State, _Index) == 0)
		return 0;
	if(AddInteger(_State, -1, _One) == -1)
		return 0;
	lua_pop(_State, 1);
	if(lua_next(_State, _Index) == 0)
		return 0;
	if(AddInteger(_State, -1, _Two) == -1)
		return 0;
	return 1;
}

int LuaKeyValue(lua_State* _State, int _Index, const char** _Value, int* _Pair) {
	int _Top = lua_gettop(_State);

	lua_pushnil(_State);
	if(lua_next(_State, _Index - 1) == 0)
		goto fail;
	if(AddString(_State, -1, _Value) == -1)
		goto fail;
	lua_pop(_State, 1);
	if(lua_next(_State, _Index - 1) == 0)
		goto fail;
	if(AddInteger(_State, -1, _Pair) == -1)
		goto fail;
	lua_pop(_State, 2);
	return 1;
	fail:
	lua_settop(_State, _Top);
	return 0;
}
