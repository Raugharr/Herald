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
#include "Date.h"

#include "sys/LuaCore.h"
#include "sys/Rule.h"
#include "sys/RBTree.h"
#include "sys/Log.h"
#include "sys/Stack.h"
#include "sys/Event.h"
#include "sys/Math.h"
#include "sys/LinkedList.h"
#include "sys/MemoryPool.h"
#include "sys/GenIterator.h"

#include "video/GuiLua.h"
#include "video/Gui.h"

#include <stdlib.h>
#include <string.h>
#include <lua/lua.h>
#include <lua/lauxlib.h>
#include <dirent.h>
#include <malloc.h>
#include <assert.h>

#define MISSION_STACKSZ (8)
#define USEDMISSION_ARRAYSZ (10)
#define MISSION_LUASTR ("InitMission")
#define MISSION_QELEMENTS (2048)
#define USEDMISSION_SIZE (10000)

static struct MemoryPool* g_MissionQueuePool = NULL;
static struct MemoryPool* g_MissionUsed = NULL;

struct MissionData {
	struct BigGuy* Triggerer;
	struct BigGuy* Stack[MISSION_STACKSZ];
	int StackSz;
	struct {
		const char* Key;
		void* Pair;
		int Type;
	} KeyVal[8];
};

static struct MissionData* g_MissionData = NULL;

/*
 * Container used to search for a BigGuy that has a trigger that is equal to TriggerMask
 */
struct UsedMissionSearch {
	const struct BigGuy* Guy;
	const struct WorldState* Trigger;
	int Type;
};

struct QueuedMission {
	struct BigGuy* Triggerer;
	struct Mission* Mission;
	DATE FireDay; //The day this mission will fire.
	struct QueuedMission* Prev;
	struct QueuedMission* Next;
};

void MissionAddTriggerSetOp(lua_State* _State, const char* _Str, const char** _StateStr, int _StateSz, struct WorldState* _WorldState, int _Value, int _OpCode) {
	int _Atom = 0;

	for(_Atom = 0; _Atom < _StateSz; ++_Atom) {
		if(strcmp(_Str, _StateStr[_Atom]) == 0) {
			goto opcodes;
		}
	}
	return (void) luaL_error(_State, "%s is not a valid trigger value.", _Str);
	opcodes:
	WorldStateSetOpCode(_WorldState, _Atom, _OpCode);
	WorldStateSetAtom(_WorldState, _Atom, _Value);
}

struct MissionData* CreateMissionData(struct BigGuy* _Triggerer) {
	struct MissionData* _MissionData = (struct MissionData*) malloc(sizeof(struct MissionData));

	_MissionData->Triggerer = _Triggerer;
	LnkLstPushBack(&g_GameWorld.MissionData, _MissionData);
	return _MissionData;
}

void DestroyMissionData(struct MissionData* _MissionData) {
	free(_MissionData);
}

const struct QueuedMission* UsedMissionEarliestDate(const struct  QueuedMission* _Queued) {
	const struct QueuedMission*  _Best = _Queued;

	_Queued = _Queued->Next;
	while(_Queued != NULL) {
		if(DateCmp(_Best->FireDay, _Queued->FireDay) > 0)
			_Best = _Queued;
		_Queued = _Queued->Next;
	}
	return _Best;
}

void MissionInsert(struct MissionEngine* _Engine, struct Mission* _Mission) {
	struct LinkedList* _List = RBSearch(&_Engine->Missions, _Mission);

	if(WorldStateEmpty(&_Mission->Trigger) != 0)
		return;
	if(RBSearch(&_Engine->MissionId, _Mission) != NULL) {
		Log(ELOG_WARNING, "Mission cannot be loaded id %i is already in use.", _Mission->Id);
		return;
	}
	RBInsert(&_Engine->MissionId, _Mission);
	if(_List == NULL) {
		_List = CreateLinkedList();
		LnkLstPushBack(_List, _Mission);
		RBInsert(&_Engine->Missions, _List);
	} else {
		LnkLstPushBack(_List, _Mission);
	}

	for(int i = 0; i < WORLDSTATESZ; ++i) {
		for(int j = 0; j < sizeof(WorldState_t); ++j) {
			if(WSAtomDontCare(&_Mission->Trigger, i , j) == 0) {
				if(_Mission->Type == MISSION_BGTYPE)
					LnkLstPushBack(&_Engine->MissionList[WSAtomIdxToInt(i, j)], _Mission);
				else
					LnkLstPushBack(&_Engine->Categories[MISSION_TYPETOCAT(_Mission->Type)].MissionList[WSAtomIdxToInt(i, j)], _Mission);
				return;
			}
		}
	}
}

