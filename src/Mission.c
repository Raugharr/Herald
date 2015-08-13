/*
 * File: Mission.c
 * Author: David Brotz
 */

#include "Mission.h"

#include "Location.h"
#include "World.h"
#include "BigGuy.h"

#include "sys/LuaCore.h"
#include "sys/Rule.h"
#include "sys/RBTree.h"
#include "sys/Log.h"
#include "video/GuiLua.h"
#include "video/Gui.h"
#include "sys/Stack.h"
#include "sys/Event.h"
#include "sys/Math.h"

#include <stdlib.h>
#include <string.h>
#include <lua/lua.h>
#include <lua/lauxlib.h>
#include <dirent.h>
#include <malloc.h>

static struct Mission** g_LoadingMission = NULL;

void LoadAllMissions(lua_State* _State, struct RBTree* _List) {
	int i = 0;
	DIR* _Dir = NULL;
	struct dirent* _Dirent = NULL;
	char* _TableName = NULL;
	char* _SubString = NULL;

	chdir("data/missions");
	_Dir = opendir("./");
	lua_settop(_State, 0);
	while((_Dirent = readdir(_Dir)) != NULL) {
		if(!strcmp(_Dirent->d_name, ".") || !strcmp(_Dirent->d_name, ".."))
			continue;
		if(LuaLoadFile(_State, _Dirent->d_name, NULL) != LUA_OK)
			goto error;
		_SubString = strrchr(_Dirent->d_name, '.');
		_TableName = alloca(sizeof(char) * (strlen(_Dirent->d_name) + 1));
		while(&_Dirent->d_name[i] != _SubString) {
			_TableName[i] = _Dirent->d_name[i];
			++i;
		}
		_TableName[i] = '\0';
		RBInsert(_List, LoadMission(_State, _TableName));
		Log(ELOG_INFO, "Loaded mission %s", _TableName);
		i = 0;
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
	struct WorldState _MissionState;

	WorldStateClear(&_MissionState);
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
	LuaGetString(_State, -1, &_Name);
	lua_pushstring(_State, "Description");
	lua_rawget(_State, _AbsIndex);
	LuaGetString(_State, -1, &_Description);
	lua_pushstring(_State, "Trigger");
	lua_rawget(_State, _AbsIndex);
	if(lua_type(_State, -1) != LUA_TTABLE) {
		luaL_error(_State, "Mission trigger is not a table.");
		return NULL;
	}
	lua_pushnil(_State);
	while(lua_next(_State, -2) != 0) {
		if(lua_type(_State, -2) != LUA_TSTRING) {
			Log(ELOG_WARNING, "Mission Trigger contains a nonstring element.");
			goto stateloop_end;
		}
		for(i = 0; i < BGBYTE_SIZE; ++i) {
			if(strcmp(g_BGStateStr[i], lua_tostring(_State, -2)) == 0) {
				WorldStateSetAtom(&_MissionState, i, lua_toboolean(_State, -1));
				break;
			}
		}
		stateloop_end:
		lua_pop(_State, 1);
	}
	lua_pop(_State, 4);
	if(_Name == NULL || _Description == NULL)
		return NULL;
	lua_pushstring(_State, "OptionNames");
	lua_rawget(_State, _AbsIndex);
	lua_rawlen(_State, -1);
	_OptionNames = calloc(lua_rawlen(_State, -1) + 1, sizeof(char*));
	lua_pushnil(_State);
	while(lua_next(_State, -2) != 0) {
		if(LuaGetString(_State, -1, &_Temp) != 0) {
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
	_Mission->LuaTable = calloc(strlen(_TableName) + 1, sizeof(char));
	_Mission->Trigger = _MissionState;
	strcpy(_Mission->Name, _Name);
	strcpy(_Mission->Description, _Description);
	strcpy(_Mission->LuaTable, _TableName);
	return _Mission;
}

void DestroyMission(struct Mission* _Mission) {
	free(_Mission->Name);
	free(_Mission->Description);
	free(_Mission->LuaTable);
	free(_Mission);
}

struct MissionOption* LoadMissionOption(lua_State* _State, int _Index) {
	int _AbsIndex = LuaAbsPos(_State, _Index);
	int i = 1;
	struct Rule* _Rules[4];
	struct MissionOption* _Option = NULL;

	if(lua_type(_State, _AbsIndex) != LUA_TTABLE) {
		luaL_error(_State, "Mission option parameter is not a a table.");
		return NULL;
	}
	for(i = 0; i < 4; ++i) {
		lua_rawgeti(_State, _AbsIndex, i + 1);

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

int CheckMissionOption(lua_State* _State, void* _None) {
	struct MissionOption* _Mission = LoadMissionOption(_State, -1);

	if(_Mission == NULL)
		return 0;
	if(RuleEval(_Mission->SuccessCon) != 0)
		RuleEval(_Mission->SuccessRwrd);
	else if(RuleEval(_Mission->FailureCon))
		RuleEval(_Mission->FailureRwrd);
	return 1;
}

void GenerateMissions_Aux(struct BigGuy* _BigGuy, lua_State* _State, struct Mission* _Mission, const struct RBTree* _Missions) {
	if(_BigGuy->IsDirty == 0)
		return;
	if((_Mission = RBSearch(_Missions, &_BigGuy->State)) == NULL)
		return;
	if(g_GameWorld.Player == _BigGuy/*strcmp((char*)g_GUIStack.Top->Data, "MissionMenu") != 0*/) {
		lua_settop(_State, 0);
		lua_pushstring(_State, "MissionMenu");
		lua_createtable(_State, 0, 2);
		lua_pushstring(_State, "Mission");
		lua_getglobal(_State, _Mission->LuaTable);
		lua_rawset(_State, -3);
		lua_pushstring(_State, "BigGuy");
		LuaCtor(_State, "BigGuy", (struct BigGuy*)_BigGuy);
		lua_rawset(_State, -3);
		lua_pushinteger(_State, 512);
		lua_pushinteger(_State, 512);
		LuaCreateWindow(_State);
		GUIMessageCallback(_State, "Mission", (int(*)(void*, void*))CheckMissionOption, _State, NULL);
	}
}

void FooTwo(int a, int b) {

}

void GenerateMissions(lua_State* _State, const struct RBTree* _BigGuys, const struct RBTree* _Missions) {
	struct Mission* _Mission = NULL;
	struct RBItrStack _Stack[_BigGuys->Size];
	struct BigGuy* _BigGuy = NULL;
	if(_BigGuys->Table == NULL)
		return;
	RBDepthFirst(_BigGuys->Table, _Stack);
	for(int i = 0; i < _BigGuys->Size; ++i, _BigGuy->IsDirty = 0) {
		_BigGuy = ((struct RBNode*) _Stack[i].Node)->Data;
		if(_BigGuy->IsDirty == 0)
			continue;
		if((_Mission = RBSearch(_Missions, &_BigGuy->State)) == NULL)
			continue;
		if(g_GameWorld.Player == _BigGuy/*strcmp((char*)g_GUIStack.Top->Data, "MissionMenu") != 0*/) {
			lua_settop(_State, 0);
			lua_pushstring(_State, "MissionMenu");
			lua_createtable(_State, 0, 2);
			lua_pushstring(_State, "Mission");
			lua_getglobal(_State, _Mission->LuaTable);
			lua_rawset(_State, -3);
			lua_pushstring(_State, "BigGuy");
			LuaCtor(_State, "BigGuy", (struct BigGuy*)_BigGuy);
			lua_rawset(_State, -3);
			lua_pushinteger(_State, 512);
			lua_pushinteger(_State, 512);
			LuaCreateWindow(_State);
			//LuaSetMenu(_State);
			GUIMessageCallback(_State, "Mission", (int(*)(void*, void*))CheckMissionOption, _State, NULL);
		}
	}
}

int MissionTreeInsert(const struct Mission* _One, const struct Mission* _Two) {
	return WorldStateCmp(&_One->Trigger, &_Two->Trigger);

}

int MissionTreeSearch(const struct WorldState* _One, const struct Mission* _Two) {
	return WorldStateCmp(_One, &_Two->Trigger);
}

int LuaMissionSetName(lua_State* _State) {
	const char* _Str = luaL_checkstring(_State, 1);

	if(g_LoadingMission == NULL)
		return 0;
	if((*g_LoadingMission)->Name != NULL)
		return luaL_error(_State, "Mission name is already set %s", (*g_LoadingMission)->Name);
	(*g_LoadingMission)->Name = calloc(sizeof(char), strlen(_Str) + 1);
	strcpy((*g_LoadingMission)->Name, _Str);
	return 0;
}

int LuaMissionSetDesc(lua_State* _State) {
	const char* _Str = luaL_checkstring(_State, 1);

	if(g_LoadingMission == NULL)
		return 0;
	if((*g_LoadingMission)->Description != NULL)
		return luaL_error(_State, "Mission description is already set %s", (*g_LoadingMission)->Description);
	(*g_LoadingMission)->Description = calloc(sizeof(char), strlen(_Str) + 1);
	strcpy((*g_LoadingMission)->Description, _Str);
	return 0;
}

int LuaMissionAddOption(lua_State* _State) {
	const char* _Str = luaL_checkstring(_State, 1);
	struct Rule* _Condition = LuaCheckClass(_State, 2, "Rule");
	struct Rule* _Action = LuaCheckClass(_State, 3, "Rule");


	return 0;
}
