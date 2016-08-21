/*
 * File: Actor.h
 * Author: David Brotz
 */
#ifndef __ACTOR_H
#define __ACTOR_H

#include "World.h"

#include "sys/RBTree.h"

#include <SDL2/SDL.h>

#define ActorPerformJob(_Actor) (TaskPoolAdd(g_TaskPool, g_TaskPool->Time + (_Actor)->Action->TotalTime, (int(*)(void*, void*))(_Actor)->Action->Job->Callback, (_Actor)->Action->Owner, &(_Actor)->Action->Pair))
#define MAX_WORKRATE (100)

extern struct RBTree g_ActorJobs;

struct Object;
struct Path;
struct Population;

struct Actor {
	int Id;
	int Type;
	void (*Think)(struct Object*);
	int LastThink; //In game ticks.
	struct LnkLst_Node* ThinkObj;
	SDL_Point Pos;
	int Gender;
	int Nutrition;
	DATE Age;
	struct ActorJob* Action;
};

void CtorActor(struct Actor* _Actor, int _Type, int _X, int _Y, ObjectThink _Think, int _Gender, int _Nutrition, int _Age);
void DtorActor(struct Actor* _Actor);

int ActorICallback(const void* _One, const void* _Two);
int ActorSCallback(const void* _One, const void* _Two);

void ActorDeath(struct Actor* _Actor);
void ActorThink(struct Actor* _Actor, const struct Population* _PopType);
void ActorFeed(struct Actor* _Actor, int _Amount);
int ActorWorkMult(struct Actor* _Actor, int _MatureAge, int _MaxNut);

#endif
