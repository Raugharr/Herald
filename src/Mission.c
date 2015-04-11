/*
 * File: Mission.c
 * Author: David Brotz
 */

#include "Mission.h"

#include "sys/LuaHelper.h"
#include "sys/Rule.h"

#include <stdlib.h>
#include <string.h>
#include <lua/lua.h>
#include <lua/lauxlib.h>

struct Mission* LoadMission(lua_State* _State, int _Index) {
	int i = 0;
	char* _Name = NULL;
	char* _Description = NULL;
	int _AbsIndex = LuaAbsPos(_State, _Index);
	struct MissionOption** _Options = NULL;
	struct Mission* _Mission = NULL;

	if(lua_type(_State, _AbsIndex) != LUA_TTABLE) {
		luaL_error(_State, "Mission is not a table.");
		return NULL;
	}
	lua_pushstring(_State, "Options");
	lua_rawget(_State, _AbsIndex);
	if(lua_type(_State, -1) != LUA_TTABLE) {
		luaL_error(_State, "Mission table does not have a field named Options or is not a table.");
		return NULL;
	}
	lua_len(_State, -1);
	_Options = calloc(lua_tointeger(_State, -1) + 1, sizeof(struct MissionOption*));
	lua_pop(_State, 1);
	lua_pushnil(_State);
	while(lua_next(_State, -2) != 0) {
		_Options[i] = LoadMissionOption(_State, -1);
		++i;
		lua_pop(_State, 1);
	}
	lua_pop(_State, 1);
	_Mission = (struct Mission*) malloc(sizeof(struct Mission));
	_Mission->Options = _Options;
	return _Mission;
}

struct MissionOption* LoadMissionOption(lua_State* _State, int _Index) {
	int _AbsIndex = LuaAbsPos(_State, _Index);
	int i = 1;
	const char* _Temp = NULL;
	char* _Text = NULL;
	struct Rule* _Rules[4];
	struct MissionOption* _Option = NULL;

	if(lua_type(_State, _Index) != LUA_TTABLE) {
		luaL_error(_State, "Mission option parameter is not a a table.");
		return NULL;
	}
	lua_rawgeti(_State, _AbsIndex, 1);
	if(lua_type(_State, -1) != LUA_TSTRING) {
		luaL_error(_State, "Mission option's first parameter is not a string.");
		return NULL;
	}
	_Temp = lua_tostring(_State, -1);
	_Text = calloc(strlen(_Temp) + 1, sizeof(char));
	strcpy(_Text, _Temp);
	lua_rawgeti(_State, _AbsIndex, 1);
	if(lua_type(_State, -1) != LUA_TTABLE) {
		luaL_error(_State, "Mission option's second parameter is not a table.");
		return NULL;
	}
	for(i = 0; i < 4; ++i) {
		lua_rawgeti(_State, -1, i + 1);
		if(_Rules[i] = (struct Rule*) LuaToObject(_State, -1, "Rule") == NULL) {
			luaL_error(_State, "Mission option's rule table contains a non rule.");
			return NULL;
		}
		lua_pop(_State, 1);
	}
	_Option = malloc(sizeof(struct MissionOption));
	_Option->Text = _Text;
	_Option->SuccessCon = _Rules[0];
	_Option->SuccessRwrd = _Rules[1];
	_Option->FailureCon = _Rules[2];
	_Option->FailureRwrd = _Rules[3];
	return _Option;
}