void MissionDataClear(struct MissionData* _Data) {
	struct LnkLst_Node* _Itr = NULL;

	_Data->Triggerer = NULL;
	_Data->StackSz = 0;
	DestroyMissionData(_Data);
	_Itr = g_GameWorld.MissionData.Front;
	while(_Itr != NULL) {
		if(_Itr->Data == _Data) {
			LnkLstRemove(&g_GameWorld.MissionData, _Itr);
			break;
		}
		_Itr = _Itr->Next;
	}
	if(g_GameWorld.MissionData.Size == 0)
		g_MissionData = NULL;
	else
		g_MissionData = (struct MissionData*) g_GameWorld.MissionData.Back->Data;
}

struct Mission* CreateMission() {
	struct Mission* _Mission = (struct Mission*) malloc(sizeof(struct Mission));

	_Mission->Type = 0;
	_Mission->Name = NULL;
	_Mission->Description = NULL;
	_Mission->LuaTable = NULL;
	_Mission->OptionCt = 0;
	_Mission->OnTrigger = NULL;
	WorldStateClear(&_Mission->Trigger);
	return _Mission;
}

void InitMissions(void) {
	g_MissionQueuePool = CreateMemoryPool(sizeof(struct QueuedMission), MISSION_QELEMENTS);
	g_MissionUsed = CreateMemoryPool(sizeof(struct QueuedMission), USEDMISSION_SIZE);
}

void QuitMissions(void) {
	DestroyMemoryPool(g_MissionQueuePool);
	DestroyMemoryPool(g_MissionUsed);
}

int UsedMissionInsert(const struct QueuedMission* _One, const struct QueuedMission* _Two) {
	int _Cmp = _One->Mission->Type - _Two->Mission->Type;

	if(_Cmp != 0)
		return _Cmp;
	_Cmp = _One->Triggerer->Id - _Two->Triggerer->Id;
	return (_Cmp == 0) ? (WSDntCrCmp(&_One->Mission->Trigger, &_Two->Mission->Trigger)) : (_Cmp);
}

int UsedMissionSearch(const struct UsedMissionSearch* _One, const struct QueuedMission* _Two) {
	int _Cmp = _One->Type - _Two->Mission->Type;

	if(_Cmp != 0)
		return _Cmp;
	_Cmp = _One->Guy->Id - _Two->Triggerer->Id;
	return (_Cmp == 0) ? (WSDntCrCmp(_One->Trigger, &_Two->Mission->Trigger)) : (_Cmp);
}

void LoadAllMissions(lua_State* _State, struct MissionEngine* _Engine) {
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
		lua_pushvalue(_State, LUA_REGISTRYINDEX);
		lua_pushstring(_State, MISSION_LUASTR);
		lua_pushlightuserdata(_State, _Mission);
		lua_rawset(_State, -3);
		//FIXME: If one Mission cannot be loaded then all missions after it will not be loaded either.
		if(LuaLoadFile(_State, _Dirent->d_name, NULL) != LUA_OK) {
			lua_pushvalue(_State, LUA_REGISTRYINDEX);
			lua_pushstring(_State, MISSION_LUASTR);
			lua_pushnil(_State);
			lua_rawset(_State, -3);
			goto error;
		}
		lua_pushvalue(_State, LUA_REGISTRYINDEX);
		lua_pushstring(_State, MISSION_LUASTR);
		lua_pushnil(_State);
		lua_rawset(_State, -3);
		_SubString = strrchr(_Dirent->d_name, '.');
		_TableName = alloca(sizeof(char) * (strlen(_Dirent->d_name) + 1));
		while(&_Dirent->d_name[i] != _SubString) {
			_TableName[i] = _Dirent->d_name[i];
			++i;
		}
		_TableName[i] = '\0';
		_Mission->LuaTable = calloc(sizeof(char), strlen(_TableName) + 1);
		strcpy(_Mission->LuaTable, _TableName);
		MissionInsert(_Engine, _Mission);
		//RBInsert(_List, _Mission/*LoadMission(_State, _TableName*/);
		Log(ELOG_INFO, "Loaded mission %s", _TableName);
		i = 0;
	}
	goto end;
	error:
	free(_Mission);
	end:
	chdir("../..");
}

