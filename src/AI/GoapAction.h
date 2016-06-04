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
typedef int (*GoapActionFunc)(struct Agent*, void*);
typedef int (*GoapActionUtility)(const struct Agent*, int*, int*, struct WorldState*, void*);
typedef int (*GoapActionComplete)(const struct Agent*, void*);
typedef int (*GoapActionCondition)(const struct Agent*);
typedef void* (*GoapActionCreate)(const struct Agent*);
typedef void (*GoapActionDestroy)(void*);

struct GoapAction {
	const char* Name;
	struct WorldState Postconditions;
	GoapActionFunc ProPostcondition;	
	struct WorldState Preconditions;
	GoapActionCondition ProPrecondition;
	GoapActionFunc Action;
	GoapActionUtility Utility;
	GoapActionCost Cost;
	GoapActionCreate Create;
	GoapActionDestroy Destroy;
	int UtilityFunction;
	GoapActionComplete IsComplete;
};

void GoapActionSetName(struct GoapAction* _Action, const char* _Name);
/**
 * Adds a precondition to _Action.
 */
void GoapActionAddPrecond(struct GoapAction* _Action, struct GOAPPlanner* _Planner, const char* _Atom, int _Value, int _OpCode);
/**
 * Adds a postcondition to _Action.
 */
void GoapActionAddPostcond(struct GoapAction* _Action, struct GOAPPlanner* _Planner, const char* _Atom, int _Value, int _OpCode);
void GoapActionClear(struct GoapAction* _Action);

#endif
