/*
 * File: GOAP.h
 * Author: David Brotz
 */
#ifndef __GOAP_H
#define __GOAP_H

#include "../WorldState.h"

#include "GoapGoal.h"
#include "GoapAction.h"

#define GOAP_ATOMS (32)
#define GOAP_ACTIONS (64)
#define GOAP_GOALSZ (8)

struct GoapPathNode;
struct GOAPPlanner;
struct Agent;

/*
 * Returns a pointer to any data that should be associated with the Action.
 *
 */
typedef void(*AgentActions[])(struct GOAPPlanner*, struct GoapAction*);
typedef void(*AgentGoals[])(struct GoapGoal*);

enum EUtilityFunctions {
	UTILITY_INVERSE = (1 << 0),
	UTILITY_LINEAR = (1 << 1),
	UTILITY_QUADRATIC = (1 << 2)
};

struct GOAPPlanner {
	const char* AtomNames[GOAP_ATOMS];
	int AtomCt;
	struct GoapAction Actions[GOAP_ACTIONS];
	int ActionCt;
	struct GoapGoal Goals[GOAP_GOALSZ];
	int GoalCt;
};

struct GoapPathNode {
	struct WorldState State;
	const struct GoapAction* Action;
	int ActionCt; //How many times Action is performed.
	const void* Data;
	const struct GoapPathNode* Prev;
	int h;
	int f;
	int g;
};

void GoapInit();
void GoapQuit();

void GoapClear(struct GOAPPlanner* _Planner);
void GoapAddAtom(struct GOAPPlanner* _Planner, const char* _Atom);
int GoapGetActionIndex(struct GOAPPlanner* _Planner, const char* _Action);
struct GoapAction* GoapGetAction(struct GOAPPlanner* _Planner, const char* _Action);

/*
 * Generates a plan based on the possible actions available in _Planner to transition from the WorldState _Start to the WorldState _End.
 * Puts the generated plan inside _Path.
 * _PathSz declares how many elements _Path can hold, when GoapPlanAction returns _PathSz will return with the number of elements inserted into _Path.
 */
void GoapPlanAction(const struct GOAPPlanner* _Planner, const struct GoapGoal* _Goal, const void* _Data, const struct WorldState* _Start, struct WorldState* _End, int* _PathSz, struct GoapPathNode** _Path);
int GoapPathDoAction(const struct GOAPPlanner* _Planner, const struct GoapPathNode* _Node, struct WorldState* _State, void* _Data);
const struct GoapAction* GoapPathGetAction(const struct GoapPathNode* _Node);
void GoapAddUtility(struct GOAPPlanner* _Planner, const char* _Utility, GoapActionUtility _Callback, int _Func);
const struct GoapGoal* GoapBestGoalUtility(const struct GOAPPlanner* _Planner, const struct Agent* _Agent, struct WorldState* _BestState);
void GoapPlanUtility(const struct GOAPPlanner* _Planner, const struct Agent* _Agent, struct WorldState* _State, int* _PathSize, struct GoapPathNode** _Path);
#endif