void DestroyMission(struct Mission* _Mission) {
	free(_Mission->Name);
	free(_Mission->Description);
	free(_Mission->LuaTable);
	free(_Mission);
}

/*
 * Returns the next atom that is not in the TriggerMask and is cared by the WorldState _State.
 */
const struct LinkedList* MissionTriggerList(struct MissionEngine* _Engine, struct LinkedList* _MissionList, const struct WorldState* _State, int* _TriggerMask, int* _Atom) {
	int i = 0;
	int j = 0;

	if((*_Atom) >= WORLDSTATE_ATOMSZ)
		return NULL;
	if((*_Atom != -1)) {
		++(*_Atom);
		i = (*_Atom) / WORLDSTATESZ;
		j = (*_Atom) % WORLDSTATESZ;
	}
	for(; i < WORLDSTATESZ; ++i) {
		for(; j < sizeof(WorldState_t); ++j) {
			if(WSAtomDontCare(_State, i, j) == 0 && (((*_TriggerMask) & (1 << (*_Atom))) == 0)) {
				(*_Atom) = WSAtomIdxToInt(i, j);
				(*_TriggerMask) = (*_TriggerMask) | (1 << (*_Atom));
				goto loop_exit;
			}
		}
		j = 0;
	}
	if((*_Atom) == -1)
		return NULL;
	loop_exit:
	return &_MissionList[(*_Atom)];
}

int MissionCompareState(const struct WorldState* _State, const struct Mission* _Mission) {
	return 0;
}

void MissionSelect(struct MissionEngine* _Engine, void* _Object, struct BigGuy* _Owner, const struct WorldState* _State,
		int* _TriggerMask, int _Type, struct LinkedList* _MissionList) {
	int _Atom = -1;
	int _Rnd = 0;
	const struct LinkedList* _List = NULL;
	const struct LnkLst_Node* _Itr = NULL;
	struct QueuedMission* _Used = NULL;
	struct QueuedMission* _Element = NULL;
	struct Mission* _Mission = NULL;
	struct UsedMissionSearch _Search;

	missiontype_select:
	if((_List = MissionTriggerList(_Engine, _MissionList, _State, _TriggerMask, &_Atom)) == NULL || _Atom >= WORLDSTATE_ATOMSZ)
		return;
	if(_List->Size == 0)
		goto missiontype_select;
	_Itr = _List->Front;
	/*
	 * Check if a similar mission type has already been fired recently.
	 */
	_Search.Guy = _Owner;
	_Search.Trigger = &((struct Mission*)_List->Front->Data)->Trigger;
	_Search.Type = _Type;
	/*
	 * FIXME: Does this mean the statement near the end of the function comparing _Used to NULL will always be true?
	 */
	if((_Used = RBSearch(&_Engine->UsedMissionTree, &_Search)) != NULL) {
		goto missiontype_select;
	}
	/*
	 * Pick a random Mission that is true when compared to _BigGuy's State.
	 */
	_Rnd = Random(0, _List->Size - 1);
	for(int i = 0; i < _Rnd; ++i) _Itr = _Itr->Next;
	_Mission = (struct Mission*)_Itr->Data;
	//NOTE: Can _Element and _Used be combined into one variable? The parameters are very similar.
	_Element = MemPoolAlloc(g_MissionQueuePool);
	_Element->Mission = _Mission;
	_Element->Triggerer = _Owner;
	_Element->FireDay = DateAddInt(g_GameWorld.Date, _Mission->MeanTime);
	BinaryHeapInsert(&_Engine->MissionQueue, _Element);
	/*
	 * Now that the Mission has been picked check if _Used is NULL create a UsedMission with _Mission as the first element of the queue.
	 */
	if(_Used == NULL) {
		_Used = MemPoolAlloc(g_MissionUsed);
		_Used->Triggerer = _Owner;
		_Used->Mission = _Mission;
		_Used->FireDay = _Element->FireDay;
		_Used->Next = NULL;
		_Used->Prev = NULL;
		RBInsert(&_Engine->UsedMissionTree, _Used);
	} else {
		struct QueuedMission* _Temp = MemPoolAlloc(g_MissionUsed);
		_Temp->Triggerer = _Owner;
		_Temp->Mission = _Mission;
		_Temp->FireDay = _Element->FireDay;
		_Temp->Prev = _Used;
		_Temp->Next = NULL;
		_Used->Next = _Temp;
	}
}

