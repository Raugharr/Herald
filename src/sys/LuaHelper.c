/*
 * File: LuaHelper.c
 * Author: David Brotz
 */

#include "LuaHelper.h"

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


