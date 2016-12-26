/*
 * Author: David Brotz
 * File: Person.c
 */

#include "Person.h"

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

	CreateObject(&_Pregancy->Object, OBJECT_PREGANCY, (void(*)(struct Object*))PregnancyThink);
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
	CreateObject(&_Person->Object, OBJECT_PERSON, PersonThink);
	_Person->Name = _Name;
	_Person->Age = _Age;
	_Person->Gender = _Gender;
	_Person->Nutrition = _Nutrition;
	_Person->Family = _Family;
	_Person->Pregnancy = NULL;

	SettlementAddPerson(_Person->Family->HomeLoc, _Person);
	return _Person;
}

void DestroyPerson(struct Person* _Person) {
	//struct Family* _Family = _Person->Family;

	DestroyObject(&_Person->Object);
	SettlementRemovePerson(_Person->Family->HomeLoc, _Person);
	if(FamilySize(_Person->Family) == 0)
		_Person->Family->IsAlive = false;
	//	DestroyFamily(_Family);
	MemPoolFree(g_PersonPool, _Person);
}

struct Person* CreateChild(struct Family* _Family) {
	struct Person* _Mother = _Family->People[WIFE];
	struct Person* _Child = CreatePerson("Foo", 0, Random(1, 2), _Mother->Nutrition, _Mother->Pos.x, _Mother->Pos.y, _Mother->Family);
	
	_Child->Family = _Family;
	++_Family->HomeLoc->YearBirths;
	return _Child;
}

void PersonThink(struct Object* _Obj) {
	struct Person* _Person = (struct Person*) _Obj;

	if(_Person->Gender == EFEMALE) {
		if(_Person->Family != NULL
				&& _Person->Pregnancy == NULL
				&& _Person == _Person->Family->People[WIFE]
				&& _Person->Family->NumChildren < CHILDREN_SIZE
				&& Random(0, 1499) < 1) {
			_Person->Pregnancy = CreatePregnancy(_Person);
		}
	} 	
	_Person->Nutrition -= (PersonMature(_Person) == true) ? (NUTRITION_DAILY) : (NUTRITION_CHILDDAILY);
	if(_Person->Nutrition < 0)
		_Person->Nutrition = 0;
	NextDay(&_Person->Age);
	if(_Person->Nutrition > MAX_NUTRITION)
		_Person->Nutrition = MAX_NUTRITION;
}

void PersonMarry(struct Person* _Father, struct Person* _Mother, struct Family* _Family) {
	struct Family* _NewFam = CreateFamily(_Family->Name, FamilyGetSettlement(_Family), _Family);

	_NewFam->People[HUSBAND] = _Father;
	_NewFam->People[WIFE] = _Mother;
	_Father->Family = _NewFam;
	_Mother->Family = _NewFam;
	_NewFam->Fields[_NewFam->FieldCt++] = CreateField(NULL, 30, _NewFam);
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

uint16_t PersonWorkMult(const struct Person* _Person) {
	double _Modifier = _Person->Nutrition / ((double) MAX_NUTRITION);
	int _WorkRate = MAX_WORKRATE;

	if(YEAR(_Person->Age) < ADULT_AGE) {
		_WorkRate = MAX_WORKRATE / 2;
	}
	return (_Modifier >= 1.0f) ? (_WorkRate) : (_WorkRate * _Modifier);
}