/*
 * Checks all dirty BigGuys in _BigGuys if they trigger a mission in _Missions then sets all BigGuys dirty state to false.
 */
void GenerateMissions(lua_State* _State, const struct RBTree* _BigGuys, struct MissionEngine* _Engine) {
	struct RBItrStack _Stack[_BigGuys->Size];
	struct BigGuy* _BigGuy = NULL;
	struct GenIterator* _Itr = NULL;
	struct MissionCat* _Category = NULL;
	void* _Obj = NULL;

	if(_BigGuys->Table == NULL)
		return;
	RBDepthFirst(_BigGuys->Table, _Stack);
	for(int i = 0; i < _BigGuys->Size; ++i, _BigGuy->IsDirty = 0) {
		_BigGuy = ((struct RBNode*) _Stack[i].Node)->Data;
		MissionSelect(_Engine, _BigGuy, _BigGuy, &_BigGuy->State, &_BigGuy->TriggerMask, 0, g_MissionEngine.MissionList);
	}

	for(int i = 0; i < MISSIONCAT_SIZE; ++i) {
		_Category = &_Engine->Categories[i];
		if(_Category->ListIsEmpty(_Category->List) != 0)
			continue;
		_Itr = _Category->CreateItr(_Category->List);
		while(_Itr->HasNext(_Itr) != 0) {
			_Obj = _Itr->NextObj(_Itr);
			MissionSelect(_Engine, _Obj, _Category->GetOwner(_Obj), _Category->GetState(_Obj), _Category->GetTriggerMask(_Obj), i + 1, _Category->MissionList);
		}
		_Category->DestroyItr(_Itr);
	}
}

void MissionCheckOption(struct lua_State* _State, struct Mission* _Mission, struct MissionData* _Data, int _Option) {
	int _Top = lua_gettop(_State);

	if(_Option < 0 || _Option >= _Mission->OptionCt)
		return;
	g_MissionData = _Data;
	RuleEval(_Mission->Options[_Option].Action);
	lua_settop(_State, _Top);
	MissionDataClear(_Data);
}

void MissionCall(lua_State* _State, struct Mission* _Mission, struct BigGuy* _Guy) {
	if(g_GameWorld.Player == _Guy) {
		struct MissionData* _Data = CreateMissionData(_Guy);

		lua_settop(_State, 0);
		lua_pushstring(_State, "MissionMenu");
		lua_createtable(_State, 0, 3);
		lua_pushstring(_State, "Mission");
		LuaCtor(_State, "Mission", _Mission);
		lua_rawset(_State, -3);
		lua_pushstring(_State, "BigGuy");
		LuaCtor(_State, "BigGuy", _Guy);
		lua_rawset(_State, -3);
		lua_pushstring(_State, "Data");
		lua_pushlightuserdata(_State, _Data);
		lua_rawset(_State, -3);
		lua_pushinteger(_State, 512);
		lua_pushinteger(_State, 512);
		g_MissionData = _Data;
		LuaCreateWindow(_State);
		if(_Mission->OnTrigger != NULL)
			RuleEval(_Mission->OnTrigger);
	}
}

