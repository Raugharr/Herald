/*
 * File: GOAP.h
 * Author: David Brotz
 */
#ifndef __GOAP_H
#define __GOAP_H

#include "../WorldState.h"

#define GOAP_ATOMS (64)
#define GOAP_ACTIONS (64)
#define GOAP_ATOMOPS (8)
#define UTILITYSZ (64)

struct LifoAllocator;
struct GoapPathNode;

/*
 * Returns a pointer to any data that should be associated with the Action.
 *
 */
typedef const void*(*GOAPActionCost)(struct LifoAllocator*, const void*, int*);
typedef int (*GOAPAction)(void*);
typedef int (*UtilityCallback)(const void*, int*, int*, struct WorldState*);

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
	int (*ActionCosts[GOAP_ACTIONS])(const void*, const void*);
	const char* ActionNames[GOAP_ACTIONS];
	GOAPAction Action[GOAP_ACTIONS];
	int ActionCt;
	UtilityCallback Utilities[UTILITYSZ];
	int UtilityFunction[UTILITYSZ]; //Contains a bitset of values from EUtilityFunctions.
	int UtilityCt;
	const char* UtilityNames[UTILITYSZ];
};

void GoapInit();
void GoapQuit();

void GoapClear(struct GOAPPlanner* _Planner);
void GoapAddAtom(struct GOAPPlanner* _Planner, const char* _Atom);
void GoapAddPrecond(struct GOAPPlanner* _Planner, const char* _Action, const char* _Atom, int _Value, int _OpCode);
void GoapAddPostcond(struct GOAPPlanner* _Planner, const char* _Action, const char* _Atom, int _Value, int _OpCode);
void GoapSetActionCost(struct GOAPPlanner* _Planner, const char* _Action, int (*_Cost)(const void*, const void*));
void GoapSetAction(struct GOAPPlanner* _Planner, const char* _ActionName, GOAPAction _Action);

/*
 * Generates a plan based on the possible actions available in _Planner to transition from the WorldState _Start to the WorldState _End.
 * Puts the generated plan inside _Path.
 * _PathSz declares how many elements _Path can hold, when GoapPlanAction returns _PathSz will return with the number of elements inserted into _Path.
 */
void GoapPlanAction(const struct GOAPPlanner* _Planner, const void* _Data, const struct WorldState* _Start, struct WorldState* _End, int* _PathSz, struct GoapPathNode** _Path);
int GoapPathDoAction(const struct GOAPPlanner* _Planner, const struct GoapPathNode* _Node, struct WorldState* _State, void* _Data);
int GoapPathGetAction(const struct GoapPathNode* _Node);
void GoapAddUtility(struct GOAPPlanner* _Planner, const char* _Utility, UtilityCallback _Callback, int _Func);
void GoapBestUtility(const struct GOAPPlanner* _Planner, const void* _Data, struct WorldState* _BestState);
void GoapPlanUtility(const struct GOAPPlanner* _Planner, const void* _Data, const struct WorldState* _State, int* _PathSize, struct GoapPathNode** _Path);
#endif
