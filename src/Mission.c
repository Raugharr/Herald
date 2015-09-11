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

static struct {
	struct BigGuy* Triggerer;
	struct BigGuy* Stack[MISSION_STACKSZ];
	int StackSz;
	struct {
		const char* Key;
		void* Pair;
		int Type;
	} KeyVal[8];
} g_MissionData;

struct QueuedMission {
	struct BigGuy* Triggerer;
	struct Mission* Mission;
	DATE FireDay; //The day this mission will fire.
	struct QueuedMission* Prev;
	struct QueuedMission* Next;
};

/*
 * FIXME: UsedMission is basically an array of QueuedMission. However MemoryPool does
 * not allow for arrays.
 */
/*struct UsedMission {
	struct BigGuy* BigGuy;
	struct {
		int Start;
		int Size;
		struct {
			struct Mission* Mission;
			DATE FireDate;
		} Table[USEDMISSION_QUEUESZ];
	} List;
};*/

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

void MissionInsert(struct RBTree* _Tree, struct Mission* _Mission) {
	struct LinkedList* _List = RBSearch(_Tree, _Mission);

	if(_List == NULL) {
		_List = CreateLinkedList();
		LnkLstPushBack(_List, _Mission);
		RBInsert(_Tree, _List);
	} else {
		LnkLstPushBack(_List, _Mission);
	}
}

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

void InitMissions(void) {
	g_MissionQueuePool = CreateMemoryPool(sizeof(struct QueuedMission), MISSION_QELEMENTS);
	g_MissionUsed = CreateMemoryPool(sizeof(struct QueuedMission), USEDMISSION_SIZE);
}

void QuitMissions(void) {
	DestroyMemoryPool(g_MissionQueuePool);
	DestroyMemoryPool(g_MissionUsed);
}

int UsedMissionInsert(struct QueuedMission* _One, struct QueuedMission* _Two) {
	return _One[0].Triggerer->Id - _Two[0].Triggerer->Id;
}

