/*
 * File: Agent.c
 * Author: David Brotz
 */

#include "Agent.h"

#include "Setup.h"
#include "goap.h"

#include "../BigGuy.h"
#include "../Plot.h"

#include "../sys/Log.h"
#include "../sys/Event.h"

#include <stdlib.h>

const char* g_BGStateStr[BGBYTE_SIZE] = {
		"IsLeader",
		"Feud",
		"FyrdRaised",
		"TargetStrong",
		"Influence"
};

void AgentSetState(struct Agent* _Agent, int _State);

int AgentICallback(const struct Agent* _One, const struct Agent* _Two) {
	return _One->Agent->Id - _Two->Agent->Id;
}

int AgentSCallback(const struct BigGuy* _One, const struct Agent* _Two) {
	return _Two->Agent->Id - _One->Id;
}

int BigGuyStateInsert(const struct Agent* _One, const struct Agent* _Two) {
	return WorldStateCmp(&_One->State, &_Two->State);
}

void AgentOnNewPlot(const struct EventData* _Data, void* _Extra1, void* _Extra2) {
	struct Agent* _Agent = _Data->One;

	_Agent->Update(_Agent, EVENT_NEWPLOT, _Extra2);	
}

void AgentOnEndPlot(const struct EventData* _Data, void* _Extra1, void* _Extra2) {
	struct Agent* _Agent = _Data->One;

	_Agent->Update(_Agent, EVENT_ENDPLOT, _Extra1);	
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
	Assert(_Guy->Motivation < g_Goap.GoalSetCt);
	InitBlackboard(&_Agent->Blackboard);
	EventHook(EVENT_NEWPLOT, AgentOnNewPlot, BigGuyHome(_Agent->Agent), _Agent, NULL);
	EventHook(EVENT_ENDPLOT, AgentOnEndPlot, BigGuyHome(_Agent->Agent), _Agent, NULL);
	AgentSetState(_Agent, AGENT_SIDLE);
	return _Agent;
}
void DestroyAgent(struct Agent* _Agent) {
	free(_Agent);
}

void AgentIdleNewPlot(struct Agent* _Agent, struct Plot* _Plot) {
	struct BigGuy* _Guy = _Agent->Agent;
	const struct BigGuyRelation* _LeaderRel = NULL; 
	const struct BigGuyRelation* _TargetRel = NULL;

	if(_Guy == PlotLeader(_Plot) || _Guy == PlotTarget(_Plot))
		return;
	_LeaderRel = BigGuyGetRelation(_Guy, PlotLeader(_Plot)); 
	if(PlotTarget(_Plot) != NULL) {
		_TargetRel = BigGuyGetRelation(_Guy, PlotTarget(_Plot));
	}
	if(BigGuyRelAtLeast(_LeaderRel, BGREL_LIKE) != 0 && BigGuyRelAtMost(_TargetRel, BGREL_LIKE) != 0) {
		PlotJoin(_Plot, PLOT_ATTACKERS, _Guy);	
	} else if(BigGuyRelAtMost(_LeaderRel, BGREL_DISLIKE) != 0 && BigGuyRelAtLeast(_TargetRel, BGREL_LIKE) != 0) {
		PlotJoin(_Plot, PLOT_DEFENDERS, _Guy);
	}
}

void AgentIdleUpdate(struct Agent* _Agent, int _Event, void* _Data) {
	if(_Event == EVENT_NEWPLOT)
		AgentIdleNewPlot(_Agent, _Data);
}

void AgentIdleThink(struct Agent* _Agent) {

}

void AgentSetState(struct Agent* _Agent, int _State) {
	switch(_State) {
		case AGENT_SACTION:
			_Agent->GoalState = _State;
			_Agent->Update = _Agent->CurrGoal->Update;
			break;
		case AGENT_SIDLE:
			_Agent->GoalState = _State;
			_Agent->Update = AgentIdleUpdate;
			break;
	}
}

/**
 * Every day try to make a plan, if we cannot make a plan then switch our state to
 * AGENT_SIDLE otherwise make our state AGENT_SACTION.
 */
void AgentThink(struct Agent* _Agent) {
	if(_Agent->Blackboard.ShouldReplan == 1) {
		AgentPlan(&g_Goap, _Agent);
	}
	if(_Agent->PlanIdx != -1 && GoapPathDoAction(&g_Goap, _Agent->Plan[_Agent->PlanIdx], &_Agent->State, _Agent) == 1) {
		const struct GoapAction* _Action = GoapPathGetAction(_Agent->Plan[_Agent->PlanIdx]);

		AgentSetState(_Agent, AGENT_SACTION);
		if(_Action != NULL) {
			if(_Action->IsComplete(_Agent, _Agent->Plan[_Agent->PlanIdx]->Data) == 0) {
				return;
			}
			if(_Action->Destroy != NULL)
				_Action->Destroy(_Agent);
			++_Agent->PlanIdx;
			if(_Agent->PlanIdx >= _Agent->PlanSz) {
				_Agent->PlanIdx = AGENT_NOPLAN;
				_Agent->PlanSz = AGENT_PLANSZ;
			}
		}
	} else {
		AgentSetState(_Agent, AGENT_SIDLE);
		_Agent->Blackboard.ShouldReplan = 1;
		AgentIdleThink(_Agent);
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
