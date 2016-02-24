/*
 * File: Actor.c
 * Author: David Brotz
 */

#include "Actor.h"

#include "Person.h"
#include "World.h"
#include "Population.h"

#include "AI/AIHelper.h"
#include "AI/Pathfind.h"

#include "sys/LinkedList.h"
#include "sys/TaskPool.h"

#include <stdio.h>
#include <stdlib.h>

void CtorActor(struct Actor* _Actor, int _Type, int _X, int _Y, ObjectThink _Think, int _Gender, int _Nutrition, int _Age) {
	CreateObject((struct Object*) _Actor, OBJECT_ACTOR, _Think);
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

void ActorThink(struct Actor* _Actor, const struct Population* _PopType) {
	int _Nutrition = NUTRITION_DAILY;

	if(_PopType != NULL) {
		//if(_PopType->Ages[0])
		_Nutrition = _PopType->Nutrition;
	} else if(PersonMature(_Actor))
		_Nutrition = NUTRITION_CHILDDAILY;
	_Actor->Nutrition -= _Nutrition;
	if(_Actor->Nutrition < 0)
		_Actor->Nutrition = 0;
	NextDay(&_Actor->Age);
}

void ActorFeed(struct Actor* _Actor, int _Amount) {
	_Actor->Nutrition += _Amount;
}

int ActorWorkMult(struct Actor* _Actor, int _MatureAge, int _AdultNut) {
	int _Modifier = _Actor->Nutrition / _AdultNut;
	int _WorkRate = MAX_WORKRATE;

	if(_Actor->Age < ADULT_AGE) {
		_Modifier = _Modifier / 2;
		_WorkRate = MAX_WORKRATE / 2;
	}
	return (_Modifier > _WorkRate) ? (_WorkRate) : (_Modifier);
}
