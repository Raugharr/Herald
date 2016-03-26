/*
 * File: GoapGoal.c
 * Author: David Brotz
 */

#include "GoapGoal.h"

#include "goap.h"

#include <stdlib.h>

void InitGoapGoal(struct GoapGoal* _Goal) {
	_Goal->ActionCt = 0;
	_Goal->UtilityFunc = NULL;
	_Goal->Planner = NULL;
}

int GoapGoalAddAction(struct GoapGoal* _Goal, const char* _Action) {
	int _ActionIdx = GoapGetActionIndex(_Goal->Planner, _Action);

	if(_Goal->Planner == NULL || (_ActionIdx < 0 || _ActionIdx >= _Goal->Planner->ActionCt))
		return 0;
	_Goal->Actions[_Goal->ActionCt] = _ActionIdx;
	++_Goal->ActionCt;
	return 1;
}
