/*
 * File: GoapAction.h
 * Author: David Brotz
 */

#ifndef __GOAPACTION_H
#define __GOAPACTION_H

#include "../WorldState.h"

struct Agent;
struct GOAPPlanner;

typedef int (*GoapActionCost)(const struct Agent*);
typedef int (*GoapAction)(void*);
typedef int (*GoapActionFunc)(struct Agent*);
typedef int (*GoapActionUtility)(const struct Agent*, int*, int*, struct WorldState*);
typedef int (*GoapActionComplete)(const struct Agent*);

struct GoapAction {
	const char* Name;
	struct WorldState Postconditions;
	struct WorldState Preconditions;
	GoapActionFunc Action;
	GoapActionUtility Utility;
	GoapActionCost Cost;
	int UtilityFunction;
	GoapActionComplete IsComplete;
};

void GoapActionAddPrecond(struct GoapAction* _Action, struct GOAPPlanner* _Planner, const char* _Atom, int _Value, int _OpCode); 
void GoapActionAddPostcond(struct GoapAction* _Action, struct GOAPPlanner* _Planner, const char* _Atom, int _Value, int _OpCode); 
void GoapActionClear(struct GoapAction* _Action);

#endif
