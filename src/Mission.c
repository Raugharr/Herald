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
#define MISSION_DEFMEANTIME (0)

struct MissionFrame;

static int g_PlayerMisCall = 0;

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
	{"Var", LuaMissionGetVar},
	{"SetVar", LuaMissionSetVar},
	{NULL, NULL},
};

static const luaL_Reg g_LuaFuncsMissionFrame[] =  {
	{"RandomPerson", LuaMissionGetRandomPerson},
	{NULL, NULL}
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

struct MissionFrame {
	struct BigGuy* From;
	struct BigGuy* Owner;
	const struct Mission* Mission;
	struct Primitive Stack[MISSION_STACKSZ];
	uint8_t StackSz;
	uint8_t IsOptSel;
};

struct MissionFrame* CreateMissionFrame(struct BigGuy* _From, struct BigGuy* _Target, const struct Mission* _Mission) {
	struct MissionFrame* _MissionFrame = (struct MissionFrame*) malloc(sizeof(struct MissionFrame));

	_MissionFrame->From = _From;
	_MissionFrame->Owner= _Target;
	LnkLstPushBack(&g_GameWorld.MissionFrame, _MissionFrame);
	_MissionFrame->StackSz = 0;
	_MissionFrame->Mission = _Mission;
	_MissionFrame->IsOptSel = 0;
	return _MissionFrame;
}

void DestroyMissionFrame(struct MissionFrame* _MissionFrame) {
	free(_MissionFrame);
}

static inline void SetupMissionFrame(lua_State* _State, struct MissionFrame* _Frame) {
	LuaCtor(_State, "MissionFrame", _Frame);
	lua_pushstring(_State, "From");
	(_Frame->From != NULL) 
		? LuaCtor(_State, "BigGuy", _Frame->From) 
		: lua_pushnil(_State);
	lua_rawset(_State, -3);
	lua_pushstring(_State, "Owner");
	LuaCtor(_State, "BigGuy", _Frame->Owner);
	lua_rawset(_State, -3);
}

static inline uint8_t CallMissionCond(lua_State* _State, MLRef _Ref, struct MissionFrame* _Frame) {
	lua_rawgeti(_State, LUA_REGISTRYINDEX, _Ref);
	SetupMissionFrame(_State, _Frame);
	LuaCallFunc(_State, 1, 1, 0);
	if(lua_toboolean(_State, -1) == 0) {
		lua_pop(_State, 1);
		return 0;
	}
	lua_pop(_State, 1);
	return 1;
}

static inline void CallMissionAction(lua_State* _State, MLRef _Ref, struct MissionFrame* _Frame) {
	lua_rawgeti(_State, LUA_REGISTRYINDEX, _Ref);
	SetupMissionFrame(_State, _Frame);
	LuaCallFunc(_State, 1, 0, 0);
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

void MissionInsert(struct MissionEngine* _Engine, struct Mission* _Mission) {
	if(RBSearch(&_Engine->MissionId, _Mission) != NULL) {
		Log(ELOG_WARNING, "Mission cannot be loaded id %f is already in use.", _Mission->Id);
		return;
	}
	RBInsert(&_Engine->MissionId, _Mission);
	if((_Mission->Flags & MISSION_FEVENT) != 0) {
		LnkLstPushBack(&_Engine->EventMissions[_Mission->TriggerEvent], _Mission);
	}
	if((_Mission->Flags & MISSION_FONLYTRIGGER) == 0) {
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
			break;
		}
		_Itr = _Itr->Next;
	}
}

struct Mission* CreateMission() {
	struct Mission* _Mission = (struct Mission*) malloc(sizeof(struct Mission));

	_Mission->TriggerType = 0;
	_Mission->Name = NULL;
	_Mission->Description = NULL;
	_Mission->MeanPercent = 0.0;
	_Mission->Trigger = 0;
	_Mission->OnTrigger = 0;
	_Mission->MeanModTrig = 0;
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
		ConstructLinkedList(&_Engine->EventMissions[i]);
	}
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
	struct MissionFrame* _Frame= NULL;
	const char* _NewDesc = NULL;

	if(_Mission == NULL)
		return;
	_Frame = CreateMissionFrame(_From, _Owner, _Mission);
	_NewDesc = _Mission->Description;
	if(g_GameWorld.Player == _Owner) {
		if(_Mission->TextFormatSz > 0) {
			//Breaks when MissionFormatText is run on a coroutine other than the main coroutine.
			//because the rule used points to the main coroutine's lua_State.
			_NewDesc = MissionFormatText(_Mission->Description, _Mission->TextFormat, _Mission->TextFormatSz, _Frame); 

		/*	if(_SizeOf == 0) {
				Log(ELOG_WARNING, "Mission %s failed: MissionFormatText failed to format text.", _Mission->Name);
				return;
			}
			*/
		}
		if(_Mission->OnTrigger != 0) {
			CallMissionAction(_State, _Mission->OnTrigger, _Frame);
		}
		if((_Mission->Flags & MISSION_FNOMENU) == MISSION_FNOMENU)
			return;
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
		lua_pushlightuserdata(_State, _Frame);
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
		//float _BestUtility = -1.0;
		//float _Utility = 0.0;

		if(_Mission->OnTrigger != 0) {
			CallMissionCond(_State, _Mission->OnTrigger, _Frame);
		}
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
		CallMissionAction(_State, _Mission->Options[_BestIndex].Action, _Frame);
		MissionFrameClear(_Frame);
	}
}

