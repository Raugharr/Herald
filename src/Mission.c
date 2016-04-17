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
#include "sys/Array.h"
#include "sys/Rule.h"
#include "sys/RBTree.h"
#include "sys/Log.h"
#include "sys/Stack.h"
#include "sys/Event.h"
#include "sys/Math.h"
#include "sys/LinkedList.h"
#include "sys/MemoryPool.h"
#include "sys/GenIterator.h"
#include "sys/Coroutine.h"

#include "video/GuiLua.h"
#include "video/Gui.h"

#include <SDL2/SDL.h>
#include <stdlib.h>
#include <string.h>
#include <lua/lua.h>
#include <lua/lauxlib.h>
#include <dirent.h>
#include <malloc.h>
#include <assert.h>
#include <ctype.h>
#include <unistd.h>

#define MISSION_STACKSZ (16)
#define USEDMISSION_ARRAYSZ (10)
#define MISSION_LUASTR ("InitMission")
#define MISSION_QELEMENTS (2048)
#define USEDMISSION_SIZE (10000)
#define MISSIONDATA_HASHSZ (32)
#define MISSION_COOLDOWN (90)
#define MISSION_DEFMEANTIME 0

struct MissionData;

static struct MemoryPool* g_MissionQueuePool = NULL;
static struct MemoryPool* g_MissionUsed = NULL;
static int g_PlayerMisCall = 0;
static struct MissionData* g_MissionData = NULL;

struct MissionDataType {
	const char* Name;
	struct Primitive Var;
};

struct MissionData {
	struct BigGuy* Sender;
	struct BigGuy* Target;
	struct MissionDataType Stack[MISSION_STACKSZ];
	int StackSz;
	struct BigGuy* BGStack[MISSION_STACKSZ];
	int BGStackSz;
	int IsOptSel;
	const struct Mission* Mission;
};

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

int MissionEventS(const struct Mission* _Mission, const void** _Args) {
	int _Index = 0;

	if(WorldStateAtomCare(&_Mission->Trigger, *((int*)_Args[1])) != 0)
		return 0;
	for(int i = 0; i < WORLDSTATESZ; ++i) {
		for(int j = 0; j < sizeof(WorldState_t); ++j) {
			if(~WSAtomDontCare(&_Mission->Trigger, i, j) != 0)
				_Index = i * sizeof(WorldState_t) + j;
		}
	}
	return _Index - *((int*)_Args[1]);
}

int MissionEngineEvent(const int* _One, const struct LnkLst_Node* _Two) {
	struct Mission* _Mission = ((struct Mission*)_Two->Data);
	int _EventId = EventUserOffset() + WorldStateFirstAtom(&_Mission->Trigger);//+ bsc(_Mission->Trigger.State[0]) - 1;

	SDL_assert(_EventId >= EventUserOffset());	
	return (*_One) - _EventId;
}

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

struct MissionData* CreateMissionData(struct BigGuy* _Sender, struct BigGuy* _Target, const struct Mission* _Mission) {
	struct MissionData* _MissionData = (struct MissionData*) malloc(sizeof(struct MissionData));

	_MissionData->Sender = _Sender;
	_MissionData->Target = _Target;
	LnkLstPushBack(&g_GameWorld.MissionData, _MissionData);
	_MissionData->StackSz = 0;
	_MissionData->BGStackSz = 0;
	_MissionData->Mission = _Mission;
	_MissionData->IsOptSel = 0;
	return _MissionData;
}

