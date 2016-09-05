/*
 * Author: David Brotz
 * File: Person.c
 */

#include "Person.h"

#include "Actor.h"
#include "Herald.h"
#include "Family.h"
#include "Crop.h"
#include "Good.h"
#include "Location.h"

#include "sys/LinkedList.h"
#include "sys/LuaCore.h"
#include "sys/RBTree.h"
#include "sys/Constraint.h"
#include "sys/Math.h"
#include "sys/Array.h"
#include "sys/MemoryPool.h"
#include "sys/Event.h"
#include "sys/Log.h"

#include "AI/LuaLib.h"
#include "AI/Setup.h"

#include <stdlib.h>
#include <string.h>
#include <lua/lua.h>
#include <lua/lauxlib.h>
#include <assert.h>

#define BIRTH_TIME (9)

struct MemoryPool* g_PersonPool = NULL;

struct Pregnancy* CreatePregnancy(struct Person* _Person) {
	struct Pregnancy* _Pregancy = (struct Pregnancy*) malloc(sizeof(struct Pregnancy));

	CreateObject((struct Object*)_Pregancy, OBJECT_PREGANCY, (void(*)(struct Object*))PregnancyThink);
	_Pregancy->TTP = TO_DAYS(BIRTH_TIME) - 15 + Random(0, 29) + 1;
	_Pregancy->Mother = _Person;
	return _Pregancy;
};

void DestroyPregnancy(struct Pregnancy* _Pregnancy) {
	_Pregnancy->Mother->Pregnancy = NULL;
	DestroyObject((struct Object*)_Pregnancy);
	free(_Pregnancy);
}

void PregnancyThink(struct Pregnancy* _Pregnancy) {
	struct Person* _Child = NULL;

	if(--_Pregnancy->TTP <= 0) {
		if(_Pregnancy->Mother->Family->NumChildren >= CHILDREN_SIZE) {
			DestroyPregnancy(_Pregnancy);
			return;
		}
		_Child = CreateChild(_Pregnancy->Mother->Family);
		_Pregnancy->Mother->Family->People[2 + _Pregnancy->Mother->Family->NumChildren++] = _Child;
		++_Pregnancy->Mother->Family->HomeLoc->YearBirths;
		PushEvent(EVENT_BIRTH, _Pregnancy->Mother, _Child);
		DestroyPregnancy(_Pregnancy);
	}
}

struct Person* CreatePerson(const char* _Name, int _Age, int _Gender, int _Nutrition, int _X, int _Y, struct Family* _Family) {
	struct Person* _Person = NULL;
	if(_Name == NULL || _Age < 0 || (_Gender != EMALE && _Gender != EFEMALE) || _Family == NULL || _X < 0 || _Y < 0) {
		Log(ELOG_WARNING, "Cannot create person, invalid attributes.");
		return NULL;
	}

	_Person = (struct Person*) MemPoolAlloc(g_PersonPool);
	CtorActor((struct Actor*)_Person, OBJECT_PERSON, _X, _Y, (ObjectThink) PersonThink, _Gender, _Nutrition, _Age);
	_Person->Name = _Name;
	_Person->Age = _Age;
	_Person->Gender = _Gender;
	_Person->Nutrition = _Nutrition;
	_Person->Family = _Family;
	_Person->Pregnancy = NULL;

	SettlementAddPerson(_Person->Family->HomeLoc, _Person);
	_Person->Behavior = NULL;
	return _Person;
}

void DestroyPerson(struct Person* _Person) {
	//struct Family* _Family = _Person->Family;

	DtorActor((struct Actor*)_Person);
	SettlementRemovePerson(_Person->Family->HomeLoc, _Person);
	//if(FamilySize(_Family) == 0)
	//		DestroyFamily(_Family);
	MemPoolFree(g_PersonPool, _Person);
}

struct Person* CreateChild(struct Family* _Family) {
	struct Person* _Mother = _Family->People[WIFE];
	struct Person* _Child = CreatePerson("Foo", 0, Random(1, 2), _Mother->Nutrition, _Mother->Pos.x, _Mother->Pos.y, _Mother->Family);
	
	_Child->Family = _Family;
	++_Family->HomeLoc->YearBirths;
	return _Child;
}

int PersonThink(struct Person* _Person) {
	if(_Person->Gender == EFEMALE) {
		if(_Person->Family != NULL
				&& _Person->Pregnancy == NULL
				&& _Person == _Person->Family->People[WIFE]
				&& _Person->Family->NumChildren < CHILDREN_SIZE
				&& Random(0, 1499) < 1) {
			_Person->Pregnancy = CreatePregnancy(_Person);
		}
	} 	
	ActorThink((struct Actor*) _Person, NULL);
	if(_Person->Nutrition > MAX_NUTRITION)
		_Person->Nutrition = MAX_NUTRITION;
	return 1;
}

void PersonMarry(struct Person* _Father, struct Person* _Mother, struct Family* _Family) {
	struct Family* _NewFam = CreateFamily(_Family->Name, FamilyGetSettlement(_Family), _Family);

	_NewFam->People[HUSBAND] = _Father;
	_NewFam->People[WIFE] = _Mother;
	_Father->Family = _NewFam;
	_Mother->Family = _NewFam;
	_NewFam->Fields[_NewFam->FieldCt++] = CreateField(_NewFam->HomeLoc->Pos.x, _NewFam->HomeLoc->Pos.y, NULL, 30, _Family);
}

void PersonDeath(struct Person* _Person) {
	struct Family* _Family = _Person->Family;

	if(_Person->Pregnancy != NULL)
		DestroyPregnancy(_Person->Pregnancy);
	for(int i = 0; i < _Family->NumChildren + CHILDREN; ++i) {
		if(_Family->People[i] == _Person) {
			_Family->People[i] = NULL;
			if(i >= CHILDREN) {
				--_Family->NumChildren;
				_Family->People[i] = _Family->People[CHILDREN + _Family->NumChildren];
				_Family->People[CHILDREN + _Family->NumChildren] = NULL;
			}
			break;
		}
	}
	PushEvent(EVENT_DEATH, _Person, NULL);
	++_Family->HomeLoc->YearDeaths;
	if(PersonMature(_Person) == 1) {
		if(_Person->Gender == EMALE)
			--_Family->HomeLoc->AdultMen;
		else
			--_Family->HomeLoc->AdultWomen;
	}
}

int PersonIsWarrior(const struct Person* _Person) {
	if(_Person->Gender != EMALE || YEAR(_Person->Age) < 15)
		return 0;
	return 1;
}

struct Person* GetFather(struct Person* _Person) {
	struct Family* _Family = _Person->Family;

	if(_Family->People[HUSBAND] != _Person)
		return _Family->People[HUSBAND];
	if(_Family->Parent != NULL)
		return _Family->Parent->People[HUSBAND];
	return NULL;
}
