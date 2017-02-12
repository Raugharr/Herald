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

struct MissionFrame* CreateMissionFrame(struct BigGuy* From, struct BigGuy* Target, const struct Mission* Mission) {
	struct MissionFrame* MissionFrame = (struct MissionFrame*) malloc(sizeof(struct MissionFrame));

	MissionFrame->From = From;
	MissionFrame->Owner = Target;
	LnkLstPushBack(&g_GameWorld.MissionFrame, MissionFrame); //FIXME: Remove g_GameWorld.FMissionFrame
	MissionFrame->StackSz = 0;
	MissionFrame->Mission = Mission;
	MissionFrame->IsOptSel = 0;
	MissionFrame->RefCt = 1;
	return MissionFrame;
}

static inline void MissionFrameIncrRef(struct MissionFrame* Frame) {
	++Frame->RefCt;
}

void DestroyMissionFrame(struct MissionFrame* MissionFrame) {
	--MissionFrame->RefCt;
	if(MissionFrame->RefCt <= 0)
		free(MissionFrame);
}

static inline void SetupMissionFrame(lua_State* State, struct MissionFrame* Frame) {
	LuaCtor(State, Frame, LOBJ_MISSIONFRAME);
	lua_pushstring(State, "From");
	(Frame->From != NULL) 
		? LuaCtor(State, Frame->From, LOBJ_BIGGUY) 
		: lua_pushnil(State);
	lua_rawset(State, -3);
	lua_pushstring(State, "Owner");
	LuaCtor(State, Frame->Owner, LOBJ_BIGGUY);
	lua_rawset(State, -3);
}

static inline bool CallMissionCond(lua_State* State, MLRef Ref, struct MissionFrame* Frame) {
	if(Ref == LUA_REFNIL)
		return true;
	lua_rawgeti(State, LUA_REGISTRYINDEX, Ref);
	SetupMissionFrame(State, Frame);
	LuaCallFunc(State, 1, 1, 0);
	if(lua_toboolean(State, -1) == 0) {
		lua_pop(State, 1);
		return false;
	}
	lua_pop(State, 1);
	return true;
}

static inline void CallMissionAction(lua_State* State, MLRef Ref, struct MissionFrame* Frame) {
	lua_rawgeti(State, LUA_REGISTRYINDEX, Ref);
	SetupMissionFrame(State, Frame);
	LuaCallFunc(State, 1, 0, 0);
}

bool MissionStrToNS(const char* Str, char** Namespace, uint8_t* Id) {
	for(int i = 0; i < MISENG_NSSTR - 1 || Str[i] != '\0'; ++i) {
		if(isalpha(Str[i]) == 0) {
			if(Str[i] == '.') {
				int TempId = 0;

				(*Namespace)[i] = '\0';
				if((TempId = atoi(&(Str[i]) + 1)) <= 0 || TempId > 0xFF) {
					return false;
				}
				*Id = TempId;
				break;
			} else {
				return false;
			}
		}
		(*Namespace)[i] = Str[i];
	}
	return true;
}

struct Mission* MissionStrToId(const char* Str) {
	char* Namespace = alloca(MISENG_NSSTR);
	uint8_t Id = 0;
	struct Array* Array = NULL;

	if(MissionStrToNS(Str, &Namespace, &Id) == false)
		return NULL;
	if((Array = HashSearch(&g_MissionEngine.Namespaces, Namespace)) == NULL)
		return NULL;
	if(Array->TblSize <= Id || Array->Table[Id] == NULL)
		return NULL;
	return Array->Table[Id];
}
	

/*	int Id = 0;
	int TempId = 0;
	const char* Namespace = strchr(Str, '.');

	if(Namespace == NULL || strchr(Namespace + 1, '.') != NULL || strlen(Namespace + 1) > 3)
		return -1;
	if(Namespace - Str > 5)
		return -1;
	for(int i = 0; i < 5; ++i) {
		if(Str[i] == '.')
			break;
		if(isalpha(Str[i]) == 0 || islower(Str[i]))
			return -1;
		Id = Id + ((Str[i] - 'A') << (5 * i));
	}
	TempId = atoi(Namespace + 1);
	if(TempId > 127)
		return -1;
	return Id + (TempId << 25);
}*/

void MissionInsert(struct MissionEngine* Engine, struct Mission* Mission, const char* Namespace, uint8_t Id) {
	if(HashSearch(&Engine->Namespaces, Namespace) == NULL) {
		//NOTE: Ensure the Arrays allocated here are freed later.
		struct Array* Array = CreateArray(MISENG_ARSZ);

		ArraySet_S(Array, Mission, Id);
		HashInsert(&Engine->Namespaces, Namespace, Array);
	}
	if(RBSearch(&Engine->MissionId, Mission->Name) != NULL) {
		Log(ELOG_WARNING, "Mission cannot be loaded id %f is already in use.", Mission->Id);
		return;
	}
	RBInsert(&Engine->MissionId, Mission);
	if((Mission->Flags & MISSION_FEVENT) == MISSION_FEVENT) {
		ArrayInsert_S(&Engine->Events[Mission->TriggerEvent], Mission);
	} else if((Mission->Flags & MISSION_FONLYTRIGGER) == 0) {
		LnkLstPushBack(&Engine->MissionsTrigger, Mission);	
	}
}

void MissionFrameClear(struct MissionFrame* Data) {
	struct LnkLst_Node* Itr = NULL;

	DestroyMissionFrame(Data);
	Itr = g_GameWorld.MissionFrame.Front;
	while(Itr != NULL) {
		if(Itr->Data == Data) {
			LnkLstRemove(&g_GameWorld.MissionFrame, Itr);
			return;
		}
		Itr = Itr->Next;
	}
	Assert(0);
}

struct Mission* CreateMission() {
	struct Mission* Mission = (struct Mission*) malloc(sizeof(struct Mission));

	Mission->TriggerType = 0;
	Mission->Name = NULL;
	Mission->Description = NULL;
	Mission->MeanPercent = 0.0;
	Mission->Trigger = LUA_REFNIL;
	Mission->OnTrigger = LUA_REFNIL;
	Mission->MeanModTrig = NULL;
	Mission->TriggerEvent = 0;
	Mission->MeanMods = 0;
	Mission->TextFormatSz = 0;
	Mission->OptionCt = 0;
	Mission->MeanModsSz = 0;
	Mission->Flags = MISSION_FNONE;
	return Mission;
}

