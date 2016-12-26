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
#include <stdbool.h>

#define MISSION_STACKSZ (16)
#define USEDMISSION_ARRAYSZ (10)
#define MISSION_LUASTR ("InitMission")
#define MISSION_QELEMENTS (2048)
#define MISSION_UTMAX ((1 << 16) - 1) //Utility max value._
#define MISENG_NSSZ (128) //How many namespaces to preallocate for the engine's namespace variable.
#define MISENG_ARSZ (16) //How many elements in each array in the engine's namespace hashtable to preallocate.
#define MISENG_NSSTR (32)

struct MissionFrame;

static int g_PlayerMisCall = 0;
static uint32_t g_MissionId = 0;

//What tables will be placed in the mission environment.
static const char* g_LuaMissionEnv[] = {
	"Settlement",
	"BigGuy",
	"Family",
	NULL
};

static const luaL_Reg g_LuaMissionRuleFuncs[] = {
	{"FireEvent", LuaMissionCallById},
	{"Load", LuaMissionLoad},
	{"StatUtility", LuaMissionStatUtility},
	{NULL, NULL}
};

static const luaL_Reg g_LuaFuncsMissionFrame[] =  {
	{"RandomPerson", LuaMissionGetRandomPerson},
	{"GetVar", LuaMissionGetVar},
	{"SetVar", LuaMissionSetVar},
	{NULL, NULL}
};

const struct LuaEnum g_LuaMissionEventEnum[] = {
	{"OnSpring", MEVENT_SPRING},
	{"OnFall", MEVENT_FALL},
	{"OnJoinRetinue", EVENT_JOINRETINUE},
	{"OnQuitRetinue", EVENT_QUITRETINUE},
	{"OnBattleStart", EVENT_BATTLESTART},
	{"OnBattleEnd", EVENT_BATTLEEND},
	{NULL, 0}
};

static const luaL_Reg g_LuaFuncsMissionOption[] = {
	{"GetName", LuaMissionOptionGetName},
	{"ConditionSatisfied", LuaMissionOptionConditionSatisfied},
	{NULL, NULL}
};

enum MissionObjectEnum {
	MOBJECT_FROM,
	MOBJECT_TARGET,
	MOBJECT_SIZE
};

enum MissionParamEnum {
	MOPCODE_FIRSTNAME,
	MOPCODE_LASTNAME,
	MOPCODE_PROFESSION,
	MOPCODE_SIZE
};

/*
 * Used in a MissionFrame's primitive value parameter to tell what
 * type of object is in the primitive.
 */
enum MissionFrameEnum {
	MADATA_BIGGUY = PRIM_PTR + 1,
};

static const char* g_MissionObjects[MOBJECT_SIZE] = {
	"From",
	"Target",
};

static const char* g_MissionParams[MOPCODE_SIZE] = {
	"FirstName",
	"LastName",
	"Profession",
};

//TODO: Add reference counter to ensure that we dont clean up a frame prematurely.
//TODO: From, Owner, and Mission values will never change but we want to avoid multiple missions from using the same stack,
// as we would run into race conditions or one mission modifying a parallel mission's data.
struct MissionFrame {
	struct BigGuy* From;
	struct BigGuy* Owner;
	const struct Mission* Mission;
	struct Primitive Stack[MISSION_STACKSZ];
	const char* StackKey[MISSION_STACKSZ];
	uint8_t StackSz;
	uint8_t IsOptSel;
	volatile uint8_t RefCt;
};

struct MissionFrame* CreateMissionFrame(struct BigGuy* _From, struct BigGuy* _Target, const struct Mission* _Mission) {
	struct MissionFrame* _MissionFrame = (struct MissionFrame*) malloc(sizeof(struct MissionFrame));

	_MissionFrame->From = _From;
	_MissionFrame->Owner = _Target;
	LnkLstPushBack(&g_GameWorld.MissionFrame, _MissionFrame); //FIXME: Remove g_GameWorld.FMissionFrame
	_MissionFrame->StackSz = 0;
	_MissionFrame->Mission = _Mission;
	_MissionFrame->IsOptSel = 0;
	_MissionFrame->RefCt = 1;
	return _MissionFrame;
}

static inline void MissionFrameIncrRef(struct MissionFrame* _Frame) {
	++_Frame->RefCt;
}

void DestroyMissionFrame(struct MissionFrame* _MissionFrame) {
	--_MissionFrame->RefCt;
	if(_MissionFrame->RefCt <= 0)
		free(_MissionFrame);
}

static inline void SetupMissionFrame(lua_State* _State, struct MissionFrame* _Frame) {
	LuaCtor(_State, _Frame, LOBJ_MISSIONFRAME);
	lua_pushstring(_State, "From");
	(_Frame->From != NULL) 
		? LuaCtor(_State, _Frame->From, LOBJ_BIGGUY) 
		: lua_pushnil(_State);
	lua_rawset(_State, -3);
	lua_pushstring(_State, "Owner");
	LuaCtor(_State, _Frame->Owner, LOBJ_BIGGUY);
	lua_rawset(_State, -3);
}

static inline bool CallMissionCond(lua_State* _State, MLRef _Ref, struct MissionFrame* _Frame) {
	if(_Ref == LUA_REFNIL)
		return true;
	lua_rawgeti(_State, LUA_REGISTRYINDEX, _Ref);
	SetupMissionFrame(_State, _Frame);
	LuaCallFunc(_State, 1, 1, 0);
	if(lua_toboolean(_State, -1) == 0) {
		lua_pop(_State, 1);
		return false;
	}
	lua_pop(_State, 1);
	return true;
}

static inline void CallMissionAction(lua_State* _State, MLRef _Ref, struct MissionFrame* _Frame) {
	lua_rawgeti(_State, LUA_REGISTRYINDEX, _Ref);
	SetupMissionFrame(_State, _Frame);
	LuaCallFunc(_State, 1, 0, 0);
}

bool MissionStrToNS(const char* _Str, char** _Namespace, uint8_t* _Id) {
	for(int i = 0; i < MISENG_NSSTR - 1 || _Str[i] != '\0'; ++i) {
		if(isalpha(_Str[i]) == 0) {
			if(_Str[i] == '.') {
				int _TempId = 0;

				(*_Namespace)[i] = '\0';
				if((_TempId = atoi(&(_Str[i]) + 1)) <= 0 || _TempId > 0xFF) {
					return false;
				}
				*_Id = _TempId;
				break;
			} else {
				return false;
			}
		}
		(*_Namespace)[i] = _Str[i];
	}
	return true;
}

