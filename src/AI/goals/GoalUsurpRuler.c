/*
 * Author: David Brotz
 * File: GoapUsurpRuler.c
 */

#include "GoalUsurpRuler.h"

#include "../goap.h"
#include "../GoapGoal.h"
#include "../Agent.h"

#include "../../BigGuy.h"
#include "../../Person.h"
#include "../../Family.h"
#include "../../Government.h"
#include "../../Location.h"
#include "../../Plot.h"

//First search if there is a valid plot that already exists, if there isn't create your own plot.
void GoalChallangeLeaderSetPlot(struct Agent* _Agent) {
	struct Plot* _Plot = NULL;

	if(_Agent->Blackboard.Plot != NULL)
		return;
	for(const struct LnkLst_Node* _Itr = SettlementPlots(PersonHome(_Agent->Agent->Person)); _Itr != NULL; _Itr = _Itr->Next) {
		_Plot = _Itr->Data;
		
		if(PlotTarget(_Plot) != _Agent->Blackboard.Target || _Plot->Type != PLOT_CHANGEPOLICY)
			continue;
		_Agent->Blackboard.Plot = _Plot;
		return;
	}
}

int GoalChallangeLeaderUtility(const struct Agent* _Agent, int* _Min, int* _Max) {
	const struct BigGuy* _Owner = _Agent->Agent;
	const struct BigGuy* _Leader = FamilyGetSettlement(_Owner->Person->Family)->Government->Leader;
	const struct BigGuyRelation* _Relation = NULL;
	int _Cost = 0;

	*_Min = 25;
	*_Max = 100;
	if(_Owner == _Leader)
		return 0;
	_Relation = BigGuyGetRelation(_Owner, _Leader);
	//if(BigGuyRelAtMost(_Relation, BGREL_DISLIKE) == 0)
	//	return 0;
	_Cost = (-_Relation->Modifier) + (BigGuyPopularity(_Agent->Agent) - BigGuyPopularity(_Leader)) * (_Agent->Greed / (float)0x80); 
	return _Cost;
}

void GoalChallangeLeaderSetup(struct Agent* _Agent) {
	_Agent->Blackboard.Target = FamilyGetSettlement(_Agent->Agent->Person->Family)->Government->Leader;
	GoalChallangeLeaderSetPlot(_Agent);
	//RBInsert(&g_GameWorld.PlotList, CreatePlot(PLOT_OVERTHROW, NULL, _Agent->Agent, _Agent->Blackboard.Target));
}

void GoalChallangeNewEvent(struct Agent* _Agent, struct Plot* _Plot) {
	if(_Plot->Type != PLOT_CHANGEPOLICY || _Agent->Blackboard.Plot != NULL)
		return;	
	_Agent->Blackboard.Plot = _Plot;
}

void GoalChallangeLeaderUpdate(struct Agent* _Agent, int _Event, void* _Data) {
	switch(_Event) {
		case EVENT_NEWPLOT:
			GoalChallangeNewEvent(_Agent, _Data);
			break;
	}
}

void GoalChallangeLeader(struct GoapGoal* _Goal) {
	GoapGoalAddAction(_Goal, "Duel");
	GoapGoalAddAction(_Goal, "Remove Policy");
	GoapGoalAddAction(_Goal, "Grow Influence");
	WorldStateAddAtom(&_Goal->GoalState, BGBYTE_ISLEADER, 1);
	_Goal->Name = "Challange Leader";
	_Goal->Utility = UTILITY_QUADRATIC;
	_Goal->UtilityFunc = GoalChallangeLeaderUtility;
	_Goal->Setup = GoalChallangeLeaderSetup;
	_Goal->Update = GoalChallangeLeaderUpdate;
}

