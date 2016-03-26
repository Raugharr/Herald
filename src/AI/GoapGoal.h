/*
 * File: GoapGoal.h
 * Author: David Brotz
 */

#ifndef __GOAPGOAL_H
#define __GOAPGOAL_H

#include "../WorldState.h"

#define GOAPGOAL_ACTIONS (12)
	
struct Agent; 
struct GOAPPlanner;
typedef int (*GoapGoalUtility)(const struct Agent*);

struct GoapGoal {
	const char* Name;
	int Actions[GOAPGOAL_ACTIONS]; //Contains the index of the action.
	int ActionCt;
	struct WorldState GoalState;
	GoapGoalUtility UtilityFunc;
};

void InitGoapGoal(struct GoapGoal* _Goal);
int GoapGoalAddAction(struct GOAPPlanner* _Planner, struct GoapGoal* _Goal, const char* _Action);

#endif
