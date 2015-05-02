/*
 * File: Mission.c
 * Author: David Brotz
 */

#include "Mission.h"

#include "Location.h"
#include "World.h"
#include "BigGuy.h"

#include "sys/LuaHelper.h"
#include "sys/Rule.h"
#include "sys/RBTree.h"
#include "sys/Log.h"
#include "video/GuiLua.h"
#include "video/Gui.h"
#include "sys/Stack.h"
#include "sys/Event.h"

#include <stdlib.h>
#include <string.h>
#include <lua/lua.h>
#include <lua/lauxlib.h>
#include <dirent.h>
#include <malloc.h>

void LoadAllMissions(lua_State* _State, struct RBTree* _List) {
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
	struct Rule* _RuleEvent = NULL;

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
	lua_pushstring(_State, "Trigger");
	lua_rawget(_State, _AbsIndex);
	if((_RuleEvent = (struct Rule*) LuaToObject(_State, -1, "Rule")) == NULL) {
		luaL_error(_State, "Mission trigger rule is a non rule.");
		return NULL;
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
	_Mission->LuaTable = calloc(strlen(_TableName) + 1, sizeof(char));
	_Mission->Trigger = _RuleEvent;
	_Mission->Next = NULL;
	strcpy(_Mission->Name, _Name);
	strcpy(_Mission->Description, _Description);
	strcpy(_Mission->LuaTable, _TableName);
	return _Mission;
}

void DestroyMission(struct Mission* _Mission) {
	free(_Mission->Name);
	free(_Mission->Description);
	free(_Mission->LuaTable);
	free(_Mission->Trigger);
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

void GenerateMissions(lua_State* _State, const struct Event* _Event, const struct RBTree* _BigGuys, const struct RBTree* _Missions) {
	struct Mission* _Mission = NULL;
	struct BigGuy* _BGItr = NULL;

	_Mission = RBSearch(_Missions, _Event);
	while(_Mission != NULL) {
			_BGItr = RBSearch(_BigGuys, _Mission);
			if(_BGItr != NULL && strcmp((char*)g_GUIStack.Top->Data, "MissionMenu") != 0) {
				lua_settop(_State, 0);
				lua_pushstring(_State, "MissionMenu");
				lua_createtable(_State, 0, 2);
				lua_pushstring(_State, "Mission");
				lua_getglobal(_State, _Mission->LuaTable);
				lua_rawset(_State, -3);
				lua_pushstring(_State, "Settlement");
				if(_Event->Location->Type == ELOC_SETTLEMENT )
					LuaCtor(_State, "Settlement", (struct Settlement*)_Event->Location);
				lua_rawset(_State, -3);
				LuaSetMenu(_State);
				GUIMessageCallback(_State, "Mission", (int(*)(void*, void*))CheckMissionOption, _State, NULL);
		}
			_Mission = _Mission->Next;
	}
	/*while(_SettleItr != NULL) {
		_MissionItr = _Missions->Front;
		_Settlement = (struct Settlement*)_SettleItr->Data;
		while(_MissionItr != NULL) {
			_Mission = (struct Mission*)_MissionItr->Data;
			lua_getglobal(_State, _Mission->LuaTable);
			lua_pushstring(_State, "Trigger");
			lua_rawget(_State, -2);
			lua_pushnil(_State);
			LuaCallFunc(_State, 1, 1, 0);
			if((_Rule = (struct Rule*)LuaToObject(_State, -1, "Rule")) == NULL) {
				Log(ELOG_WARNING, "%s's trigger does not return a rule object.");
				goto mission_end;
			}
			if(RuleEval(_Rule) == 1) {
				if(_Settlement->Leader == g_Player) {
					lua_settop(_State, 0);
					lua_pushstring(_State, "MissionMenu");
					lua_createtable(_State, 0, 2);
					lua_pushstring(_State, "Mission");
					lua_getglobal(_State, _Mission->LuaTable);
					lua_rawset(_State, -3);
					lua_pushstring(_State, "Settlement");
					LuaCtor(_State, "Settlement", _Settlement);
					lua_rawset(_State, -3);
					LuaSetMenu(_State);
					GUIMessageCallback(_State, "Mission", (int(*)(void*, void*))CheckMissionOption, _State, NULL);
				}
			}
			mission_end:
			_MissionItr = _MissionItr->Next;
		}
		_SettleItr = _SettleItr->Next;
	}*/
}

int MissionTreeInsert(const struct Mission* _One, const struct Mission* _Two) {
	return RuleEventCompare(_One->Trigger, _Two->Trigger);
}

int MissionTreeSearch(const struct Event* _One, const struct Mission* _Two) {
	if(_Two->Trigger->Type != RULE_EVENT)
		return -1;
	return _One->Type - ((struct RuleEvent*)_Two->Trigger)->Event;
}