void ConstructMissionEngine(struct MissionEngine* Engine) {
	Engine->MissionId.Table = NULL;
	Engine->MissionId.Size = 0;
	Engine->MissionId.ICallback = (RBCallback) MissionIdSearch;
	Engine->MissionId.SCallback = (RBCallback) MissionIdInsert;

	for(int i = 0; i < MEVENT_SIZE; ++i) {
		CtorArray(&Engine->Events[i], 32);//FIXME: use macro instead of literal value 32.
	}
	ConstructHashTable(&Engine->Namespaces, MISENG_NSSZ);
}

void LoadAllMissions(lua_State* State, struct MissionEngine* Engine) {
	DIR* Dir = NULL;
	struct dirent* Dirent = NULL;
	const char* Ext = NULL;

	chdir("data/missions");
	Dir = opendir("./");
	lua_settop(State, 0);
	while((Dirent = readdir(Dir)) != NULL) {
		if(((!strcmp(Dirent->d_name, ".") || !strcmp(Dirent->d_name, "..")))
			|| ((Ext = strrchr(Dirent->d_name, '.')) == NULL) || strncmp(Ext, ".lua", 4) != 0)
			continue;
		//FIXME: If one Mission cannot be loaded then all missions after it will not be loaded either.
		if(LuaLoadFile(State, Dirent->d_name, "Mission") != LUA_OK) {
		//	goto error;
		}
	}
	//error:
	chdir("../..");
}

void DestroyMission(struct Mission* Mission) {
	free(Mission->Name);
	free(Mission->Description);
	free(Mission->TextFormat);
	free(Mission);
}

void MissionCheckOption(struct lua_State* State, struct Mission* Mission, struct MissionFrame* Frame, int Option) {
	int Top = lua_gettop(State);

	if(Option < 0 || Option >= Mission->OptionCt)
		return;
	if(Frame->IsOptSel == 0) {
		Frame->IsOptSel = 1;
		CoResume(g_PlayerMisCall);
		g_PlayerMisCall = 0;
	}
	CallMissionAction(State, Mission->Options[Option].Action, Frame);
	lua_settop(State, Top);
	MissionFrameClear(Frame);
}

void MissionCall(lua_State* State, const struct Mission* Mission, struct BigGuy* Owner, struct BigGuy* From) {
	const char* NewDesc = NULL;
	struct MissionFrame* Frame = CreateMissionFrame(From, Owner, Mission);
	double Percent = 0;

	if(Mission == NULL)
		return;
	Percent = Mission->MeanPercent;
	if(CallMissionCond(g_LuaState, Mission->Trigger, Frame) == 0)
		return;
	for(uint8_t i = 0; i < Mission->MeanModsSz; ++i) {
		if(Mission->MeanModTrig[i] == 0 || CallMissionCond(g_LuaState, Mission->MeanModTrig[i], Frame) != 0)
			Percent = Percent * Mission->MeanMods[i];
	}
	if(Rand() / ((double)0xFFFFFFFFFFFFFFFF) > Percent)
		return;
	NewDesc = Mission->Description;
	if(g_GameWorld.Player == Owner) {
		if(Mission->TextFormatSz > 0) {
			//Breaks when MissionFormatText is run on a coroutine other than the main coroutine.
			//because the rule used points to the main coroutine's lua_State.
			NewDesc = MissionFormatText(Mission->Description, Mission->TextFormat, Mission->TextFormatSz, Frame); 
			if(NewDesc ==  NULL)
				return (void) luaL_error(State, "Mission (%s) cannot fire, cannot format description.", Mission->Name);

		/*	if(SizeOf == 0) {
				Log(ELOG_WARNING, "Mission %s failed: MissionFormatText failed to format text.", Mission->Name);
				return;
			}
			*/
		}
		if(Mission->OnTrigger != 0)
			CallMissionAction(State, Mission->OnTrigger, Frame);
		if((Mission->Flags & MISSION_FNOMENU) == MISSION_FNOMENU)
			return;
		lua_settop(State, 0);
		//lua_pushstring(State, "MissionMenu");
		lua_createtable(State, 0, 3);
		lua_pushstring(State, "Mission");
		LuaConstCtor(State, Mission, LOBJ_MISSION);
		lua_rawset(State, -3);

		lua_pushstring(State, "BigGuy");
		LuaCtor(State, Owner, LOBJ_BIGGUY);
		lua_rawset(State, -3);

		lua_pushstring(State, "Data");
		SetupMissionFrame(State, Frame);
		lua_rawset(State, -3);

		lua_pushstring(State, "Description");
		lua_pushstring(State, NewDesc);
		lua_rawset(State, -3);

		//LuaCreateWindow(State);
		CreateMenu("MissionMenu");
		if(CoRunning() != 0) {
			g_PlayerMisCall = CoRunning();
			CoYield();
		}
	} else {
		int BestIndex = -1;
		//float BestUtility = -1.0;
		//float Utility = 0.0;

		if(Mission->OnTrigger != 0)
			CallMissionCond(State, Mission->OnTrigger, Frame);
		if((Mission->Flags & MISSION_FNOMENU) == MISSION_FNOMENU)
			return;
		/*for(int i = 0; i < Mission->OptionCt; ++i) {
			if(RuleEval(Mission->Options[i].Condition) == 0)
				continue;
			Utility = RuleEval(Mission->Options[i].Utility);
			if(Utility > BestUtility) {
				BestIndex = i;
				BestUtility = Utility;
			}
		}*/
		BestIndex = 0;
		Frame->IsOptSel = 1;
		if(Mission->Options[BestIndex].Action != 0)
			CallMissionAction(State, Mission->Options[BestIndex].Action, Frame);
		MissionFrameClear(Frame); //FIXME: When an AI event fires an event to a player this will clear the frame before the player can select their option.
	}
}

void MissionAction(const char* Name, struct BigGuy* From, struct BigGuy* Target) {
	lua_settop(g_LuaState, 0);
	CoResume(CoSpawn(MissionCall, 4, g_LuaState, MissionStrToId(Name), From, Target));
}

