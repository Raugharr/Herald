/*
 * File: Agent.c
 * Author: David Brotz
 */

#include "Agent.h"

#include "Setup.h"
#include "goap.h"

#include "../BigGuy.h"

#include <stdlib.h>

int AgentICallback(const struct Agent* _One, const struct Agent* _Two) {
	return _One->Agent->Id - _Two->Agent->Id;
}

int AgentSCallback(const struct BigGuy* _One, const struct Agent* _Two) {
	return _Two->Agent->Id - _One->Id;
}

struct Agent* CreateAgent(struct BigGuy* _Guy) {
	struct Agent* _Agent = (struct Agent*) malloc(sizeof(struct Agent));

	_Agent->Agent = _Guy;
	_Agent->PlanIdx = AGENT_NOPLAN;
	_Agent->PlanSz = AGENT_PLANSZ;
	_Agent->Plan[0] = NULL;
	InitBlackboard(&_Agent->Blackboard);
	return _Agent;
}
void DestroyAgent(struct Agent* _Agent) {
	free(_Agent);
}

void AgentThink(struct Agent* _Agent) {
	if(AgentHasPlan(_Agent) == 0) {
		AgentPlan(&g_Goap, _Agent);
	}
	if(GoapPathDoAction(&g_Goap, _Agent->Plan[_Agent->PlanIdx], &_Agent->Agent->State, _Agent->Agent) == 1) {
		++_Agent->PlanIdx;
		if(_Agent->PlanIdx >= _Agent->PlanSz) {
			_Agent->PlanIdx = AGENT_NOPLAN;
			_Agent->PlanSz = AGENT_PLANSZ;
		}
	}
}

int AgentGetAction(const struct Agent* _Agent) {
	if(_Agent->PlanIdx == AGENT_NOPLAN)
		return AGENT_NOPLAN;
	return GoapPathGetAction(_Agent->Plan[_Agent->PlanIdx]);
}
