/*
 * File: GoapGoal.h
 * Author: David Brotz
 */

#ifndef __GOAPGOAL_H
#define __GOAPGOAL_H

#include "../WorldState.h"

#define GOAPGOAL_ACTIONS (12)
	
struct Agent; 
typedef int (*GoapGoalUtility)(const struct Agent*);

struct GoapGoal {
	int Actions[GOAPGOAL_ACTIONS]; //Contains the index of the action.
	int ActionCt;
	struct WorldState GoalState;
	GoapGoalUtility UtilityFunc;
	struct GOAPPlanner* Planner;
};

void InitGoapGoal(struct GoapGoal* _Goal);
int GoapGoalAddAction(struct GoapGoal* _Goal, int _Index);

#endif