void DestroyMissionEngine(struct MissionEngine* Engine) {
	for(int i = 0; i < MISSION_MAXOPTIONS; ++i)
		DtorArray(&Engine->Events[i]);
}

void MissionOnEvent(struct MissionEngine* Engine, uint32_t EventType, struct BigGuy* Guy, void* Extra) {
	Assert(EventType < EventUserOffset());
	int EventId = 0;//FIXME: Instead of checking if the EventId is valid (No events exist for the event type check it after all missions are loaded.
	struct Mission* Mission = Engine->Events[EventType].Table[EventId];
	struct MissionFrame Frame = {0};

	if(Engine->Events[EventType].Size == 0)
		return;

	EventId = Random(0, Engine->Events[EventType].Size - 1);//FIXME: Instead of checking if the EventId is valid (No events exist for the event type check it after all missions are loaded.
	Frame.Owner = Guy;
	Frame.Mission = Mission;
	switch(EventType) {
	case EVENT_JOINRETINUE:
	case EVENT_QUITRETINUE:
		Frame.From = Guy;
		Frame.Owner = ((struct Retinue*)Extra)->Leader;
		break;
	}
	MissionCall(g_LuaState, Mission, Frame.Owner, Frame.From);
}

void MissionCompare(struct RBNode* GuyNode, struct LinkedList* Missions, struct Mission* (*ActionList)[BGACT_SIZE]) {
	struct BigGuy* Guy = NULL;

	if(GuyNode == NULL)
		return;
	Guy = GuyNode->Data;
	for(struct LnkLst_Node* Itr = Missions->Front; Itr != NULL; Itr = Itr->Next) {
		struct Mission* Mission = Itr->Data;
		static int Fired = 0;

		MissionCall(g_LuaState, Mission, Guy, NULL);
		if(Fired == 0 && Mission == MissionStrToId("BANDT.1")) {
			Fired = 1;
			MissionCall(g_LuaState, Mission, g_GameWorld.Player, NULL);
		}
	}
	if(((struct BigGuy*)GuyNode->Data)->Action != BGACT_NONE)
		MissionCall(g_LuaState, (*ActionList)[Guy->Action], Guy, Guy->ActionTarget);
	MissionCompare(GuyNode->Left, Missions, ActionList);
	MissionCompare(GuyNode->Right, Missions, ActionList);
}

void RBMissionOnEvent(struct MissionEngine* Engine, uint32_t EventId, struct RBNode* GuyNode) {
	if(GuyNode == NULL)
		return;
	MissionOnEvent(Engine, EventId, GuyNode->Data, NULL);
	RBMissionOnEvent(Engine, EventId, GuyNode->Left);
	RBMissionOnEvent(Engine, EventId, GuyNode->Right);
}

void MissionEngineThink(struct MissionEngine* Engine, lua_State* State, const struct RBTree* BigGuys) {
	MissionCompare(BigGuys->Table, &Engine->MissionsTrigger, &Engine->ActionMissions);
	
	if(DAY(g_GameWorld.Date) == 0 && MONTH(g_GameWorld.Date) == MARCH)
		RBMissionOnEvent(Engine, MEVENT_SPRING, BigGuys->Table);
	else if(DAY(g_GameWorld.Date) == 0 && MONTH(g_GameWorld.Date) == SEPTEMBER)
		RBMissionOnEvent(Engine, MEVENT_SPRING, BigGuys->Table);
}

int MissionIdInsert(const int* One, const struct Mission* Two) {
	return (*One) - Two->Id;
}

int MissionIdSearch(const int* Id, const struct Mission* Mission) {
	return (*Id) - Mission->Id;
}

const char* MissionFormatText(const char* restrict FormatIn, const struct MissionTextFormat* FormatOps,
	int FormatOpsSz, const struct MissionFrame* Frame) {
	void* Obj = NULL;
	char* DestStr = NULL;
	const char* FrontStr = FormatIn;
	const char* BackStr = FrontStr;
	char** restrict Strings = alloca(sizeof(char*) * FormatOpsSz);
	size_t ExtraSz = 0;
	size_t StringSz = 0;
	size_t SizeOf = 0;
	uint8_t EscapeChar = 0;
	uint8_t OpIdx = 0;
	
	for(int i = 0; i < FormatOpsSz; ++i) {
		switch(FormatOps[i].Object) {
			case MOBJECT_TARGET:
				if(Frame->Owner == NULL) {
					Log(ELOG_WARNING, "Warning: cannot format Frame.Owner: Frame.Owner is null.");
					return NULL;
				}
				Obj = Frame->Owner;
				break;
			case MOBJECT_FROM:
				if(Frame->From == NULL) {
					Log(ELOG_WARNING, "Warning: cannot format Frame.From: Frame.From is null.");
					return NULL;
				}
				Obj = Frame->From;
				break;
		}
		switch(FormatOps[i].Param) {
			case MOPCODE_FIRSTNAME:
				StringSz = strlen(((struct BigGuy*)Obj)->Person->Name);
				Strings[i] = FrameAlloc(StringSz + 1);
				strcpy(Strings[i], ((struct BigGuy*)Obj)->Person->Name);
				break;
			case MOPCODE_LASTNAME:
				StringSz = strlen(((struct BigGuy*)Obj)->Person->Family->Name);
				Strings[i] = FrameAlloc(StringSz + 1);
				strcpy(Strings[i], ((struct BigGuy*)Obj)->Person->Family->Name);
				break;
			default:
				Assert(0);
				break;
		}
		ExtraSz += StringSz;	
	}
	SizeOf = strlen(FormatIn) + ExtraSz + 1;
	DestStr = FrameAlloc(SizeOf);
	DestStr[0] = '\0';
	while((*FrontStr) != '\0') {
		switch(*FrontStr) {
			case '\\':
				EscapeChar = 1;
				break;
			case '[':
				if(EscapeChar == 1) {
					EscapeChar = 0;
					break;
				}
				strncat(DestStr, BackStr, FrontStr - BackStr);
				strcat(DestStr, Strings[OpIdx]);
				++OpIdx;
				break;
			case ']':
				if(EscapeChar == 1) {
					EscapeChar = 0;
					break;
				}
				BackStr = FrontStr + 1;
				break;
			default:
				EscapeChar = 0;
				break;
		}
		++FrontStr;	
	}
	strcat(DestStr, BackStr);
	FrameReduce(SizeOf);
	return DestStr;
}

