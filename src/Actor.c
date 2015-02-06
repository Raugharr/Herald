/*
 * File: Actor.c
 * Author: David Brotz
 */

#include "Actor.h"

void CtorActor(struct Actor* _Actor, int _X, int _Y, int(*_Think)(struct Object*), int _Gender, int _Nutrition, int _Age) {
	CreateObject((struct Object*)_Actor, _X, _Y, (int(*)(struct Object*))_Think);
	_Actor->Age = _Age;
	_Actor->Gender = _Gender;
	_Actor->Nutrition = _Nutrition;
}

void DtorActor(struct Actor* _Actor) {

}

void ActorDeath(struct Actor* _Actor) {

}

void ActorThink(struct Actor* _Actor) {
	_Actor->Nutrition -= NUTRITION_LOSS;
	NextDay(&_Actor->Age);
}

void ActorFeed(struct Actor* _Actor, int _Amount) {
	_Actor->Nutrition += _Amount;
}

int ActorWorkMult(struct Actor* _Actor) {
	return (_Actor->Nutrition / 15);
}
