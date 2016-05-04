/**
 * File: StealCattle.c
 * Author: David Brotz
 */

#include "ActionStealCattle.h"

#include "../Agent.h"
#include "../goap.h"

#include "../../BigGuy.h"
#include "../../Person.h"
#include "../../Population.h"
#include "../../Family.h"
#include "../../Herald.h"

#include "../../sys/Array.h"

static int ActionCost(const struct Agent* _Agent) {
	return 4;
}

static int ActionFunction(struct Agent* _Agent) {
	struct BigGuy* _Guy = _Agent->Agent;

	BigGuySetAction(_Guy, BGACT_STEALCATTLE, _Agent->Blackboard.Target, NULL);
	return 1;
}

static int ActionUtility(const struct Agent* _Agent, int* _Min, int* _Max, struct WorldState* _State) {
	const struct BigGuy* _Guy = _Agent->Agent;
	const struct Family* _GuyFamily = _Guy->Agent->Agent->Person->Family;
	const struct Family* _TargetFamily = _Agent->Blackboard.Target->Person->Family;
	struct BigGuyRelation* _Relation = BigGuyGetRelation(_Guy, _Agent->Blackboard.Target);
	struct Population* _Cattle = HashSearch(&g_Populations, "Cow");
	int _TargetCows = CountAnimal(_Cattle, (const struct Animal**)_TargetFamily->Animals->Table, _TargetFamily->Animals->TblSize);

	if(_Relation->Relation >= BGREL_NEUTURAL)
		return 0;

	*_Min = CountAnimal(_Cattle, (const struct Animal**)_GuyFamily->Animals->Table, _GuyFamily->Animals->TblSize);
	*_Max = *_Min * 2;
	return (_TargetCows >= *_Max) ? (*_Max) : (_TargetCows);
}

static int ActionIsComplete(const struct Agent* _Agent) {
	return 1;
}

static int ActionPrecondition(const struct Agent* _Agent) {
	struct BigGuyActionHist _Search = {_Agent->Agent, BGACT_DUEL, 0};

	return (RBSearch(&g_GameWorld.ActionHistory, &_Search) == NULL) ? (1) : (0);
}

void ActionStealCattle(struct GOAPPlanner* _Planner, struct GoapAction* _Action) {
	_Action->Name = "Steal Cattle";
	_Action->Cost = ActionCost;
	_Action->Action = ActionFunction;
	_Action->ProPrecondition = ActionPrecondition;
	_Action->IsComplete = ActionIsComplete;
	_Action->Utility = ActionUtility;
	_Action->UtilityFunction = UTILITY_QUADRATIC;
}

