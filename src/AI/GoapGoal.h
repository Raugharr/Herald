/*
 * File: GoapGoal.h
 * Author: David Brotz
 */

#ifndef __GOAPGOAL_H
#define __GOAPGOAL_H

#include "../WorldState.h"

#define GOAPGOAL_ACTIONS (12)
#define GOAP_ATOMOPS (8)
#define GOAPGOAL_ATOMS (32)
#define GOAPGS_GOALMAX (12)
	
struct Agent; 
struct GOAPPlanner;
struct GoapPathNode;

typedef double (*GoapGoalUtility)(const struct Agent*, int*, int*);
typedef void (*GoapGoalSetup)(struct Agent*);

struct GoapGoal {
	const char* Name;
	const struct GoapAction* Actions[GOAPGOAL_ACTIONS]; //Contains the index of the action.
	int AtomActions[GOAPGOAL_ATOMS][GOAP_ATOMOPS];//Contains all atoms and the actions that modify them.
	int ActionCt;
	struct WorldState GoalState;
	GoapGoalUtility UtilityFunc;
	GoapGoalSetup Setup;
	int Utility;
	struct GOAPPlanner* Planner;
};

struct GoapGoalSet {
	const char* Name;
	struct GoapGoal* Goals[GOAPGS_GOALMAX];
};

void InitGoapGoal(struct GoapGoal* _Goal);
int GoapGoalAddAction(struct GoapGoal* _Goal, const char* _Action);
const struct GoapAction* GoapGoalBestAction(const struct GoapGoal* _Goal, int _Atom, const struct Agent* _Agent, const struct GoapPathNode* _Node);
#endif
