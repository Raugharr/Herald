/*
 * File: Utility.h
 * Author: David Brotz
 */
#ifndef __UTILITY_H
#define __UTILITY_H

#include "goap.h"

#define UTILITYSZ (64)

typedef int (*UtilityCallback)(const void*, int*, int*, struct WorldState*);

enum {
	UTILITY_INVERSE = (1 << 0),
	UTILITY_LINEAR = 2,
	UTILITY_QUADRATIC = 4
};

struct AgentUtility {
	UtilityCallback Utilities[UTILITYSZ];
	int UtilityFunction[UTILITYSZ];
	int UtilityCt;
	const char* UtilityNames[UTILITYSZ];
	struct GOAPPlanner Planner;
};

void AUtilityClear(struct AgentUtility* _Planner);
void AUtilityAdd(struct AgentUtility* _Planner, const char* _Utility, UtilityCallback _Callback, int _Func);
void AUtilityBest(const struct AgentUtility* _Planner, const void* _Data, struct WorldState* _BestState);
void AUtilityPlan(const struct AgentUtility* _Planner, const void* _Data, const struct WorldState* _State, int* _PathSize, struct GoapPathNode** _Path);

struct GOAPPlanner* AUtilityGetGoap(struct AgentUtility* _Planner);

#endif
