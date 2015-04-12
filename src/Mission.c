/*
 * File: Mission.c
 * Author: David Brotz
 */

#include "Mission.h"

#include "sys/LuaHelper.h"
#include "sys/Rule.h"
#include "sys/LinkedList.h"
#include "sys/Log.h"

#include <stdlib.h>
#include <string.h>
#include <lua/lua.h>
#include <lua/lauxlib.h>
#include <dirent.h>
#include <malloc.h>

void LoadAllMissions(lua_State* _State, struct LinkedList* _List) {
	int i = 0;
	DIR* _Dir = NULL;
	struct dirent* _Dirent = NULL;
	char* _TableName = NULL;
	char* _SubString = NULL;

	chdir("data/missions");
	_Dir = opendir("./");
	while((_Dirent = readdir(_Dir)) != NULL) {
		if(!strcmp(_Dirent->d_name, ".") || !strcmp(_Dirent->d_name, ".."))
			continue;
		if(LuaLoadFile(_State, _Dirent->d_name) != LUA_OK)
			goto error;
		_SubString = strrchr(_Dirent->d_name, '.');
		_TableName = alloca(sizeof(char) * (strlen(_Dirent->d_name) + 1));
		while(&_Dirent->d_name[i] != _SubString) {
			_TableName[i] = _Dirent->d_name[i];
			++i;
		}
		_TableName[i] = '\0';
		LnkLst_PushBack(_List, LoadMission(_State, _TableName));
		Log(ELOG_INFO, "Loaded mission %s", _TableName);
	}
	error:
	chdir("../..");
}

struct Mission* LoadMission(lua_State* _State, const char* _TableName) {
	const char* _Name = NULL;
	const char* _Description = NULL;
	char** _OptionNames = NULL;
	const char* _Temp = NULL;
	int i = 0;
	int _AbsIndex = 0;
	struct Mission* _Mission = NULL;

	lua_getglobal(_State, _TableName);
	_AbsIndex = LuaAbsPos(_State, -1);
	if(lua_type(_State, -1) != LUA_TTABLE) {
		luaL_error(_State, "Mission is not a table.");
		return NULL;
	}
	lua_pushstring(_State, "Options");
	lua_rawget(_State, _AbsIndex);
	if(lua_type(_State, -1) != LUA_TFUNCTION) {
		luaL_error(_State, "Mission table does not have a field named Options or is not a function.");
		return NULL;
	}
	lua_pushstring(_State, "Name");
	lua_rawget(_State, _AbsIndex);
	AddString(_State, -1, &_Name);
	lua_pushstring(_State, "Description");
	lua_rawget(_State, _AbsIndex);
	AddString(_State, -1, &_Description);
	lua_pop(_State, 3);
	if(_Name == NULL || _Description == NULL)
		return NULL;
	lua_pushstring(_State, "OptionNames");
	lua_rawget(_State, _AbsIndex);
	lua_rawlen(_State, -1);
	_OptionNames = calloc(lua_rawlen(_State, -1) + 1, sizeof(char*));
	lua_pushnil(_State);
	while(lua_next(_State, -2) != 0) {
		if(AddString(_State, -1, &_Temp) != 0) {
			_OptionNames[i] = calloc(strlen(_Temp) + 1, sizeof(char));
			strcpy(_OptionNames[i], _Temp);
		}
		lua_pop(_State, 1);
	}
	lua_pop(_State, 2);
	_OptionNames[i] = NULL;
	_Mission = (struct Mission*) malloc(sizeof(struct Mission));
	_Mission->Name = calloc(strlen(_Name) + 1, sizeof(char));
	_Mission->Description = calloc(strlen(_Description) + 1, sizeof(char));
	_Mission->OptionNames = _OptionNames;
	strcpy(_Mission->Name, _Name);
	strcpy(_Mission->Description, _Description);
	return _Mission;
}

void DestroyMission(struct Mission* _Mission) {
	free(_Mission->Name);
	free(_Mission->Description);
	free(_Mission);
}

struct MissionOption* LoadMissionOption(lua_State* _State, int _Index) {
	int _AbsIndex = LuaAbsPos(_State, _Index);
	int i = 1;
	struct Rule* _Rules[4];
	struct MissionOption* _Option = NULL;

	if(lua_type(_State, _Index) != LUA_TTABLE) {
		luaL_error(_State, "Mission option parameter is not a a table.");
		return NULL;
	}
	lua_rawgeti(_State, _AbsIndex, 1);
	if(lua_type(_State, -1) != LUA_TTABLE) {
		luaL_error(_State, "Mission option's second parameter is not a table.");
		return NULL;
	}
	for(i = 0; i < 4; ++i) {
		lua_rawgeti(_State, -1, i + 1);
		if((_Rules[i] = (struct Rule*) LuaToObject(_State, -1, "Rule")) == NULL) {
			luaL_error(_State, "Mission option's rule table contains a non rule.");
			return NULL;
		}
		lua_pop(_State, 1);
	}
	_Option = malloc(sizeof(struct MissionOption));
	_Option->SuccessCon = _Rules[0];
	_Option->SuccessRwrd = _Rules[1];
	_Option->FailureCon = _Rules[2];
	_Option->FailureRwrd = _Rules[3];
	return _Option;
}