struct Mission* MissionStrToId(const char* _Str) {
	char* _Namespace = alloca(MISENG_NSSTR);
	uint8_t _Id = 0;
	struct Array* _Array = NULL;

	if(MissionStrToNS(_Str, &_Namespace, &_Id) == false)
		return NULL;
	if((_Array = HashSearch(&g_MissionEngine.Namespaces, _Namespace)) == NULL)
		return NULL;
	if(_Array->TblSize <= _Id || _Array->Table[_Id] == NULL)
		return NULL;
	return _Array->Table[_Id];
}
	

/*	int _Id = 0;
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
}*/

void MissionInsert(struct MissionEngine* _Engine, struct Mission* _Mission, const char* _Namespace, uint8_t _Id) {
	if(HashSearch(&_Engine->Namespaces, _Namespace) == NULL) {
		//NOTE: Ensure the Arrays allocated here are freed later.
		struct Array* _Array = CreateArray(MISENG_ARSZ);

		ArraySet_S(_Array, _Mission, _Id);
		HashInsert(&_Engine->Namespaces, _Namespace, _Array);
	}
	if(RBSearch(&_Engine->MissionId, _Mission->Name) != NULL) {
		Log(ELOG_WARNING, "Mission cannot be loaded id %f is already in use.", _Mission->Id);
		return;
	}
	RBInsert(&_Engine->MissionId, _Mission);
	if((_Mission->Flags & MISSION_FEVENT) == MISSION_FEVENT) {
		ArrayInsert_S(&_Engine->Events[_Mission->TriggerEvent], _Mission);
	} else if((_Mission->Flags & MISSION_FONLYTRIGGER) == 0) {
		LnkLstPushBack(&_Engine->MissionsTrigger, _Mission);	
	}
}

void MissionFrameClear(struct MissionFrame* _Data) {
	struct LnkLst_Node* _Itr = NULL;

	DestroyMissionFrame(_Data);
	_Itr = g_GameWorld.MissionFrame.Front;
	while(_Itr != NULL) {
		if(_Itr->Data == _Data) {
			LnkLstRemove(&g_GameWorld.MissionFrame, _Itr);
			return;
		}
		_Itr = _Itr->Next;
	}
	Assert(0);
}

struct Mission* CreateMission() {
	struct Mission* _Mission = (struct Mission*) malloc(sizeof(struct Mission));

	_Mission->TriggerType = 0;
	_Mission->Name = NULL;
	_Mission->Description = NULL;
	_Mission->MeanPercent = 0.0;
	_Mission->Trigger = LUA_REFNIL;
	_Mission->OnTrigger = LUA_REFNIL;
	_Mission->MeanModTrig = NULL;
	_Mission->TriggerEvent = 0;
	_Mission->MeanMods = 0;
	_Mission->TextFormatSz = 0;
	_Mission->OptionCt = 0;
	_Mission->MeanModsSz = 0;
	_Mission->Flags = MISSION_FNONE;
	return _Mission;
}

void ConstructMissionEngine(struct MissionEngine* _Engine) {
	_Engine->MissionId.Table = NULL;
	_Engine->MissionId.Size = 0;
	_Engine->MissionId.ICallback = (RBCallback) MissionIdSearch;
	_Engine->MissionId.SCallback = (RBCallback) MissionIdInsert;

	for(int i = 0; i < MEVENT_SIZE; ++i) {
		CtorArray(&_Engine->Events[i], 32);//FIXME: use macro instead of literal value 32.
	}
	ConstructHashTable(&_Engine->Namespaces, MISENG_NSSZ);
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
		//	goto error;
		}
	}
	//error:
	chdir("../..");
}

void DestroyMission(struct Mission* _Mission) {
	free(_Mission->Name);
	free(_Mission->Description);
	free(_Mission->TextFormat);
	free(_Mission);
}

void MissionCheckOption(struct lua_State* _State, struct Mission* _Mission, struct MissionFrame* _Frame, int _Option) {
	int _Top = lua_gettop(_State);

	if(_Option < 0 || _Option >= _Mission->OptionCt)
		return;
	if(_Frame->IsOptSel == 0) {
		_Frame->IsOptSel = 1;
		CoResume(g_PlayerMisCall);
		g_PlayerMisCall = 0;
	}
	CallMissionAction(_State, _Mission->Options[_Option].Action, _Frame);
	lua_settop(_State, _Top);
	MissionFrameClear(_Frame);
}

void MissionCall(lua_State* _State, const struct Mission* _Mission, struct BigGuy* _Owner, struct BigGuy* _From) {
	const char* _NewDesc = NULL;
	struct MissionFrame* _Frame = CreateMissionFrame(_From, _Owner, _Mission);
	double _Percent = 0;

	if(_Mission == NULL)
		return;
	_Percent = _Mission->MeanPercent;
	if(CallMissionCond(g_LuaState, _Mission->Trigger, _Frame) == 0)
		return;
	for(uint8_t i = 0; i < _Mission->MeanModsSz; ++i) {
		if(_Mission->MeanModTrig[i] == 0 || CallMissionCond(g_LuaState, _Mission->MeanModTrig[i], _Frame) != 0)
			_Percent = _Percent * _Mission->MeanMods[i];
	}
	if(Rand() / ((double)0xFFFFFFFFFFFFFFFF) > _Percent)
		return;
	_NewDesc = _Mission->Description;
	if(g_GameWorld.Player == _Owner) {
		if(_Mission->TextFormatSz > 0) {
			//Breaks when MissionFormatText is run on a coroutine other than the main coroutine.
			//because the rule used points to the main coroutine's lua_State.
			_NewDesc = MissionFormatText(_Mission->Description, _Mission->TextFormat, _Mission->TextFormatSz, _Frame); 
			if(_NewDesc ==  NULL)
				return (void) luaL_error(_State, "Mission (%s) cannot fire, cannot format description.", _Mission->Name);

		/*	if(_SizeOf == 0) {
				Log(ELOG_WARNING, "Mission %s failed: MissionFormatText failed to format text.", _Mission->Name);
				return;
			}
			*/
		}
		if(_Mission->OnTrigger != 0)
			CallMissionAction(_State, _Mission->OnTrigger, _Frame);
		if((_Mission->Flags & MISSION_FNOMENU) == MISSION_FNOMENU)
			return;
		lua_settop(_State, 0);
		lua_pushstring(_State, "MissionMenu");
		lua_createtable(_State, 0, 3);
		lua_pushstring(_State, "Mission");
		LuaConstCtor(_State, _Mission, LOBJ_MISSION);
		lua_rawset(_State, -3);

		lua_pushstring(_State, "BigGuy");
		LuaCtor(_State, _Owner, LOBJ_BIGGUY);
		lua_rawset(_State, -3);

		lua_pushstring(_State, "Data");
		SetupMissionFrame(_State, _Frame);
		lua_rawset(_State, -3);

		lua_pushstring(_State, "Description");
		lua_pushstring(_State, _NewDesc);
		lua_rawset(_State, -3);

		LuaCreateWindow(_State);
		if(CoRunning() != 0) {
			g_PlayerMisCall = CoRunning();
			CoYield();
		}
	} else {
		int _BestIndex = -1;
		//float _BestUtility = -1.0;
		//float _Utility = 0.0;

		if(_Mission->OnTrigger != 0)
			CallMissionCond(_State, _Mission->OnTrigger, _Frame);
		if((_Mission->Flags & MISSION_FNOMENU) == MISSION_FNOMENU)
			return;
		/*for(int i = 0; i < _Mission->OptionCt; ++i) {
			if(RuleEval(_Mission->Options[i].Condition) == 0)
				continue;
			_Utility = RuleEval(_Mission->Options[i].Utility);
			if(_Utility > _BestUtility) {
				_BestIndex = i;
				_BestUtility = _Utility;
			}
		}*/
		_BestIndex = 0;
		_Frame->IsOptSel = 1;
		if(_Mission->Options[_BestIndex].Action != 0)
			CallMissionAction(_State, _Mission->Options[_BestIndex].Action, _Frame);
		MissionFrameClear(_Frame); //FIXME: When an AI event fires an event to a player this will clear the frame before the player can select their option.
	}
}

