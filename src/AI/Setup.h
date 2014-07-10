/*
 * File: Setup.h
 * Author: David Brotz
 */

#ifndef __SETUP_H
#define __SETUP_H

struct Behavior;
struct Person;
struct HashTable;
struct Array;

#define AI_MAKEGOOD "MakeGood"
#define AI_MAKEBUILD "MakeBuild"
#define AI_REAP "Reap"
#define AI_PLOW "Plow"
#define AI_HOUSE "House"
#define AI_SHELTER "Shelter"

extern struct Behavior* g_AIMan;
extern struct Behavior* g_AIWoman;
extern struct Behavior* g_AIChild;

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
int BHVNothing(struct Person* _Person, struct HashTable* _Table);

void AIInit();
void AIQuit();

#endif
