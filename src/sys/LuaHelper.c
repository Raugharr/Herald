/*
 * File: LuaHelper.c
 * Author: David Brotz
 */

#include "LuaHelper.h"

#include "LinkedList.h"

#include <stdlib.h>
#include <string.h>
#include <lua/lauxlib.h>

lua_State* g_LuaState = NULL;

int LoadLuaFile(lua_State* _State, const char* _File) {
	int _Error = luaL_loadfile(_State, _File);
	char _Buffer[256];

	if(_Error != 0)
		goto error;
	if((_Error = lua_pcall(_State, 0, LUA_MULTRET, 0)) != 0)
		goto error;
	return 1;

	error:
	switch(_Error) {
		case LUA_ERRSYNTAX:
			printf("%s", lua_tostring(_State, -1));
			return _Error;
		case LUA_ERRFILE:
			printf("Cannot load file: %s", _File);
			return _Error;
		case LUA_ERRRUN:
			sprintf(_Buffer, "Cannot run file: %s", lua_tostring(_State, -1));
			return _Error;

	}
	return 0;
}

void LoadLuaToList(lua_State* _State, const char* _File, const char* _Global, void*(*_Callback)(lua_State*, int), struct LinkedList* _Return) {
	void* _CallRet = NULL;

	if(LoadLuaFile(_State, _File) != 1)
		return;
	lua_getglobal(_State, _Global);
	if(!lua_istable(_State, -1))
		return;
	lua_pushnil(_State);
	while(lua_next(_State, -2) != 0) {
		if(!lua_istable(_State, -1)) {
			printf("Warning: index is not a table.");
			lua_pop(_State, 1);
			continue;
		}
		if((_CallRet = _Callback(_State, -1)) != NULL)
				LnkLst_PushBack(_Return, _CallRet);
		lua_pop(_State, 1);
	}
	lua_pop(_State, 1);
}

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