int UsedMissionSearch(int* _One, struct QueuedMission* _Two) {
	return (*_One) - _Two[0].Triggerer->Id;
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
		lua_pushvalue(_State, LUA_REGISTRYINDEX);
		lua_pushstring(_State, MISSION_LUASTR);
		lua_pushlightuserdata(_State, _Mission);
		lua_rawset(_State, -3);
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
		MissionInsert(_List, _Mission);
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
 * Checks all dirty BigGuys in _BigGuys if they trigger a mission in _Missions then sets all BigGuys dirty state to false.
 */
void GenerateMissions(lua_State* _State, const struct RBTree* _BigGuys, struct MissionEngine* _Engine) {
	struct LinkedList* _List = NULL;
	struct LnkLst_Node* _Itr = NULL;
	struct Mission* _Mission = NULL;
	struct RBItrStack _Stack[_BigGuys->Size];
	struct BigGuy* _BigGuy = NULL;
	struct QueuedMission* _Element = NULL;
	struct QueuedMission* _Used = NULL;
	int _Rnd = 0;

	if(_BigGuys->Table == NULL)
		return;
	RBDepthFirst(_BigGuys->Table, _Stack);
	for(int i = 0; i < _BigGuys->Size; ++i, _BigGuy->IsDirty = 0) {
		_BigGuy = ((struct RBNode*) _Stack[i].Node)->Data;
		if(_BigGuy->IsDirty == 0)
			;//continue;
		if((_List = RBSearch(&_Engine->Missions, &_BigGuy->State)) == NULL)
			continue;
		_Itr = _List->Front;
		/*
		 * Check if a similar mission type has already been fired recently.
		 */
		if((_Used = RBSearch(&_Engine->UsedMissionTree, _BigGuy)) != NULL) {
			_Element = _Used;
			while(_Element != NULL) {
				if(WorldStateTruth(&_BigGuy->State, &_Element->Mission->Trigger) != 0)
					goto loop_end;
				_Element = _Element->Next;
			}
		}
		/*
		 * Pick a random Mission that is true when compared to _BigGuy's State.
		 */
		_Rnd = Random(0, _List->Size - 1);
		for(int i = 0; i < _Rnd; ++i) _Itr = _Itr->Next;
		_Mission = (struct Mission*)_Itr->Data;
		_Element = MemPoolAlloc(g_MissionQueuePool);
		_Element->Mission = _Mission;
		_Element->Triggerer = _BigGuy;
		_Element->FireDay = DateAddInt(g_GameWorld.Date, _Mission->MeanTime);
		BinaryHeapInsert(&_Engine->MissionQueue, _Element);
		/*
		 * Now that the Mission has been picked check if _Used is NULL create a UsedMission with _Mission as the first element of the queue.
		 */
		if(_Used == NULL) {
			_Used = MemPoolAlloc(g_MissionUsed);
			_Used->Triggerer = _BigGuy;
			_Used->Mission = _Mission;
			_Used->FireDay = _Element->FireDay;
			_Used->Next = NULL;
			_Used->Prev = NULL;
			RBInsert(&_Engine->UsedMissionTree, _Used);
		} else {
			struct QueuedMission* _Temp = MemPoolAlloc(g_MissionUsed);
			_Temp->Triggerer = _BigGuy;
			_Temp->Mission = _Mission;
			_Temp->FireDay = _Element->FireDay;
			_Temp->Prev = _Used;
			_Temp->Next = NULL;
			_Used->Next = _Temp;
		}
		loop_end:
		;
	}
}

void MissionPump(lua_State* _State, const struct QueuedMission* _Queue) {
	if(g_GameWorld.Player == _Queue->Triggerer/*strcmp((char*)g_GUIStack.Top->Data, "MissionMenu") != 0*/) {
		g_MissionData.Triggerer = _Queue->Triggerer;
		lua_settop(_State, 0);
		lua_pushstring(_State, "MissionMenu");
		lua_createtable(_State, 0, 2);
		lua_pushstring(_State, "Mission");
		LuaCtor(_State, "Mission", _Queue->Mission);
		lua_rawset(_State, -3);
		lua_pushstring(_State, "BigGuy");
		LuaCtor(_State, "BigGuy", _Queue->Triggerer);
		lua_rawset(_State, -3);
		lua_pushinteger(_State, 512);
		lua_pushinteger(_State, 512);
		LuaCreateWindow(_State);
		GUIMessageCallback(_State, "Mission", CheckMissionOption, _Queue->Mission, NULL);
	}
}

int CheckMissionOption(struct GUIMessagePacket* _Packet) {
	int _Top = lua_gettop(_Packet->State);

	RuleEval(((struct Mission*)_Packet->One)->Options[_Packet->RecvPrim.Value.Int].Action);
	lua_settop(_Packet->State, _Top);
	MissionDataClear();
	return 1;
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
		_Mission = RBSearch(&_Engine->UsedMissionTree, _Mission->Triggerer);
		MissionPump(_State, _Mission);
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

int MissionTreeInsert(const struct Mission* _One, const struct LinkedList* _Two) {
	return WorldStateAtomCmp(&_One->Trigger, &((struct Mission*)((struct LinkedList*)_Two)->Front->Data)->Trigger);
}

int MissionTreeSearch(const struct WorldState* _One, const struct LinkedList* _Two) {
	return WorldStateAtomCmp(_One, &((struct Mission*)((struct LinkedList*)_Two)->Front->Data)->Trigger);
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
	const char* _Str = luaL_checkstring(_State, 1);
	int _OpCode = luaL_checkinteger(_State, 2);
	int _Value = luaL_checkinteger(_State, 3);
	int _Atom = 0;
	struct Mission* _Mission = NULL;

	lua_pushvalue(_State, LUA_REGISTRYINDEX);
	lua_pushstring(_State, MISSION_LUASTR);
	lua_rawget(_State, -2);
	_Mission = lua_touserdata(_State, -1);
	if(_Mission == NULL)
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
	WorldStateSetOpCode(&_Mission->Trigger, _Atom, _OpCode);
	WorldStateSetAtom(&_Mission->Trigger, _Atom, _Value);
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

void MissionDataGetVal(lua_State* _State, int _Index) {

}

int LuaMissionGetKey(lua_State* _State) {
	const char* _Key = luaL_checkstring(_State, 1);

	for(int i = 0; i < 8; ++i) {
		if(strcmp(g_MissionData.KeyVal[i].Key, _Key) == 0) {
			MissionDataGetVal(_State, i);
			return 1;
		}
	}
	return 0;
}