void DestroyMissionData(struct MissionData* _MissionData) {
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

/*
 * Sort missions based on their state op code.
 */
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
	struct MissionCatList* _CatList = NULL;
	struct MissionCatList _Search;

	if(WorldStateEmpty(&_Mission->Trigger) != 0)
		return;
	if(RBSearch(&_Engine->MissionId, _Mission) != NULL) {
		Log(ELOG_WARNING, "Mission cannot be loaded id %f is already in use.", _Mission->Id);
		return;
	}
	RBInsert(&_Engine->MissionId, _Mission);
	if((_Mission->Flags & MISSION_FEVENT) != 0) {
		struct LnkLst_Node* _MisList = RBSearch(&_Engine->EventMissions, _Mission);

		if(_MisList == NULL) {
			struct LnkLst_Node* _Node = CreateLnkLstNode(_Mission);

			RBInsert(&_Engine->EventMissions, _Node);
		} else {
			struct LnkLst_Node* _Temp = _MisList->Next;

			if(_Temp == NULL) {
				_MisList->Next = CreateLnkLstNode(_Mission);
				_MisList->Next->Prev = _MisList;
			} else {
				_Temp = _MisList->Next = CreateLnkLstNode(_Mission);

				_MisList->Next->Prev = _Temp;
				_Temp->Next = _MisList->Next;
				_Temp->Prev = _MisList;
				_MisList->Next = _Temp;
			}
		}
	}
	if((_Mission->Flags & MISSION_FONLYTRIGGER) != 0 || (_Mission->Flags & MISSION_FEVENT) != 0)
		return;
	_Search.Category = _Mission->TriggerType;
	_Search.State = 0;
	_Search.State = WSDntCrComp(&_Mission->Trigger);
	if((_CatList = RBSearch(&_Engine->Missions, &_Search)) == NULL) {
		_CatList = (struct MissionCatList*) malloc(sizeof(struct MissionCatList));
		_CatList->Category = _Search.Category;
		_CatList->State = _Search.State;
		_CatList->List.Size = 0;
		_CatList->List.Front = NULL;
		_CatList->List.Back = NULL;
		LnkLstPushBack(&_CatList->List, _Mission);
		RBInsert(&_Engine->Missions, _CatList);
	}
}

void MissionDataClear(struct MissionData* _Data) {
	struct LnkLst_Node* _Itr = NULL;

	_Data->Sender = NULL;
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

	_Mission->TriggerType = 0;
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
	int _Cmp = _One->Mission->TriggerType - _Two->Mission->TriggerType;

	if(_Cmp != 0)
		return _Cmp;
	_Cmp = _One->Triggerer->Id - _Two->Triggerer->Id;
	return (_Cmp == 0) ? (WSDntCrCmp(&_One->Mission->Trigger, &_Two->Mission->Trigger)) : (_Cmp);
}

int UsedMissionSearch(const struct UsedMissionSearch* _One, const struct QueuedMission* _Two) {
	int _Cmp = _One->Type - _Two->Mission->TriggerType;

	if(_Cmp != 0)
		return _Cmp;
	_Cmp = _One->Guy->Id - _Two->Triggerer->Id;
	return (_Cmp == 0) ? (WSDntCrCmp(_One->Trigger, &_Two->Mission->Trigger)) : (_Cmp);
}

int MissionTreeIS(const struct MissionCatList* _TriggerType, const struct MissionCatList* _List) {
	int _Cmp = _TriggerType->Category - _List->Category;

	return (_Cmp == 0) ? (_TriggerType->State - _List->State) : (_Cmp);
}