struct PersonSelector {
	uint16_t Count;
	int8_t Male;
	int8_t Adult;
	int8_t Caste;
	uint8_t OnlyBigGuy;
};

void LuaPersonSelector(lua_State* State, struct PersonSelector* Selector) {
	lua_pushstring(State, "Male");
	lua_rawget(State, -2);
	if(lua_type(State, -1) != LUA_TBOOLEAN) {
		return (void) luaL_error(State, "Male is not a boolean.");
	}
	Selector->Male = (lua_toboolean(State, -1) == 1) ? (MALE) : (FEMALE);
	lua_pop(State, 1);

	lua_pushstring(State, "Adult");
	lua_rawget(State, -2);
	if(lua_type(State, -1) != LUA_TBOOLEAN) {
		if(lua_type(State, -1) == LUA_TNIL) {
			Selector->Adult = 1;
		} else {
			return (void) luaL_error(State, "Adult is not a boolean.");
		}
	} else {
		Selector->Adult = lua_toboolean(State, -1);
	}
	lua_pop(State, 1);
}

static inline uint8_t LuaPersonSelect(const struct Person* Person, const struct MissionFrame* Frame, const struct PersonSelector* Selector) {
	if((Selector->Male != -1 && Gender(Person) != Selector->Male)
		|| (Selector->Adult != -1 && PersonMature(Person) != Selector->Adult)) {
			return true;
	}
	return false;
}

static inline int RandomPersonItr(lua_State* State, const struct Settlement* Settlement, 
	const struct PersonSelector* Selector, const struct MissionFrame* Frame, int Ct, int* Skipped) {
	for(struct Person* Person = Settlement->People; Person != NULL && Ct > 0; Person = Person->Next) {
		//Pick someone who didnt fire the trigger.
		if(LuaPersonSelect(Person, Frame, Selector)) {
			++(*Skipped);
			continue;
		}
		LuaCtor(State, Person, LOBJ_PERSON);
		lua_rawseti(State, -2, Ct);
		--Ct;
	}
	return Ct;
}

static inline int RandomBigGuyItr(lua_State* State, const struct Settlement* Settlement,
	const struct PersonSelector* Selector, const struct MissionFrame* Frame, int Ct, int* Skipped) {
	struct LnkLst_Node* Itr = NULL;
	struct BigGuy* Guy = NULL;
	struct Person* Person = NULL;

	assert(Settlement->BigGuys.Size != 0 && "LuaMissionGetRandomPerson: Settlement has no BigGuys.");
	Itr = Settlement->BigGuys.Front;
	Guy = (struct BigGuy*)Itr->Data;
	Person = Guy->Person;
	while(Itr != NULL && Ct > 0) {
		if(LuaPersonSelect(Person, Frame, Selector)) {
			++Skipped;
			goto loop_end;
		}
		LuaCtor(State, Guy, LOBJ_BIGGUY);
		lua_rawseti(State, -2, Ct);
		--Ct;
		loop_end:
		Itr = Itr->Next;
		Guy = Itr->Data;
		Person = Guy->Person;	
	}
	return Ct;
}

int LuaMissionGetRandomPerson(lua_State* State) {
	enum LuaArgs {
		LARG_FRAME = 1,
		LARG_SELECTORS,
		LRET_PLIST //List of people to return.
	};
	struct MissionFrame* Frame = LuaCheckClass(State, LARG_FRAME, LOBJ_MISSIONFRAME);
	struct Settlement* Settlement = NULL;
	int Ct = 0;
	int Skipped = 0;
	struct PersonSelector Selector = {0};
	
	lua_settop(State, 2);
	luaL_checktype(State, LARG_SELECTORS, LUA_TTABLE);
	Selector.Count = 1;
	Selector.OnlyBigGuy = true;
	LuaPersonSelector(State, &Selector);
	lua_newtable(State);
	Settlement = FamilyGetSettlement(Frame->Owner->Person->Family);
	Ct = Selector.Count;
	loop_start:
	if(Selector.OnlyBigGuy == true)
		Ct = RandomBigGuyItr(State, Settlement, &Selector, Frame, Ct, &Skipped);
	else
		Ct = RandomPersonItr(State, Settlement, &Selector, Frame, Ct, &Skipped);
	if(Skipped >= Settlement->NumPeople)
		goto error;
	if(Ct > 0)
		goto loop_start;
	if(Selector.Count == 1)
		lua_rawgeti(State, LRET_PLIST, 1);
	else
		lua_settop(State, LRET_PLIST);
	return 1;
	error:
	return luaL_error(State, "LuaMissionGetRandomPerson: No available person to select.");
}

int LuaMissionCallById(lua_State* State) {
	const char* Str = NULL;
	struct BigGuy* Owner = LuaCheckClass(State, 2, LOBJ_BIGGUY);
	struct BigGuy* From = NULL;
	struct Mission* Mission = NULL;

	//if(lua_type(State, 1) == LUA_TSTRING) {
	//	Str = lua_tostring(State, 1);
	Str = luaL_checkstring(State, 1);
	if((Mission = MissionStrToId(Str)) == NULL)
		return luaL_error(State, "%s is an invalid mission name.", Str);
	//	if((Id = MissionStrToId(Str)) == -1)
	//		return 0;
	//} else {
		//_Id = luaL_checkinteger(State, 1);
	//}
	/*if((Mission = RBSearch(&g_MissionEngine.MissionId, &Id)) == NULL)
		return (Str != NULL) ? 
			(luaL_error(State, "Attempted to call nil mission %s", Str)) :
			(luaL_error(State, "Attempted to call nil mission %d", Id));
			*/
	if(lua_gettop(State) >= 3) {
		From = LuaCheckClass(State, 3, LOBJ_BIGGUY);
	}
	MissionCall(State, Mission, Owner, From);
	return 0;
}

