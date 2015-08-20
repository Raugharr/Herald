/*
 * File: Mission.c
 * Author: David Brotz
 */

#include "Mission.h"

#include "Location.h"
#include "World.h"
#include "BigGuy.h"
#include "Family.h"
#include "Person.h"

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
#include <assert.h>

#define MISSION_STACKSZ (8)

static struct {
	struct BigGuy* Triggerer;
	struct BigGuy* Stack[MISSION_STACKSZ];
	int StackSz;
} g_MissionData;

static struct Mission** g_LoadingMission = NULL;

void MissionDataClear() {
	g_MissionData.Triggerer = NULL;
	g_MissionData.StackSz = 0;
}

struct Mission* CreateMission() {
	struct Mission* _Mission = (struct Mission*) malloc(sizeof(struct Mission));

	_Mission->Name = NULL;
	_Mission->Description = NULL;
	_Mission->LuaTable = NULL;
	_Mission->OptionCt = 0;
	WorldStateClear(&_Mission->Trigger);
	return _Mission;
}

void LoadAllMissions(lua_State* _State, struct RBTree* _List) {
	int i = 0;
	DIR* _Dir = NULL;
	struct dirent* _Dirent = NULL;
	char* _TableName = NULL;
	char* _SubString = NULL;
	struct Mission* _Mission = NULL;

	chdir("data/missions");
	_Dir = opendir("./");
	lua_settop(_State, 0);
	while((_Dirent = readdir(_Dir)) != NULL) {
		if(!strcmp(_Dirent->d_name, ".") || !strcmp(_Dirent->d_name, ".."))
			continue;
		_Mission = CreateMission();
		g_LoadingMission = &_Mission;
		if(LuaLoadFile(_State, _Dirent->d_name, NULL) != LUA_OK)
			goto error;

		_SubString = strrchr(_Dirent->d_name, '.');
		_TableName = alloca(sizeof(char) * (strlen(_Dirent->d_name) + 1));
		while(&_Dirent->d_name[i] != _SubString) {
			_TableName[i] = _Dirent->d_name[i];
			++i;
		}
		_TableName[i] = '\0';
		_Mission->LuaTable = calloc(sizeof(char), strlen(_TableName) + 1);
		strcpy(_Mission->LuaTable, _TableName);
		RBInsert(_List, _Mission/*LoadMission(_State, _TableName*/);
		Log(ELOG_INFO, "Loaded mission %s", _TableName);
		i = 0;
	}
	goto end;
	error:
	free(_Mission);
	end:
	g_LoadingMission = NULL;
	chdir("../..");
}

struct Mission* LoadMission(lua_State* _State, const char* _TableName) {
	/*const char* _Name = NULL;
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
	return _Mission;*/
	return NULL;
}

void DestroyMission(struct Mission* _Mission) {
	free(_Mission->Name);
	free(_Mission->Description);
	free(_Mission->LuaTable);
	free(_Mission);
}

struct MissionOption* LoadMissionOption(lua_State* _State, int _Index) {
	/*int _AbsIndex = LuaAbsPos(_State, _Index);
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
	return _Option;*/
	return NULL;
}

int CheckMissionOption(struct GUIMessagePacket* _Packet) {
	int _Top = lua_gettop(_Packet->State);

	RuleEval(((struct Mission*)_Packet->One)->Options[_Packet->RecvPrim.Value.Int].Action);
	lua_settop(_Packet->State, _Top);
	MissionDataClear();
	return 1;
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
			g_MissionData.Triggerer = _BigGuy;
			lua_settop(_State, 0);
			lua_pushstring(_State, "MissionMenu");
			lua_createtable(_State, 0, 2);
			lua_pushstring(_State, "Mission");
			LuaCtor(_State, "Mission", _Mission);
			lua_rawset(_State, -3);
			lua_pushstring(_State, "BigGuy");
			LuaCtor(_State, "BigGuy", (struct BigGuy*)_BigGuy);
			lua_rawset(_State, -3);
			lua_pushinteger(_State, 512);
			lua_pushinteger(_State, 512);
			LuaCreateWindow(_State);
			GUIMessageCallback(_State, "Mission", CheckMissionOption, _Mission, NULL);
		}
	}
}

int MissionTreeInsert(const struct Mission* _One, const struct Mission* _Two) {
	return WorldStateAtomCmp(&_One->Trigger, &_Two->Trigger);

}

