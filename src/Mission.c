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
#include <ctype.h>
#include <unistd.h>

#define MISSION_STACKSZ (8)
#define USEDMISSION_ARRAYSZ (10)
#define MISSION_LUASTR ("InitMission")
#define MISSION_QELEMENTS (2048)
#define USEDMISSION_SIZE (10000)
#define MISSIONDATA_HASHSZ (32)

static struct MemoryPool* g_MissionQueuePool = NULL;
static struct MemoryPool* g_MissionUsed = NULL;

struct MissionDataType {
	void* Data;
	char* Class;
};

struct MissionData {
	struct BigGuy* Triggerer;
	struct BigGuy* Target;
	struct BigGuy* Stack[MISSION_STACKSZ];
	int StackSz;
	struct HashTable Data;
	const struct Mission* Mission;
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

struct MissionData* CreateMissionData(struct BigGuy* _Triggerer, struct BigGuy* _Target, struct Mission* _Mission) {
	struct MissionData* _MissionData = (struct MissionData*) malloc(sizeof(struct MissionData));

	_MissionData->Triggerer = _Triggerer;
	_MissionData->Target = _Target;
	LnkLstPushBack(&g_GameWorld.MissionData, _MissionData);
	_MissionData->Data.Size = 0;
	_MissionData->Data.TblSize = MISSIONDATA_HASHSZ;
	_MissionData->Data.Table = calloc(MISSIONDATA_HASHSZ, sizeof(void*));
	_MissionData->StackSz = 0;
	_MissionData->Mission = _Mission;
	return _MissionData;
}

void DestroyMissionData(struct MissionData* _MissionData) {
	struct HashItr* _Itr = HashCreateItr(&_MissionData->Data);

	while(_Itr != NULL) {
		((struct Rule*)_Itr->Node->Pair)->Destroy((struct Rule*)_Itr->Node->Pair);
		_Itr = HashNext(&g_MissionData->Data, _Itr);
	}
	HashDeleteItr(_Itr);
	free(_MissionData->Data.Table);
	free(_MissionData);
}

int MissionStrToId(const char* _Str) {
	int _Id = 0;
	int _TempId = 0;
	const char* _Namespace = strchr(_Str, '.');

	if(_Namespace == NULL || strchr(_Namespace + 1, '.') != NULL || strlen(_Namespace + 1) > 3)
		return -1;
	if(_Namespace - _Str > 5)
		return -1;
	for(int i = 0; i < 5; ++i) {
		if(_Str[i] == '.')
			break;
		if(isalpha(_Str[i]) == 0 || islower(_Str[i]))
			return -1;
		_Id = _Id + ((_Str[i] - 'A') << (5 * i));
	}
	_TempId = atoi(_Namespace + 1);
	if(_TempId > 127)
		return -1;
	return _Id + (_TempId << 25);
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

void MissionListSort(struct LinkedList* _List, struct Mission* _Mission) {
	int _Cmp = 0;
	struct LnkLst_Node* _Itr = _List->Front;
	struct Mission* _Temp;

	while(_Itr != NULL) {
		_Temp = (struct Mission*) _Itr->Data;
		if((_Cmp = WorldStateOpCmp(&_Mission->Trigger, &_Temp->Trigger)) >= 0) {
			LnkLstInsertBefore(_List, _Itr, _Mission);
			return;
		}
		_Itr = _Itr->Next;
	}
	LnkLstPushBack(_List, _Mission);
}

void MissionInsert(struct MissionEngine* _Engine, struct Mission* _Mission) {
	struct LinkedList* _List = RBSearch(&_Engine->Missions, _Mission);
	struct LinkedList* _MissionCat = NULL;

	if(WorldStateEmpty(&_Mission->Trigger) != 0)
		return;
	if(RBSearch(&_Engine->MissionId, _Mission) != NULL) {
		Log(ELOG_WARNING, "Mission cannot be loaded id %i is already in use.", _Mission->Id);
		return;
	}
	RBInsert(&_Engine->MissionId, _Mission);\
	if((_Mission->Flags & MISSION_FONLYTRIGGER) != 0)
		return;
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
					_MissionCat = &_Engine->MissionList[WSAtomIdxToInt(i, j)];
				else
					_MissionCat = &_Engine->Categories[MISSION_TYPETOCAT(_Mission->Type)].MissionList[WSAtomIdxToInt(i, j)];
				MissionListSort(_MissionCat, _Mission);
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
	_Mission->OptionCt = 0;
	_Mission->OnTrigger = NULL;
	_Mission->PostTrigger= NULL;
	_Mission->Flags = MISSION_FNONE;
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
	DIR* _Dir = NULL;
	struct dirent* _Dirent = NULL;

	chdir("data/missions");
	_Dir = opendir("./");
	lua_settop(_State, 0);
	while((_Dirent = readdir(_Dir)) != NULL) {
		if(!strcmp(_Dirent->d_name, ".") || !strcmp(_Dirent->d_name, ".."))
			continue;
		//FIXME: If one Mission cannot be loaded then all missions after it will not be loaded either.
		if(LuaLoadFile(_State, _Dirent->d_name, "Mission") != LUA_OK) {
			goto error;
		}
	}
	error:
	chdir("../..");
}

void DestroyMission(struct Mission* _Mission) {
	free(_Mission->MeanTime);
	free(_Mission->Name);
	free(_Mission->Description);
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
	int _Ct = 0;
	const struct LinkedList* _List = NULL;
	const struct LnkLst_Node* _Itr = NULL;
	const struct LnkLst_Node* _FirstItr = NULL;
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
	while(_Itr != NULL) {
		if(WorldStateTruth(_State, &((struct Mission*)_Itr->Data)->Trigger) != 0)
			goto found_mission;
		_Itr = _Itr->Next;
		++_Ct;
	}
	goto missiontype_select;
	found_mission:
	_Rnd = Random(0, (_List->Size - _Ct) + 1);
	_Mission = (struct Mission*)_Itr->Data;
	_FirstItr = _Itr;
	for(int i = 0; i < _Rnd;) {
		if(WorldStateTruth(_State, &((struct Mission*)_Itr->Data)->Trigger) != 0) {
			if(_Mission->PostTrigger != NULL) {
				if(RuleEval(_Mission->PostTrigger) == 0) {
					goto missionrnd_end;
				}
			}
			++i;
		}
		missionrnd_end:
		_Itr = _Itr->Next;
		if(_Itr == NULL) {
			_Itr = _List->Front;
			if(_Itr == _FirstItr && i == 0) {
				goto missiontype_select;
			}
		}
	}
	//NOTE: Can _Element and _Used be combined into one variable? The parameters are very similar.
	_Element = MemPoolAlloc(g_MissionQueuePool);
	_Element->Mission = _Mission;
	_Element->Triggerer = _Owner;
	_Element->FireDay = DateAddInt(g_GameWorld.Date, RuleEval(_Mission->MeanTime));
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

void MissionCall(lua_State* _State, struct Mission* _Mission, struct BigGuy* _Guy, struct BigGuy* _Target) {
	struct MissionData* _Data = CreateMissionData(_Guy, _Target, _Mission);

	g_MissionData = _Data;
	if(_Mission->OnTrigger != NULL)
		RuleEval(_Mission->OnTrigger);
	if(g_GameWorld.Player == _Guy) {
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
		LuaCreateWindow(_State);
	} else {
		int _BestIndex = -1;
		float _BestUtility = -1.0;
		float _Utility = 0.0;

		for(int i = 0; i < _Mission->OptionCt; ++i) {
			if(RuleEval(_Mission->Options[i].Condition) == 0)
				continue;
			_Utility = RuleEval(_Mission->Options[i].Utility);
			if(_Utility > _BestUtility) {
				_BestIndex = i;
				_BestUtility = _Utility;
			}
		}
		RuleEval(_Mission->Options[_BestIndex].Action);
		MissionDataClear(_Data);
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
		MissionCall(_State, _Mission->Mission, _Mission->Triggerer, NULL);
		BinaryHeapPop(&_Engine->MissionQueue);
		_Mission->FireDay = DateAddInt(_Mission->FireDay, 30);
		BinaryHeapInsert(&_Engine->UsedMissionQueue, _Mission);
	}
	while((_Mission = BinaryHeapTop(&_Engine->UsedMissionQueue)) != NULL) {
		_Best = UsedMissionEarliestDate(_Mission);
		if(DateCmp(_Best->FireDay, g_GameWorld.Date) <= 0) {
			if(_Best->Next != NULL && _Best->Prev != NULL) {
				_Best->Prev->Next = _Best->Next;
				_Best->Next->Prev = _Best->Prev;
			} else if(_Best->Next == NULL && _Best->Prev != NULL) {
				_Best->Prev->Next = NULL;
			} else if(_Best->Next != NULL && _Best->Prev == NULL) {
				_Best->Next->Prev = NULL;
			} else {
				struct UsedMissionSearch _Search;

				_Search.Guy = _Best->Triggerer;
				_Search.Trigger = &_Best->Mission->Trigger;
				_Search.Type = _Best->Mission->Type;
				RBDelete(&_Engine->UsedMissionTree, &_Search);
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

int LuaMissionGetTarget_Aux(lua_State* _State) {
	if(g_MissionData->Target == NULL)
		return luaL_error(_State, "%s: Target is nil.", g_MissionData->Mission->Name);
	LuaCtor(_State, "BigGuy", g_MissionData->Target);
	return 1;
}

int LuaMissionGetTarget(lua_State* _State) {
	lua_pushcfunction(_State, LuaMissionGetTarget_Aux);
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
	while(_Itr != NULL && _Ct >= 0) {
		loop_start:
		_Guy = (struct BigGuy*)_Itr->Data;
		//Pick someone who didnt fire the trigger.
		if(_Guy == g_MissionData->Triggerer) {
			_Guy = NULL;
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
	if(_Itr == NULL && _Ct >= 0) {
		_Itr = _Settlement->BigGuys.Front;
		goto loop_start;
	}
	g_MissionData->Stack[g_MissionData->StackSz] = _Guy;
	++g_MissionData->StackSz;
	LuaCtor(_State, "BigGuy", _Guy);
	return 1;
	error:
	return luaL_error(_State, "LuaMissionGetRandomPerson: No available person to select.");
}

int LuaMissionGetRandomPerson(lua_State* _State) {
	lua_pushcfunction(_State, LuaMissionGetRandomPerson_Aux);
	lua_insert(_State, 1);
	LuaRuleLuaCall(_State);
	return 1;
}

int LuaMissionCallById_Aux(lua_State* _State) {
	const char* _Str = luaL_checkstring(_State, 1);
	int _Id = 0;
	struct BigGuy* _Owner = LuaCheckClass(_State, 2, "BigGuy");
	struct BigGuy* _Target = NULL;
	struct Mission* _Mission = NULL;

	if((_Id = MissionStrToId(_Str)) == -1)
			return 0;
	if((_Mission = RBSearch(&g_MissionEngine.MissionId, &_Id)) == NULL)
		return luaL_error(_State, "Attempted to call nil mission %s", _Str);
	if(lua_gettop(_State) >= 3) {
		_Target = LuaCheckClass(_State, 3, "BigGuy");
	}
	MissionCall(_State, _Mission, _Owner, _Target);
	return 0;
}

int LuaMissionCallById(lua_State* _State) {
	lua_pushcfunction(_State, LuaMissionCallById_Aux);
	lua_insert(_State, 1);
	LuaRuleLuaCall(_State);
	return 1;
}

int LuaMissionNormalize_Aux(lua_State* _State) {
	int _Min = luaL_checkint(_State, 1);
	int _Max = luaL_checkint(_State, 2);

	if(_Min < 0 || _Max <= 0)
		lua_pushnumber(_State, 0.0);
		return 1;
	if(_Min > _Max) {
		lua_pushnumber(_State, 1.0);
		return 1;
	}
	lua_pushnumber(_State, ((float)_Min) / ((float)_Max));
	return 1;
}

int LuaMissionNormalize(lua_State* _State) {
	lua_pushcfunction(_State, LuaMissionNormalize_Aux);
	lua_insert(_State, 1);
	LuaRuleLuaCall(_State);
	return 1;
}

int LuaMissionData_Aux(lua_State* _State) {
	const char* _Str = luaL_checkstring(_State, 1);
	struct Rule* _Rule = HashSearch(&g_MissionData->Data, _Str);

	if(_Rule == NULL) {
		lua_pushnil(_State);
		return 1;
	}
	if(_Rule->Type == RULE_PRIMITIVE)
		PrimitiveLuaPush(_State, &((struct RulePrimitive*)_Rule)->Value);
	else if(_Rule->Type == RULE_LUAOBJ) {
		LuaCtor(_State, ((struct RuleLuaObj*)_Rule)->Class, ((struct RuleLuaObj*)_Rule)->Object);
	} else {
		return luaL_error(_State, "Invalid rule type");
	}
	return 1;
}

int LuaMissionData(lua_State* _State) {
	lua_pushcfunction(_State, LuaMissionData_Aux);
	lua_insert(_State, 1);
	LuaRuleLuaCall(_State);
	return 1;
}

int LuaMissionAddData_Aux(lua_State* _State) {
	const char* _Str = luaL_checkstring(_State, 1);
	const char* _Class = NULL;
	struct Rule* _Rule = NULL;

	if(lua_type(_State, 2) == LUA_TTABLE) {
		_Class = luaL_checkstring(_State, 3);
		lua_pushstring(_State, "__self");
		lua_rawget(_State, 2);
		if(lua_type(_State, -1) != LUA_TLIGHTUSERDATA)
			return luaL_error(_State, "Argument #2 is not an object.");
		_Rule = (struct Rule*) CreateRuleLuaObj(lua_touserdata(_State, -1), _Class);
	} else {
		struct Primitive _Primitive;

		LuaToPrimitive(_State, 2, &_Primitive);
		_Rule = (struct Rule*) CreateRulePrimitive(&_Primitive);
	}
	HashInsert(&g_MissionData->Data, _Str, _Rule);
	return 0;
}

int LuaMissionAddData(lua_State* _State) {
	lua_pushcfunction(_State, LuaMissionAddData_Aux);
	lua_insert(_State, 1);
	LuaRuleLuaCall(_State);
	return 1;
}

void MissionLoadOption(lua_State* _State, struct Mission* _Mission) {
	const char* _Text = NULL;
	struct Rule* _Condition = NULL;
	struct Rule* _Trigger = NULL;
	struct Rule* _Utility = NULL;

	lua_pushstring(_State, "Options");
	lua_rawget(_State, -2);
	if(lua_type(_State, -1) != LUA_TTABLE)
		luaL_error(_State, "Mission's options is not a table.");
	lua_pushnil(_State);
	while(lua_next(_State, -2) != 0) {
		if(_Mission->OptionCt >= MISSION_MAXOPTIONS) {
			lua_pop(_State, 1);
			break;
		}
		if(lua_type(_State, -1) != LUA_TTABLE)
			return (void) luaL_error(_State, "Mission.Options entry is not a table.");
		_Text = NULL;
		_Condition = NULL;
		_Trigger = NULL;
		lua_pushstring(_State, "Text");
		lua_rawget(_State, -2);
		LuaGetString(_State, -1, &_Text);
		lua_pop(_State, 1);

		lua_pushstring(_State, "Condition");
		lua_rawget(_State, -2);
		if(lua_isnil(_State, -1) == 0)
			_Condition = LuaCheckClass(_State, -1, "Rule");
		else
			_Condition = (struct Rule*) CreateRuleBoolean(1);
		lua_pop(_State, 1);

		lua_pushstring(_State, "Trigger");
		lua_rawget(_State, -2);
		_Trigger = LuaCheckClass(_State, -1, "Rule");
		lua_pop(_State, 1);

		lua_pushstring(_State, "Utility");
		lua_rawget(_State, -2);
		_Utility = LuaCheckClass(_State, -1, "Rule");
		lua_pop(_State, 1);

		if(_Text == NULL || _Trigger == NULL || _Utility == NULL)
			return (void) luaL_error(_State, "Mission.Option entry is incomplete.");
		if(_Condition == NULL)
			_Condition = (struct Rule*) CreateRuleBoolean(1);
		_Mission->Options[_Mission->OptionCt].Name = calloc(sizeof(char), strlen(_Text) + 1);
		strcpy(_Mission->Options[_Mission->OptionCt].Name, _Text);
		_Mission->Options[_Mission->OptionCt].Condition = _Condition;
		_Mission->Options[_Mission->OptionCt].Action = _Trigger;
		_Mission->Options[_Mission->OptionCt].Utility = _Utility;
		++_Mission->OptionCt;
		lua_pop(_State, 1);
	}
}

void MissionLoadTrigger(lua_State* _State, struct Mission* _Mission) {
	const char* _Name = NULL;
	const char* _Type = NULL;
	int _OpCode = 0;
	int _Value = 0;

	lua_pushstring(_State, "Name");
	lua_rawget(_State, -2);
	LuaGetString(_State, -1, &_Name);
	lua_pop(_State, 1);

	lua_pushstring(_State, "Type");
	lua_rawget(_State, -2);
	LuaGetString(_State, -1, &_Type);
	lua_pop(_State, 1);

	lua_pushstring(_State, "OpCode");
	lua_rawget(_State, -2);
	LuaGetInteger(_State, -1, &_OpCode);
	lua_pop(_State, 1);

	lua_pushstring(_State, "Value");
	lua_rawget(_State, -2);
	LuaGetInteger(_State, -1, &_Value);
	lua_pop(_State, 1);

	if(_OpCode < WSOP_NOT || _OpCode > WSOP_LESSTHANEQUAL)
		return (void) luaL_error(_State, "%d is an invalid trigger op code.", _OpCode);
	if(strcmp(_Name, "BigGuy") != 0) {
		for(int i = 0; i < MISSIONCAT_SIZE; ++i)
			if(strcmp(_Name, g_MissionEngine.Categories[i].Name) == 0) {
				MissionAddTriggerSetOp(_State, _Type, g_MissionEngine.Categories[i].StateStr, g_MissionEngine.Categories[i].StateSz, &_Mission->Trigger, _Value, _OpCode);
				_Mission->Type = MISSION_TYPE(i);
				return;
			}
	} else {
		MissionAddTriggerSetOp(_State, _Type, g_BGStateStr, BGBYTE_SIZE, &_Mission->Trigger, _Value, _OpCode);
		_Mission->Type = MISSION_BGTYPE;
		return;
	}
	return (void) luaL_error(_State, "%s: is not a valid trigger type.", _Name);
}

void MissionLoadMeanTime(lua_State* _State, struct Mission* _Mission) {
	int _MeanTime = 0;
	struct Primitive _Primitive;

	if(LuaGetInteger(_State, -1, &_MeanTime) == 0) {
		_Mission->MeanTime = LuaCheckClass(_State, -1, "Rule");
		return;
	}
	PrimitiveSetInt(&_Primitive, _MeanTime);
	_Mission->MeanTime = (struct Rule*) CreateRulePrimitive(&_Primitive);
}

int LuaMissionLoad(lua_State* _State) {
	struct Mission* _Mission = CreateMission();
	const char* _TempStr = NULL;
	int _Id = 0;

	luaL_checktype(_State, 1, LUA_TTABLE);
	lua_pushstring(_State, "Name");
	lua_rawget(_State, 1);
	if(lua_type(_State, -1) != LUA_TSTRING)
		luaL_error(_State, "Mission's name is not a string.");
	_TempStr = lua_tostring(_State, -1);
	_Mission->Name = calloc(sizeof(char), strlen(_TempStr) + 1);
	strcpy(_Mission->Name, _TempStr);
	lua_pop(_State, 1);

	lua_pushstring(_State, "Description");
	lua_rawget(_State, 1);
	if(lua_type(_State, -1) != LUA_TSTRING)
		luaL_error(_State, "Mission's description is not a string.");
	_TempStr = lua_tostring(_State, -1);
	_Mission->Description = calloc(sizeof(char), strlen(_TempStr) + 1);
	strcpy(_Mission->Description, _TempStr);
	lua_pop(_State, 1);

	MissionLoadOption(_State, _Mission);
	lua_pop(_State, 1);

	lua_pushstring(_State, "Id");
	lua_rawget(_State, 1);
	if(lua_type(_State, -1) != LUA_TSTRING)
		luaL_error(_State, "Mission's Id is not a string.");
	if((_Id = MissionStrToId((_TempStr = lua_tostring(_State, -1)))) == -1)
		return 0;
	if(RBSearch(&g_MissionEngine.MissionId, &_Id) != NULL) {
		return luaL_error(_State, "Cannot load mission with id %d. Id is already in use.", _Id);
	}
	_Mission->Id = _Id;
	lua_pop(_State, 1);

	lua_pushstring(_State, "OnlyTriggered");
	lua_rawget(_State, 1);
	if(lua_isnil(_State, -1) == 0) {
		if(lua_isboolean(_State, -1) != 0) {
			_Mission->Flags = (_Mission->Flags | MISSION_FONLYTRIGGER);
		} else {
			return luaL_error(_State, "Mission.OnlyTriggered must be a boolean.");
		}
	}

	if((_Mission->Flags & MISSION_FONLYTRIGGER) == 0) {
		lua_pushstring(_State, "Trigger");
		lua_rawget(_State, 1);
		if(lua_type(_State, -1) != LUA_TTABLE)
			return luaL_error(_State, "Mission.Trigger is not a table.");
		MissionLoadTrigger(_State, _Mission);
		lua_pop(_State, 1);
	}

	lua_pushstring(_State, "MeanTime");
	lua_rawget(_State, 1);
	MissionLoadMeanTime(_State, _Mission);
	lua_pop(_State, 1);

	lua_pushstring(_State, "PostTrigger");
	lua_rawget(_State, 1);
	if(lua_isnil(_State, -1) == 0) {
		_Mission->PostTrigger = LuaCheckClass(_State, -1, "Rule");
	}
	lua_pop(_State, 1);

	lua_pushstring(_State, "OnTrigger");
	lua_rawget(_State, 1);
	if(lua_isnil(_State, -1) == 0)
		_Mission->OnTrigger = LuaCheckClass(_State, -1, "Rule");
	else
		_Mission->OnTrigger = NULL;
	lua_pop(_State, 1);

	MissionInsert(&g_MissionEngine, _Mission);
	Log(ELOG_INFO, "Loaded mission %s", _Mission->Name);
	return 0;
}

int LuaMissionFuncWrapper(lua_State* _State) {
	int _Index = lua_upvalueindex(1);

	lua_pushvalue(_State, _Index);
	lua_insert(_State, 1);
	LuaRuleLuaCall(_State);
	return 1;
}

struct GenIterator* CrisisCreateItr(void* _Tree) {
	return CreateRBItr(_Tree, ((struct RBTree*)_Tree)->Size);
}
