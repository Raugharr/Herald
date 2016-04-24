/*
 * Author: David Brotz
 * File: ActionMurder.c
 */

#include "ActionMurder.h"

#include "../../World.h"
#include "../../BigGuy.h"

#include "../Agent.h"
#include "../goap.h"
#include "../GoapAction.h"

static int ActionCost(const struct Agent* _Agent) {
	return 8;
}

static int ActionFunction(struct Agent* _Agent) {
	struct BigGuy* _Guy = _Agent->Agent;

	BigGuySetAction(_Guy, BGACT_MURDER, _Agent->Blackboard.Target, NULL);
	return 1;
}

static int ActionUtility(const struct Agent* _Agent, int* _Min, int* _Max, struct WorldState* _State) {
	*_Min = 0;
	*_Max = 1;
	return 0;
}

static int ActionIsComplete(const struct Agent* _Agent) {
	return 1;
}

static int ActionPrecondition(const struct Agent* _Agent) {
	struct BigGuyActionHist _Search = {_Agent->Agent, BGACT_DUEL, 0};

	return (RBSearch(&g_GameWorld.ActionHistory, &_Search) == NULL) ? (1) : (0);
}

void ActionMurder(struct GOAPPlanner* _Planner, struct GoapAction* _Action) {
	_Action->Name = "Murder";
	_Action->Cost = ActionCost;
	_Action->Action = ActionFunction;
	_Action->ProPrecondition = ActionPrecondition;
	_Action->IsComplete = ActionIsComplete;
	_Action->Utility = ActionUtility;
	_Action->UtilityFunction = UTILITY_LINEAR;
}