void MissionAction(const char* _Name, struct BigGuy* _From, struct BigGuy* _Target) {
	lua_settop(g_LuaState, 0);
	CoResume(CoSpawn(MissionCall, 4, g_LuaState, MissionStrToId(_Name), _From, _Target));
}

void DestroyMissionEngine(struct MissionEngine* _Engine) {
	for(int i = 0; i < MISSION_MAXOPTIONS; ++i)
		DtorArray(&_Engine->Events[i]);
}

void MissionOnEvent(struct MissionEngine* _Engine, uint32_t _EventType, struct BigGuy* _Guy, void* _Extra) {
	Assert(_EventType < EventUserOffset());
	int _EventId = 0;//FIXME: Instead of checking if the EventId is valid (No events exist for the event type check it after all missions are loaded.
	struct Mission* _Mission = _Engine->Events[_EventType].Table[_EventId];
	struct MissionFrame _Frame = {0};

	if(_Engine->Events[_EventType].Size == 0)
		return;

	_EventId = Random(0, _Engine->Events[_EventType].Size - 1);//FIXME: Instead of checking if the EventId is valid (No events exist for the event type check it after all missions are loaded.
	_Frame.Owner = _Guy;
	_Frame.Mission = _Mission;
	switch(_EventType) {
	case EVENT_JOINRETINUE:
	case EVENT_QUITRETINUE:
		_Frame.From = _Guy;
		_Frame.Owner = ((struct Retinue*)_Extra)->Leader;
		break;
	}
	MissionCall(g_LuaState, _Mission, _Frame.Owner, _Frame.From);
}

void MissionCompare(struct RBNode* _GuyNode, struct LinkedList* _Missions, struct Mission* (*_ActionList)[BGACT_SIZE]) {
	struct BigGuy* _Guy = NULL;

	if(_GuyNode == NULL)
		return;
	_Guy = _GuyNode->Data;
	for(struct LnkLst_Node* _Itr = _Missions->Front; _Itr != NULL; _Itr = _Itr->Next) {
		struct Mission* _Mission = _Itr->Data;
		static int _Fired = 0;

		MissionCall(g_LuaState, _Mission, _Guy, NULL);
		if(_Fired == 0 && _Mission == MissionStrToId("BANDT.1")) {
			_Fired = 1;
			MissionCall(g_LuaState, _Mission, g_GameWorld.Player, NULL);
		}
	}
	if(((struct BigGuy*)_GuyNode->Data)->Action != BGACT_NONE)
		MissionCall(g_LuaState, (*_ActionList)[_Guy->Action], _Guy, _Guy->ActionTarget);
	MissionCompare(_GuyNode->Left, _Missions, _ActionList);
	MissionCompare(_GuyNode->Right, _Missions, _ActionList);
}

void RBMissionOnEvent(struct MissionEngine* _Engine, uint32_t _EventId, struct RBNode* _GuyNode) {
	if(_GuyNode == NULL)
		return;
	MissionOnEvent(_Engine, _EventId, _GuyNode->Data, NULL);
	RBMissionOnEvent(_Engine, _EventId, _GuyNode->Left);
	RBMissionOnEvent(_Engine, _EventId, _GuyNode->Right);
}

void MissionEngineThink(struct MissionEngine* _Engine, lua_State* _State, const struct RBTree* _BigGuys) {
	MissionCompare(_BigGuys->Table, &_Engine->MissionsTrigger, &_Engine->ActionMissions);
	
	if(DAY(g_GameWorld.Date) == 0 && MONTH(g_GameWorld.Date) == MARCH)
		RBMissionOnEvent(_Engine, MEVENT_SPRING, _BigGuys->Table);
	else if(DAY(g_GameWorld.Date) == 0 && MONTH(g_GameWorld.Date) == SEPTEMBER)
		RBMissionOnEvent(_Engine, MEVENT_SPRING, _BigGuys->Table);
}

int MissionIdInsert(const int* _One, const struct Mission* _Two) {
	return (*_One) - _Two->Id;
}

int MissionIdSearch(const int* _Id, const struct Mission* _Mission) {
	return (*_Id) - _Mission->Id;
}

