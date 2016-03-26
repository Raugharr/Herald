/*
 * File: Goals.c
 * Author: David Brotz
 */

#include "Goals.h"

#include "GoapGoal.h"

void GoalBigGuy(struct GOAPPlanner* _Planner, struct GoapGoal* _Goal) {
	GoapGoalAddAction(_Planner, _Goal, "Improve Relations");
	GoapGoalAddAction(_Planner, _Goal, "Duel");	
}