void DestroyMissionEngine(struct MissionEngine* _Engine) {
	free(_Engine->Missions.Table);
	free(_Engine->MissionQueue.Table);
	free(_Engine->UsedMissionQueue.Table);
	free(_Engine->UsedMissionTree.Table);
}

void MissionEngineThink(struct MissionEngine* _Engine, lua_State* _State, const struct RBTree* _BigGuys) {
	struct QueuedMission* _Mission = NULL;
	const struct QueuedMission* _Best = NULL;

	GenerateMissions(_State, _BigGuys, _Engine);
	while((_Mission = BinaryHeapTop(&_Engine->MissionQueue)) != NULL && _Mission->FireDay == g_GameWorld.Date) {
		struct UsedMissionSearch _Search;

		_Search.Guy = _Mission->Triggerer;
		_Search.Trigger = &_Mission->Mission->Trigger;
		_Search.Type = _Mission->Mission->Type;
		_Mission = RBSearch(&_Engine->UsedMissionTree, &_Search);
		MissionCall(_State, _Mission->Mission, _Mission->Triggerer);
		BinaryHeapPop(&_Engine->MissionQueue);
		_Mission->FireDay = DateAddInt(_Mission->FireDay, 30);
		BinaryHeapInsert(&_Engine->UsedMissionQueue, _Mission);
	}
	while((_Mission = BinaryHeapTop(&_Engine->UsedMissionQueue)) != NULL) {
		_Best = UsedMissionEarliestDate(_Mission);
		if(DateCmp(_Best->FireDay, g_GameWorld.Date) < 0) {
			if(_Best->Next != NULL && _Best->Prev != NULL) {
				_Best->Prev->Next = _Best->Next;
				_Best->Next->Prev = _Best->Prev;
			} else if(_Best->Next == NULL && _Best->Prev != NULL) {
				_Best->Prev->Next = NULL;
			} else if(_Best->Next != NULL && _Best->Prev == NULL) {
				_Best->Next->Prev = NULL;
			} else {
				RBDelete(&_Engine->UsedMissionTree, _Mission->Triggerer);
			}
			BinaryHeapPop(&_Engine->UsedMissionQueue);
		} else
			break;
	}
}

int MissionIdInsert(const int* _One, const struct Mission* _Two) {
	return (*_One) - _Two->Id;
}

int MissionIdSearch(const int* _Id, const struct Mission* _Mission) {
	return (*_Id) - _Mission->Id;
}

int MissionStateCmp(const struct WorldState* _One, const struct WorldState* _Two) {
	int _DontCare = 0;

	for(int i = 0; i < WORLDSTATESZ; ++i) {
		for(int j = 0; j < sizeof(WorldState_t); ++j) {
			_DontCare = WSAtomDontCare(_Two, i, j);
			if(_DontCare == 0 && ((_DontCare & WSAtomDontCare(_One, i, j)) != 0)) {
				return -1;
			} else {
				if(WorldStateTruthAtom(_One, _Two, (i + 1) * j) == 0)
					return WSToByte(_One, i, j) - WSToByte(_Two, i, j);
			}
		}
	}
	return 0;
}

int MissionTreeInsert(const struct Mission* _One, const struct LinkedList* _Two) {
	const struct WorldState* _State = &((struct Mission*)_Two->Front->Data)->Trigger;

	return MissionStateCmp(&_One->Trigger, _State);
}

int MissionTreeSearch(const struct WorldState* _One, const struct LinkedList* _Two) {
	const struct WorldState* _State = &((struct Mission*)_Two->Front->Data)->Trigger;

	return MissionStateCmp(_One, _State);
}

int MissionHeapInsert(const struct QueuedMission* _One, const struct QueuedMission* _Two) {
	return DateCmp(_Two->FireDay, _One->FireDay);
}