const char* MissionFormatText(const char* restrict _FormatIn, const struct MissionTextFormat* _FormatOps,
	int _FormatOpsSz, const struct MissionFrame* _Frame) {
	void* _Obj = NULL;
	char* _DestStr = NULL;
	const char* _FrontStr = _FormatIn;
	const char* _BackStr = _FrontStr;
	char** restrict _Strings = alloca(sizeof(char*) * _FormatOpsSz);
	size_t _ExtraSz = 0;
	size_t _StringSz = 0;
	size_t _SizeOf = 0;
	uint8_t _EscapeChar = 0;
	uint8_t _OpIdx = 0;
	
	for(int i = 0; i < _FormatOpsSz; ++i) {
		switch(_FormatOps[i].Object) {
			case MOBJECT_TARGET:
				if(_Frame->Owner == NULL) {
					Log(ELOG_WARNING, "Warning: cannot format Frame.Owner: Frame.Owner is null.");
					return NULL;
				}
				_Obj = _Frame->Owner;
				break;
			case MOBJECT_FROM:
				if(_Frame->From == NULL) {
					Log(ELOG_WARNING, "Warning: cannot format Frame.From: Frame.From is null.");
					return NULL;
				}
				_Obj = _Frame->From;
				break;
		}
		switch(_FormatOps[i].Param) {
			case MOPCODE_FIRSTNAME:
				_StringSz = strlen(((struct BigGuy*)_Obj)->Person->Name);
				_Strings[i] = FrameAlloc(_StringSz + 1);
				strcpy(_Strings[i], ((struct BigGuy*)_Obj)->Person->Name);
				break;
			case MOPCODE_LASTNAME:
				_StringSz = strlen(((struct BigGuy*)_Obj)->Person->Family->Name);
				_Strings[i] = FrameAlloc(_StringSz + 1);
				strcpy(_Strings[i], ((struct BigGuy*)_Obj)->Person->Family->Name);
				break;
			default:
				Assert(0);
				break;
		}
		_ExtraSz += _StringSz;	
	}
	_SizeOf = strlen(_FormatIn) + _ExtraSz + 1;
	_DestStr = FrameAlloc(_SizeOf);
	_DestStr[0] = '\0';
	while((*_FrontStr) != '\0') {
		switch(*_FrontStr) {
			case '\\':
				_EscapeChar = 1;
				break;
			case '[':
				if(_EscapeChar == 1) {
					_EscapeChar = 0;
					break;
				}
				strncat(_DestStr, _BackStr, _FrontStr - _BackStr);
				strcat(_DestStr, _Strings[_OpIdx]);
				++_OpIdx;
				break;
			case ']':
				if(_EscapeChar == 1) {
					_EscapeChar = 0;
					break;
				}
				_BackStr = _FrontStr + 1;
				break;
			default:
				_EscapeChar = 0;
				break;
		}
		++_FrontStr;	
	}
	strcat(_DestStr, _BackStr);
	FrameReduce(_SizeOf);
	return _DestStr;
}

struct PersonSelector {
	uint16_t Count;
	int8_t Male;
	int8_t Adult;
	int8_t Caste;
	uint8_t OnlyBigGuy;
};

void LuaPersonSelector(lua_State* _State, struct PersonSelector* _Selector) {
	lua_pushstring(_State, "Male");
	lua_rawget(_State, -2);
	if(lua_type(_State, -1) != LUA_TBOOLEAN) {
		return (void) luaL_error(_State, "Male is not a boolean.");
	}
	_Selector->Male = (lua_toboolean(_State, -1) == 1) ? (EMALE) : (EFEMALE);
	lua_pop(_State, 1);

	lua_pushstring(_State, "Adult");
	lua_rawget(_State, -2);
	if(lua_type(_State, -1) != LUA_TBOOLEAN) {
		if(lua_type(_State, -1) == LUA_TNIL) {
			_Selector->Adult = 1;
		} else {
			return (void) luaL_error(_State, "Adult is not a boolean.");
		}
	} else {
		_Selector->Adult = lua_toboolean(_State, -1);
	}
	lua_pop(_State, 1);
}

static inline uint8_t LuaPersonSelect(const struct Person* _Person, const struct MissionFrame* _Frame, const struct PersonSelector* _Selector) {
	if((_Selector->Male != -1 && _Person->Gender != _Selector->Male)
		|| (_Selector->Adult != -1 && PersonMature(_Person) != _Selector->Adult)) {
			return true;
	}
	return false;
}

static inline int RandomPersonItr(lua_State* _State, const struct Settlement* _Settlement, 
	const struct PersonSelector* _Selector, const struct MissionFrame* _Frame, int _Ct, int* _Skipped) {
	for(struct Person* _Person = _Settlement->People; _Person != NULL && _Ct > 0; _Person = _Person->Next) {
		//Pick someone who didnt fire the trigger.
		if(LuaPersonSelect(_Person, _Frame, _Selector)) {
			++(*_Skipped);
			continue;
		}
		LuaCtor(_State, _Person, LOBJ_PERSON);
		lua_rawseti(_State, -2, _Ct);
		--_Ct;
	}
	return _Ct;
}

static inline int RandomBigGuyItr(lua_State* _State, const struct Settlement* _Settlement,
	const struct PersonSelector* _Selector, const struct MissionFrame* _Frame, int _Ct, int* _Skipped) {
	struct LnkLst_Node* _Itr = NULL;
	struct BigGuy* _Guy = NULL;
	struct Person* _Person = NULL;

	assert(_Settlement->BigGuys.Size != 0 && "LuaMissionGetRandomPerson: Settlement has no BigGuys.");
	_Itr = _Settlement->BigGuys.Front;
	_Guy = (struct BigGuy*)_Itr->Data;
	_Person = _Guy->Person;
	while(_Itr != NULL && _Ct > 0) {
		if(LuaPersonSelect(_Person, _Frame, _Selector)) {
			++_Skipped;
			goto loop_end;
		}
		LuaCtor(_State, _Guy, LOBJ_BIGGUY);
		lua_rawseti(_State, -2, _Ct);
		--_Ct;
		loop_end:
		_Itr = _Itr->Next;
		_Guy = _Itr->Data;
		_Person = _Guy->Person;	
	}
	return _Ct;
}

int LuaMissionGetRandomPerson(lua_State* _State) {
	enum LuaArgs {
		LARG_FRAME = 1,
		LARG_SELECTORS,
		LRET_PLIST //List of people to return.
	};
	struct MissionFrame* _Frame = LuaCheckClass(_State, LARG_FRAME, LOBJ_MISSIONFRAME);
	struct Settlement* _Settlement = NULL;
	int _Ct = 0;
	int _Skipped = 0;
	struct PersonSelector _Selector = {0};
	
	lua_settop(_State, 2);
	luaL_checktype(_State, LARG_SELECTORS, LUA_TTABLE);
	_Selector.Count = 1;
	_Selector.OnlyBigGuy = true;
	LuaPersonSelector(_State, &_Selector);
	lua_newtable(_State);
	_Settlement = FamilyGetSettlement(_Frame->Owner->Person->Family);
	_Ct = _Selector.Count;
	loop_start:
	if(_Selector.OnlyBigGuy == true)
		_Ct = RandomBigGuyItr(_State, _Settlement, &_Selector, _Frame, _Ct, &_Skipped);
	else
		_Ct = RandomPersonItr(_State, _Settlement, &_Selector, _Frame, _Ct, &_Skipped);
	if(_Skipped >= _Settlement->NumPeople)
		goto error;
	if(_Ct > 0)
		goto loop_start;
	if(_Selector.Count == 1)
		lua_rawgeti(_State, LRET_PLIST, 1);
	else
		lua_settop(_State, LRET_PLIST);
	return 1;
	error:
	return luaL_error(_State, "LuaMissionGetRandomPerson: No available person to select.");
}

