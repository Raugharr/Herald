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
struct Family;
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
	int Arguments;
};

extern struct LuaBhvAction g_BhvActions[];

int LuaBaCmp(const void* _One, const void* _Two);

int PAIHasField(struct Family* _Family, struct HashTable* _Vars, const struct Primitive* _Args, int _ArgSize);
int PAIHasHouse(struct Family* _Family, struct HashTable* _Vars, const struct Primitive* _Args, int _ArgSize);
int PAIBuildHouse(struct Family* _Family, struct HashTable* _Vars, const struct Primitive* _Args, int _ArgSize);
int PAICanFarm(struct Family* _Family, struct HashTable* _Vars, const struct Primitive* _Args, int _ArgSize);
int PAIHasPlow(struct Family* _Family, struct HashTable* _Vars, const struct Primitive* _Args, int _ArgSize);
int PAIMakeGood(struct Family* _Family, struct HashTable* _Vars, const struct Primitive* _Args, int _ArgSize);
int PAIHasReap(struct Family* _Family, struct HashTable* _Vars, const struct Primitive* _Args, int _ArgSize);
int PAIHasAnimals(struct Family* _Family, struct HashTable* _Vars, const struct Primitive* _Args, int _ArgSize);
int PAIConstructBuild(struct Family* _Family, struct HashTable* _Vars, const struct Primitive* _Args, int _ArgSize);
int PAIHasShelter(struct Family* _Family, struct HashTable* _Vars, const struct Primitive* _Args, int _ArgSize);
int PAIFeedAnimals(struct Family* _Family, struct HashTable* _Vars, const struct Primitive* _Args, int _ArgSize);
int PAIEat(struct Family* _Family, struct HashTable* _Vars, const struct Primitive* _Args, int _ArgSize);
int PAIMakeFood(struct Family* _Family, struct HashTable* _Vars, const struct Primitive* _Args, int _ArgSize);

int PAIHasAnimal(struct Family* _Family, struct HashTable* _Vars, const struct Primitive* _Args, int _ArgSize);
int PAIBuyAnimal(struct Family* _Family, struct HashTable* _Vars, const struct Primitive* _Args, int _ArgSize);
int PAIHasGood(struct Family* _Family, struct HashTable* _Vars, const struct Primitive* _Args, int _ArgSize);
int PAIBuyGood(struct Family* _Family, struct HashTable* _Vars, const struct Primitive* _Args, int _ArgSize);

int BHVNothing(struct Family* _Family, struct HashTable* _Vars, const struct Primitive* _Args, int _ArgSize);

int LuaActionLen(const struct LuaBhvAction* _Action);

const struct AgentUtility* GetBGPlanner();

void AIInit(lua_State* _State);
void AIQuit();

extern int g_BhvActionsSz;

#endif
