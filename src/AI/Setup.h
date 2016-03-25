/*
 * File: Setup.h
 * Author: David Brotz
 */

#ifndef __SETUP_H
#define __SETUP_H

#include "../sys/Array.h"

#ifndef NULL
#define NULL ((void*)0)
#endif

struct WorldState;
struct AgentUtility;
struct GOAPPlanner;
typedef struct lua_State lua_State;
typedef void(*AgentActions[])(struct GOAPPlanner*, struct AgentUtility*);

extern struct Array g_BhvList;
typedef int(*AgentActionFunc)(void*);
typedef int(*AgentUtilityFunc)(const void*, int*, int*, struct WorldState*);

const struct AgentUtility* GetBGPlanner();

void BGSetup(struct AgentUtility* _AgentPlan, const char** _Atoms, int _AtomSz, AgentActions _Actions);
void AIInit(lua_State* _State);
void AIQuit();

extern int g_BhvActionsSz;

#endif