int LuaMissionCallById(lua_State* _State) {
	const char* _Str = NULL;
	struct BigGuy* _Owner = LuaCheckClass(_State, 2, LOBJ_BIGGUY);
	struct BigGuy* _From = NULL;
	struct Mission* _Mission = NULL;

	//if(lua_type(_State, 1) == LUA_TSTRING) {
	//	_Str = lua_tostring(_State, 1);
	_Str = luaL_checkstring(_State, 1);
	if((_Mission = MissionStrToId(_Str)) == NULL)
		return luaL_error(_State, "%s is an invalid mission name.", _Str);
	//	if((_Id = MissionStrToId(_Str)) == -1)
	//		return 0;
	//} else {
		//_Id = luaL_checkinteger(_State, 1);
	//}
	/*if((_Mission = RBSearch(&g_MissionEngine.MissionId, &_Id)) == NULL)
		return (_Str != NULL) ? 
			(luaL_error(_State, "Attempted to call nil mission %s", _Str)) :
			(luaL_error(_State, "Attempted to call nil mission %d", _Id));
			*/
	if(lua_gettop(_State) >= 3) {
		_From = LuaCheckClass(_State, 3, LOBJ_BIGGUY);
	}
	MissionCall(_State, _Mission, _Owner, _From);
	return 0;
}

void MissionLoadOption(lua_State* _State, struct Mission* _Mission) {
#define FUNC_FORMATSZ (4)
	int _FormatSz = 0;
	const char* _Text = NULL;
	const char* _FormatStr = NULL;
	MLRef _Condition = 0;
	MLRef _Trigger = 0;
	uint8_t _Object[FUNC_FORMATSZ];
	uint8_t _Param[FUNC_FORMATSZ];

	lua_pushstring(_State, "Options");
	lua_rawget(_State, -2);
	if(lua_type(_State, -1) != LUA_TTABLE) {
		if(lua_type(_State, -1) != LUA_TNIL)
			return (void) luaL_error(_State, "Mission's options is not a table.");
		/* If we leave out the Options table assume we want a single
		 * option that has the text "Ok" and does nothing.
		 */
		 default_option:
		 if((_Mission->Flags & MISSION_FNOMENU) == MISSION_FNOMENU) {
			_Mission->OptionCt = 0;
			return;
		 }
		_Mission->OptionCt = 1;
		_Mission->Options[0].Name = "Ok";
		_Mission->Options[0].Condition = 0;
		_Mission->Options[0].Action = 0;
		_Mission->Options[0].Utility = 0; 
		_Mission->Options[0].UtilitySz = 0; 
		_Mission->Options[0].TextFormatSz = 0; 
		return;
	} else if(lua_rawlen(_State, -1) == 0) {
		goto default_option;
	} 
	if((_Mission->Flags & MISSION_FNOMENU) == MISSION_FNOMENU)
		return (void) luaL_error(_State, "Mission cannot have NoMenu be true and have an option table.");
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
		_Condition = 0;
		_Trigger = 0;
		lua_pushstring(_State, "Text");
		lua_rawget(_State, -2);
		if(LuaGetString(_State, -1, &_Text) == 0)
			return (void) luaL_error(_State, "Mission.Options Text is not a string.");
		lua_pop(_State, 1);

		_FormatStr = _Text;
		while((_FormatStr = MissionParseStr(_FormatStr, &_Object[_FormatSz], &_Param[_FormatSz])) != NULL &&
			_FormatSz < FUNC_FORMATSZ) ++_FormatSz;
		_Mission->Options[_Mission->OptionCt].TextFormatSz = _FormatSz;
		if(_FormatSz > 0)
			_Mission->Options[_Mission->OptionCt].TextFormat = calloc(sizeof(struct MissionTextFormat), _FormatSz);
		else 
			_Mission->Options[_Mission->OptionCt].TextFormat = NULL;
		lua_pushstring(_State, "Condition");
		lua_rawget(_State, -2);
		if(lua_isnil(_State, -1) == 0) {
			if(lua_type(_State, -1) != LUA_TFUNCTION)
				return (void) luaL_error(_State, "Trigger.Condition is not a function.");
			_Condition = luaL_ref(_State, LUA_REGISTRYINDEX);
		} else {
			_Condition = 0;
			lua_pop(_State, 1);
		}

		lua_pushstring(_State, "Trigger");
		lua_rawget(_State, -2);
		if(lua_type(_State, -1) != LUA_TNIL) {
			if(lua_type(_State, -1) != LUA_TFUNCTION)
				return (void) luaL_error(_State, "Mission.Trigger entry is not a function.");
			_Trigger = luaL_ref(_State, LUA_REGISTRYINDEX);
		} else {
			_Trigger = 0;
			lua_pop(_State, 1);
		}

		lua_pushstring(_State, "AIUtility");
		lua_rawget(_State, -2);
		if(lua_type(_State, -1) != LUA_TFUNCTION)
			return (void) luaL_error(_State, "Mission.AIUtility is not a function.");
		_Mission->Options[_Mission->OptionCt].Utility = luaL_ref(_State, LUA_REGISTRYINDEX);
		_Mission->Options[_Mission->OptionCt].Name = calloc(sizeof(char), strlen(_Text) + 1);
		strcpy(_Mission->Options[_Mission->OptionCt].Name, _Text);
		_Mission->Options[_Mission->OptionCt].Condition = _Condition;
		_Mission->Options[_Mission->OptionCt].Action = _Trigger;
		++_Mission->OptionCt;
		lua_pop(_State, 1);
	}
#undef FUNC_FORMATSZ
}