int UsedMissionHeapInsert(const struct QueuedMission* _One, const struct QueuedMission* _Two) {
	return DateCmp(_One->FireDay, _Two->FireDay);
}

int LuaMissionSetName(lua_State* _State) {
	const char* _Str = luaL_checkstring(_State, 1);
	struct Mission* _Mission = NULL;

	lua_pushvalue(_State, LUA_REGISTRYINDEX);
	lua_pushstring(_State, MISSION_LUASTR);
	lua_rawget(_State, -2);
	_Mission = lua_touserdata(_State, -1);
	if(_Mission == NULL)
		return 0;
	if(_Mission->Name != NULL)
		return luaL_error(_State, "Mission name is already set %s", _Mission->Name);
	_Mission->Name = calloc(sizeof(char), strlen(_Str) + 1);
	strcpy(_Mission->Name, _Str);
	return 0;
}

int LuaMissionSetDesc(lua_State* _State) {
	const char* _Str = luaL_checkstring(_State, 1);
	struct Mission* _Mission = NULL;

	lua_pushvalue(_State, LUA_REGISTRYINDEX);
	lua_pushstring(_State, MISSION_LUASTR);
	lua_rawget(_State, -2);
	_Mission = lua_touserdata(_State, -1);
	if(_Mission == NULL)
		return 0;
	if(_Mission->Description != NULL)
		return luaL_error(_State, "Mission description is already set %s", _Mission->Description);
	_Mission->Description = calloc(sizeof(char), strlen(_Str) + 1);
	strcpy(_Mission->Description, _Str);
	return 0;
}

int LuaMissionAddOption(lua_State* _State) {
	const char* _Str = luaL_checkstring(_State, 1);
	struct Rule* _Condition = LuaCheckClass(_State, 2, "Rule");
	struct Rule* _Action = LuaCheckClass(_State, 3, "Rule");
	struct Mission* _Mission = NULL;

	lua_pushvalue(_State, LUA_REGISTRYINDEX);
	lua_pushstring(_State, MISSION_LUASTR);
	lua_rawget(_State, -2);
	_Mission = lua_touserdata(_State, -1);
	if(_Mission == NULL)
		return 0;
	if(_Mission->OptionCt >= MISSION_MAXOPTIONS)
		return luaL_error(_State, "Mission has already exceded the maximum amount of options.");

	_Mission->Options[_Mission->OptionCt].Name = calloc(sizeof(char), strlen(_Str) + 1);
	strcpy(_Mission->Options[_Mission->OptionCt].Name, _Str);
	_Mission->Options[_Mission->OptionCt].Condition = _Condition;
	_Mission->Options[_Mission->OptionCt].Action = _Action;
	++_Mission->OptionCt;
	return 0;
}

int LuaMissionAddTrigger(lua_State* _State) {
	const char* _Name = luaL_checkstring(_State, 1);
	const char* _Str = luaL_checkstring(_State, 2);
	int _OpCode = luaL_checkinteger(_State, 3);
	int _Value = luaL_checkinteger(_State, 4);
	struct Mission* _Mission = NULL;

	lua_pushvalue(_State, LUA_REGISTRYINDEX);
	lua_pushstring(_State, MISSION_LUASTR);
	lua_rawget(_State, -2);
	_Mission = lua_touserdata(_State, -1);
	if(_Mission == NULL)
		return 0;
	if(_OpCode < WSOP_NOT || _OpCode > WSOP_LESSTHANEQUAL)
		return luaL_error(_State, "%d is an invalid trigger op code.", _OpCode);
	if(strcmp(_Name, "BigGuy") != 0) {
		for(int i = 0; i < MISSIONCAT_SIZE; ++i)
			if(strcmp(_Name, g_MissionEngine.Categories[i].Name) == 0) {
				MissionAddTriggerSetOp(_State, _Str, g_MissionEngine.Categories[i].StateStr, g_MissionEngine.Categories[i].StateSz, &_Mission->Trigger, _Value, _OpCode);
				_Mission->Type = MISSION_TYPE(i);
				return 0;
			}
	} else {
		MissionAddTriggerSetOp(_State, _Str, g_BGStateStr, BGBYTE_SIZE, &_Mission->Trigger, _Value, _OpCode);
		_Mission->Type = MISSION_BGTYPE;
		return 0;
	}
	return luaL_error(_State, "%s: is not a valid trigger type.", _Name);
}

