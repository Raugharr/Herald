/*
 * File: ActionDuel.c
 * Author: David Brotz
 */

#include "ActionDuel.h"

#include <stdbool.h>

#include "../../BigGuy.h"
#include "../../Location.h"
#include "../../Family.h"
#include "../../Person.h"
#include "../../Government.h"

#include "../Agent.h"
#include "../goap.h"
#include "../GoapAction.h"

static int ActionCost(const struct Agent* _Agent) {
	return 1;
}

static int ActionFunction(struct Agent* _Agent, void* _Data) {
	struct BigGuy* _Guy = _Agent->Agent;

	BigGuySetAction(_Guy, BGACT_DUEL, _Agent->Blackboard.Target, NULL);
	return 1;
}

static int ActionUtility(const struct Agent* _Agent, int* _Min, int* _Max, struct WorldState* _State, void* _Data) {
	const struct BigGuy* _Guy = _Agent->Agent;
	struct BigGuy* _Leader =  _Agent->Blackboard.Target;
	struct BigGuyRelation* _Relation = BigGuyGetRelation(_Guy, _Leader);
	int _Utility = 0;

	*_Min = 0;
	*_Max = 255;
	if(_Guy->Stats[BGSKILL_COMBAT] < _Leader->Stats[BGSKILL_COMBAT] || (_Relation != NULL && _Relation->Relation == BGREL_LOVE))
		return 0;
	_Utility = _Utility + ((_Guy->Stats[BGSKILL_COMBAT] - _Leader->Stats[BGSKILL_COMBAT]) * 5);
	if(_Relation != NULL)
		_Utility = _Utility + ((-_Relation->Modifier) * 2);
	return (_Utility >= 255) ? (255) : (_Utility);
}

static int ActionIsComplete(const struct Agent* _Agent, void* _Data) {
	return true;
}

static int ActionPrecondition(const struct Agent* _Agent) {
	return true;
}

void ActionDuel(struct GOAPPlanner* _Planner, struct GoapAction* _Action) {
	//Both of these should use the ImproveRelations action as a precond.
//	GoapActionAddPrecond(_Action, _Planner, "SufficientEnemies", 1, WSOP_EQUAL);
	GoapActionAddPrecond(_Action, _Planner, "Influence", 1, WSOP_GREATERTHANEQUAL);
	GoapActionAddPostcond(_Action, _Planner, "IsLeader", 1, WSOP_SET);
	_Action->Name = "Duel";
	_Action->Cost = ActionCost;
	_Action->Action = ActionFunction;
	_Action->ProPrecondition = ActionPrecondition;
	_Action->IsComplete = ActionIsComplete;
	_Action->Utility = ActionUtility;
	_Action->UtilityFunction = UTILITY_LINEAR;
	_Action->Create = NULL;
	_Action->Destroy = NULL;
}
