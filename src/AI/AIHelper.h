/*
 * File: AIHelper.h
 * Author: David Brotz
 */
#ifndef __AIHELPER_H
#define __AIHELPER_H

struct Person;
struct Actor;
struct Object;
struct ActorJobPair;

enum {
	ACTORJOB_EAT,
	ACTORJOB_PLANT,
	ACTORJOB_HARVEST,
	ACTORJOB_SIZE
};

int ActorJobTempTime(struct Actor* _Worker, struct Object* _NoObj, void* _NoVoid);

int ActorJobEat(struct Actor* _Worker, struct ActorJobPair* _Pair);
int ActorJobPlant(struct Actor* _Worker, struct ActorJobPair* _Pair);
int ActorJobHarvest(struct Actor* _Worker, struct ActorJobPair* _Pair);

#endif
