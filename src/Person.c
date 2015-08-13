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
	DestroyObject((struct Object*)_Pregnancy);
	free(_Pregnancy);
}

void PregnancyThink(struct Pregnancy* _Pregnancy) {
	if(--_Pregnancy->TTP <= 0) {
		if(_Pregnancy->Mother->Family->NumChildren >= CHILDREN_SIZE)
			DestroyPregnancy(_Pregnancy);
		struct Person* _Child = CreateChild(_Pregnancy->Mother->Family);
		_Pregnancy->Mother->Family->People[2 + _Pregnancy->Mother->Family->NumChildren++] = _Child;
		EventPush(CreateEventBirth(_Pregnancy->Mother, _Child));
		DestroyPregnancy(_Pregnancy);
	}
}

struct Person* CreatePerson(const char* _Name, int _Age, int _Gender, int _Nutrition, int _X, int _Y, struct Family* _Family) {
	struct Person* _Person = NULL;

	_Person = (struct Person*) MemPool_Alloc(g_PersonPool);
	CtorActor((struct Actor*)_Person, OBJECT_PERSON, _X, _Y, (int(*)(struct Object*))PersonThink, _Gender, _Nutrition, _Age);
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
	DtorActor((struct Actor*)_Person);
	SettlementRemovePerson(_Person->Family->HomeLoc, _Person);
	MemPool_Free(g_PersonPool, _Person);
}

struct Person* CreateChild(struct Family* _Family) {
	struct Person* _Mother = _Family->People[WIFE];
	struct Person* _Child = CreatePerson("Foo", 0, Random(1, 2), _Mother->Nutrition, _Mother->Pos.x, _Mother->Pos.y, _Mother->Family);
	
	_Child->Family = _Family;
	return _Child;
}

int PersonThink(struct Person* _Person) {
	if(_Person->Gender == EFEMALE) {
		if(_Person->Family != NULL
				&& _Person->Pregnancy == NULL
				&& _Person == _Person->Family->People[WIFE]
				&& _Person->Family->NumChildren < CHILDREN_SIZE
				&& Random(0, 999) < 20)
			CreatePregnancy(_Person);
	}
	ActorThink((struct Actor*) _Person);
	/*if(Random(0, 999) < (MAX_NUTRITION - _Person->Nutrition) / 500) {
		++g_Deaths;
		PersonDeath(_Person);
		return 1;
	}*/
	return 1;
}

int PersonEat(struct Person* _Person, struct Food* _Food) {
	/*int _Size = _Person->Family->Goods->Size;
	struct Good** _Tbl = (struct Good**)_Person->Family->Goods->Table;
	int _NutReq = NUTRITION_LOSS;*/
	struct Food* _FoodPtr = NULL;
	int _Nut = 0;
	int i = 0;
	struct Family* _Family = _Person->Family;

		for(i = 0; i < _Family->Goods->Size; ++i) {
			if(((struct Good*)_Family->Goods->Table[i])->Base->Category == EFOOD) {
				_FoodPtr = (struct Food*)_Family->Goods->Table[i];
			}
		}
	if(_Nut == 0)
		Log(ELOG_WARNING, "Day %i: %i has no food to eat.", DateToDays(g_GameWorld.Date), _Person->Id);
	_Person->Nutrition += _Nut;
	return _Nut;
}

void PersonDeath(struct Person* _Person) {
	int i;
	struct Family* _Family = _Person->Family;

	free(_Person->Pregnancy);
	//if(_Person->Gender == EFEMALE)
	//	ATimerRmNode(&g_ATimer, _Person);
	for(i = 0; i < _Family->NumChildren + CHILDREN; ++i)
		if(_Family->People[i] == _Person) {
			_Family->People[i] = NULL;
			if(i >= CHILDREN) {
				--_Family->NumChildren;
				_Family->People[i] = _Family->People[CHILDREN + _Family->NumChildren];
				_Family->People[CHILDREN + _Family->NumChildren] = NULL;
			}
			break;
		}
	RBDelete(&g_GameWorld.BigGuys, _Person);
	EventPush(CreateEventDeath(_Person));
	DestroyPerson(_Person);
}

int PersonIsWarrior(const struct Person* _Person) {
	if(_Person->Gender != EMALE || YEAR(_Person->Age) < 15)
		return 0;
	return 1;
}
