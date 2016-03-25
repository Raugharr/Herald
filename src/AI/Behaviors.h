/*
 * File: Behaviors.h
 * Author: David Brotz
 */
#ifndef __BEHAVIORS_H
#define __BEHAVIORS_H

#include "BehaviorTree.h"

#define AI_MAKEGOOD "MakeGood"
#define AI_MAKEBUILD "MakeBuild"
#define AI_REAP "Stone Sickle"
#define AI_PLOW "Scratch Plow"
#define AI_HOUSE "House"
#define AI_SHELTER "Shelter"

struct Family;
struct HashTable;
struct Primitive;
typedef struct lua_State lua_State;

struct LuaBhvAction {
	const char* Name;
	BhvAction Action;
	int Arguments;
};

extern struct LuaBhvAction g_BhvActions[];
extern int g_BhvActionsSz;

int BehaviorsInit(lua_State* _State);

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
int PAIMakeFood(struct Family* _Family, struct HashTable* _Vars, const struct Primitive* _Args, int _ArgSize);

int PAIHasAnimal(struct Family* _Family, struct HashTable* _Vars, const struct Primitive* _Args, int _ArgSize);
int PAIBuyAnimal(struct Family* _Family, struct HashTable* _Vars, const struct Primitive* _Args, int _ArgSize);
int PAIHasGood(struct Family* _Family, struct HashTable* _Vars, const struct Primitive* _Args, int _ArgSize);
int PAIBuyGood(struct Family* _Family, struct HashTable* _Vars, const struct Primitive* _Args, int _ArgSize);

int BHVNothing(struct Family* _Family, struct HashTable* _Vars, const struct Primitive* _Args, int _ArgSize);

int LuaActionLen(const struct LuaBhvAction* _Action);

#endif