int LuaMissionLoad(lua_State* _State) {
#define FUNC_FORMATSZ (16)

	struct Mission* _Mission = CreateMission();
	const char* _TempStr = NULL;
	int _MeanTime = 0;
	uint8_t _Id = 0;
	const char* _FormatStr = NULL;
	const char* _ErrorStr = NULL;
	//const struct Array* _NSArray = NULL;
	char* _Namespace = alloca(MISENG_NSSTR);
	double _Prob = 0;
	uint8_t _Object[FUNC_FORMATSZ];
	uint8_t _Param[FUNC_FORMATSZ];
	uint8_t _FormatSz = 0;
	uint8_t _Action = 0;

	lua_pushstring(_State, "Id");
	lua_rawget(_State, 1);
	if(lua_type(_State, -1) != LUA_TSTRING)
		luaL_error(_State, "Mission's Id is not a string.");
	_TempStr = lua_tostring(_State, -1);
	if(MissionStrToId(_TempStr) != NULL) {
		_ErrorStr = "Mission Id %s is not a valid Id.";
		goto error;
	}
	MissionStrToNS(_TempStr, &_Namespace, &_Id);
	/*for(int i = 0; i < MISENG_NSSTR - 1; ++i) {
		if(isalpha(_TempStr[i]) == 0) {
			if(_TempStr[i] == '.') {
				int _TempId = 0;
				_Namespace[i + 1] = '\0';
				if((_TempId = atoi(&(_TempStr[i]) + 1)) <= 0 || _TempId > 0xFF) {
					_ErrorStr = "Mission id is not valid.";
					goto error;
				}
				_Id = _TempId;
				break;
			} else {
				_ErrorStr = "Mission Id contains a namespace but not an id.";
				goto error;
			}
		}
		_Namespace[i] = _TempStr[i];
	}
	if((_NSArray = HashSearch(&g_MissionEngine.Namespaces, _Namespace)) != NULL) {
		if(_NSArray->TblSize <= _Id) {
			if(_NSArray->Table[_Id] != NULL) {
				g_MissionEngine.ActionMissions[_Action] = NULL;
				return luaL_error(_State, "Cannot load mission %s.%d id is already in use.", _Namespace, _Id);
			}
		}
	}*/
	_Mission->Id = g_MissionId++;
	lua_pop(_State, 1);

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
	_FormatStr = _TempStr;
	while((_FormatStr = MissionParseStr(_FormatStr, &_Object[_FormatSz], &_Param[_FormatSz])) != NULL &&
		_FormatSz < FUNC_FORMATSZ) ++_FormatSz;
	_Mission->Description = calloc(sizeof(char), strlen(_TempStr) + 1);
	_Mission->TextFormatSz = _FormatSz;
	if(_FormatSz > 0)
		_Mission->TextFormat = calloc(sizeof(struct MissionTextFormat), _FormatSz);
	else 
		_Mission->TextFormat = NULL;
	for(int i = 0; i < _FormatSz; ++i) {
		_Mission->TextFormat[i].Object = _Object[i];
		_Mission->TextFormat[i].Param = _Param[i];
	}
	strcpy(_Mission->Description, _TempStr);
	lua_pop(_State, 1);

	MissionLoadOption(_State, _Mission);
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
	lua_pop(_State, 1);

	lua_pushstring(_State, "Action");
	lua_rawget(_State, 1);
	if(lua_type(_State, -1) != LUA_TNIL) { 
		if(lua_type(_State, -1) != LUA_TNUMBER)
			return luaL_error(_State, "Mission.Action is not an integer.");
		_Action = lua_tointeger(_State, -1);
		if(_Action <= 0 || _Action >= BGACT_SIZE) {
			_ErrorStr = "Mission.Action is not a valid integer. (%d is invalid.)";
			goto error;
		}
		g_MissionEngine.ActionMissions[_Action] = _Mission;
	}
	lua_pop(_State, 1);


	lua_pushstring(_State, "Trigger");
	lua_rawget(_State, 1);
	if(lua_type(_State, -1) != LUA_TNIL) {
		if(lua_type(_State, -1) != LUA_TFUNCTION)
			return luaL_error(_State, "Mission.Trigger is not a function.");
		_Mission->Trigger = luaL_ref(_State, LUA_REGISTRYINDEX); 
	} else {
		lua_pop(_State, 1);
	}
	lua_pushstring(_State, "Event");
	lua_rawget(_State, 1);
	if(lua_type(_State, -1) != LUA_TNIL) {
		if(lua_type(_State, -1) != LUA_TNUMBER)
			return luaL_error(_State, "Mission.Event is not an integer.");
		_Mission->TriggerEvent = lua_tointeger(_State, -1);
		lua_pop(_State, 1);
		_Mission->Flags = _Mission->Flags | MISSION_FEVENT;
		lua_pushstring(_State, "EventChance");
		lua_rawget(_State, 1);
		if(lua_type(_State, -1) != LUA_TNUMBER) {
			_Mission->MeanPercent = 1.0;
		} else {
				_Mission->MeanPercent = lua_tonumber(_State, -1);
			if(_Mission->MeanPercent <= 0 || _Mission->MeanPercent > 1.0)
				return luaL_error(_State, "Mission.EventChance not greater than 0 and less than 1.(EventChance is %f).", _Mission->MeanPercent);
		}
		//goto skip_meantime;
	}
	lua_pop(_State, 1);

	lua_pushstring(_State, "MeanTime");
	lua_rawget(_State, 1);
	if(lua_type(_State, -1) != LUA_TTABLE) {
		if(lua_type(_State, -1) == LUA_TNIL) {
			_MeanTime = 1;
		} else if(lua_type(_State, -1) != LUA_TNUMBER) {
			return luaL_error(_State, "Mission.MeanTime must be a table.");
		}
		LuaGetInteger(_State, -1, &_MeanTime);
		goto escape_meantime;
	}
	lua_pushstring(_State, "Base");
	lua_rawget(_State, -2);
	if(LuaGetInteger(_State, -1, &_MeanTime) == 0) {
		return luaL_error(_State, "Mission.MeanTime must be an integer");
	}
	lua_pop(_State, 1);
	escape_meantime:
	_Mission->MeanModsSz = lua_rawlen(_State, -1);
	_Mission->MeanMods = calloc(sizeof(float), _Mission->MeanModsSz);	
	_Mission->MeanModTrig = calloc(sizeof(float), _Mission->MeanModsSz);	
	for(uint8_t i = 0; i < _Mission->MeanModsSz; ++i) {
		lua_rawgeti(_State, -1, i + 1);
		if(lua_type(_State, -1) != LUA_TTABLE)
			return luaL_error(_State, "Mission.MeanTime elements must be a table.");
		lua_pushstring(_State, "Modifier");
		lua_rawget(_State, -2);
		if(lua_type(_State, -1) != LUA_TNUMBER)
			return luaL_error(_State, "Mission.MeanTime element #%d does not contain key 'Modifier' containing a number.");
		_Mission->MeanMods[i] = lua_tonumber(_State, -1);
		lua_pop(_State, 1);
		lua_pushstring(_State, "Trigger");
		lua_rawget(_State, -2);
		if(lua_type(_State, -1) != LUA_TFUNCTION) {
			_ErrorStr = "Mission.MeanTime element #%d does not contain key 'Trigger' containing a function.";
			goto error;
		}
		_Mission->MeanModTrig[i] = luaL_ref(_State, LUA_REGISTRYINDEX);
		lua_pop(_State, 1);
	}
	_Mission->MeanTime = (uint16_t) _MeanTime;
	_Prob = (((double) 1) / _MeanTime);
	_Mission->MeanPercent = pow(1 - _Prob, _MeanTime - 1) * _Prob;
	lua_pop(_State, 1);
	lua_pushstring(_State, "NoMenu");
	lua_rawget(_State, 1);
	if(lua_type(_State, -1) != LUA_TNIL) {
		if(lua_type(_State, -1) == LUA_TBOOLEAN) {
			_Mission->Flags = _Mission->Flags | MISSION_FNOMENU;
		} else {
			return luaL_error(_State, "Mission.NoMenu must be a boolean.");
		}
	}
	lua_pushstring(_State, "OnTrigger");
	lua_rawget(_State, 1);
	if(lua_isnil(_State, -1) == 0) {
		if(lua_type(_State, -1) == LUA_TFUNCTION) {
			_Mission->OnTrigger = luaL_ref(_State, LUA_REGISTRYINDEX);
		} else {
			return luaL_error(_State, "Mission.OnTrigger must be a function.");
		}
	} else {
		_Mission->OnTrigger = 0;
	}
	MissionInsert(&g_MissionEngine, _Mission, _Namespace, _Id);
	Log(ELOG_DEBUG, "Loaded mission %s", _Mission->Name);
	return 0;
	error:
	g_MissionEngine.ActionMissions[_Action] = NULL;
	luaL_error(_State, _ErrorStr);
	return 0;
#undef FUNC_FORMATSZ
}

