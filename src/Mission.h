/*
 * File: Mission.h
 * Author: David Brotz
 */
#ifndef __MISSION_H
#define __MISSION_H

typedef struct lua_State lua_State;
struct Rule;

struct MissionOption {
	char* Text;
	struct Rule* SuccessCon;
	struct Rule* SuccessRwrd;
	struct Rule* FailureCon;
	struct Rule* FailureRwrd;
};

struct Mission {
	char* Name;
	char* Description;
	struct Event* EventType;
	struct MissionOption** Options;
};

struct Mission* LoadMission(lua_State* _State, int _Index);
struct MissionOption* LoadMissionOption(lua_State* _State, int _Index);

#endif
