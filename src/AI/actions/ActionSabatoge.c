/*
 * File: ActionSabotage.c
 * Author: David Brotz
 */

#include "ActionSabotage.h"

static int ActionCost(const struct Agent* _Agent) {
	return 1;
}

static int ActionFunction(struct Agent* _Agent) {
	struct BigGuy* _Guy = _Agent->Agent;

	BigGuySetAction(_Guy, BGACT_SABREL, _Agent->Blackboard.Target, NULL);
	return 1;
}

static int ActionUtility(const struct Agent* _Agent, int* _Min, int* _Max, struct WorldState* _State) {
	*_Min = 0;
	*_Max = 1;
	return 1;
}

void ActionSabotage(struct GOAPPlanner* _GoPlan) {
	GoapSetActionCost(_GoPlan, "Sabotage Relations", ActionCost);
	GoapSetAction(_GoPlan, "Sabotage Relations", (AgentActionFunc) ActionFunction);
	GoapAddUtility(_GoPlan, "Sabotage Relations", (AgentUtilityFunc)ActionUtility, UTILITY_LINEAR);
}