int LuaMissionSetVar(lua_State* _State) {
	struct MissionFrame* _Frame = LuaCheckClass(_State, 1, LOBJ_MISSIONFRAME);
	const char* _Key = luaL_checkstring(_State, 2);

	for(int i = 0; i < _Frame->StackSz; ++i) {
		if(strcmp(_Frame->StackKey[i], _Key) == 0) {
			LuaToPrimitive(_State, 3, &_Frame->Stack[i]);
			return 0;
		}
	}
	if(_Frame->StackSz >= MISSION_STACKSZ)
		return luaL_error(_State, "Cannot add Frame var: to many variables exist.");
	LuaToPrimitive(_State, 3, &_Frame->Stack[_Frame->StackSz]);
	_Frame->StackKey[_Frame->StackSz] = calloc(strlen(_Key) + 1, sizeof(char));
	++_Frame->StackSz;
	return 0;
}

int LuaMissionStatUtility(lua_State* _State) {
	int _StatOne = luaL_checkinteger(_State, 1);
	int _StatTwo = luaL_checkinteger(_State, 2);

	if(_StatOne < STAT_MIN || _StatOne > STAT_MAX)	
		return luaL_error(_State, "Argument #1 is not a valid value for a stat. Expected [1, 100] got %d", _StatOne);
	if(_StatTwo < STAT_MIN || _StatTwo > STAT_MAX)	
		return luaL_error(_State, "Argument #2 is not a valid value for a stat. Expected [1, 100] got %d", _StatTwo);
	lua_pushinteger(_State, (MISSION_UTMAX / 2) + ((_StatOne - _StatTwo) * MISSION_UTMAX/ 100));
	return 1;
}

int LuaMissionGetVar(lua_State* _State) {
	struct MissionFrame* _Frame = LuaCheckClass(_State, 1, LOBJ_MISSIONFRAME);
	const char* _Key = luaL_checkstring(_State, 2);

	for(int i = 0; i < _Frame->StackSz; ++i) {
		if(strcmp(_Key, _Frame->StackKey[i]) == 0) {
			PrimitiveLuaPush(_State, &_Frame->Stack[i]);
			return 1;
		}
	}
	lua_pushnil(_State);
	return 1;
}

int LuaMissionCombatRound(lua_State* _State) {
	struct BigGuy* _One = LuaCheckClass(_State, 1, LOBJ_BIGGUY);
	struct BigGuy* _Two = LuaCheckClass(_State, 2, LOBJ_BIGGUY);
	int _Result = 0;

	_Result = BigGuyOpposedCheck(_One, _Two, BGSKILL_COMBAT);
	if(_Result > 0) {
		_Result = BigGuySkillCheck(_One, BGSKILL_STRENGTH, 50 + _Two->Stats[BGSKILL_TOUGHNESS]) + 1;
		goto end;
	} else if(_Result < 0) {
		_Result = -(BigGuySkillCheck(_Two, BGSKILL_STRENGTH, 50 + _One->Stats[BGSKILL_TOUGHNESS] + 1));
		goto end;
	} else {
		lua_pushinteger(_State, 0);
	}
	end:
	lua_pushinteger(_State, _Result);
	return 1;
}

const char* MissionParseStr(const char* _Str, uint8_t* _ObjId, uint8_t* _ParamId) {
#define PARSESTR_BUFLEN (64)

	enum MissionParseStrEnum {
		PARSESTR_NONE,
		PARSESTR_OPNBRCK,
		PARSESTR_CLSBRCK,
		PARSESTR_DOT,
		PARSESTR_BCKSLASH
	};

	const char* _Pos = _Str;
	uint8_t _State = PARSESTR_NONE;
	uint8_t _ObjectSz= 0;
	uint8_t _ParamSz = 0;
	char _Object[PARSESTR_BUFLEN];
	char _Param[PARSESTR_BUFLEN];

	do {
		switch(_State) {
			case PARSESTR_NONE:
				switch(*_Pos) {
					case '[':
						_State = PARSESTR_OPNBRCK;
						break;
					case ']':
					//case '.':
					//	return NULL;
					case '\\':
						_State = PARSESTR_BCKSLASH;
						break;
					case '\0':
						return NULL;
					default:
						break;
				}
			break;
			case PARSESTR_OPNBRCK:	
				switch(*_Pos) {
					case '[':
						return NULL;
					case ']':
						return NULL;	
					case '.':
						if(_ObjectSz <= 0)
							return NULL;
						_Object[_ObjectSz] = '\0';
						_State = PARSESTR_DOT;
						break;
					default:
						if(_ObjectSz - 1 >= PARSESTR_BUFLEN)
							return NULL;
						_Object[_ObjectSz++] = *_Pos;
						break;
				}
				break;
			case PARSESTR_DOT:
				switch(*_Pos) {
					case '[':
					case '.':
						return NULL;
					case ']':
						_State = PARSESTR_NONE;
						if(_ParamSz <= 0)
							return NULL;
						_Param[_ParamSz] = '\0';
						++_Pos;
						goto found_token;
					default:
						if(_ParamSz - 1 >= PARSESTR_BUFLEN)
							return NULL;
						_Param[_ParamSz++] = *_Pos;
				}
				break;
			case PARSESTR_BCKSLASH:
				_State = PARSESTR_NONE;
				break;
		}
	} while(_Pos++ != NULL);
	found_token:
	for(int i = 0; i < MOBJECT_SIZE; ++i) {
		if(strcmp(g_MissionObjects[i], _Object) == 0) {
			*_ObjId = i;
			break;
		}
	}
	for(int i = 0; i < MOPCODE_SIZE; ++i) {
		if(strcmp(g_MissionParams[i], _Param) == 0) {
			*_ParamId = i;
			break;
		}
	}
	return _Pos;
#undef PARSESTR_BUFLEN
}

