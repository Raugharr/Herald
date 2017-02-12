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
#define MISSION_TYPE(Cat) ((Cat) + 1)
#define MISSION_TYPETOCAT(Type) ((Type) - 1)

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

void ConstructMissionEngine(struct MissionEngine* Engine);

int MissionEngineEvent(const int* One, const struct LnkLst_Node* Two);
void LoadAllMissions(lua_State* State, struct MissionEngine* Engine);
void DestroyMission(struct Mission* Mission);
void MissionCheckOption(struct lua_State* State, struct Mission* Mission, struct MissionFrame* Data, int Option);
void MissionCall(lua_State* State, const struct Mission* Mission, struct BigGuy* From, struct BigGuy* Target);
void MissionAction(const char* Name, struct BigGuy* From, struct BigGuy* Target);

void DestroyMissionEngine(struct MissionEngine* Engine);
void MissionOnEvent(struct MissionEngine* Engine, uint32_t EventType, struct BigGuy* Guy, void* Extra);
void MissionEngineThink(struct MissionEngine* Engine, lua_State* State, const struct RBTree* BigGuys);

int MissionIdInsert(const int* One, const struct Mission* Two);
int MissionIdSearch(const int* Id, const struct Mission* Mission);
int MissionHeapInsert(const struct QueuedMission* One, const struct QueuedMission* Two);
int UsedMissionHeapInsert(const struct QueuedMission* One, const struct QueuedMission* Two);
/**
 * \brief Searces a string and puts the value of any found format in FormatOut.
 * \format used is [Object.Param].
 * \note FormatOut should be a pointer to a pointer that is an array with a length of FormatOpsSz.
 */
const char* MissionFormatText(const char* restrict FormatIn, const struct MissionTextFormat* FormatOps, int FormatOpsSz,
	const struct MissionFrame* Frame);
struct Mission* MissionStrToId(const char* Str);

//FIXME: These should be seperated into a header for Lua functions.
int LuaMissionGetOwner(lua_State* State);
int LuaMissionGetFrom(lua_State* State);
int LuaMissionGetRandomPerson(lua_State* State);
int LuaMissionCallById_Aux(lua_State* State);
int LuaMissionCallById(lua_State* State);
int LuaMissionNormalize(lua_State* State);
int LuaMissionFrame(lua_State* State);
int LuaMissionAddData(lua_State* State);

int LuaMissionLoad(lua_State* State);
int LuaMissionFuncWrapper(lua_State* State);

struct GenIterator* CrisisCreateItr(void* Tree);

struct MissionFrame* MissionFrameTop();
struct BigGuy* MissionFrameOwner(struct MissionFrame* Data);
int LuaMissionSetVar(lua_State* State);
int LuaMissionGetVar(lua_State* State);
const char* MissionParseStr(const char* Str, uint8_t* ObjId, uint8_t* ParamId);
void InitMissionLua(lua_State* State);
int LuaMissionStatUtility(lua_State* State);
int LuaUtilityLinear(lua_State* State);
int LuaUtilityQuadratic(lua_State* State);
#endif
