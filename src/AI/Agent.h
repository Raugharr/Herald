/*
 * File: Agent.h
 * Author: David Brotz
 */
#ifndef __AGENT_H
#define __AGENT_H

#define AGENT_PLANSZ (16)
#define AGENT_NOPLAN (-1)
#define AgentPlan(_Agent) \
	AUtilityPlan(GetBGPlanner(), (_Agent)->Agent, &(_Agent)->Agent->State, &(_Agent)->PlanSz, (_Agent)->Plan);	\
	(_Agent)->PlanIdx = 0
#define AgentHasPlan(_Agent) (_Agent->PlanIdx != AGENT_NOPLAN)

struct BigGuy;
struct GoapPathNode;

struct Agent {
	struct BigGuy* Agent;
	int PlanIdx; //The current plan we are impelenting -1 if none.
	int PlanSz;
	struct GoapPathNode* Plan[AGENT_PLANSZ];
};

int AgentICallback(const struct Agent* _One, const struct Agent* _Two);
int AgentSCallback(const struct BigGuy* _One, const struct Agent* _Two);

struct Agent* CreateAgent(struct BigGuy* _Guy);
void DestroyAgent(struct Agent* _Agent);

void AgentThink(struct Agent* _Agent);
int AgentGetAction(const struct Agent* _Agent);

#endif
