/*
 * File: Actor.c
 * Author: David Brotz
 */

#include "Actor.h"

#include "AI/AIHelper.h"
#include "AI/Pathfind.h"
#include "sys/LinkedList.h"
#include "sys/TaskPool.h"
#include "World.h"

#include <stdio.h>
#include <stdlib.h>

void CtorActor(struct Actor* _Actor, int _Type, int _X, int _Y, int(*_Think)(struct Object*), int _Gender, int _Nutrition, int _Age) {
	CreateObject((struct Object*) _Actor, OBJECT_ACTOR, (void(*)(struct Object*))ActorThink);
	_Actor->Pos.x = _X;
	_Actor->Pos.y = _Y;
	_Actor->Age = _Age;
	_Actor->Gender = _Gender;
	_Actor->Nutrition = _Nutrition;
	_Actor->Action = NULL;
}

void DtorActor(struct Actor* _Actor) {
	DestroyObject((struct Object*)_Actor);
}

void ActorDeath(struct Actor* _Actor) {

}

void ActorThink(struct Actor* _Actor) {
	_Actor->Nutrition -= NUTRITION_LOSS;
	if(_Actor->Nutrition < 0)
		_Actor->Nutrition = 0;
	NextDay(&_Actor->Age);
}

void ActorFeed(struct Actor* _Actor, int _Amount) {
	_Actor->Nutrition += _Amount;
}

int ActorWorkMult(struct Actor* _Actor) {
	return (_Actor->Nutrition / 15);
}