void MissionLoadOption(lua_State* State, struct Mission* Mission) {
#define FUNC_FORMATSZ (4)
	int FormatSz = 0;
	const char* Text = NULL;
	const char* FormatStr = NULL;
	MLRef Condition = 0;
	MLRef Trigger = 0;
	uint8_t Object[FUNC_FORMATSZ];
	uint8_t Param[FUNC_FORMATSZ];

	lua_pushstring(State, "Options");
	lua_rawget(State, -2);
	if(lua_type(State, -1) != LUA_TTABLE) {
		if(lua_type(State, -1) != LUA_TNIL)
			return (void) luaL_error(State, "Mission's options is not a table.");
		/* If we leave out the Options table assume we want a single
		 * option that has the text "Ok" and does nothing.
		 */
		 default_option:
		 if((Mission->Flags & MISSION_FNOMENU) == MISSION_FNOMENU) {
			Mission->OptionCt = 0;
			return;
		 }
		Mission->OptionCt = 1;
		Mission->Options[0].Name = "Ok";
		Mission->Options[0].Condition = 0;
		Mission->Options[0].Action = 0;
		Mission->Options[0].Utility = 0; 
		Mission->Options[0].UtilitySz = 0; 
		Mission->Options[0].TextFormatSz = 0; 
		return;
	} else if(lua_rawlen(State, -1) == 0) {
		goto default_option;
	} 
	if((Mission->Flags & MISSION_FNOMENU) == MISSION_FNOMENU)
		return (void) luaL_error(State, "Mission cannot have NoMenu be true and have an option table.");
	lua_pushnil(State);
	while(lua_next(State, -2) != 0) {
		if(Mission->OptionCt >= MISSION_MAXOPTIONS) {
			Log(ELOG_WARNING, "Mission %s has exceeded the maximum amount of options.", Mission->Name);
			lua_pop(State, 1);
			break;
		}
		if(lua_type(State, -1) != LUA_TTABLE)
			return (void) luaL_error(State, "Mission.Options entry is not a table.");
		Text = NULL;
		Condition = 0;
		Trigger = 0;
		lua_pushstring(State, "Text");
		lua_rawget(State, -2);
		if(LuaGetString(State, -1, &Text) == 0)
			return (void) luaL_error(State, "Mission.Options Text is not a string.");
		lua_pop(State, 1);

		FormatStr = Text;
		while((FormatStr = MissionParseStr(FormatStr, &Object[FormatSz], &Param[FormatSz])) != NULL &&
			FormatSz < FUNC_FORMATSZ) ++FormatSz;
		Mission->Options[Mission->OptionCt].TextFormatSz = FormatSz;
		if(FormatSz > 0)
			Mission->Options[Mission->OptionCt].TextFormat = calloc(sizeof(struct MissionTextFormat), FormatSz);
		else 
			Mission->Options[Mission->OptionCt].TextFormat = NULL;
		lua_pushstring(State, "Condition");
		lua_rawget(State, -2);
		if(lua_isnil(State, -1) == 0) {
			if(lua_type(State, -1) != LUA_TFUNCTION)
				return (void) luaL_error(State, "Trigger.Condition is not a function.");
			Condition = luaL_ref(State, LUA_REGISTRYINDEX);
		} else {
			Condition = 0;
			lua_pop(State, 1);
		}

		lua_pushstring(State, "Trigger");
		lua_rawget(State, -2);
		if(lua_type(State, -1) != LUA_TNIL) {
			if(lua_type(State, -1) != LUA_TFUNCTION)
				return (void) luaL_error(State, "Mission.Trigger entry is not a function.");
			Trigger = luaL_ref(State, LUA_REGISTRYINDEX);
		} else {
			Trigger = 0;
			lua_pop(State, 1);
		}

		lua_pushstring(State, "AIUtility");
		lua_rawget(State, -2);
		if(lua_type(State, -1) != LUA_TFUNCTION)
			return (void) luaL_error(State, "Mission.AIUtility is not a function.");
		Mission->Options[Mission->OptionCt].Utility = luaL_ref(State, LUA_REGISTRYINDEX);
		Mission->Options[Mission->OptionCt].Name = calloc(sizeof(char), strlen(Text) + 1);
		strcpy(Mission->Options[Mission->OptionCt].Name, Text);
		Mission->Options[Mission->OptionCt].Condition = Condition;
		Mission->Options[Mission->OptionCt].Action = Trigger;
		++Mission->OptionCt;
		lua_pop(State, 1);
	}
#undef FUNC_FORMATSZ
}

