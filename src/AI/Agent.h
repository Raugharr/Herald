/*
 * File: Agent.h
 * Author: David Brotz
 */
#ifndef __AGENT_H
#define __AGENT_H

#define AGENT_PLANSZ (16)
#define AGENT_NOPLAN (-1)
#define AgentPlan(_Goap, _Agent) \
	GoapPlanUtility(_Goap, (_Agent), &(_Agent)->Agent->State, &(_Agent)->PlanSz, (_Agent)->Plan);	\
	(_Agent)->PlanIdx = 0;																			\
	(_Agent)->Blackboard.ShouldReplan = 0
#define AgentHasPlan(_Agent) (_Agent->PlanIdx != AGENT_NOPLAN)

#include "Blackboard.h"

struct BigGuy;
struct GoapPathNode;
struct Agent;
struct AgentSensor;

typedef void (*AgentSensorCall)(struct AgentSensor*, struct Agent*);

struct AgentSensor {
	AgentSensorCall Update;
	int TickTime; //How much time is left before this sensor updates.
	int TickMax; //interval of this sensor.
};

struct Agent {
	struct BigGuy* Agent;
	int PlanIdx; //The current plan we are impelenting -1 if none.
	int PlanSz;
	struct GoapPathNode* Plan[AGENT_PLANSZ];
	struct Blackboard Blackboard;
	struct AgentSensor Sensors[16];
};

int AgentICallback(const struct Agent* _One, const struct Agent* _Two);
int AgentSCallback(const struct BigGuy* _One, const struct Agent* _Two);

struct Agent* CreateAgent(struct BigGuy* _Guy);
void DestroyAgent(struct Agent* _Agent);

void AgentThink(struct Agent* _Agent);
const struct GoapAction* AgentGetAction(const struct Agent* _Agent);

#endif
