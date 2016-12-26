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
	return _One->Agent->Object.Id - _Two->Agent->Object.Id;
}

int AgentSCallback(const struct BigGuy* _One, const struct Agent* _Two) {
	return _One->Object.Id - _Two->Agent->Object.Id;
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
	_Agent->Honor = Random(0, 0xFF);
	_Agent->Greed = Random(0, 0xFF);
	for(int i = 0; i < AGENT_PLANSZ; ++i)
		_Agent->Plan[i] = NULL;
	//_Agent->GoalSet = g_Goap.GoalSets[_Guy->Motivation];
	_Agent->GoalSet = g_Goap.GoalSets[0];
	Assert(_Guy->Motivation < g_Goap.GoalSetCt);
	InitBlackboard(&_Agent->Blackboard);
	EventHook(EVENT_NEWPLOT, AgentOnNewPlot, BigGuyHome(_Agent->Agent), _Agent, NULL);
	EventHook(EVENT_ENDPLOT, AgentOnEndPlot, BigGuyHome(_Agent->Agent), _Agent, NULL);
	AgentSetState(_Agent, AGENT_SIDLE);
	return _Agent;
}
void DestroyAgent(struct Agent* _Agent) {
	EventHookRemove(EVENT_NEWPLOT, BigGuyHome(_Agent->Agent), _Agent, NULL);
	EventHookRemove(EVENT_ENDPLOT, BigGuyHome(_Agent->Agent), _Agent, NULL);
	free(_Agent);
}

uint16_t ScorePlotLeader(const struct BigGuy* const _From, const struct BigGuy* const _Leader) {
	struct BigGuyRelation* _Rel = BigGuyGetRelation(_From, _Leader); 
	uint16_t _Score = 100;

	if(_Rel == NULL)
		return _Score;
	_Score += _Rel->Modifier;
	_Score += _Leader->Glory * 2;
	return _Score;
}

void AgentIdleNewPlot(struct Agent* _Agent, struct Plot* _Plot) {
	struct BigGuy* _Guy = _Agent->Agent;
	uint16_t _Scores[PLOT_SIDES];
	uint8_t _HighScore = PLOT_ATTACKERS;

	if(_Guy == PlotLeader(_Plot) || _Guy == PlotTarget(_Plot))
		return;
	_Scores[PLOT_ATTACKERS] = ScorePlotLeader(_Agent->Agent, PlotLeader(_Plot));
	_Scores[PLOT_DEFENDERS] = ScorePlotLeader(_Agent->Agent, PlotTarget(_Plot));
	if(_Scores[PLOT_ATTACKERS] < _Scores[PLOT_DEFENDERS])
		_HighScore = PLOT_DEFENDERS;
	if(_Scores[_HighScore] * 0.9 <= _Scores[(~_HighScore) & 1])
		return;
	PlotJoin(_Plot, _HighScore, _Guy);
	/*_LeaderRel = BigGuyGetRelation(_Guy, PlotLeader(_Plot)); 
	if(PlotTarget(_Plot) != NULL) {
		_TargetRel = BigGuyGetRelation(_Guy, PlotTarget(_Plot));
	}
	if(BigGuyRelAtLeast(_LeaderRel, BGREL_LIKE) == true && BigGuyRelAtMost(_TargetRel, BGREL_LIKE) == true) {
		PlotJoin(_Plot, PLOT_ATTACKERS, _Guy);	
	} else if(BigGuyRelAtMost(_LeaderRel, BGREL_DISLIKE) == true && BigGuyRelAtLeast(_TargetRel, BGREL_LIKE) == true) {
		PlotJoin(_Plot, PLOT_DEFENDERS, _Guy);
	}*/
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
		const struct GoapAction* _Action = _Agent->Plan[_Agent->PlanIdx]->Action;

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
	return _Agent->Plan[_Agent->PlanIdx]->Action;
}

void SensorTargetStrong(struct AgentSensor* _Sensor, struct Agent* _Agent) {
	if(_Agent->Blackboard.Target == NULL)
		return;
}

void ConstructSensorTargetStrong(struct AgentSensor* _Sensor) {
	_Sensor->Update = SensorTargetStrong;
	_Sensor->TickMax = 10;	
}