int MissionTreeSearch(const struct WorldState* _One, const struct Mission* _Two) {
	return WorldStateAtomCmp(_One, &_Two->Trigger);
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

	if(g_LoadingMission == NULL)
		return 0;
	if((*g_LoadingMission)->OptionCt >= MISSION_MAXOPTIONS)
		return luaL_error(_State, "Mission has already exceded the maximum amount of options.");

	(*g_LoadingMission)->Options[(*g_LoadingMission)->OptionCt].Name = calloc(sizeof(char), strlen(_Str) + 1);
	strcpy((*g_LoadingMission)->Options[(*g_LoadingMission)->OptionCt].Name, _Str);
	(*g_LoadingMission)->Options[(*g_LoadingMission)->OptionCt].Condition = _Condition;
	(*g_LoadingMission)->Options[(*g_LoadingMission)->OptionCt].Action = _Action;
	++(*g_LoadingMission)->OptionCt;
	return 0;
}

int LuaMissionAddTrigger(lua_State* _State) {
	const char* _Str = luaL_checkstring(_State, 1);
	int _OpCode = luaL_checkinteger(_State, 2);
	int _Value = luaL_checkinteger(_State, 3);
	int _Atom = 0;

	if(g_LoadingMission == NULL)
		return 0;
	if(_OpCode < WSOP_NOT || _OpCode > WSOP_LESSTHANEQUAL)
		return 0;
	for(_Atom = 0; _Atom < BGBYTE_SIZE; ++_Atom) {
		if(strcmp(_Str, g_BGStateStr[_Atom]) == 0) {
			goto opcodes;
		}
	}
	return 0;
	opcodes:
	WorldStateSetOpCode(&(*g_LoadingMission)->Trigger, _Atom, _OpCode);
	WorldStateSetAtom(&(*g_LoadingMission)->Trigger, _Atom, _Value);
	return 0;
}

int LuaMissionGetOwner_Aux(lua_State* _State) {
	if(g_MissionData.Triggerer == NULL)
		lua_pushnil(_State);
	else {
		LuaCtor(_State, "BigGuy", g_MissionData.Triggerer);
	}
	return 1;
}

int LuaMissionGetOwner(lua_State* _State) {
	lua_pushcfunction(_State, LuaMissionGetOwner_Aux);
	LuaRuleLuaCall(_State);
	return 1;
}

int LuaMissionGetRandomPerson_Aux(lua_State* _State) {
	int _IsUnique = 0;
	struct Settlement* _Settlement = NULL;
	struct LnkLst_Node* _Itr = NULL;
	struct BigGuy* _Guy = NULL;
	int _Ct = 0;
	int _SkipedGuys = 0;

	luaL_checktype(_State, 1, LUA_TBOOLEAN);
	if(g_MissionData.StackSz >= MISSION_STACKSZ)
		return luaL_error(_State, "LuaMissionGetRandomPerson: Stack is full.");
	_Settlement = FamilyGetSettlement(g_MissionData.Triggerer->Person->Family);
	assert(_Settlement->BigGuys.Size != 0 && "LuaMissionGetRandomPerson: Settlement has no BigGuys.");
	_IsUnique = lua_toboolean(_State, 1);
	_Itr = _Settlement->BigGuys.Front;
	_Ct = Random(0, _Settlement->BigGuys.Size);
	while(_Itr != NULL && _Ct > 0) {
		loop_start:
		_Guy = (struct BigGuy*)_Itr->Data;
		if(_Guy == g_MissionData.Triggerer) {
			++_SkipedGuys;
			goto loop_end;
		}
		if(_IsUnique != 0) {
			for(int i = 0; i < g_MissionData.StackSz; ++i) {
				if(_Guy == g_MissionData.Stack[i]) {
					++_SkipedGuys;
					goto loop_end;
				}
			}
		}
		--_Ct;
		loop_end:
		_Itr = _Itr->Next;
	}
	if(_SkipedGuys >= _Settlement->BigGuys.Size)
		goto error;
	if(_Itr == NULL && _Ct > 0) {
		_Itr = _Settlement->BigGuys.Front;
		goto loop_start;
	}
	g_MissionData.Stack[g_MissionData.StackSz] = _Guy;
	++g_MissionData.StackSz;
	LuaCtor(_State, "BigGuy", _Guy);
	return 1;
	error:
	return luaL_error(_State, "LuaMissionGetRandomPerson: No avaliable person to select.");
}

int LuaMissionGetRandomPerson(lua_State* _State) {
	lua_pushcfunction(_State, LuaMissionGetRandomPerson_Aux);
	lua_insert(_State, 1);
	LuaRuleLuaCall(_State);
	return 1;
}
