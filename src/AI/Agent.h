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
#define AgentPlan(_Goap, Agent) \
	Agent->PlanSz = 0;																				\
	GoapPlanUtility(_Goap, (_Agent), &(_Agent)->State, &(_Agent)->PlanSz, (_Agent)->Plan);			\
	(_Agent)->PlanIdx = 0;																			\
	(_Agent)->Blackboard.ShouldReplan = 0
#define AgentHasPlan(_Agent) (_Agent->PlanIdx != AGENT_NOPLAN)
#define AGENT_MOTSZ (4)

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

//NOTE: Motivations should be used for AI only and should be moved to the AI file.
struct MotivationRevenge {
	uint8_t Type;
	struct Person* Target;
};

union Motivation {
	uint8_t Type;
	struct MotivationRevenge Revenge;
};

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
	union Motivation* Motivations[AGENT_MOTSZ];
	struct Blackboard Blackboard;
	struct AgentSensor Sensors[WorldStateBytes];
	struct WorldState State;
	int8_t PlanIdx; //The current plan we are impelenting -1 if none.
	uint8_t Greed;
	uint8_t Honor;
	uint8_t GoalState;
	uint8_t PlanSz;
};

int AgentICallback(const struct Agent* One, const struct Agent* Two);
int AgentSCallback(const struct BigGuy* One, const struct Agent* Two);
int BigGuyStateInsert(const struct Agent* One, const struct Agent* Two);

struct Agent* CreateAgent(struct BigGuy* Guy);
void DestroyAgent(struct Agent* Agent);

void AgentThink(struct Agent* Agent);
const struct GoapAction* AgentGetAction(const struct Agent* Agent);

#endif
