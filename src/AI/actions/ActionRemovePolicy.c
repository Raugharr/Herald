/*
 * Author: David Brotz
 * File: ActionRemovePolicy.c
 */

#include "ActionRemovePolicy.h"

#include "ActionHelper.h"

#include "../goap.h"
#include "../Agent.h"

#include "../../BigGuy.h"
#include "../../Plot.h"
#include "../../Policy.h"
#include "../../Person.h"
#include "../../Government.h"
#include "../../Location.h"

enum {
	ACTIONSTATE_INIT
};

struct ActionRemovePolicy {
	int State;
};

static int ActionCost(const struct Agent* _Agent) {
	return 3;
}

static int ActionFunction(struct Agent* _Agent, void* _Data) {
	struct ActionRemovePolicy* _Action = _Data;

	/**
	 *NOTE: Create Plot to remove policy, then as long as the plot exists check to see if we can invite
	 * more people to the plot or if we are to weak to stop plotting.
	 */
	 switch(_Action->State) {
		 case ACTIONSTATE_INIT:
			 if(_Agent->Blackboard.Plot == NULL) {
				_Agent->Blackboard.Plot = CreateRemovePolicyPlot(_Agent->Blackboard.Policy, _Agent->Agent, _Agent->Blackboard.Target);
			 } else {
				PlotJoin(_Agent->Blackboard.Plot, PLOT_ATTACKERS, _Agent->Agent);
			 }
		 break;
	}
	return 1;
}

/**
 * TODO: Get the friends of _Agent and get the sum of theirs and _Agent's highest skill rating of Combat, Charisma, and Intrique,
 * then do the same for the target and get the difference. Add to the difference how unsatisfied each caste is.
 */
static int ActionUtility(const struct Agent* _Agent, int* _Min, int* _Max, struct WorldState* _State, void* _Data) {
	int _Utility = 0;
	int _Caste = PERSON_CASTE(_Agent->Agent->Person); 
	int _Preference = -PersonGetGovernment(_Agent->Agent->Person)->CastePreference[_Caste];
	struct BigGuyRelation* _Relation = _Agent->Agent->Relations;

	*_Min = 0;
	*_Max = 255;
	if(_Preference <= 0)
		return 0;
	//Check for an existing plot to change a policy, if there is join it if it is strong enough.
	if(_Agent->Blackboard.Plot != NULL) {
		_Utility = PlotPower(_Agent->Blackboard.Plot, PLOT_ATTACKERS) - PlotPower(_Agent->Blackboard.Plot, PLOT_DEFENDERS);
		goto func_end;
	}
	for(_Relation = _Agent->Agent->Relations; _Relation != NULL; _Relation = _Relation->Next) {
		if(BigGuyRelAtLeast(_Relation, BGREL_LIKE) != 0) {
			_Utility += BigGuyPlotPower(_Relation->Person);
		}
	}
	for(_Relation = _Agent->Blackboard.Target->Relations; _Relation != NULL; _Relation = _Relation->Next) {
		if(BigGuyRelAtLeast(_Relation, BGREL_LIKE) != 0) {
			_Utility -= BigGuyPlotPower(_Relation->Person);
		}
	}
	func_end:
	//Iterate through _Agent's relations adding a constant for each friend and then add this to its utility.	
	return (_Utility >= 255) ? (255) : ((_Utility <= 40) ? (0) : (_Utility));
}

static int ActionIsComplete(const struct Agent* _Agent, void* _Data) {
	return (BigGuyHasPlot(_Agent->Agent) == 0);
}

static int ActionPrecondition(const struct Agent* _Agent) {
	if(_Agent->Blackboard.Plot == NULL || _Agent->Blackboard.Policy == NULL)
		return 0;
	if(BigGuyHasPlot(_Agent->Agent) == 0)
		return 1;
	return (PlotPower(_Agent->Blackboard.Plot, PLOT_ATTACKERS) > PlotPower(_Agent->Blackboard.Plot, PLOT_DEFENDERS));
}

void ActionRemovePolicy(struct GOAPPlanner* _Planner, struct GoapAction* _Action) {
	GoapActionAddPostcond(_Action, _Planner, "IsLeader", 1, WSOP_SET);
	_Action->Name = "Remove Policy";
	_Action->Cost = ActionCost;
	_Action->Action = ActionFunction;
	_Action->ProPrecondition = ActionPrecondition;
	_Action->IsComplete = ActionIsComplete;
	_Action->Utility = ActionUtility;
	_Action->UtilityFunction = UTILITY_QUADRATIC;
	_Action->Create = NULL;
	_Action->Destroy = NULL;
}
