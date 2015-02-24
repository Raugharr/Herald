/*
 * File: Actor.h
 * Author: David Brotz
 */
#ifndef __ACTOR_H
#define __ACTOR_H

#include "World.h"

#include "sys/RBTree.h"
#include "AI/AIHelper.h"

#define NUTRITION_LOSS (16)
#define ActorPerformJob(_Actor) (TaskPoolAdd(g_TaskPool, g_TaskPool->Time + (_Actor)->Action->TotalTime, (int(*)(void*, void*))(_Actor)->Action->Job->Callback, (_Actor)->Action->Owner, &(_Actor)->Action->Pair))

extern struct RBTree g_ActorJobs;

struct Object;
struct Path;

struct ActorJobPair {
	struct Object* Object;
	void* Extra;
};

struct ActorJobBase {
	int Id;
	int(*Callback)(struct Actor*, struct ActorJobPair*);
	int(*TimeCalc)(struct Actor*, struct Object*, void*);
};

extern struct ActorJobBase g_ActorJobBases[];

struct ActorJob {
	struct ActorJobBase* Job;
	struct Actor* Owner;
	struct ActorJobPair Pair;
	int TotalTime;
};

struct Actor {
	int Id;
	int Type;
	int X;
	int Y;
	int(*Think)(struct Object*);
	int Gender;
	int Nutrition;
	DATE Age;
	struct ActorJob* Action;
};

void CtorActor(struct Actor* _Actor, int _Type, int _X, int _Y, int(*_Think)(struct Object*), int _Gender, int _Nutrition, int _Age);
void DtorActor(struct Actor* _Actor);

int ActorICallback(const void* _One, const void* _Two);
int ActorSCallback(const void* _One, const void* _Two);

void ActorDeath(struct Actor* _Actor);
void ActorThink(struct Actor* _Actor);
void ActorFeed(struct Actor* _Actor, int _Amount);
int ActorWorkMult(struct Actor* _Actor);
void ActorMove(struct Actor* _Actor, struct Path* _Path);
int ActorMoveDir(struct Actor* _Actor, int _Direction);

int ActorNextJob(struct Actor* _Actor);
int ActorHasJob(struct Actor* _Actor);
void ActorAddJob(int _JobId, struct Actor* _Owner, struct Object* _Object, void* _Extra);
struct ActorJob* ActorPopJob(struct Actor* _Owner);

#endif