int LuaMissionLoad(lua_State* State) {
#define FUNC_FORMATSZ (16)

	struct Mission* Mission = CreateMission();
	const char* TempStr = NULL;
	int MeanTime = 0;
	uint8_t Id = 0;
	const char* FormatStr = NULL;
	const char* ErrorStr = NULL;
	//const struct Array* NSArray = NULL;
	char* Namespace = alloca(MISENG_NSSTR);
	double Prob = 0;
	uint8_t Object[FUNC_FORMATSZ];
	uint8_t Param[FUNC_FORMATSZ];
	uint8_t FormatSz = 0;
	uint8_t Action = 0;

	lua_pushstring(State, "Id");
	lua_rawget(State, 1);
	if(lua_type(State, -1) != LUA_TSTRING)
		luaL_error(State, "Mission's Id is not a string.");
	TempStr = lua_tostring(State, -1);
	if(MissionStrToId(TempStr) != NULL) {
		ErrorStr = "Mission Id %s is not a valid Id.";
		goto error;
	}
	MissionStrToNS(TempStr, &Namespace, &Id);
	/*for(int i = 0; i < MISENG_NSSTR - 1; ++i) {
		if(isalpha(TempStr[i]) == 0) {
			if(TempStr[i] == '.') {
				int TempId = 0;
				Namespace[i + 1] = '\0';
				if((TempId = atoi(&(TempStr[i]) + 1)) <= 0 || TempId > 0xFF) {
					ErrorStr = "Mission id is not valid.";
					goto error;
				}
				Id = TempId;
				break;
			} else {
				ErrorStr = "Mission Id contains a namespace but not an id.";
				goto error;
			}
		}
		Namespace[i] = TempStr[i];
	}
	if((NSArray = HashSearch(&g_MissionEngine.Namespaces, Namespace)) != NULL) {
		if(NSArray->TblSize <= Id) {
			if(NSArray->Table[Id] != NULL) {
				g_MissionEngine.ActionMissions[Action] = NULL;
				return luaL_error(State, "Cannot load mission %s.%d id is already in use.", Namespace, Id);
			}
		}
	}*/
	Mission->Id = g_MissionId++;
	lua_pop(State, 1);

	luaL_checktype(State, 1, LUA_TTABLE);
	lua_pushstring(State, "Name");
	lua_rawget(State, 1);
	if(lua_type(State, -1) != LUA_TSTRING)
		luaL_error(State, "Mission's name is not a string.");
	TempStr = lua_tostring(State, -1);
	Mission->Name = calloc(sizeof(char), strlen(TempStr) + 1);
	strcpy(Mission->Name, TempStr);
	lua_pop(State, 1);

	lua_pushstring(State, "Description");
	lua_rawget(State, 1);
	if(lua_type(State, -1) != LUA_TSTRING)
		luaL_error(State, "Mission's description is not a string.");
	TempStr = lua_tostring(State, -1);
	FormatStr = TempStr;
	while((FormatStr = MissionParseStr(FormatStr, &Object[FormatSz], &Param[FormatSz])) != NULL &&
		FormatSz < FUNC_FORMATSZ) ++FormatSz;
	Mission->Description = calloc(sizeof(char), strlen(TempStr) + 1);
	Mission->TextFormatSz = FormatSz;
	if(FormatSz > 0)
		Mission->TextFormat = calloc(sizeof(struct MissionTextFormat), FormatSz);
	else 
		Mission->TextFormat = NULL;
	for(int i = 0; i < FormatSz; ++i) {
		Mission->TextFormat[i].Object = Object[i];
		Mission->TextFormat[i].Param = Param[i];
	}
	strcpy(Mission->Description, TempStr);
	lua_pop(State, 1);

	MissionLoadOption(State, Mission);
	lua_pop(State, 1);

	lua_pushstring(State, "OnlyTriggered");
	lua_rawget(State, 1);
	if(lua_isnil(State, -1) == 0) {
		if(lua_isboolean(State, -1) != 0) {
			Mission->Flags = (Mission->Flags | MISSION_FONLYTRIGGER);
		} else {
			return luaL_error(State, "Mission.OnlyTriggered must be a boolean.");
		}
	}
	lua_pop(State, 1);

	lua_pushstring(State, "Action");
	lua_rawget(State, 1);
	if(lua_type(State, -1) != LUA_TNIL) { 
		if(lua_type(State, -1) != LUA_TNUMBER)
			return luaL_error(State, "Mission.Action is not an integer.");
		Action = lua_tointeger(State, -1);
		if(Action <= 0 || Action >= BGACT_SIZE) {
			ErrorStr = "Mission.Action is not a valid integer. (%d is invalid.)";
			goto error;
		}
		g_MissionEngine.ActionMissions[Action] = Mission;
	}
	lua_pop(State, 1);


	lua_pushstring(State, "Trigger");
	lua_rawget(State, 1);
	if(lua_type(State, -1) != LUA_TNIL) {
		if(lua_type(State, -1) != LUA_TFUNCTION)
			return luaL_error(State, "Mission.Trigger is not a function.");
		Mission->Trigger = luaL_ref(State, LUA_REGISTRYINDEX); 
	} else {
		lua_pop(State, 1);
	}
	lua_pushstring(State, "Event");
	lua_rawget(State, 1);
	if(lua_type(State, -1) != LUA_TNIL) {
		if(lua_type(State, -1) != LUA_TNUMBER)
			return luaL_error(State, "Mission.Event is not an integer.");
		Mission->TriggerEvent = lua_tointeger(State, -1);
		lua_pop(State, 1);
		Mission->Flags = Mission->Flags | MISSION_FEVENT;
		lua_pushstring(State, "EventChance");
		lua_rawget(State, 1);
		if(lua_type(State, -1) != LUA_TNUMBER) {
			Mission->MeanPercent = 1.0;
		} else {
				Mission->MeanPercent = lua_tonumber(State, -1);
			if(Mission->MeanPercent <= 0 || Mission->MeanPercent > 1.0)
				return luaL_error(State, "Mission.EventChance not greater than 0 and less than 1.(EventChance is %f).", Mission->MeanPercent);
		}
		//goto skip_meantime;
	}
	lua_pop(State, 1);

	lua_pushstring(State, "MeanTime");
	lua_rawget(State, 1);
	if(lua_type(State, -1) != LUA_TTABLE) {
		if(lua_type(State, -1) == LUA_TNIL) {
			MeanTime = 1;
		} else if(lua_type(State, -1) != LUA_TNUMBER) {
			return luaL_error(State, "Mission.MeanTime must be a table.");
		}
		LuaGetInteger(State, -1, &MeanTime);
		goto escape_meantime;
	}
	lua_pushstring(State, "Base");
	lua_rawget(State, -2);
	if(LuaGetInteger(State, -1, &MeanTime) == 0) {
		return luaL_error(State, "Mission.MeanTime must be an integer");
	}
	lua_pop(State, 1);
	escape_meantime:
	Mission->MeanModsSz = lua_rawlen(State, -1);
	Mission->MeanMods = calloc(sizeof(float), Mission->MeanModsSz);	
	Mission->MeanModTrig = calloc(sizeof(float), Mission->MeanModsSz);	
	for(uint8_t i = 0; i < Mission->MeanModsSz; ++i) {
		lua_rawgeti(State, -1, i + 1);
		if(lua_type(State, -1) != LUA_TTABLE)
			return luaL_error(State, "Mission.MeanTime elements must be a table.");
		lua_pushstring(State, "Modifier");
		lua_rawget(State, -2);
		if(lua_type(State, -1) != LUA_TNUMBER)
			return luaL_error(State, "Mission.MeanTime element #%d does not contain key 'Modifier' containing a number.");
		Mission->MeanMods[i] = lua_tonumber(State, -1);
		lua_pop(State, 1);
		lua_pushstring(State, "Trigger");
		lua_rawget(State, -2);
		if(lua_type(State, -1) != LUA_TFUNCTION) {
			ErrorStr = "Mission.MeanTime element #%d does not contain key 'Trigger' containing a function.";
			goto error;
		}
		Mission->MeanModTrig[i] = luaL_ref(State, LUA_REGISTRYINDEX);
		lua_pop(State, 1);
	}
	Mission->MeanTime = (uint16_t) MeanTime;
	Prob = (((double) 1) / MeanTime);
	Mission->MeanPercent = pow(1 - Prob, MeanTime - 1) * Prob;
	lua_pop(State, 1);
	lua_pushstring(State, "NoMenu");
	lua_rawget(State, 1);
	if(lua_type(State, -1) != LUA_TNIL) {
		if(lua_type(State, -1) == LUA_TBOOLEAN) {
			Mission->Flags = Mission->Flags | MISSION_FNOMENU;
		} else {
			return luaL_error(State, "Mission.NoMenu must be a boolean.");
		}
	}
	lua_pushstring(State, "OnTrigger");
	lua_rawget(State, 1);
	if(lua_isnil(State, -1) == 0) {
		if(lua_type(State, -1) == LUA_TFUNCTION) {
			Mission->OnTrigger = luaL_ref(State, LUA_REGISTRYINDEX);
		} else {
			return luaL_error(State, "Mission.OnTrigger must be a function.");
		}
	} else {
		Mission->OnTrigger = 0;
	}
	MissionInsert(&g_MissionEngine, Mission, Namespace, Id);
	Log(ELOG_DEBUG, "Loaded mission %s", Mission->Name);
	return 0;
	error:
	g_MissionEngine.ActionMissions[Action] = NULL;
	luaL_error(State, ErrorStr);
	return 0;
#undef FUNC_FORMATSZ
}

