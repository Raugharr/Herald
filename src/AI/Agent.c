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

void AgentSetState(struct Agent* Agent, int State);

int AgentICallback(const struct Agent* One, const struct Agent* Two) {
	return One->Agent->Object.Id - Two->Agent->Object.Id;
}

int AgentSCallback(const struct BigGuy* One, const struct Agent* Two) {
	return One->Object.Id - Two->Agent->Object.Id;
}

int BigGuyStateInsert(const struct Agent* One, const struct Agent* Two) {
	return WorldStateCmp(&One->State, &Two->State);
}

void AgentOnNewPlot(const struct EventData* Data, void* Extra1, void* Extra2) {
	struct Agent* Agent = Data->One;

	Agent->Update(Agent, EVENT_NEWPLOT, Extra2);	
}

void AgentOnEndPlot(const struct EventData* Data, void* Extra1, void* Extra2) {
	struct Agent* Agent = Data->One;

	Agent->Update(Agent, EVENT_ENDPLOT, Extra1);	
}

struct Agent* CreateAgent(struct BigGuy* Guy) {
	struct Agent* Agent = (struct Agent*) malloc(sizeof(struct Agent));

	WorldStateClear(&Agent->State);
	WorldStateCare(&Agent->State);
	Agent->Agent = Guy;
	Agent->PlanIdx = AGENT_NOPLAN;
	Agent->PlanSz = AGENT_PLANSZ;
	Agent->CurrGoal = NULL;
	Agent->Honor = Random(0, 0xFF);
	Agent->Greed = Random(0, 0xFF);
	for(int i = 0; i < AGENT_PLANSZ; ++i)
		Agent->Plan[i] = NULL;
	//_Agent->GoalSet = g_Goap.GoalSets[Guy->Motivation];
	Agent->GoalSet = g_Goap.GoalSets[0];
	InitBlackboard(&Agent->Blackboard);
	EventHook(EVENT_NEWPLOT, AgentOnNewPlot, BigGuyHome(Agent->Agent), Agent, NULL);
	EventHook(EVENT_ENDPLOT, AgentOnEndPlot, BigGuyHome(Agent->Agent), Agent, NULL);
	AgentSetState(Agent, AGENT_SIDLE);
	return Agent;
}
void DestroyAgent(struct Agent* Agent) {
	EventHookRemove(EVENT_NEWPLOT, BigGuyHome(Agent->Agent), Agent, NULL);
	EventHookRemove(EVENT_ENDPLOT, BigGuyHome(Agent->Agent), Agent, NULL);
	free(Agent);
}

uint16_t ScorePlotLeader(const struct BigGuy* const From, const struct BigGuy* const Leader) {
	const struct Relation* Rel = GetRelation(From->Relations, Leader); 
	uint16_t Score = 100;

	if(Rel == NULL)
		return Score;
	Score += Rel->Modifier;
	Score += Leader->Glory * 2;
	return Score;
}

void AgentIdleNewPlot(struct Agent* Agent, struct Plot* Plot) {
	struct BigGuy* Guy = Agent->Agent;
	uint16_t Scores[PLOT_SIDES];
	uint8_t HighScore = PLOT_ATTACKERS;

	if(Plot == NULL) return;
	if(Guy == PlotLeader(Plot) || Guy == PlotTarget(Plot))
		return;
	Scores[PLOT_ATTACKERS] = ScorePlotLeader(Agent->Agent, PlotLeader(Plot));
	Scores[PLOT_DEFENDERS] = ScorePlotLeader(Agent->Agent, PlotTarget(Plot));
	if(Scores[PLOT_ATTACKERS] < Scores[PLOT_DEFENDERS])
		HighScore = PLOT_DEFENDERS;
	if(Scores[HighScore] * 0.9 <= Scores[(~HighScore) & 1])
		return;
	PlotJoin(Plot, HighScore, Guy);
	/*_LeaderRel = BigGuyGetRelation(Guy, PlotLeader(Plot)); 
	if(PlotTarget(Plot) != NULL) {
		TargetRel = BigGuyGetRelation(Guy, PlotTarget(Plot));
	}
	if(BigGuyRelAtLeast(LeaderRel, BGREL_LIKE) == true && BigGuyRelAtMost(TargetRel, BGREL_LIKE) == true) {
		PlotJoin(Plot, PLOT_ATTACKERS, Guy);	
	} else if(BigGuyRelAtMost(LeaderRel, BGREL_DISLIKE) == true && BigGuyRelAtLeast(TargetRel, BGREL_LIKE) == true) {
		PlotJoin(Plot, PLOT_DEFENDERS, Guy);
	}*/
}

void AgentIdleUpdate(struct Agent* Agent, int Event, void* Data) {
	if(Event == EVENT_NEWPLOT)
		AgentIdleNewPlot(Agent, Data);
}

void AgentIdleThink(struct Agent* Agent) {

}

void AgentSetState(struct Agent* Agent, int State) {
	switch(State) {
		case AGENT_SACTION:
			Agent->GoalState = State;
			Agent->Update = Agent->CurrGoal->Update;
			break;
		case AGENT_SIDLE:
			Agent->GoalState = State;
			Agent->Update = AgentIdleUpdate;
			break;
	}
}

/**
 * Every day try to make a plan, if we cannot make a plan then switch our state to
 * AGENT_SIDLE otherwise make our state AGENT_SACTION.
 */
void AgentThink(struct Agent* Agent) {
/*	if(Agent->Blackboard.ShouldReplan == 1) {
		AgentPlan(&g_Goap, Agent);
	}
	if(Agent->PlanIdx != -1 && GoapPathDoAction(&g_Goap, Agent->Plan[Agent->PlanIdx], &Agent->State, Agent) == 1) {
		const struct GoapAction* Action = Agent->Plan[Agent->PlanIdx]->Action;

		AgentSetState(Agent, AGENT_SACTION);
		if(Action != NULL) {
			if(Action->IsComplete(Agent, Agent->Plan[Agent->PlanIdx]->Data) == 0) {
				return;
			}
			if(Action->Destroy != NULL)
				Action->Destroy(Agent);
			++Agent->PlanIdx;
			if(Agent->PlanIdx >= Agent->PlanSz) {
				Agent->PlanIdx = AGENT_NOPLAN;
				Agent->PlanSz = AGENT_PLANSZ;
			}
		}
	} else {
		AgentSetState(Agent, AGENT_SIDLE);
		Agent->Blackboard.ShouldReplan = 1;
		AgentIdleThink(Agent);
	}*/
}

const struct GoapAction* AgentGetAction(const struct Agent* Agent) {
	if(Agent->PlanIdx == AGENT_NOPLAN)
		return NULL;
	return Agent->Plan[Agent->PlanIdx]->Action;
}

void SensorTargetStrong(struct AgentSensor* Sensor, struct Agent* Agent) {
	if(Agent->Blackboard.Target == NULL)
		return;
}

void ConstructSensorTargetStrong(struct AgentSensor* Sensor) {
	Sensor->Update = SensorTargetStrong;
	Sensor->TickMax = 10;	
}
