/*
 * File: Mission.h
 * Author: David Brotz
 */
#ifndef __MISSION_H
#define __MISSION_H

/*
 * A Mission is a type of event that displays a pop up box with a description of an event and several
 * options to choose to resolve the event. When one of the options is chosen the mission's MissionOption's
 * Action that corrisponds to the option selected will fire. Each Mission has one or more MissionOption's
 * which have a name that will be displayed for the user to see, a condition that if true will show the option,
 * and an action that will be activated if the MissionOption is selected. Missions also have a trigger that
 * is represented by a WorldState. This trigger can represent a state for one of the objects that is declared
 * in EMissionCategories. Which object in EMissionCategories a trigger represnts is defined in the TriggerType
 * variable.
 */

#include "WorldState.h"

#include "sys/BinaryHeap.h"
#include "sys/LinkedList.h"
#include "sys/RBTree.h"
#include "sys/Event.h"
#include "sys/FrameAllocator.h"

#include "BigGuy.h"

#include <SDL2/SDL.h>

#define MISSION_MAXOPTIONS (6)
#define MISSION_TYPE(_Cat) ((_Cat) + 1)
#define MISSION_TYPETOCAT(_Type) ((_Type) - 1)

typedef struct lua_State lua_State;
struct RBTree;
struct Event;
struct BinaryHeap;
struct GUIMessagePacket;
struct LinkedList;
struct BigGuy;
struct QueuedMission;
struct UsedMissionSearch;
struct MissionFrame;
struct LnkLst_Node;
struct MissionFrame;

typedef int32_t MLRef;

enum MissionEventEnum {
	MEVENT_SPRING = EVENT_SIZE,
	MEVENT_FALL,
	MEVENT_SIZE
};

enum EMissionCategories {
	MISSIONCAT_BIGGUY,
	MISSIONCAT_CRISIS,
	MISSIONCAT_SIZE
};

enum EMissionFlags {
	MISSION_FNONE = 0,
	MISSION_FONLYTRIGGER = (1 << 0),
	MISSION_FEVENT = (1 << 1),
	MISSION_FNOMENU = (1 << 2)
};

/*
 * TODO: Trigger should be replaced with a new struct that can contain multiple WorlState's. This new struct should also be able to handle the
 * relationship these different Triggers have.
 * struct MissionTrigger {
 *     struct WorldState State;
 *     struct MissionTrigger* Or; Trigger fires if this state or the Or variable is equal to the current state.
 *     struct MissionTrigger* And; Trigger fires if this state or the And variable is equal to the current state.
 * };
 * By doing this complex triggers of checking if a BigGuy's Authority is between 0 and 100 becomes possible.
 */

struct MissionOption {
	char* Name;
	struct MissionTextFormat* TextFormat;
	MLRef Condition;
	MLRef Action;
	//struct Rule* Utility;//Used by AI to determine which is the best option.
	MLRef Utility; //Pointer to array.
	uint8_t UtilitySz;
	uint8_t TextFormatSz;
};

struct MissionTextFormat {
	uint8_t Object;
	uint8_t Param;
};

struct Mission {
	uint32_t Id;
	char* Name;
	char* Description;
	struct MissionTextFormat* TextFormat;
	float* MeanMods;
	double MeanPercent;
	MLRef Trigger; //Must be true for the mission to be run. Is checked after Trigger is true. //FIXME: Rename to TrigCond. 
	MLRef OnTrigger;
	MLRef* MeanModTrig; //Array of Rule* which size is MeanModsSz.
	struct MissionOption Options[MISSION_MAXOPTIONS];
	uint32_t TriggerEvent; //List of events that will trigger this mission.
	uint16_t MeanTime; //FIXME: Only assigned to never read from.
	uint8_t TextFormatSz;
	uint8_t OptionCt;
	uint8_t TriggerType;
	uint8_t Flags;
	uint8_t MeanModsSz;
};

struct MissionEngine {
	/**
	 * FIXME: Instead of using multiple diffenent data structures for ActionMissions and MissionsTrigger,
	 * simply use a single array where the missions are then sorted if they are an action or a trigger, etc.
	 * Then store the index where these missions begin and then they can simply be iterated.
	 */
	struct RBTree MissionId; //Tree sorted by Mission id.
	//FIXME: Use array not LinkedList.
	struct LinkedList MissionsTrigger; //List of all missions that can trigger.
	struct Mission* ActionMissions[BGACT_SIZE];
	struct Array Events[MEVENT_SIZE];
	struct HashTable Namespaces;
};

void ConstructMissionEngine(struct MissionEngine* _Engine);

int MissionEngineEvent(const int* _One, const struct LnkLst_Node* _Two);
void LoadAllMissions(lua_State* _State, struct MissionEngine* _Engine);
void DestroyMission(struct Mission* _Mission);
void MissionCheckOption(struct lua_State* _State, struct Mission* _Mission, struct MissionFrame* _Data, int _Option);
void MissionCall(lua_State* _State, const struct Mission* _Mission, struct BigGuy* _From, struct BigGuy* _Target);
void MissionAction(const char* _Name, struct BigGuy* _From, struct BigGuy* _Target);

void DestroyMissionEngine(struct MissionEngine* _Engine);
void MissionOnEvent(struct MissionEngine* _Engine, uint32_t _EventType, struct BigGuy* _Guy, void* _Extra);
void MissionEngineThink(struct MissionEngine* _Engine, lua_State* _State, const struct RBTree* _BigGuys);

int MissionIdInsert(const int* _One, const struct Mission* _Two);
int MissionIdSearch(const int* _Id, const struct Mission* _Mission);
int MissionHeapInsert(const struct QueuedMission* _One, const struct QueuedMission* _Two);
int UsedMissionHeapInsert(const struct QueuedMission* _One, const struct QueuedMission* _Two);
/**
 * \brief Searces a string and puts the value of any found format in _FormatOut.
 * \format used is [Object.Param].
 * \note _FormatOut should be a pointer to a pointer that is an array with a length of _FormatOpsSz.
 */
const char* MissionFormatText(const char* restrict _FormatIn, const struct MissionTextFormat* _FormatOps, int _FormatOpsSz,
	const struct MissionFrame* _Frame);
struct Mission* MissionStrToId(const char* _Str);

//FIXME: These should be seperated into a header for Lua functions.
int LuaMissionGetOwner(lua_State* _State);
int LuaMissionGetFrom(lua_State* _State);
int LuaMissionGetRandomPerson(lua_State* _State);
int LuaMissionCallById_Aux(lua_State* _State);
int LuaMissionCallById(lua_State* _State);
int LuaMissionNormalize(lua_State* _State);
int LuaMissionFrame(lua_State* _State);
int LuaMissionAddData(lua_State* _State);

int LuaMissionLoad(lua_State* _State);
int LuaMissionFuncWrapper(lua_State* _State);

struct GenIterator* CrisisCreateItr(void* _Tree);

struct MissionFrame* MissionFrameTop();
struct BigGuy* MissionFrameOwner(struct MissionFrame* _Data);
int LuaMissionSetVar(lua_State* _State);
int LuaMissionGetVar(lua_State* _State);
const char* MissionParseStr(const char* _Str, uint8_t* _ObjId, uint8_t* _ParamId);
void InitMissionLua(lua_State* _State);
int LuaMissionStatUtility(lua_State* _State);
int LuaUtilityLinear(lua_State* _State);
int LuaUtilityQuadratic(lua_State* _State);
#endif