int LuaMissionSetVar(lua_State* State) {
	struct MissionFrame* Frame = LuaCheckClass(State, 1, LOBJ_MISSIONFRAME);
	const char* Key = luaL_checkstring(State, 2);

	for(int i = 0; i < Frame->StackSz; ++i) {
		if(strcmp(Frame->StackKey[i], Key) == 0) {
			LuaToPrimitive(State, 3, &Frame->Stack[i]);
			return 0;
		}
	}
	if(Frame->StackSz >= MISSION_STACKSZ)
		return luaL_error(State, "Cannot add Frame var: to many variables exist.");
	LuaToPrimitive(State, 3, &Frame->Stack[Frame->StackSz]);
	Frame->StackKey[Frame->StackSz] = calloc(strlen(Key) + 1, sizeof(char));
	++Frame->StackSz;
	return 0;
}

int LuaMissionStatUtility(lua_State* State) {
	int StatOne = luaL_checkinteger(State, 1);
	int StatTwo = luaL_checkinteger(State, 2);

	if(StatOne < STAT_MIN || StatOne > STAT_MAX)	
		return luaL_error(State, "Argument #1 is not a valid value for a stat. Expected [1, 100] got %d", StatOne);
	if(StatTwo < STAT_MIN || StatTwo > STAT_MAX)	
		return luaL_error(State, "Argument #2 is not a valid value for a stat. Expected [1, 100] got %d", StatTwo);
	lua_pushinteger(State, (MISSION_UTMAX / 2) + ((StatOne - StatTwo) * MISSION_UTMAX/ 100));
	return 1;
}

int LuaMissionGetVar(lua_State* State) {
	struct MissionFrame* Frame = LuaCheckClass(State, 1, LOBJ_MISSIONFRAME);
	const char* Key = luaL_checkstring(State, 2);

	for(int i = 0; i < Frame->StackSz; ++i) {
		if(strcmp(Key, Frame->StackKey[i]) == 0) {
			PrimitiveLuaPush(State, &Frame->Stack[i]);
			return 1;
		}
	}
	lua_pushnil(State);
	return 1;
}

int LuaMissionCombatRound(lua_State* State) {
	struct BigGuy* One = LuaCheckClass(State, 1, LOBJ_BIGGUY);
	struct BigGuy* Two = LuaCheckClass(State, 2, LOBJ_BIGGUY);
	int Result = 0;

	Result = BigGuyOpposedCheck(One, Two, BGSKILL_COMBAT);
	if(Result > 0) {
		Result = BigGuySkillCheck(One, BGSKILL_STRENGTH, 50 + Two->Stats[BGSKILL_TOUGHNESS]) + 1;
		goto end;
	} else if(Result < 0) {
		Result = -(BigGuySkillCheck(Two, BGSKILL_STRENGTH, 50 + One->Stats[BGSKILL_TOUGHNESS] + 1));
		goto end;
	} else {
		lua_pushinteger(State, 0);
	}
	end:
	lua_pushinteger(State, Result);
	return 1;
}

const char* MissionParseStr(const char* Str, uint8_t* ObjId, uint8_t* ParamId) {
#define PARSESTR_BUFLEN (64)

	enum MissionParseStrEnum {
		PARSESTR_NONE,
		PARSESTR_OPNBRCK,
		PARSESTR_CLSBRCK,
		PARSESTR_DOT,
		PARSESTR_BCKSLASH
	};

	const char* Pos = Str;
	uint8_t State = PARSESTR_NONE;
	uint8_t ObjectSz= 0;
	uint8_t ParamSz = 0;
	char Object[PARSESTR_BUFLEN];
	char Param[PARSESTR_BUFLEN];

	do {
		switch(State) {
			case PARSESTR_NONE:
				switch(*Pos) {
					case '[':
						State = PARSESTR_OPNBRCK;
						break;
					case ']':
					//case '.':
					//	return NULL;
					case '\\':
						State = PARSESTR_BCKSLASH;
						break;
					case '\0':
						return NULL;
					default:
						break;
				}
			break;
			case PARSESTR_OPNBRCK:	
				switch(*Pos) {
					case '[':
						return NULL;
					case ']':
						return NULL;	
					case '.':
						if(ObjectSz <= 0)
							return NULL;
						Object[ObjectSz] = '\0';
						State = PARSESTR_DOT;
						break;
					default:
						if(ObjectSz - 1 >= PARSESTR_BUFLEN)
							return NULL;
						Object[ObjectSz++] = *Pos;
						break;
				}
				break;
			case PARSESTR_DOT:
				switch(*Pos) {
					case '[':
					case '.':
						return NULL;
					case ']':
						State = PARSESTR_NONE;
						if(ParamSz <= 0)
							return NULL;
						Param[ParamSz] = '\0';
						++Pos;
						goto found_token;
					default:
						if(ParamSz - 1 >= PARSESTR_BUFLEN)
							return NULL;
						Param[ParamSz++] = *Pos;
				}
				break;
			case PARSESTR_BCKSLASH:
				State = PARSESTR_NONE;
				break;
		}
	} while(Pos++ != NULL);
	found_token:
	for(int i = 0; i < MOBJECT_SIZE; ++i) {
		if(strcmp(g_MissionObjects[i], Object) == 0) {
			*ObjId = i;
			break;
		}
	}
	for(int i = 0; i < MOPCODE_SIZE; ++i) {
		if(strcmp(g_MissionParams[i], Param) == 0) {
			*ParamId = i;
			break;
		}
	}
	return Pos;
#undef PARSESTR_BUFLEN
}