void InitMissionLua(lua_State* _State) {
	const char* _Temp = NULL;
	static const char* _GlobalVars[] = {
		"Stat",
		"Relation",
		"ipairs",
		"pairs",
		"print",
		"Action",
		"Null",
		NULL
	};

	LuaRegisterObject(_State, "MissionOption", LOBJ_MISSIONOPTION, LUA_REFNIL, g_LuaFuncsMissionOption);
	lua_settop(_State, 0);
	lua_newtable(_State);
	lua_pushstring(_State, "Mission");
	luaL_newlib(_State, g_LuaMissionRuleFuncs);
	lua_rawset(_State, 1);

	for(int i = 0; _GlobalVars[i] != NULL; ++i) {
		lua_pushstring(_State, _GlobalVars[i]);
		lua_getglobal(_State, _GlobalVars[i]);
		lua_rawset(_State, 1);
	}

/*
	lua_pushstring(_State, "Action");
	lua_getglobal(_State, "BigGuy");
	lua_pushstring(_State, "Action");
	lua_rawget(_State, -2);
	lua_remove(_State, -2);
	lua_rawset(_State, 1);
*/
	lua_pushstring(_State, "Rule");
	luaL_getmetatable(_State, "Rule");
	lua_rawset(_State, 1);
	for(int i = 0; g_LuaMissionEnv[i] != NULL; ++i) {
		luaL_getmetatable(_State, g_LuaMissionEnv[i]);
		if(lua_type(_State, -1) != LUA_TTABLE) {
			Log(ELOG_WARNING, "%s is not a valid table to include the Mission env table.", g_LuaMissionEnv[i]);
			lua_pop(_State, 1);
			continue;
		}
		lua_pushstring(_State, g_LuaMissionEnv[i]);
		lua_newtable(_State);
		lua_pushnil(_State);
		while(lua_next(_State, -4) != 0) {
			if(lua_iscfunction(_State, -1) == 0) {
				_Temp = lua_tostring(_State, -2);
				if(_Temp[0] == '_') {
					lua_pop(_State, 1);
					continue;
				}
			}
			Assert(lua_type(_State, -1) == LUA_TFUNCTION);
			lua_pushvalue(_State, -2);
			lua_pushvalue(_State, -2);
			lua_remove(_State, -3);
			lua_rawset(_State, -4);
		}
		lua_rawset(_State, 1);
		lua_pop(_State, 1);
	}
	lua_pushstring(_State, "Random");
	lua_pushcfunction(_State, LuaRandom);
	lua_rawset(_State, 1);

	lua_pushstring(_State, "Event");
	lua_newtable(_State);
	LuaAddEnum(_State, -1, g_LuaMissionEventEnum);
	lua_rawset(_State, 1);
	LuaSetEnv(_State, "Mission");
	LuaRegisterObject(_State, "MissionFrame", LOBJ_MISSIONFRAME, LUA_REFNIL, g_LuaFuncsMissionFrame);
}

int LuaMissionOptionGetName(lua_State* _State) {
	struct MissionOption* _Option = LuaCheckClass(_State, 1, LOBJ_MISSIONOPTION);
	struct MissionFrame* _Frame = LuaCheckClass(_State, 2, LOBJ_MISSIONFRAME);

	if(lua_gettop(_State) < 1) {
		lua_pushstring(_State, _Option->Name);
		return 1;
	}
	lua_pushstring(_State, MissionFormatText(_Option->Name, _Option->TextFormat, _Option->TextFormatSz, _Frame));
	return 1;
}

int LuaMissionOptionConditionSatisfied(lua_State* _State) {
	struct MissionOption* _Option = LuaCheckClass(_State, 1, LOBJ_MISSIONOPTION);
	struct MissionFrame* _Frame = LuaCheckClass(_State, 2, LOBJ_MISSIONFRAME);

	if(_Option->Condition == 0) {
		lua_pushboolean(_State, 1);
		return 1;
	}
	CallMissionCond(_State, _Option->Condition, _Frame);
	return 1;
}

int LuaUtilityLinear(lua_State* _State) {
	int _Min = luaL_checkinteger(_State, 1);
	int _Max = luaL_checkinteger(_State, 2);
	int _Num = luaL_checkinteger(_State, 3);

	if(_Num <= _Min) {
		lua_pushinteger(_State, 0);
		return 1;
	}
	if(_Num >= _Max) {
		lua_pushinteger(_State, MISSION_UTMAX);
		return 1;
	}

	lua_pushinteger(_State, (_Num - _Min) / ((float)(_Max - _Min)) * MISSION_UTMAX);
	return 1;
}

int LuaUtilityQuadratic(lua_State* _State) {
	int _Min = luaL_checkinteger(_State, 1);
	int _Max = luaL_checkinteger(_State, 2);
	int _Num = luaL_checkinteger(_State, 3);
	int _Mult = 0;
	int _MaxMult = 0;

	if(_Num <= _Min) {
		lua_pushinteger(_State, 0);
		return 1;
	}
	if(_Num >= _Max) {
		lua_pushinteger(_State, MISSION_UTMAX);
		return 1;
	}
	_Mult = (_Num - _Min);
	_Mult *= _Mult;
	_MaxMult = (_Max - _Min);
	_MaxMult *= _MaxMult;
	lua_pushnumber(_State, _Mult / ((float)_MaxMult) * MISSION_UTMAX);
	return 1;
}