void MissionAction(const char* _Name, struct BigGuy* _From, struct BigGuy* _Target) {
	int _MissionId = MissionStrToId(_Name);

	lua_settop(g_LuaState, 0);
	CoResume(CoSpawn(MissionCall, 4, g_LuaState, RBSearch(&g_MissionEngine.MissionId, &_MissionId)/*_Mission*/, _From, _Target));
}

void DestroyMissionEngine(struct MissionEngine* _Engine) {
}

void MissionOnEvent(struct MissionEngine* _Engine, uint32_t _EventType, struct BigGuy* _Guy) {
	Assert(_EventType < EventUserOffset());
	_EventType = _EventType - EventUserOffset(); 
	for(struct LnkLst_Node* _Itr = _Engine->EventMissions[_EventType].Front; _Itr != NULL; _Itr = _Itr->Next) {
		struct Mission* _Mission = _Itr->Data;
		struct MissionFrame* _Frame = CreateMissionFrame(NULL, _Guy, _Mission);

		if(CallMissionCond(g_LuaState, _Mission->Trigger, _Frame) != 0)
			MissionCall(g_LuaState, _Mission, NULL, _Guy);
	}
}

void MissionCompare(struct RBNode* _GuyNode, struct LinkedList* _Missions) {
	if(_GuyNode == NULL)
		return;
	for(struct LnkLst_Node* _Itr = _Missions->Front; _Itr != NULL; _Itr = _Itr->Next) {
		struct Mission* _Mission = _Itr->Data;
		double _Percent = _Mission->MeanPercent;
		struct MissionFrame _Frame = {0};
		static int _Fired = 0;

		_Frame.Owner = _GuyNode->Data;
		if(_Mission->Trigger != 0 && CallMissionCond(g_LuaState, _Mission->Trigger, &_Frame) == 0)
				continue;
		if(_Fired == 0 && _Mission == StrToMission("BANDT.1")) {
			_Fired = 1;
			MissionCall(g_LuaState, _Mission, g_GameWorld.Player, NULL);
		}
		for(uint8_t i = 0; i < _Mission->MeanModsSz; ++i) {
			if(_Mission->MeanModTrig[i] == 0 || CallMissionCond(g_LuaState, _Mission->MeanModTrig[i], &_Frame) != 0)
				_Percent = _Percent * _Mission->MeanMods[i];
		}
		if(Rand() / ((double)0xFFFFFFFFFFFFFFFF) <= _Percent)
			MissionCall(g_LuaState, _Mission, _GuyNode->Data, NULL);
	}
	MissionCompare(_GuyNode->Left, _Missions);
	MissionCompare(_GuyNode->Right, _Missions);
}

void RBMissionOnEvent(struct MissionEngine* _Engine, uint32_t _EventId, struct RBNode* _GuyNode) {
	if(_GuyNode == NULL)
		return;
	MissionOnEvent(_Engine, _EventId, _GuyNode->Data);
	RBMissionOnEvent(_Engine, _EventId, _GuyNode->Left);
	RBMissionOnEvent(_Engine, _EventId, _GuyNode->Right);
}

