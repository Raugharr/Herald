/*
 * File: Agent.c
 * Author: David Brotz
 */

#include "Agent.h"

#include "Setup.h"
#include "goap.h"

#include "../BigGuy.h"

#include <stdlib.h>
#include <assert.h>

const char* g_BGStateStr[BGBYTE_SIZE] = {
		"IsLeader",
		"Feud",
		"FyrdRaised",
		"TargetStrong"
};

int AgentICallback(const struct Agent* _One, const struct Agent* _Two) {
	return _One->Agent->Id - _Two->Agent->Id;
}

int AgentSCallback(const struct BigGuy* _One, const struct Agent* _Two) {
	return _Two->Agent->Id - _One->Id;
}

int BigGuyStateInsert(const struct Agent* _One, const struct Agent* _Two) {
	return WorldStateCmp(&_One->State, &_Two->State);
}

struct Agent* CreateAgent(struct BigGuy* _Guy) {
	struct Agent* _Agent = (struct Agent*) malloc(sizeof(struct Agent));

	WorldStateClear(&_Agent->State);
	WorldStateCare(&_Agent->State);
	_Agent->Agent = _Guy;
	_Agent->PlanIdx = AGENT_NOPLAN;
	_Agent->PlanSz = AGENT_PLANSZ;
	_Agent->CurrGoal = NULL;
	for(int i = 0; i < AGENT_PLANSZ; ++i)
		_Agent->Plan[i] = NULL;
	_Agent->GoalSet = g_Goap.GoalSets[_Guy->Motivation];
	assert(_Guy->Motivation < g_Goap.GoalSetCt);
	InitBlackboard(&_Agent->Blackboard);
	return _Agent;
}
void DestroyAgent(struct Agent* _Agent) {
	free(_Agent);
}

void AgentThink(struct Agent* _Agent) {
	if(_Agent->Blackboard.ShouldReplan == 1) {
		AgentPlan(&g_Goap, _Agent);
	}
	if(_Agent->PlanIdx != -1 && GoapPathDoAction(&g_Goap, _Agent->Plan[_Agent->PlanIdx], &_Agent->State, _Agent) == 1) {
		const struct GoapAction* _Action = GoapPathGetAction(_Agent->Plan[_Agent->PlanIdx]);
		if(_Action != NULL && _Action->IsComplete(_Agent, _Agent->Plan[_Agent->PlanIdx]->Data) != 0) {
			++_Agent->PlanIdx;
			if(_Agent->PlanIdx >= _Agent->PlanSz) {
				_Agent->PlanIdx = AGENT_NOPLAN;
				_Agent->PlanSz = AGENT_PLANSZ;
			}
		}
	} else {
		_Agent->Blackboard.ShouldReplan = 1;
	}
}

const struct GoapAction* AgentGetAction(const struct Agent* _Agent) {
	if(_Agent->PlanIdx == AGENT_NOPLAN)
		return NULL;
	return GoapPathGetAction(_Agent->Plan[_Agent->PlanIdx]);
}

void SensorTargetStrong(struct AgentSensor* _Sensor, struct Agent* _Agent) {
	if(_Agent->Blackboard.Target == NULL)
		return;
	
}

void ConstructSensorTargetStrong(struct AgentSensor* _Sensor) {
	_Sensor->Update = SensorTargetStrong;
	_Sensor->TickMax = 10;	
}
