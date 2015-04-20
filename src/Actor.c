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
	CreateObject((struct Object*)_Actor, _Type, _X, _Y, (int(*)(struct Object*))_Think);
	_Actor->Age = _Age;
	_Actor->Gender = _Gender;
	_Actor->Nutrition = _Nutrition;
	_Actor->Action = NULL;
}

void DtorActor(struct Actor* _Actor) {
	ObjectRmPos((struct Object*) _Actor);
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

void ActorMove(struct Actor* _Actor, struct Path* _Path) {
	struct Path* _PathItr = NULL;
	int _TileCt = 0;

	while(_Path->Next == NULL)
		_PathItr = _Path;
		while(_PathItr != NULL) {
			for(_TileCt = _PathItr->Tiles; _TileCt > 0; --_TileCt)
				ActorMoveDir(_Actor, _PathItr->Direction);
			_PathItr = _PathItr->Next;
		}
		_PathItr = _Path;
		while(_PathItr != NULL) {
			_PathItr = _PathItr->Next;
			DestroyPath(_Path);
			_Path = _PathItr;
		}
}

int ActorMoveDir(struct Actor* _Actor, int _Direction) {
	switch(_Direction) {
	case EDIR_NORTH:
		++_Actor->Y;
		break;
	case EDIR_EAST:
		++_Actor->X;
		break;
	case EDIR_SOUTH:
		--_Actor->Y;
		break;
	case EDIR_WEST:
		--_Actor->X;
		break;
	}
	ObjectMove((struct Object*) _Actor, _Actor->X, _Actor->Y);
	return 1;
}
