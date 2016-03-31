/*
 * File: GoapGoal.c
 * Author: David Brotz
 */

#include "GoapGoal.h"

#include "goap.h"

#include <stdlib.h>

void InitGoapGoal(struct GoapGoal* _Goal) {
	_Goal->Name = NULL;
	_Goal->ActionCt = 0;
	_Goal->UtilityFunc = NULL;
	_Goal->Setup = NULL;
}

int GoapGoalAddAction(struct GoapGoal* _Goal, struct GOAPPlanner* _Planner, const char* _Action) {
	_Goal->Actions[_Goal->ActionCt] = GoapGetAction(_Planner, _Action);
	++_Goal->ActionCt;
	return 1;
}
