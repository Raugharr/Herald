/*
 * Author: David Brotz
 * File: Goals.c
 */

#include "Goals.h"

#include "goap.h"
#include "GoapGoal.h"
#include "Agent.h"

#include "../BigGuy.h"
#include "../Person.h"
#include "../Family.h"
#include "../Government.h"
#include "../Location.h"
#include "../Plot.h"

double GoalChallangeLeaderUtility(const struct Agent* _Agent, int* _Min, int* _Max) {
	const struct BigGuy* _Owner = _Agent->Agent;
	const struct BigGuy* _Leader = FamilyGetSettlement(_Owner->Person->Family)->Government->Leader;

	if(_Owner == _Leader)
		return 0.0f;
	*_Min = _Leader->Stats[BGSKILL_WARFARE] * 0.75;
	*_Max = _Leader->Stats[BGSKILL_WARFARE] * 1.25;
	if(_Owner->Stats[BGSKILL_WARFARE] <= *_Min)
		return 0.0f;
	else if(_Owner->Stats[BGSKILL_WARFARE] >= *_Max)
		return 1.0f;
	return _Owner->Stats[BGSKILL_WARFARE];
}

void GoalChallangeLeaderSetup(struct Agent* _Agent) {
	_Agent->Blackboard.Target = FamilyGetSettlement(_Agent->Agent->Person->Family)->Government->Leader;
	RBInsert(&g_GameWorld.PlotList, CreatePlot(PLOT_OVERTHROW, _Agent->Agent, _Agent->Blackboard.Target));
}

void GoalChallangeLeader(struct GoapGoal* _Goal) {
	GoapGoalAddAction(_Goal, "Duel");
	//GoapGoalAddAction(_Goal, "Dissent");
	//GoapGoalAddAction(_Goal, "Convince");
	WorldStateAddAtom(&_Goal->GoalState, BGBYTE_ISLEADER, 1);
	_Goal->Name = "Challange Leader";
	_Goal->Utility = UTILITY_QUADRATIC;
	_Goal->UtilityFunc = GoalChallangeLeaderUtility;
	_Goal->Setup = GoalChallangeLeaderSetup;
}
