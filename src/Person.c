/*
 * Author: David Brotz
 * File: Person.c
 */

#include "Person.h"

#include "Herald.h"
#include "Family.h"
#include "sys/Constraint.h"
#include "sys/Random.h"
#include "sys/LinkedList.h"
#include "sys/MemoryPool.h"
#include "events/Event.h"


#include <stdlib.h>

#define POOLSIZE (10000)

#ifndef NULL
#define NULL (((void*)0))
#endif
#define BIRTH_TIME (10)

static struct MemoryPool* g_PersonPool = NULL;

void Person_Init() {
	g_PersonPool = (struct MemoryPool*) CreateMemoryPool(sizeof(struct Person), POOLSIZE);
}

void Person_Quit() {
	DestroyMemoryPool(g_PersonPool);
}

struct Person* CreatePerson(const char* _Name, int _Age, int _Gender, int _Nutrition) {
	struct Person* _Person = (struct Person*) MemPool_Alloc(g_PersonPool);

	_Person->Name = _Name;
	_Person->Age = _Age;
	_Person->Gender = _Gender;
	_Person->Nutrition = _Nutrition;
	_Person->Family = NULL;
	_Person->Parent = NULL;
	_Person->Occupation = NULL;
	return _Person;
}

void DestroyPerson(struct Person* _Person) {
	MemPool_Free(g_PersonPool, _Person);
}

struct Person* CreateChild(struct Family* _Family) {
	struct Person* _Child = (struct Person*) MemPool_Alloc(g_PersonPool);
	
	_Child->Gender = Random(1, 2);
	if(_Child->Gender == EMALE)
		_Child->Name = "Foo";
	else
		_Child->Name = "Foo";
	_Child->Age = 0;
	_Child->Nutrition = _Family->People[WIFE]->Nutrition;
	_Child->Family = NULL;
	_Child->Parent = _Family;
	return _Child;
}

struct Pregancy* CreatePregancy(struct Person* _Person) {
	struct Pregancy* _Pregancy = (struct Pregancy*) malloc(sizeof(struct Pregancy));

	_Pregancy->TTP = BIRTH_TIME + 1;
	_Pregancy->Mother = _Person;
	return _Pregancy;
};

void Person_Update(struct Person* _Person, int _NutVal) {
	if(_Person->Nutrition == -1)
		return; //Don't update dead people.
	if(_Person->Gender == EFEMALE) {
		if(_Person->Family->NumChildren < CHILDREN_SIZE && Random(1, 100) > Fuzify(g_BabyAvg, _Person->Family->NumChildren) && Random(1, 100) > 10)
			CreatePregancy(_Person);
	}
	if(_NutVal > _Person->Nutrition)
		_Person->Nutrition = _Person->Nutrition + (_NutVal - _Person->Nutrition);
	else
		_Person->Nutrition = _Person->Nutrition - (_Person->Nutrition - _NutVal);
	if(Random(0, 99) < (MAX_NUTRITION - _Person->Nutrition) / 100) {
		_Person->Nutrition = -1;//Dead
		Event_Push(CreateEventDeath(_Person));
	}
}
	
void Birth(struct Pregancy* _Pregancy) {
	if(--_Pregancy->TTP <= 0) {
		struct Person* _Child = CreateChild(_Pregancy->Mother->Family);
		_Pregancy->Mother->Family->People[CHILDREN + _Pregancy->Mother->Family->NumChildren++] = _Child;
		Event_Push(CreateEventBirth(_Pregancy->Mother, _Child));
	}
}

void Person_DeleteNames() {

}
