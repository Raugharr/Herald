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

int GoapGoalAddAction(struct GoapGoal* _Goal, int _Index) {
	if(_Goal->Planner == NULL || (_Index < 0 || _Index >= _Goal->Planner->ActionCt))
		return 0;
	_Goal->Actions[_Goal->ActionCt] = _Index;
	++_Goal->ActionCt;
	return 1;
}