void LoadAllMissions(lua_State* _State, struct MissionEngine* _Engine) {
	DIR* _Dir = NULL;
	struct dirent* _Dirent = NULL;
	const char* _Ext = NULL;

	chdir("data/missions");
	_Dir = opendir("./");
	lua_settop(_State, 0);
	while((_Dirent = readdir(_Dir)) != NULL) {
		if(((!strcmp(_Dirent->d_name, ".") || !strcmp(_Dirent->d_name, "..")))
			|| ((_Ext = strrchr(_Dirent->d_name, '.')) == NULL) || strncmp(_Ext, ".lua", 4) != 0)
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
	for(int i = 0; _Mission->TextFormat[i] != NULL; ++i)
		free(_Mission->TextFormat[i]);
	free(_Mission->TextFormat);
	free(_Mission);
}

int MissionCompareState(const struct WorldState* _State, const struct Mission* _Mission) {
	return 0;
}

void MissionSelect(struct MissionEngine* _Engine, void* _Object, struct BigGuy* _Owner, const struct WorldState* _State,
		int* _TriggerMask, int _Type, struct RBTree* _MissionList) {
	int _Rnd = 0;
	int _Ct = 0;
	int _StackCount = 0;
	int _DntCare = WSDntCrComp(_State);
	const struct LinkedList* _List = NULL;
	const struct LnkLst_Node* _Itr = NULL;
	const struct LnkLst_Node* _FirstItr = NULL;
	struct QueuedMission* _Used = NULL;
	struct QueuedMission* _Element = NULL;
	struct Mission* _Mission = NULL;
	struct UsedMissionSearch _Search;
	struct RBItrStack _Stack[_Engine->Missions.Size];
	struct MissionCatList* _CatList = NULL;

	RBDepthFirst(_Engine->Missions.Table, _Stack);
	missiontype_select:
	for(;_StackCount < _Engine->Missions.Size; ++_StackCount) {
		_CatList = (struct MissionCatList*) _Stack[_StackCount].Node->Data;
		if(_CatList->Category != _Type)
			continue;
		if((_CatList->State & _DntCare) != _CatList->State)
			continue;
		if((_CatList->State & (*_TriggerMask)) != 0)
			continue;
		_List = &_CatList->List;
		++_StackCount;
		break;
	}
	if(_StackCount >= _Engine->Missions.Size)
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
		MissionSelect(_Engine, _BigGuy, _BigGuy, &_BigGuy->State, &_BigGuy->TriggerMask, MISSIONCAT_BIGGUY, &g_MissionEngine.Missions);
	}

	for(int i = 1; i < MISSIONCAT_SIZE; ++i) {
		_Category = &_Engine->Categories[i];
		if(_Category->ListIsEmpty(_Category->List) != 0)
			continue;
		_Itr = _Category->CreateItr(_Category->List);
		while(_Itr->HasNext(_Itr) != 0) {
			_Obj = _Itr->NextObj(_Itr);
			MissionSelect(_Engine, _Obj, _Category->GetOwner(_Obj), _Category->GetState(_Obj), _Category->GetTriggerMask(_Obj), MISSION_TYPE(i),&g_MissionEngine.Missions);
		}
		_Category->DestroyItr(_Itr);
	}
}

void MissionCheckOption(struct lua_State* _State, struct Mission* _Mission, struct MissionData* _Data, int _Option) {
	int _Top = lua_gettop(_State);

	//CoYield(GAME_CORO);
	if(_Option < 0 || _Option >= _Mission->OptionCt)
		return;
	if(g_MissionData->IsOptSel == 0) {
		g_MissionData->IsOptSel = 1;
		CoResume(g_PlayerMisCall);
		g_PlayerMisCall = 0;
	}
	g_MissionData = _Data;
	RuleEval(_Mission->Options[_Option].Action);
	lua_settop(_State, _Top);
	MissionDataClear(_Data);
}

