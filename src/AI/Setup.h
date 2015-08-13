/*
 * File: Setup.h
 * Author: David Brotz
 */

#ifndef __SETUP_H
#define __SETUP_H

#include "../sys/Array.h"
#include "BehaviorTree.h"

struct Behavior;
struct Person;
struct HashTable;
struct Array;
struct GOAPPlanner;
typedef struct lua_State lua_State;

#define AI_MAKEGOOD "MakeGood"
#define AI_MAKEBUILD "MakeBuild"
#define AI_REAP "Stone Sickle"
#define AI_PLOW "Scratch Plow"
#define AI_HOUSE "House"
#define AI_SHELTER "Shelter"

#ifndef NULL
#define NULL ((void*)0)
#endif

extern struct Array g_BhvList;

struct LuaBhvAction {
	const char* Name;
	BhvAction Action;
};

extern struct LuaBhvAction g_BhvActions[];

int LuaBaCmp(const void* _One, const void* _Two);

int PAIHasField(struct Person* _Person, struct HashTable* _Table);
int PAIHasHouse(struct Person* _Person, struct HashTable* _Table);
int PAIWorkField(struct Person* _Person, struct HashTable* _Table);
int PAIBuildHouse(struct Person* _Person, struct HashTable* _Table);
int PAICanFarm(struct Person* _Person, struct HashTable* _Table);
int PAIHasPlow(struct Person* _Person, struct HashTable* _Table);
int PAIMakeGood(struct Person* _Person, struct HashTable* _Table);
int PAIHasReap(struct Person* _Person, struct HashTable* _Table);
int PAIHasAnimals(struct Person* _Person, struct HashTable* _Table);
int PAIConstructBuild(struct Person* _Person, struct HashTable* _Table);
int PAIHasShelter(struct Person* _Person, struct HashTable* _Table);
int PAIFeedAnimals(struct Person* _Person, struct HashTable* _Table);
int PAIEat(struct Person* _Person, struct HashTable* _Table);
int PAIMakeFood(struct Person* _Person, struct HashTable* _Table);
int PAIIsMale(struct Person* _Person, struct HashTable* _Table);
int BHVNothing(struct Person* _Person, struct HashTable* _Table);

int LuaActionLen(const struct LuaBhvAction* _Action);

const struct AgentUtility* GetBGPlanner();

void AIInit(lua_State* _State);
void AIQuit();

extern int g_BhvActionsSz;

#endif
