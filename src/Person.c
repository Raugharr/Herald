/*
 * Author: David Brotz
 * File: Person.c
 */

#include "Person.h"

#include "Herald.h"
#include "Family.h"
#include "Crop.h"
#include "Good.h"
#include "sys/RBTree.h"
#include "sys/Constraint.h"
#include "sys/Random.h"
#include "sys/Array.h"
#include "sys/MemoryPool.h"
#include "events/Event.h"

#include <stdlib.h>
#include <string.h>

#ifndef NULL
#define NULL (((void*)0))
#endif
#define BIRTH_TIME (9)

struct MemoryPool* g_PersonPool = NULL;
struct Person* g_PersonList = NULL;

struct Pregancy* CreatePregancy(struct Person* _Person) {
	struct Pregancy* _Pregancy = (struct Pregancy*) malloc(sizeof(struct Pregancy));

	_Pregancy->Prev = NULL;
	_Pregancy->Next = NULL;
	_Pregancy->Type = ATT_PREGANCY;
	_Pregancy->TTP = TO_DAYS(BIRTH_TIME) - 15 + Random(0, 29) + 1;
	_Pregancy->Mother = _Person;
	return _Pregancy;
};

void DestroyPregancy(struct Pregancy* _Pregancy) {
	free(_Pregancy);
}

int PregancyUpdate(struct Pregancy* _Pregancy) {
	if(--_Pregancy->TTP <= 0) {
		if(_Pregancy->Mother->Family->NumChildren >= CHILDREN_SIZE)
			return 1;
		struct Person* _Child = CreateChild(_Pregancy->Mother->Family);
		_Pregancy->Mother->Family->People[2 + _Pregancy->Mother->Family->NumChildren++] = _Child;
		Event_Push(CreateEventBirth(_Pregancy->Mother, _Child));
		return 1;
	}
	return 0;
}

struct Person* CreatePerson(const char* _Name, int _Age, int _Gender, int _Nutrition) {
	struct Person* _Person = (struct Person*) MemPool_Alloc(g_PersonPool);

	_Person->Name = _Name;
	_Person->Id = NextId();
	_Person->Age = _Age;
	_Person->Gender = _Gender;
	_Person->Nutrition = _Nutrition;
	_Person->Family = NULL;
	_Person->Parent = NULL;
	_Person->Occupation = NULL;
	if(g_PersonList == NULL)
		_Person->Next = NULL;
	else {
		_Person->Next = g_PersonList;
		g_PersonList->Prev = _Person;
	}
	_Person->Prev = NULL;
	g_PersonList = _Person;
	return _Person;
}

void DestroyPerson(struct Person* _Person) {
	if(g_PersonList == _Person) {
		g_PersonList = _Person->Next;
	} else {
		if(_Person->Prev != NULL)
		_Person->Prev->Next = _Person->Next;
		if(_Person->Next != NULL)
			_Person->Next->Prev = _Person->Prev;
	}
	MemPool_Free(g_PersonPool, _Person);
}

struct Person* CreateChild(struct Family* _Family) {
	struct Person* _Child = CreatePerson("Foo", 0, Random(1, 2), _Family->People[WIFE]->Nutrition);
	
	_Child->Family = _Family;
	return _Child;
}

int PersonUpdate(struct Person* _Person, struct HashTable* _Table) {
	int _NutVal = 1500;

	if(PersonDead(_Person) == 1)
		return 0;
	if(_Person->Gender == EFEMALE) {
		if(_Person->Family != NULL
				&& ATimerSearch(&g_ATimer, (struct Object*)_Person, ATT_PREGANCY) == NULL
				&& _Person == _Person->Family->People[WIFE]
				&& _Person->Family->NumChildren < CHILDREN_SIZE
				&& Random(1, 100) < 10)
			ATimerInsert(&g_ATimer, CreatePregancy(_Person));
	}
	if(_NutVal > _Person->Nutrition)
		_Person->Nutrition = _Person->Nutrition + (_NutVal - _Person->Nutrition);
	else
		_Person->Nutrition = _Person->Nutrition - (_Person->Nutrition - _NutVal);
	if(Random(0, 999) < (MAX_NUTRITION - _Person->Nutrition) / 500) {
		PersonDeath(_Person);
		return 1;
	}
	NextDay(&_Person->Age);
	return 1;
}

void PersonDeath(struct Person* _Person) {
	int i;
	struct Family* _Family = _Person->Family;

	_Person->Nutrition = 0;
	if(_Person->Gender == EFEMALE)
		ATimerRmNode(&g_ATimer, _Person);
	for(i = 0; i < _Family->NumChildren + CHILDREN; ++i)
		if(_Family->People[i] == _Person) {
			_Family->People[i] = NULL;
			if(i >= CHILDREN) {
				//if(_Family->NumChildren < CHILDREN_SIZE)
				_Family->People[i] = _Family->People[CHILDREN + _Family->NumChildren];
				--_Family->NumChildren;
			}
			break;
		}
	DestroyPerson(_Person);
	Event_Push(CreateEventDeath(_Person));
}

int PersonWorkMult(struct Person* _Person) {
	return (_Person->Nutrition / 15);
}
