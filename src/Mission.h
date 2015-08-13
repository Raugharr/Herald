/*
 * File: Mission.h
 * Author: David Brotz
 */
#ifndef __MISSION_H
#define __MISSION_H

#include "WorldState.h"

#define MISSION_MAXOPTIONS (6)

typedef struct lua_State lua_State;
struct Rule;
struct RBTree;
struct Event;

struct MissionOption {
	struct Rule* SuccessCon;
	struct Rule* SuccessRwrd;
	struct Rule* FailureCon;
	struct Rule* FailureRwrd;
};

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

/*struct MissionOption {
	char* Name;
	struct Rule* Condition;
	struct Rule* Action;
};*/

struct Mission {
	char* Name;
	char* Description;
	char* LuaTable;
	struct WorldState Trigger;
	char** OptionNames;
	struct MissionOption Options[MISSION_MAXOPTIONS];
};

void LoadAllMissions(lua_State* _State, struct RBTree* _List);
struct Mission* LoadMission(lua_State* _State, const char* _TableName);
void DestroyMission(struct Mission* _Mission);
struct MissionOption* LoadMissionOption(lua_State* _State, int _Index);
int CheckMissionOption(lua_State* _State, void* _None);
/*
 * Checks all dirty BigGuys in _BigGuys if they trigger a mission in _Missions then sets all BigGuys dirty state to false.
 */
void GenerateMissions(lua_State* _Stat, const struct RBTree* _BigGuys, const struct RBTree* _Missions);

int MissionTreeInsert(const struct Mission* _One, const struct Mission* _Two);
int MissionTreeSearch(const struct WorldState* _One, const struct Mission* _Two);

#endif
