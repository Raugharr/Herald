/*
 * File: Setup.h
 * Author: David Brotz
 */

#ifndef __SETUP_H
#define __SETUP_H

#include "goap.h"

#include "../sys/Array.h"

#ifndef NULL
#define NULL ((void*)0)
#endif

struct WorldState;
struct AgentUtility;
struct GOAPPlanner;
typedef struct lua_State lua_State;

extern struct Array g_BhvList;

const struct AgentUtility* GetBGPlanner();

void BGSetup(struct GOAPPlanner* _Planner, const char** _Atoms, int _AtomSz, AgentActions _Actions, AgentGoals _Goals);
void AIInit(lua_State* _State);
void AIQuit();

extern int g_BhvActionsSz;
extern struct GOAPPlanner g_Goap;

#endif

