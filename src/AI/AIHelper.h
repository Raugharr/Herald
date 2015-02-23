/*
 * File: AIHelper.h
 * Author: David Brotz
 */
#ifndef __AIHELPER_H
#define __AIHELPER_H

struct Person;
struct Actor;
struct Object;

enum {
	ACTORJOB_EAT,
	ACTORJOB_PLANT,
	ACTORJOB_HARVEST,
	ACTORJOB_SIZE
};

int ActorJobTempTime(struct Actor* _Worker, struct Object* _NoObj, void* _NoVoid);

int ActorJobEat(struct Actor* _Worker, struct Object* _FoodPtr, void* _None);
int ActorJobPlant(struct Actor* _Worker, struct Object* _Field, void* _Seed);
int ActorJobHarvest(struct Actor* _Worker, struct Object* _Field, void* _Goods);

#endif