void MissionCall(lua_State* _State, const struct Mission* _Mission, struct BigGuy* _Owner, struct BigGuy* _Sender) {
	struct MissionData* _Data = NULL;
	const char* _NewDesc = NULL;

	if(_Mission == NULL)
		return;
	_Data = CreateMissionData(_Sender, _Owner, _Mission);
	_NewDesc = _Mission->Description;
	if(g_MissionData != NULL) {
		for(int i = 0; i < g_MissionData->StackSz; ++i) {
			_Data->Stack[i] = g_MissionData->Stack[i];
		}
		_Data->StackSz = g_MissionData->StackSz;
	}
	g_MissionData = _Data;
	if(g_GameWorld.Player == _Owner) {
		if(_Mission->TextFormat != NULL) {
			//Breaks when MissionFormatText is run on a coroutine other than the main coroutine.
			//because the rule used points to the main coroutine's lua_State.
			const char** restrict _Strings = alloca(sizeof(char*) * ArrayLen(_Mission->TextFormat));
			size_t _SizeOf = MissionFormatText(_State, _Mission, _Strings);
			char* restrict _DescStr = NULL;

			if(_SizeOf == 0) {
				Log(ELOG_WARNING, "Mission %s failed: MissionFormatText failed to format text.", _Mission->Name);
				return;
			}
			_DescStr = alloca(_SizeOf);
			_DescStr[0] = '\0';
			vsprintf(_DescStr, _Mission->Description, (va_list)_Strings); 
			_NewDesc = _DescStr;
		}
		if(_Mission->OnTrigger != NULL)
			RuleEval(_Mission->OnTrigger);
		lua_settop(_State, 0);
		lua_pushstring(_State, "MissionMenu");
		lua_createtable(_State, 0, 3);
		lua_pushstring(_State, "Mission");
		LuaConstCtor(_State, "Mission", _Mission);
		lua_rawset(_State, -3);

		lua_pushstring(_State, "BigGuy");
		LuaCtor(_State, "BigGuy", _Owner);
		lua_rawset(_State, -3);

		lua_pushstring(_State, "Data");
		lua_pushlightuserdata(_State, _Data);
		lua_rawset(_State, -3);

		lua_pushstring(_State, "Description");
		lua_pushstring(_State, _NewDesc);
		lua_rawset(_State, -3);

		lua_pushinteger(_State, 512);
		lua_pushinteger(_State, 512);
		LuaCreateWindow(_State);
		if(CoRunning() != 0) {
			g_PlayerMisCall = CoRunning();
			CoYield();
		}
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
		_Data->IsOptSel = 1;
		RuleEval(_Mission->Options[_BestIndex].Action);
		MissionDataClear(_Data);
	}
}

void MissionAction(const char* _Name, struct BigGuy* _Sender, struct BigGuy* _Target) {
	int _MissionId = MissionStrToId(_Name);
//	void* _Mission = RBSearch(&g_MissionEngine.MissionId, &_MissionId);

	//SDL_assert(_Mission != NULL);
	lua_settop(g_LuaState, 0);
	CoResume(CoSpawn(MissionCall, 4, g_LuaState, RBSearch(&g_MissionEngine.MissionId, &_MissionId)/*_Mission*/, _Sender, _Target));
}

void DestroyMissionEngine(struct MissionEngine* _Engine) {
	struct RBItrStack _Stack[_Engine->Missions.Size];
	struct MissionCatList* _CatList = NULL;
	struct LinkedList* _List = NULL;

	RBDepthFirst(_Engine->Missions.Table, _Stack);
	for(int i = 0; i < _Engine->Missions.Size; ++i) {
		_CatList = ((struct RBNode*) _Stack[i].Node)->Data;
		_List = &_CatList->List;
		while(_List->Size > 0) {
			free(_List->Front->Data);
			LnkLstPopFront(_List);
		}
		free(_CatList);
	}
	_Engine->Missions.Size = 0;
	free(_Engine->Missions.Table);
	free(_Engine->MissionQueue.Table);
	free(_Engine->UsedMissionQueue.Table);
	free(_Engine->UsedMissionTree.Table);
}

void MissionOnEvent(struct MissionEngine* _Engine, int _EventType, struct BigGuy* _Guy) {
	struct LnkLst_Node* _List = NULL;
	struct Mission* _Mission = NULL;
	struct QueuedMission* _Element = NULL;
	struct QueuedMission* _Used = NULL;

	if((_List = RBSearch(&_Engine->EventMissions, &_EventType)) == NULL)
		return;
	_Mission = _List->Data;
	_Element = MemPoolAlloc(g_MissionQueuePool);
	_Element->Mission = _Mission;
	_Element->Triggerer = _Guy;
	_Element->FireDay = DateAddInt(g_GameWorld.Date, RuleEval(_Mission->MeanTime));
	BinaryHeapInsert(&_Engine->MissionQueue, _Element);
	_Used = MemPoolAlloc(g_MissionUsed);
	_Used->Triggerer = _Guy;
	_Used->Mission = _Mission;
	_Used->FireDay = _Element->FireDay;
	_Used->Next = NULL;
	_Used->Prev = NULL;
	RBInsert(&_Engine->UsedMissionTree, _Used);
}

