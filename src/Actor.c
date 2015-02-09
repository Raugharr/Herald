/*
 * File: Actor.c
 * Author: David Brotz
 */

#include "Actor.h"

#include "AI/AIHelper.h"
#include "sys/LinkedList.h"

#include <stdio.h>
#include <stdlib.h>

struct RBTree g_ActorJobs = {NULL, 0, ActorICallback, ActorSCallback};
struct LinkedList g_ActorJobsList = {0, NULL, NULL};

void CtorActor(struct Actor* _Actor, int _Type, int _X, int _Y, int(*_Think)(struct Object*), int _Gender, int _Nutrition, int _Age) {
	CreateObject((struct Object*)_Actor, _Type, _X, _Y, (int(*)(struct Object*))_Think);
	_Actor->Age = _Age;
	_Actor->Gender = _Gender;
	_Actor->Nutrition = _Nutrition;
	_Actor->Action = NULL;
}

void DtorActor(struct Actor* _Actor) {

}

int ActorICallback(const void* _One, const void* _Two) {
	return ObjCmp(((struct LnkLst_Node*)_One)->Data, ((struct LnkLst_Node*)_Two)->Data);
}
int ActorSCallback(const void* _One, const void* _Two) {
	return ObjCmp(((struct Actor*)_One), ((struct ActorJob*)((struct LnkLst_Node*)_Two)->Data)->Owner);
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

int ActorMove(struct Actor* _Actor, int _Direction) {
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

void ActorNextJob(struct Actor* _Actor) {
	struct ActorJob* _Job = NULL;
	struct Path _AIPath;
	struct Path* _PathItr = NULL;
	int _TileCt = 0;

	if((_Job = ActorPopJob(_Actor)) != NULL) {
		if(Distance(_Actor->X, _Actor->Y, _Job->X, _Job->Y) > 1) {
			Pathfind(_Actor->X, _Actor->Y, _Job->X, _Job->Y, &_AIPath);
			_PathItr = &_AIPath;
			while(_PathItr != NULL) {
				for(_TileCt = _PathItr->Tiles; _TileCt > 0; --_TileCt)
					ActorMove(_Actor, _PathItr->Direction);
				_PathItr = _PathItr->Next;
			}
		}
		_PathItr = &_AIPath;
		while(_PathItr != NULL) {
			_PathItr = _PathItr->Next;
			DestroyPath(&_AIPath);
			_AIPath = *_PathItr;
		}
		_Job->Job->Callback(_Job->Owner, _Job->Extra);
	}
}

int ActorHasJob(struct Actor* _Actor) {
	return RBSearch(&g_ActorJobs, _Actor) != NULL;
}

void ActorAddJob(struct Actor* _Owner, int _JobId, void* _Extra, int _JobX, int _JobY) {
	struct ActorJob* _Job = (struct ActorJob*) malloc(sizeof(struct ActorJob));
	struct LnkLst_Node* _Node = NULL;

	_Job->Job = &g_ActorJobBases[_JobId];
	_Job->Owner = _Owner;
	_Job->Extra = _Extra;
	_Job->X = _JobX;
	_Job->Y = _JobY;
	if((_Node = RBSearch(&g_ActorJobs, _Owner)) == NULL) {
		LnkLstPushBack(&g_ActorJobsList, _Job);
		RBInsert(&g_ActorJobs, g_ActorJobsList.Back);
	} else
		LnkLstInsertAfter(&g_ActorJobsList, _Node, _Job);
}

struct ActorJob* ActorPopJob(struct Actor* _Owner) {
	struct RBNode* _Node = RBSearchNode(&g_ActorJobs, _Owner);
	struct LnkLst_Node* _Job = NULL;
	struct ActorJob* _Data = NULL;

	if(_Node == NULL)
		return NULL;
	_Job = _Node->Data;
	if(_Job->Next != NULL && ((struct ActorJob*)_Job->Data)->Owner->Id == ((struct ActorJob*)_Job->Next->Data)->Owner->Id)
		_Node->Data = _Job->Next;
	else
		RBDeleteNode(&g_ActorJobs, _Node);
	_Data = (struct ActorJob*) _Job->Data;
	LnkLstRemove(&g_ActorJobsList, _Job);
	return _Data;
}
