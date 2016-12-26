/*
 * File: Agent.h
 * Author: David Brotz
 */
#ifndef __AGENT_H
#define __AGENT_H

#include "Blackboard.h"

#include "../WorldState.h"

#define AGENT_PLANSZ (16)
#define AGENT_NOPLAN (-1)
#define AgentPlan(_Goap, _Agent) \
	_Agent->PlanSz = 0;																				\
	GoapPlanUtility(_Goap, (_Agent), &(_Agent)->State, &(_Agent)->PlanSz, (_Agent)->Plan);			\
	(_Agent)->PlanIdx = 0;																			\
	(_Agent)->Blackboard.ShouldReplan = 0
#define AgentHasPlan(_Agent) (_Agent->PlanIdx != AGENT_NOPLAN)

enum {
	BGBYTE_ISLEADER,
	BGBYTE_FEUDCOUNT,
	BGBYTE_FYRDRAISED,
	BGBYTE_TARGETSTRONG,
	BGBYTE_INFLUENCE,
	BGBYTE_SIZE
};

enum {
	AGENT_SACTION,
	AGENT_SIDLE
};

extern const char* g_BGStateStr[BGBYTE_SIZE];

struct BigGuy;
struct GoapPathNode;
struct Agent;
struct AgentSensor;

typedef void (*AgentSensorCall)(struct AgentSensor*, struct Agent*);
typedef void (*AgentStateUpdate)(struct Agent*, int, void*);

struct AgentSensor {
	AgentSensorCall Update;
	int TickTime; //How much time is left before this sensor updates.
	int TickMax; //interval of this sensor.
};

struct AgentInfo {
	struct BigGuy* Target;
	int TarInfo;
};

struct Agent {
	struct BigGuy* Agent;
	void* PlanData;
	AgentStateUpdate Update;
	const struct GoapGoalSet* GoalSet;
	const struct GoapGoal* CurrGoal;
	struct GoapPathNode* Plan[AGENT_PLANSZ];
	struct Blackboard Blackboard;
	struct AgentSensor Sensors[WorldStateBytes];
	struct WorldState State;
	int8_t PlanIdx; //The current plan we are impelenting -1 if none.
	uint8_t Greed;
	uint8_t Honor;
	uint8_t GoalState;
	uint8_t PlanSz;
};

int AgentICallback(const struct Agent* _One, const struct Agent* _Two);
int AgentSCallback(const struct BigGuy* _One, const struct Agent* _Two);
int BigGuyStateInsert(const struct Agent* _One, const struct Agent* _Two);

struct Agent* CreateAgent(struct BigGuy* _Guy);
void DestroyAgent(struct Agent* _Agent);

void AgentThink(struct Agent* _Agent);
const struct GoapAction* AgentGetAction(const struct Agent* _Agent);

#endif