void MissionEngineThink(struct MissionEngine* _Engine, lua_State* _State, const struct RBTree* _BigGuys) {
	struct QueuedMission* _Mission = NULL;
	const struct QueuedMission* _Best = NULL;

	GenerateMissions(_State, _BigGuys, _Engine);
	while((_Mission = BinaryHeapTop(&_Engine->MissionQueue)) != NULL && _Mission->FireDay == g_GameWorld.Date) {
		struct UsedMissionSearch _Search;

		_Search.Guy = _Mission->Triggerer;
		_Search.Trigger = &_Mission->Mission->Trigger;
		_Search.Type = _Mission->Mission->TriggerType;
		_Mission = RBSearch(&_Engine->UsedMissionTree, &_Search);
		MissionCall(_State, _Mission->Mission, _Mission->Triggerer, NULL);
		BinaryHeapPop(&_Engine->MissionQueue);
		_Mission->FireDay = DateAddInt(_Mission->FireDay, MISSION_COOLDOWN);
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
				_Search.Type = _Best->Mission->TriggerType;
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

int MissionHeapInsert(const struct QueuedMission* _One, const struct QueuedMission* _Two) {
	return DateCmp(_Two->FireDay, _One->FireDay);
}

int UsedMissionHeapInsert(const struct QueuedMission* _One, const struct QueuedMission* _Two) {
	return DateCmp(_One->FireDay, _Two->FireDay);
}

int MissionFormatText(lua_State* _State, const struct Mission* _Mission, const char** restrict _Strings) {
	int _ExtraSz = 0;

	for(int i = 0; _Mission->TextFormat[i] != NULL; ++i) {
		if(LuaRuleEval(_Mission->TextFormat[i], _State) == 0)
			return 0;
		switch(lua_type(_State, -1)) {
			case LUA_TSTRING:
			case LUA_TNUMBER:
				_Strings[i] = lua_tostring(_State, -1);
				_ExtraSz += strlen(_Strings[i]) - 2; //minus 2 for the %s.
				lua_pop(_State, 1);
			break;
			default:
				lua_pop(_State, 1);
				_Strings[i] = NULL;
				continue;
		}
	}
	return strlen(_Mission->Description) + _ExtraSz + 1;
}

struct Mission* StrToMission(const char* _Str) {
	int _Id = MissionStrToId(_Str);
	struct Mission* _Mission = NULL;

	if(_Id == -1 || (_Mission = RBSearch(&g_MissionEngine.MissionId, &_Id)) == NULL)
		return NULL;
	return _Mission;

}

int LuaMissionGetOwner_Aux(lua_State* _State) {
	if(g_MissionData->Target == NULL)
		lua_pushnil(_State);
	else 
		LuaCtor(_State, "BigGuy", g_MissionData->Target);
	return 1;
}

int LuaMissionGetOwner(lua_State* _State) {
	lua_pushcfunction(_State, LuaMissionGetOwner_Aux);
	LuaRuleLuaCall(_State);
	return 1;
}

int LuaMissionGetSender_Aux(lua_State* _State) {
	if(g_MissionData->Sender == NULL)
		return luaL_error(_State, "%s: Target is nil.", g_MissionData->Mission->Name);
	LuaCtor(_State, "BigGuy", g_MissionData->Sender);
	return 1;
}

int LuaMissionGetSender(lua_State* _State) {
	lua_pushcfunction(_State, LuaMissionGetSender_Aux);
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
	if(g_MissionData->BGStackSz >= MISSION_STACKSZ)
		return luaL_error(_State, "LuaMissionGetRandomPerson: Stack is full.");
	_Settlement = FamilyGetSettlement(g_MissionData->Sender->Person->Family);
	assert(_Settlement->BigGuys.Size != 0 && "LuaMissionGetRandomPerson: Settlement has no BigGuys.");
	_IsUnique = lua_toboolean(_State, 1);
	_Itr = _Settlement->BigGuys.Front;
	_Ct = Random(0, _Settlement->BigGuys.Size);
	while(_Itr != NULL && _Ct >= 0) {
		loop_start:
		_Guy = (struct BigGuy*)_Itr->Data;
		//Pick someone who didnt fire the trigger.
		if(_Guy == g_MissionData->Sender) {
			_Guy = NULL;
			++_SkipedGuys;
			goto loop_end;
		}
		if(_IsUnique != 0) {
			for(int i = 0; i < g_MissionData->BGStackSz; ++i) {
				if(_Guy == g_MissionData->BGStack[i]) {
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
	g_MissionData->BGStack[g_MissionData->BGStackSz] = _Guy;
	++g_MissionData->BGStackSz;
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
	const char* _Str = NULL;
	int _Id = 0;
	struct BigGuy* _Owner = LuaCheckClass(_State, 2, "BigGuy");
	struct BigGuy* _Sender = NULL;
	struct Mission* _Mission = NULL;

	if(lua_type(_State, 1) == LUA_TSTRING) {
		_Str = lua_tostring(_State, 1);
		if((_Id = MissionStrToId(_Str)) == -1)
			return 0;
	} else
		_Id = luaL_checkinteger(_State, 1);
	if((_Mission = RBSearch(&g_MissionEngine.MissionId, &_Id)) == NULL)
		return (_Str != NULL) ? 
			(luaL_error(_State, "Attempted to call nil mission %s", _Str)) :
			(luaL_error(_State, "Attempted to call nil mission %d", _Id));
	if(lua_gettop(_State) >= 3) {
		_Sender = LuaCheckClass(_State, 3, "BigGuy");
	}
	MissionCall(_State, _Mission, _Owner, _Sender);
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

void MissionLoadOption(lua_State* _State, struct Mission* _Mission) {
	const char* _Text = NULL;
	struct Rule* _Condition = NULL;
	struct Rule* _Trigger = NULL;
	struct Rule* _Utility = NULL;

	lua_pushstring(_State, "Options");
	lua_rawget(_State, -2);
	if(lua_type(_State, -1) != LUA_TTABLE) {
		if(lua_type(_State, -1) != LUA_TNIL)
			return (void) luaL_error(_State, "Mission's options is not a table.");
		/* If we leave out the Options table assume we want a single
		 * option that has the text "Ok" and does nothing.
		 */
		_Mission->OptionCt = 1;
		_Mission->Options[0].Name = "Ok";
		_Mission->Options[0].Condition = (struct Rule*) CreateRuleBoolean(1);
		_Mission->Options[0].Action = (struct Rule*) CreateRuleBoolean(1);
		lua_createtable(_State, 3, 0);
		lua_pushcfunction(_State, LuaMissionNormalize_Aux);
		lua_rawseti(_State, -2, 1);
		lua_pushinteger(_State, 1);
		lua_rawseti(_State, -2, 2);
		lua_pushinteger(_State, 1);
		lua_rawseti(_State, -2, 3);
		_Mission->Options[0].Utility = (struct Rule*) CreateRuleLuaCall(_State, luaL_ref(_State, LUA_REGISTRYINDEX)); 
		return;
	}
	lua_pushnil(_State);
	while(lua_next(_State, -2) != 0) {
		if(_Mission->OptionCt >= MISSION_MAXOPTIONS) {
			Log(ELOG_WARNING, "Mission %s has exceeded the maximum amount of options.", _Mission->Name);
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
		if(_Trigger == NULL)
			return (void) luaL_error(_State, "Mission.Trigger entry is nil.");
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

void MissionLoadEventTrigger(lua_State* _State, struct Mission* _Mission) {
	const char* _Trigger = NULL;
	int _EventId = 0;

	lua_pushstring(_State, "Triggers");
	lua_rawget(_State, -2);
	lua_pushnil(_State);
	while(lua_next(_State, -2) != 0) {
		if(LuaGetString(_State, -1, &_Trigger) == 0) {
			Log(ELOG_WARNING, "Mission's trigger table contains a non-string.");
			goto loop_end;
		}
		if((_EventId = StringToEvent(_Trigger)) == -1) {
			Log(ELOG_WARNING, "Mission's trigger table contains a non-event %s.", _Trigger);
			goto loop_end;
		}
		WorldStateSetAtom(&_Mission->Trigger, _EventId - 1, 1);
		loop_end:
		lua_pop(_State, 1);
	}
	_Mission->Flags = _Mission->Flags | MISSION_FEVENT;
}

void MissionLoadTriggerList(lua_State* _State, struct Mission* _Mission) {
	const char* _Name = NULL;
	const char* _Type = NULL;
	const char** _StateStr = NULL;
	int _StateSz = 0;
	int _OpCode = 0;
	int _Value = 0;

	lua_pushstring(_State, "Name");
	lua_rawget(_State, -2);
	LuaGetString(_State, -1, &_Name);
	lua_pop(_State, 1);

	if(strcmp(_Name, "BigGuy") == 0) {
		_StateStr = g_BGStateStr;
		_StateSz = BGBYTE_SIZE;
		_Mission->TriggerType = MISSIONCAT_BIGGUY;
		goto load_triggers;
	} else if(strcmp(_Name, "Event") == 0) {
		MissionLoadEventTrigger(_State, _Mission);
		return;
	} else {
		for(int i = 1; i < MISSIONCAT_SIZE; ++i) {
			if(strcmp(_Name, g_MissionEngine.Categories[i].Name) == 0) {
				_StateStr = g_MissionEngine.Categories[i].StateStr;
				_StateSz = g_MissionEngine.Categories[i].StateSz;
				_Mission->TriggerType = MISSION_TYPE(i);
				goto load_triggers;
			}
		}
	}
	return (void) luaL_error(_State, "%s: is not a valid trigger type.", _Name);
	load_triggers:
	lua_pushstring(_State, "Triggers");
	lua_rawget(_State, -2);
	lua_pushnil(_State);
	while(lua_next(_State, -2) != 0) {
		lua_pushstring(_State, "Type");
		lua_rawget(_State, -2);
		LuaGetString(_State, -1, &_Type);
		lua_pop(_State, 1);

		lua_pushstring(_State, "OpCode");
		lua_rawget(_State, -2);
		LuaGetInteger(_State, -1, &_OpCode);
		lua_pop(_State, 1);
		if(_OpCode < WSOP_NOT || _OpCode > WSOP_LESSTHANEQUAL)
			return (void) luaL_error(_State, "%d is an invalid trigger op code.", _OpCode);

		lua_pushstring(_State, "Value");
		lua_rawget(_State, -2);
		LuaGetInteger(_State, -1, &_Value);
		MissionAddTriggerSetOp(_State, _Type, _StateStr, _StateSz, &_Mission->Trigger, _Value, _OpCode);
		lua_pop(_State, 2);
	}
	lua_pop(_State, 1);
}

void MissionLoadMeanTime(lua_State* _State, struct Mission* _Mission) {
	int _MeanTime = 0;
	struct Primitive _Primitive;

	if(LuaGetInteger(_State, -1, &_MeanTime) == 0) {
		_Mission->MeanTime = LuaCheckClass(_State, -1, "Rule");
		return;
	} else {
		PrimitiveSetInt(&_Primitive, _MeanTime);
		_Mission->MeanTime = (struct Rule*) CreateRulePrimitive(&_Primitive);
	}
	if(_Mission->MeanTime == NULL) {
		PrimitiveSetInt(&_Primitive, MISSION_DEFMEANTIME);
		_Mission->MeanTime = (struct Rule*) CreateRulePrimitive(&_Primitive);
	}
}

int LuaMissionLoad(lua_State* _State) {
	struct Mission* _Mission = CreateMission();
	const char* _TempStr = NULL;
	int _Id = 0;
	int i = 0;

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
		MissionLoadTriggerList(_State, _Mission);
		lua_pop(_State, 1);
	}

	lua_pushstring(_State, "MeanTime");
	lua_rawget(_State, 1);
	MissionLoadMeanTime(_State, _Mission);
	lua_pop(_State, 1);

	lua_pushstring(_State, "TextFormat");
	lua_rawget(_State, 1);

	if(lua_type(_State, -1) != LUA_TTABLE) {
		_Mission->TextFormat = NULL;
		goto escape_textformat;
	}
	_Mission->TextFormat = calloc(lua_rawlen(_State, -1) + 1, sizeof(struct Rule*));
	i = 0;
	lua_pushnil(_State);
	while(lua_next(_State, -2) != 0) {
		_Mission->TextFormat[i] = LuaToClass(_State, -1);
		++i;
		lua_pop(_State, 1);
	}
	_Mission->TextFormat[i] = NULL;
	lua_pop(_State, 1);

	escape_textformat:
	lua_pushstring(_State, "PostTrigger");
	lua_rawget(_State, 1);
	if(lua_isnil(_State, -1) == 0) {
		_Mission->PostTrigger = LuaCheckClass(_State, -1, "Rule");
	} else
		_Mission->PostTrigger = NULL;
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

int LuaMissionSetVar_Aux(lua_State* _State) {
	struct MissionData* _Data = g_MissionData;
	const char* _Name = luaL_checkstring(_State, 1);
	
	for(int i = 0; i < _Data->StackSz; ++i) {
		if(strcmp(_Data->Stack[i].Name, _Name) == 0) {
			LuaToPrimitive(_State, 2, &_Data->Stack[_Data->StackSz].Var);
			return 0;
		}
	}
	LuaToPrimitive(_State, 2, &_Data->Stack[_Data->StackSz].Var);
	_Data->Stack[_Data->StackSz].Name = malloc(strlen(_Name) + 1);
	strcpy((char*) _Data->Stack[_Data->StackSz].Name, _Name);
	++_Data->StackSz;
	return 0;
}

int LuaMissionSetVar(lua_State* _State) {
	lua_pushcfunction(_State, LuaMissionSetVar_Aux);
	lua_insert(_State, 1);
	LuaRuleLuaCall(_State);
	return 1;
}

int LuaMissionGetVar_Aux(lua_State* _State) {
	struct MissionData* _Data = g_MissionData;
	const char* _Name = luaL_checkstring(_State, 1);

	for(int i = 0; i < _Data->StackSz; ++i) {
		if(strcmp(_Name, _Data->Stack[i].Name) == 0) {
			PrimitiveLuaPush(_State, &_Data->Stack[i].Var);
			return 1;
		}
	}
	return luaL_error(_State, "%s is not defined.", _Name);
}

int LuaMissionGetVar(lua_State* _State) {
	lua_pushcfunction(_State, LuaMissionGetVar_Aux);
	lua_insert(_State, 1);
	LuaRuleLuaCall(_State);
	return 1;
}

int LuaMissionFuncWrapper(lua_State* _State) {
	int _Index = lua_upvalueindex(1);

	lua_pushvalue(_State, _Index);
	lua_insert(_State, 1);
	SDL_assert(lua_type(_State, 1) == LUA_TFUNCTION);
	LuaRuleLuaCall(_State);
	return 1;
}

struct GenIterator* CrisisCreateItr(void* _Tree) {
	return CreateRBItr(_Tree, ((struct RBTree*)_Tree)->Size);
}

struct MissionData* MissionDataTop() {
	return g_MissionData;
}

struct BigGuy* MissionDataOwner(struct MissionData* _Data) {
	return _Data->Target;
}
