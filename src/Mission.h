/*
 * File: Mission.h
 * Author: David Brotz
 */
#ifndef __MISSION_H
#define __MISSION_H

#include "WorldState.h"

#include "sys/BinaryHeap.h"
#include "sys/LinkedList.h"
#include "sys/RBTree.h"

#define MISSION_MAXOPTIONS (6)
#define MISSION_BGTYPE (0)
#define MISSION_TYPE(_Cat) ((_Cat) + 1)
#define MISSION_TYPETOCAT(_Type) ((_Type) - 1)

enum {
	//MISSIONCAT_SETTLEMENT,
	MISSIONCAT_CRISIS,
	MISSIONCAT_SIZE
};

enum {
	MISSION_FNONE = 0,
	MISSION_FONLYTRIGGER = (1 << 0)
};

typedef struct lua_State lua_State;
struct Rule;
struct RBTree;
struct Event;
struct BinaryHeap;
struct GUIMessagePacket;
struct LinkedList;
struct BigGuy;
struct QueuedMission;
struct UsedMissionSearch;
struct MissionData;

/*
 * Each mission has a variable named Trigger that represents the state the world must be in for the Mission to be fired.
 * In order to achieve this in an efficient manner every BigGuy has a dirty variable where only BigGuys that are considered
 * dirty will have their state compared to a mission.
 */

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
	struct Rule* Condition;
	struct Rule* Action;
	struct Rule* Utility;//Used by AI to determine which is the best option.
};

struct Mission {
	int Id;
	int Type;
	int Flags;
	char* Name;
	char* Description;
	struct Rule* PostTrigger; //Must be true for the mission to be run. Is checked after Trigger is true.
	struct WorldState Trigger;
	struct MissionOption Options[MISSION_MAXOPTIONS];
	struct Rule* OnTrigger;
	int OptionCt;
	struct Rule* MeanTime;
};

struct MissionCat {
	struct GenIterator* (*CreateItr)(void*);
	void (*DestroyItr)(struct GenIterator*);
	struct WorldState* (*GetState)(void*);
	int* (*GetTriggerMask)(void*);
	struct BigGuy* (*GetOwner)(void*);
	int (*ListIsEmpty)(void*);
	void* List;
	const char** StateStr;
	int StateSz;
	const char* Name;
	struct LinkedList MissionList[WORLDSTATE_ATOMSZ];
};

struct MissionEngine {
	struct BinaryHeap MissionQueue; //Queue of when missions should fire.
	struct BinaryHeap UsedMissionQueue; //Queue for when mission types go off cooldown.
	struct RBTree Missions; //Tree of linked lists comprised of missions that have the same trigger.
	struct RBTree UsedMissionTree; //Tree for missions that have been used for lookup.
	struct RBTree MissionId; //Tree sorted by Mission id.
	struct MissionCat Categories[MISSIONCAT_SIZE];
	struct LinkedList MissionList[WORLDSTATE_ATOMSZ];
	//struct LinkedList GeneralList;
};

void InitMissions(void);
void QuitMissions(void);

int UsedMissionInsert(const struct QueuedMission* _One, const struct QueuedMission* _Two);
int UsedMissionSearch(const struct UsedMissionSearch* _One, const struct QueuedMission* _Two);
void LoadAllMissions(lua_State* _State, struct MissionEngine* _Engine);
void DestroyMission(struct Mission* _Mission);
//int CheckMissionOption(struct GUIMessagePacket* _Packet);
void MissionCheckOption(struct lua_State* _State, struct Mission* _Mission, struct MissionData* _Data, int _Option);
void MissionCall(lua_State* _State, struct Mission* _Mission, struct BigGuy* _Guy, struct BigGuy* _Target);

void DestroyMissionEngine(struct MissionEngine* _Engine);
void MissionEngineThink(struct MissionEngine* _Engine, lua_State* _State, const struct RBTree* _BigGuys);

int MissionIdInsert(const int* _One, const struct Mission* _Two);
int MissionIdSearch(const int* _Id, const struct Mission* _Mission);
int MissionTreeInsert(const struct Mission* _One, const struct LinkedList* _Two);
int MissionTreeSearch(const struct WorldState* _One, const struct LinkedList* _Two);
int MissionHeapInsert(const struct QueuedMission* _One, const struct QueuedMission* _Two);
int UsedMissionHeapInsert(const struct QueuedMission* _One, const struct QueuedMission* _Two);

//FIXME: These should be seperated into a header for Lua functions.
int LuaMissionSetName(lua_State* _State);
int LuaMissionSetDesc(lua_State* _State);
int LuaMissionAddOption(lua_State* _State);
int LuaMissionAddTrigger(lua_State* _State);
int LuaMissionGetOwner(lua_State* _State);
int LuaMissionGetTarget(lua_State* _State);
int LuaMissionGetRandomPerson(lua_State* _State);
int LuaMissionCallById(lua_State* _State);
int LuaMissionNormalize(lua_State* _State);
int LuaMissionData(lua_State* _State);
int LuaMissionAddData(lua_State* _State);

int LuaMissionLoad(lua_State* _State);
int LuaMissionFuncWrapper(lua_State* _State);

struct GenIterator* CrisisCreateItr(void* _Tree);

#endif
