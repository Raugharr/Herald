/*
 * File: Mission.h
 * Author: David Brotz
 */
#ifndef __MISSION_H
#define __MISSION_H

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

struct Mission {
	char* Name;
	char* Description;
	char* LuaTable;
	struct Event* EventType;
	struct Rule* Trigger;
	char** OptionNames;
	struct Mission* Next;
};

void LoadAllMissions(lua_State* _State, struct RBTree* _List);
struct Mission* LoadMission(lua_State* _State, const char* _TableName);
void DestroyMission(struct Mission* _Mission);
struct MissionOption* LoadMissionOption(lua_State* _State, int _Index);
int CheckMissionOption(lua_State* _State, void* _None);
void GenerateMissions(lua_State* _Stat, const struct Event* _Evente, const struct RBTree* _BigGuys, const struct RBTree* _Missions);

int MissionTreeInsert(const struct Mission* _One, const struct Mission* _Two);
int MissionTreeSearch(const struct Event* _One, const struct Mission* _Two);

#endif