void MissionEngineThink(struct MissionEngine* _Engine, lua_State* _State, const struct RBTree* _BigGuys) {
	MissionCompare(_BigGuys->Table, &_Engine->MissionsTrigger);
	
	if(MONTH(g_GameWorld.Date) == MARCH)
		RBMissionOnEvent(_Engine, MEVENT_SPRING, _BigGuys->Table);
	else if(MONTH(g_GameWorld.Date) == SEPTEMBER)
		RBMissionOnEvent(_Engine,MEVENT_SPRING, _BigGuys->Table);
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
				_Obj = _Frame->Owner;
				break;
			case MOBJECT_FROM:
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

struct Mission* StrToMission(const char* _Str) {
	int _Id = MissionStrToId(_Str);
	struct Mission* _Mission = NULL;

	if(_Id == -1 || (_Mission = RBSearch(&g_MissionEngine.MissionId, &_Id)) == NULL)
		return NULL;
	return _Mission;

}

int LuaMissionGetRandomPerson(lua_State* _State) {
	enum LuaArgs {
		LARG_FRAME = 1,
		LARG_TABLE
	};
	//int _IsUnique = 0;
	struct MissionFrame* _Frame = LuaCheckClass(_State, LARG_FRAME, "MissionFrame");
	struct Settlement* _Settlement = NULL;
	struct LnkLst_Node* _Itr = NULL;
	struct BigGuy* _Guy = NULL;
	int _Ct = 0;
	int _SkipedGuys = 0;
	int8_t _Male = -1;
	int8_t _Adult = -1;

	luaL_checktype(_State, LARG_TABLE, LUA_TTABLE);
	lua_pushstring(_State, "Male");
	lua_rawget(_State, LARG_TABLE);
	if(lua_type(_State, -1) != LUA_TBOOLEAN) {
		return luaL_error(_State, "Male is not a boolean.");
	}
	_Male = (lua_toboolean(_State, -1) == 1) ? (EMALE) : (EFEMALE);
	lua_pop(_State, 1);
	lua_pushstring(_State, "Adult");
	lua_rawget(_State, LARG_TABLE);
	if(lua_type(_State, -1) != LUA_TBOOLEAN) {
		return luaL_error(_State, "Adult is not a boolean.");
	}
	_Adult = lua_toboolean(_State, -1);
	lua_pop(_State, 1);
	_Settlement = FamilyGetSettlement(_Frame->Owner->Person->Family);
	assert(_Settlement->BigGuys.Size != 0 && "LuaMissionGetRandomPerson: Settlement has no BigGuys.");
	//_IsUnique = lua_toboolean(_State, 1);
	_Itr = _Settlement->BigGuys.Front;
	//_Ct = Random(0, _Settlement->BigGuys.Size);
	_Ct = 1;
	while(_Itr != NULL && _Ct >= 0) {
		loop_start:
		_Guy = (struct BigGuy*)_Itr->Data;
		//Pick someone who didnt fire the trigger.
		if(_Guy == _Frame->From || _Guy == _Frame->Owner
	   || (_Male != -1 && _Guy->Person->Gender != _Male)
	   || (_Adult != -1 && PersonMature(_Guy->Person) != _Adult)) {
			_Guy = NULL;
			++_SkipedGuys;
			goto loop_end;
		}
		/*if(_IsUnique != 0) {
			for(int i = 0; i < g_MissionFrame->StackSz; ++i) {
				if(_Guy == g_MissionFrame->Stack[i]) {
					++_SkipedGuys;
					goto loop_end;
				}
			}
		}*/
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
	//g_MissionFrame->Stack[g_MissionFrame->StackSz].Value.Ptr = _Guy;
	//g_MissionFrame->Stack[g_MissionFrame->StackSz].Type = MADATA_BIGGUY;
	//++g_MissionFrame->StackSz;
	LuaCtor(_State, "BigGuy", _Guy);
	return 1;
	error:
	return luaL_error(_State, "LuaMissionGetRandomPerson: No available person to select.");
}

/*int LuaMissionGetRandomPerson(lua_State* _State) {
	lua_pushcfunction(_State, LuaMissionGetRandomPerson_Aux);
	lua_insert(_State, 1);
	LuaRuleLuaCall(_State);
	return 1;
}*/

int LuaMissionCallById(lua_State* _State) {
	const char* _Str = NULL;
	int _Id = 0;
	struct BigGuy* _Owner = LuaCheckClass(_State, 2, "BigGuy");
	struct BigGuy* _From = NULL;
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
		_From = LuaCheckClass(_State, 3, "BigGuy");
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
			_Mission->TextFormat = NULL;
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

void MissionLoadEventTrigger(lua_State* _State, struct Mission* _Mission) {
	const char* _Trigger = NULL;
	int _EventId = 0;

	if(lua_type(_State, -1) == LUA_TTABLE) {
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
			loop_end:
			lua_pop(_State, 1);
		}
	} else {
		if(LuaGetString(_State, -1, &_Trigger) == 0) {
			Log(ELOG_WARNING, "Mission's trigger table contains a non-string.");
			return;
		}
		if((_EventId = StringToEvent(_Trigger)) == -1) {
			Log(ELOG_WARNING, "Mission's trigger table contains a non-event %s.", _Trigger);
			return;
		}
	}
	_Mission->Flags = _Mission->Flags | MISSION_FEVENT;
}

void MissionLoadTriggerList(lua_State* _State, struct Mission* _Mission) {
	struct Rule* _Trigger = NULL;
	const char* _Name = NULL;

	lua_pushstring(_State, "Name");
	lua_rawget(_State, -2);
	LuaGetString(_State, -1, &_Name);
	lua_pop(_State, 1);
	lua_pushstring(_State, "Trigger");
	lua_rawget(_State, -2);
	_Trigger = LuaCheckClass(_State, -1, "Rule");
	if(_Trigger == NULL)
		return (void) luaL_error(_State, "Trigger is not a rule.");
	lua_pop(_State, 1);
}

int LuaMissionLoad(lua_State* _State) {
#define FUNC_FORMATSZ (16)

	struct Mission* _Mission = CreateMission();
	const char* _TempStr = NULL;
	int _MeanTime = 0;
	int _Id = 0;
	const char* _FormatStr = NULL;
	uint8_t _Object[FUNC_FORMATSZ];
	uint8_t _Param[FUNC_FORMATSZ];
	uint8_t _FormatSz = 0;

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
		double _Prob = 0;

		lua_pushstring(_State, "Trigger");
		lua_rawget(_State, 1);
		if(lua_type(_State, -1) != LUA_TNIL) {
			if(lua_type(_State, -1) != LUA_TFUNCTION)
				return luaL_error(_State, "Mission.Trigger is not a function.");
			_Mission->Trigger = luaL_ref(_State, LUA_REGISTRYINDEX); 
		} else {
			lua_pushstring(_State, "Event");
			lua_rawget(_State, 1);
			if(lua_type(_State, -1) != LUA_TNIL) {
				if(lua_type(_State, -1) != LUA_TNUMBER)
					return luaL_error(_State, "Mission.Event is not an integer.");
				_Mission->TriggerEvent = lua_tointeger(_State, -1);
				lua_pop(_State, 1);
			}
		}

		lua_pushstring(_State, "MeanTime");
		lua_rawget(_State, 1);
		if(lua_type(_State, -1) != LUA_TTABLE)
			return luaL_error(_State, "Mission.MeanTime must be a table.");
		lua_pushstring(_State, "Base");
		lua_rawget(_State, -2);
		if(LuaGetInteger(_State, -1, &_MeanTime) == 0) {
			return luaL_error(_State, "Mission.MeanTime must be an integer");
		}
		lua_pop(_State, 1);
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
			if(lua_type(_State, -1) != LUA_TFUNCTION)
				return luaL_error(_State, "Mission.MeanTime element #%d does not contain key 'Trigger' containing a function.");
			_Mission->MeanModTrig[i] = luaL_ref(_State, LUA_REGISTRYINDEX);
			lua_pop(_State, 1);
		}
		_Mission->MeanTime = (uint16_t) _MeanTime;
		_Prob = (((double) 1) / _MeanTime);
		_Mission->MeanPercent = pow(1 - _Prob, _MeanTime - 1) * _Prob;
		lua_pop(_State, 1);
	}
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
	MissionInsert(&g_MissionEngine, _Mission);
	Log(ELOG_DEBUG, "Loaded mission %s", _Mission->Name);
	return 0;
#undef FUNC_FORMATSZ
}

