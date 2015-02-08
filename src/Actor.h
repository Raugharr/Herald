/*
 * File: Actor.h
 * Author: David Brotz
 */
#ifndef __ACTOR_H
#define __ACTOR_H

#include "World.h"

#define NUTRITION_LOSS (16)

struct Actor {
	int Id;
	int X;
	int Y;
	int(*Think)(struct Object*);
	int Gender;
	int Nutrition;
	DATE Age;
};

void CtorActor(struct Actor* _Actor, int _X, int _Y, int(*_Think)(struct Object*), int _Gender, int _Nutrition, int _Age);
void DtorActor(struct Actor* _Actor);

void ActorDeath(struct Actor* _Actor);
void ActorThink(struct Actor* _Actor);
void ActorFeed(struct Actor* _Actor, int _Amount);
int ActorWorkMult(struct Actor* _Actor);

#endif
