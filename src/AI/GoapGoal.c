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
}

int GoapGoalAddAction(struct GoapGoal* _Goal, struct GOAPPlanner* _Planner, const char* _Action) {
	int _ActionIdx = GoapGetActionIndex(_Planner, _Action);

	if(_Planner == NULL || (_ActionIdx < 0 || _ActionIdx >= _Planner->ActionCt))
		return 0;
	_Goal->Actions[_Goal->ActionCt] = _ActionIdx;
	++_Goal->ActionCt;
	return 1;
}
