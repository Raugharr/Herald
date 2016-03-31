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

typedef double (*GoapGoalUtility)(const struct Agent*, int*, int*);
typedef void (*GoapGoalSetup)(struct Agent*);

struct GoapGoal {
	const char* Name;
	struct GoapAction* Actions[GOAPGOAL_ACTIONS]; //Contains the index of the action.
	int ActionCt;
	struct WorldState GoalState;
	GoapGoalUtility UtilityFunc;
	GoapGoalSetup Setup;
	int Utility;
};

void InitGoapGoal(struct GoapGoal* _Goal);
int GoapGoalAddAction(struct GoapGoal* _Goal, struct GOAPPlanner* _Planner, const char* _Action);

#endif
