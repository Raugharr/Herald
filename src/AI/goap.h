/*
 * File: GOAP.h
 * Author: David Brotz
 */
#ifndef __GOAP_H
#define __GOAP_H

#include "../WorldState.h"

#include "GoapGoal.h"

#define GOAP_ATOMS (64)
#define GOAP_ACTIONS (64)
#define GOAP_ATOMOPS (8)
#define UTILITYSZ (64)
#define GOAP_GOALSZ (8)

struct GoapPathNode;
struct GOAPPlanner;
struct Agent;

/*
 * Returns a pointer to any data that should be associated with the Action.
 *
 */
typedef int (*GOAPActionCost)(const struct Agent*);
typedef int (*GOAPAction)(void*);
typedef int(*AgentActionFunc)(void*);
typedef void(*AgentActions[])(struct GOAPPlanner*);
typedef void(*AgentGoals[])(struct GOAPPlanner*, struct GoapGoal*);
typedef int(*AgentUtilityFunc)(const void*, int*, int*, struct WorldState*);

enum EUtilityFunctions {
	UTILITY_INVERSE = (1 << 0),
	UTILITY_LINEAR = (1 << 1),
	UTILITY_QUADRATIC = (1 << 2)
};

struct GOAPPlanner {
	const char* AtomNames[GOAP_ATOMS];
	int AtomActions[GOAP_ATOMS][GOAP_ATOMOPS];//Contains all atoms and the actions that modify them.
	int AtomCt;
	struct WorldState Preconditions[GOAP_ACTIONS];
	struct WorldState Postconditions[GOAP_ACTIONS];
	GOAPActionCost ActionCosts[GOAP_ACTIONS];
	const char* ActionNames[GOAP_ACTIONS];
	GOAPAction Action[GOAP_ACTIONS];
	int ActionCt;
	AgentUtilityFunc Utilities[UTILITYSZ];
	int UtilityFunction[UTILITYSZ]; //Contains a bitset of values from EUtilityFunctions.
	int UtilityCt;
	const char* UtilityNames[UTILITYSZ];
	struct GoapGoal Goals[GOAP_GOALSZ];
	int GoalCt;
};

void GoapInit();
void GoapQuit();

void GoapClear(struct GOAPPlanner* _Planner);
void GoapAddAtom(struct GOAPPlanner* _Planner, const char* _Atom);
void GoapAddPrecond(struct GOAPPlanner* _Planner, const char* _Action, const char* _Atom, int _Value, int _OpCode);
void GoapAddPostcond(struct GOAPPlanner* _Planner, const char* _Action, const char* _Atom, int _Value, int _OpCode);
void GoapSetActionCost(struct GOAPPlanner* _Planner, const char* _Action, GOAPActionCost _Cost);
void GoapSetAction(struct GOAPPlanner* _Planner, const char* _ActionName, GOAPAction _Action);
int GoapGetActionIndex(struct GOAPPlanner* _Planner, const char* _Action);


/*
 * Generates a plan based on the possible actions available in _Planner to transition from the WorldState _Start to the WorldState _End.
 * Puts the generated plan inside _Path.
 * _PathSz declares how many elements _Path can hold, when GoapPlanAction returns _PathSz will return with the number of elements inserted into _Path.
 */
void GoapPlanAction(const struct GOAPPlanner* _Planner, const void* _Data, const struct WorldState* _Start, struct WorldState* _End, int* _PathSz, struct GoapPathNode** _Path);
int GoapPathDoAction(const struct GOAPPlanner* _Planner, const struct GoapPathNode* _Node, struct WorldState* _State, void* _Data);
int GoapPathGetAction(const struct GoapPathNode* _Node);
void GoapAddUtility(struct GOAPPlanner* _Planner, const char* _Utility, AgentUtilityFunc _Callback, int _Func);
void GoapBestUtility(const struct GOAPPlanner* _Planner, const struct Agent* _Agent, struct WorldState* _BestState);
void GoapPlanUtility(const struct GOAPPlanner* _Planner, const struct Agent* _Agent, struct WorldState* _State, int* _PathSize, struct GoapPathNode** _Path);
#endif
