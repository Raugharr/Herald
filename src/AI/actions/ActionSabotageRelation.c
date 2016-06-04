/*
 * File: ActionSabotageRelation.c
 * Author: David Brotz
 */

#include "ActionSabotageRelation.h"

static int ActionCost(const struct Agent* _Agent) {
	return 1;
}

static int ActionFunction(struct Agent* _Agent, void* _Data) {
	struct BigGuy* _Guy = _Agent->Agent;

	BigGuySetAction(_Guy, BGACT_SABREL, _Agent->Blackboard.Target, NULL);
	return 1;
}

static int ActionUtility(const struct Agent* _Agent, int* _Min, int* _Max, struct WorldState* _State, void* _Data) {
	const struct BigGuy* _Guy = _Agent->Agent;
	const struct BigGuy* _Target = _Agent->Blackboard->Target;
	struct BigGuyRelation* _Relation = BigGuyGetRelation(_Guy, _Target);

	*_Min = 0;
	*_Max = 100;
	if(_Relation->Modifier > 0)
		return 0;
	return -_Relation;
}

void ActionDuel(struct GOAPPlanner* _GoPlan) {
	//Both of these should use the ImproveRelations action as a precond.
	GoapSetActionCost(_GoPlan, "Sabotage Relation", ActionCost);
	GoapSetAction(_GoPlan, "Sabotage Relation", (AgentActionFunc) ActionFunction);
	GoapAddUtility(_GoPlan, "Sabotage Relation", (AgentUtilityFunc)ActionUtility, UTILITY_LINEAR);
	_Action->Create = NULL;
	_Action->Destroy = NULL;
}