void InitMissionLua(lua_State* State) {
	const char* Temp = NULL;
	static const char* GlobalVars[] = {
		"Stat",
		"Relation",
		"ipairs",
		"pairs",
		"print",
		"Action",
		"Null",
		NULL
	};

	LuaRegisterObject(State, "MissionOption", LOBJ_MISSIONOPTION, LUA_REFNIL, g_LuaFuncsMissionOption);
	lua_settop(State, 0);
	lua_newtable(State);
	lua_pushstring(State, "Mission");
	luaL_newlib(State, g_LuaMissionRuleFuncs);
	lua_rawset(State, 1);

	for(int i = 0; GlobalVars[i] != NULL; ++i) {
		lua_pushstring(State, GlobalVars[i]);
		lua_getglobal(State, GlobalVars[i]);
		lua_rawset(State, 1);
	}

/*
	lua_pushstring(State, "Action");
	lua_getglobal(State, "BigGuy");
	lua_pushstring(State, "Action");
	lua_rawget(State, -2);
	lua_remove(State, -2);
	lua_rawset(State, 1);
*/
	lua_pushstring(State, "Rule");
	luaL_getmetatable(State, "Rule");
	lua_rawset(State, 1);
	for(int i = 0; g_LuaMissionEnv[i] != NULL; ++i) {
		luaL_getmetatable(State, g_LuaMissionEnv[i]);
		if(lua_type(State, -1) != LUA_TTABLE) {
			Log(ELOG_WARNING, "%s is not a valid table to include the Mission env table.", g_LuaMissionEnv[i]);
			lua_pop(State, 1);
			continue;
		}
		lua_pushstring(State, g_LuaMissionEnv[i]);
		lua_newtable(State);
		lua_pushnil(State);
		while(lua_next(State, -4) != 0) {
			if(lua_iscfunction(State, -1) == 0) {
				Temp = lua_tostring(State, -2);
				if(Temp[0] == '_') {
					lua_pop(State, 1);
					continue;
				}
			}
			Assert(lua_type(State, -1) == LUA_TFUNCTION);
			lua_pushvalue(State, -2);
			lua_pushvalue(State, -2);
			lua_remove(State, -3);
			lua_rawset(State, -4);
		}
		lua_rawset(State, 1);
		lua_pop(State, 1);
	}
	lua_pushstring(State, "Random");
	lua_pushcfunction(State, LuaRandom);
	lua_rawset(State, 1);

	lua_pushstring(State, "Event");
	lua_newtable(State);
	LuaAddEnum(State, -1, g_LuaMissionEventEnum);
	lua_rawset(State, 1);
	LuaSetEnv(State, "Mission");
	LuaRegisterObject(State, "MissionFrame", LOBJ_MISSIONFRAME, LUA_REFNIL, g_LuaFuncsMissionFrame);
}

int LuaMissionOptionGetName(lua_State* State) {
	struct MissionOption* Option = LuaCheckClass(State, 1, LOBJ_MISSIONOPTION);
	struct MissionFrame* Frame = LuaCheckClass(State, 2, LOBJ_MISSIONFRAME);

	if(lua_gettop(State) < 1) {
		lua_pushstring(State, Option->Name);
		return 1;
	}
	lua_pushstring(State, MissionFormatText(Option->Name, Option->TextFormat, Option->TextFormatSz, Frame));
	return 1;
}

int LuaMissionOptionConditionSatisfied(lua_State* State) {
	struct MissionOption* Option = LuaCheckClass(State, 1, LOBJ_MISSIONOPTION);
	struct MissionFrame* Frame = LuaCheckClass(State, 2, LOBJ_MISSIONFRAME);

	if(Option->Condition == 0) {
		lua_pushboolean(State, 1);
		return 1;
	}
	CallMissionCond(State, Option->Condition, Frame);
	return 1;
}

int LuaUtilityLinear(lua_State* State) {
	int Min = luaL_checkinteger(State, 1);
	int Max = luaL_checkinteger(State, 2);
	int Num = luaL_checkinteger(State, 3);

	if(Num <= Min) {
		lua_pushinteger(State, 0);
		return 1;
	}
	if(Num >= Max) {
		lua_pushinteger(State, MISSION_UTMAX);
		return 1;
	}

	lua_pushinteger(State, (Num - Min) / ((float)(Max - Min)) * MISSION_UTMAX);
	return 1;
}

int LuaUtilityQuadratic(lua_State* State) {
	int Min = luaL_checkinteger(State, 1);
	int Max = luaL_checkinteger(State, 2);
	int Num = luaL_checkinteger(State, 3);
	int Mult = 0;
	int MaxMult = 0;

	if(Num <= Min) {
		lua_pushinteger(State, 0);
		return 1;
	}
	if(Num >= Max) {
		lua_pushinteger(State, MISSION_UTMAX);
		return 1;
	}
	Mult = (Num - Min);
	Mult *= Mult;
	MaxMult = (Max - Min);
	MaxMult *= MaxMult;
	lua_pushnumber(State, Mult / ((float)MaxMult) * MISSION_UTMAX);
	return 1;
}
