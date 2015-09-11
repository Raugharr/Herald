/*
 * File: Mission.h
 * Author: David Brotz
 */
#ifndef __MISSION_H
#define __MISSION_H

#include "WorldState.h"

#include "sys/BinaryHeap.h"
#include "sys/RBTree.h"

#define MISSION_MAXOPTIONS (6)

typedef struct lua_State lua_State;
struct Rule;
struct RBTree;
struct Event;
struct BinaryHeap;
struct GUIMessagePacket;
struct LinkedList;
struct QueuedMission;

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
};

struct Mission {
	char* Name;
	char* Description;
	char* LuaTable;
	struct WorldState Trigger;
	struct MissionOption Options[MISSION_MAXOPTIONS];
	int OptionCt;
	int MeanTime;
};

struct MissionEngine {
	struct BinaryHeap MissionQueue; //Queue of when missions should fire.
	struct BinaryHeap UsedMissionQueue; //Queue for when mission types go off cooldown.
	struct RBTree Missions;
	struct RBTree UsedMissionTree; //Tree for missions that have been used for lookup.
};

void InitMissions(void);
void QuitMissions(void);

int UsedMissionInsert(struct QueuedMission* _One, struct QueuedMission* _Two);
int UsedMissionSearch(int* _One, struct QueuedMission* _Two);
void LoadAllMissions(lua_State* _State, struct RBTree* _List);
void DestroyMission(struct Mission* _Mission);
int CheckMissionOption(struct GUIMessagePacket* _Packet);

void DestroyMissionEngine(struct MissionEngine* _Engine);
void MissionEngineThink(struct MissionEngine* _Engine, lua_State* _State, const struct RBTree* _BigGuys);

int MissionTreeInsert(const struct Mission* _One, const struct LinkedList* _Two);
int MissionTreeSearch(const struct WorldState* _One, const struct LinkedList* _Two);
int MissionHeapInsert(const struct QueuedMission* _One, const struct QueuedMission* _Two);
int UsedMissionHeapInsert(const struct QueuedMission* _One, const struct QueuedMission* _Two);

int LuaMissionSetName(lua_State* _State);
int LuaMissionSetDesc(lua_State* _State);
int LuaMissionAddOption(lua_State* _State);
int LuaMissionAddTrigger(lua_State* _State);
int LuaMissionGetOwner(lua_State* _State);
int LuaMissionGetRandomPerson(lua_State* _State);
int LuaMissionSetMeanTime(lua_State* _State);

void MissionDataGetVal(lua_State* _State, int _Index);

#endif
