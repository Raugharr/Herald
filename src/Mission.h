/*
 * File: Mission.h
 * Author: David Brotz
 */
#ifndef __MISSION_H
#define __MISSION_H

typedef struct lua_State lua_State;
struct Rule;
struct LinkedList;

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
	char** OptionNames;
};

void LoadAllMissions(lua_State* _State, struct LinkedList* _List);
struct Mission* LoadMission(lua_State* _State, const char* _TableName);
void DestroyMission(struct Mission* _Mission);
struct MissionOption* LoadMissionOption(lua_State* _State, int _Index);

#endif
