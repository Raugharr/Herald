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

int AddString(lua_State* _State, int _Index, char** _String) {
	const char* _Temp = NULL;

	if(lua_isstring(_State, _Index)) {
		_Temp = lua_tostring(_State, _Index);
		*_String = (char*)malloc(strlen(_Temp) + 1);
		strcpy(*_String, _Temp);
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