int LuaMissionSetVar(lua_State* _State) {
	return 1;
}

int LuaMissionGetVar(lua_State* _State) {
	struct MissionFrame* _Frame = LuaCheckClass(_State, 1, "MissionFrame");
	int _Idx = luaL_checkinteger(_State, 2);

	if(_Frame->StackSz - _Idx < 0 || _Idx >= MISSION_STACKSZ)
		return luaL_error(_State, "Invalid index %d used", _Idx);
	if(_Frame->Stack[_Idx].Type < PRIM_PTR) {
		PrimitiveLuaPush(_State, &_Frame->Stack[_Idx]);
		return 1;
	}
	return 1;
}

int LuaMissionCombatRound(lua_State* _State) {
	struct BigGuy* _One = LuaCheckClass(_State, 1, "BigGuy");
	struct BigGuy* _Two = LuaCheckClass(_State, 2, "BigGuy");
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

	while(_Pos++ != NULL) {
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
	}
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

	LuaRegisterObject(_State, "MissionOption", NULL, g_LuaFuncsMissionOption);
	lua_settop(_State, 0);
	lua_newtable(_State);
	lua_pushstring(_State, "Mission");
	luaL_newlib(_State, g_LuaMissionRuleFuncs);

	lua_pushstring(_State, "LessThan");
	lua_pushinteger(_State, WSOP_LESSTHAN);
	lua_rawset(_State, -3);

	lua_pushstring(_State, "GreaterThan");
	lua_pushinteger(_State, WSOP_GREATERTHAN);
	lua_rawset(_State, -3);

	lua_pushstring(_State, "Equal");
	lua_pushinteger(_State, WSOP_EQUAL);
	lua_rawset(_State, -3);
	lua_rawset(_State, -3);

	lua_pushstring(_State, "Stat");
	lua_getglobal(_State, "Stat");
	lua_rawset(_State, -3);


	lua_pushstring(_State, "Rule");
	luaL_getmetatable(_State, "Rule");
	lua_rawset(_State, 1);
	for(int i = 0; g_LuaMissionEnv[i] != NULL; ++i) {
		luaL_getmetatable(_State, g_LuaMissionEnv[i]);
		if(lua_type(_State, -1) != LUA_TTABLE) {
			Log(ELOG_WARNING, "%s is not a valid table to include the Mission env table.", g_LuaMissionEnv[i]);
			lua_pop(_State, 2);
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
			SDL_assert(lua_type(_State, -1) == LUA_TFUNCTION);
			//lua_pushcclosure(_State, LuaMissionFuncWrapper, 1);
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
	//lua_pushcclosure(_State, LuaMissionFuncWrapper, 1);
	lua_rawset(_State, -3);
	lua_pushstring(_State, "pairs");
	lua_getglobal(_State, "pairs");
	lua_rawset(_State, -3);
	lua_pushstring(_State, "print");
	lua_getglobal(_State, "print");
	lua_rawset(_State, -3);
	LuaSetEnv(_State, "Mission");
	LuaRegisterObject(_State, "MissionFrame", NULL, g_LuaFuncsMissionFrame);
}

int LuaMissionOptionGetName(lua_State* _State) {
	struct MissionOption* _Option = LuaCheckClass(_State, 1, "MissionOption");
	struct MissionFrame* _Frame = NULL;

	if(lua_gettop(_State) < 1) {
		lua_pushstring(_State, _Option->Name);
		return 1;
	}
	lua_pushstring(_State, MissionFormatText(_Option->Name, _Option->TextFormat, _Option->TextFormatSz, _Frame));
	return 1;
}

int LuaMissionOptionConditionSatisfied(lua_State* _State) {
	struct MissionOption* _Option = LuaCheckClass(_State, 1, "MissionOption");
	struct MissionFrame* _Frame = LuaCheckClass(_State, 2, "MissionFrame");

	if(_Option->Condition == 0) {
		lua_pushboolean(_State, 1);
		return 1;
	}
	CallMissionCond(_State, _Option->Condition, _Frame);
	return 1;
}
