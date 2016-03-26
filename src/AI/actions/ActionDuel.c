/*
 * File: ActionDuel.c
 * Author: David Brotz
 */

#include "ActionDuel.h"

#include "../../BigGuy.h"
#include "../../Location.h"
#include "../../Family.h"
#include "../../Person.h"
#include "../../Government.h"

#include "../Agent.h"
#include "../goap.h"

static int ActionCost(const struct Agent* _Agent) {
	return 1;
}

static int ActionFunction(struct Agent* _Agent) {
	struct BigGuy* _Guy = _Agent->Agent;

	BigGuySetAction(_Guy, BGACT_DUEL, _Agent->Blackboard.Target, NULL);
	return 1;
}

static int ActionUtility(const struct Agent* _Agent, int* _Min, int* _Max, struct WorldState* _State) {
	const struct BigGuy* _Guy = _Agent->Agent;
	struct BigGuy* _Leader =  FamilyGetSettlement(_Guy->Person->Family)->Government->Leader;
	struct BigGuyRelation* _Relation = BigGuyGetRelation(_Guy, _Leader);
	int _Utility = 0;

	*_Min = 0;
	*_Max = 255;
	if(_Guy->Stats.Warfare < _Leader->Stats.Warfare || (_Relation != NULL && _Relation->Relation == BGREL_LOVE))
		return 0;
	_Utility = _Utility + ((_Guy->Stats.Warfare - _Leader->Stats.Warfare) * 5);
	if(_Relation != NULL)
		_Utility = _Utility + ((-_Relation->Modifier) * 2);
	return (_Utility >= 255) ? (255) : (_Utility);
}

void ActionDuel(struct GOAPPlanner* _GoPlan) {
	//Both of these should use the ImproveRelations action as a precond.
	GoapAddPrecond(_GoPlan, "Challenge Leader", "SufficientEnemies", 1, WSOP_EQUAL);
	GoapAddPrecond(_GoPlan, "Challenge Leader", "SufficientFriends", 1, WSOP_EQUAL);
	GoapAddPostcond(_GoPlan, "Challenge Leader", "IsLeader", 1, WSOP_SET);
	GoapSetActionCost(_GoPlan, "Challenge Leader", ActionCost);
	GoapSetAction(_GoPlan, "Challenge Leader", (AgentActionFunc) ActionFunction);
	GoapAddUtility(_GoPlan, "Challenge Leader", (AgentUtilityFunc)ActionUtility, UTILITY_LINEAR);
}