int LuaMissionGetOwner_Aux(lua_State* _State) {
	if(g_MissionData->Triggerer == NULL)
		lua_pushnil(_State);
	else {
		LuaCtor(_State, "BigGuy", g_MissionData->Triggerer);
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
	if(g_MissionData->StackSz >= MISSION_STACKSZ)
		return luaL_error(_State, "LuaMissionGetRandomPerson: Stack is full.");
	_Settlement = FamilyGetSettlement(g_MissionData->Triggerer->Person->Family);
	assert(_Settlement->BigGuys.Size != 0 && "LuaMissionGetRandomPerson: Settlement has no BigGuys.");
	_IsUnique = lua_toboolean(_State, 1);
	_Itr = _Settlement->BigGuys.Front;
	_Ct = Random(0, _Settlement->BigGuys.Size);
	while(_Itr != NULL && _Ct > 0) {
		loop_start:
		_Guy = (struct BigGuy*)_Itr->Data;
		if(_Guy == g_MissionData->Triggerer) {
			++_SkipedGuys;
			goto loop_end;
		}
		if(_IsUnique != 0) {
			for(int i = 0; i < g_MissionData->StackSz; ++i) {
				if(_Guy == g_MissionData->Stack[i]) {
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
	g_MissionData->Stack[g_MissionData->StackSz] = _Guy;
	++g_MissionData->StackSz;
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

int LuaMissionSetMeanTime(lua_State* _State) {
	struct Mission* _Mission = NULL;
	int _MeanTime = luaL_checkinteger(_State, 1);

	lua_pushvalue(_State, LUA_REGISTRYINDEX);
	lua_pushstring(_State, MISSION_LUASTR);
	lua_rawget(_State, -2);
	_Mission = lua_touserdata(_State, -1);
	if(_Mission == NULL)
		return 0;
	_Mission->MeanTime = _MeanTime;
	return 0;
}

int LuaMissionSetId(lua_State* _State) {
	struct Mission* _Mission = NULL;
	int _Id = luaL_checkinteger(_State, 1);

	lua_pushvalue(_State, LUA_REGISTRYINDEX);
	lua_pushstring(_State, MISSION_LUASTR);
	lua_rawget(_State, -2);
	_Mission = lua_touserdata(_State, -1);
	if(_Mission == NULL)
		return 0;
	if(RBSearch(&g_MissionEngine.MissionId, &_Id) != NULL) {
		return luaL_error(_State, "Cannot load mission with id %d. Id is already in use.", _Id);
	}
	_Mission->Id = _Id;
	return 0;
}

int LuaMissionOnTrigger(lua_State* _State) {
	struct Mission* _Mission = NULL;

	lua_pushvalue(_State, LUA_REGISTRYINDEX);
	lua_pushstring(_State, MISSION_LUASTR);
	lua_rawget(_State, -2);
	_Mission = lua_touserdata(_State, -1);
	if(_Mission == NULL)
		return 0;
	_Mission->OnTrigger = LuaCheckClass(_State, 2, "Rule");
	return 0;
}

int LuaMissionCallById(lua_State* _State) {
	int _Id = luaL_checkint(_State, 1);
	struct BigGuy* _Guy = LuaCheckClass(_State, 2, "BigGuy");
	struct Mission* _Mission = NULL;

	if((_Mission = RBSearch(&g_MissionEngine.MissionId, &_Id)) == NULL)
		return luaL_error(_State, "Attempted to call nil mission #%d", _Id);
	MissionCall(_State, _Mission, _Guy);
	return 0;
}

struct GenIterator* CrisisCreateItr(void* _Tree) {
	return CreateRBItr(_Tree, ((struct RBTree*)_Tree)->Size);
}
