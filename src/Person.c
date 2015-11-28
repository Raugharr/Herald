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

	_Person = (struct Person*) MemPoolAlloc(g_PersonPool);
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
	struct Family* _Family = _Person->Family;
	struct Good* _Good = NULL;
	int _Nutrition = 0;

	if(_Person->Gender == EFEMALE) {
		if(_Person->Family != NULL
				&& _Person->Pregnancy == NULL
				&& _Person == _Person->Family->People[WIFE]
				&& _Person->Family->NumChildren < CHILDREN_SIZE
				&& Random(0, 1499) < 1) {
			_Person->Pregnancy = CreatePregnancy(_Person);
		}
	} else {
		if(YEAR(_Person->Age) > 18 && _Person->Family->People[HUSBAND] != _Person) {
			struct LnkLst_Node* _Itr = _Family->HomeLoc->Families.Front;
			struct Family* _BrideFam = NULL;

			while(_Itr != NULL) {
				_BrideFam = (struct Family*) _Itr->Data;
				for(int i = 2; i < FAMILY_PEOPLESZ; ++i) {
					if(_BrideFam->People[i] != NULL && _BrideFam->People[i]->Gender == EFEMALE && YEAR(_BrideFam->People[i]->Age) > 16) {
						struct Family* _NewFam = CreateFamily(_Family->Name, FamilyGetSettlement(_Family), _Family->FamilyId, _Family);

						_NewFam->People[HUSBAND] = _Person;
						_NewFam->People[WIFE] = _BrideFam->People[i];
						_BrideFam->People[i] = NULL;
						for(int j = 2; j < FAMILY_PEOPLESZ; ++j)
							if(_Person == _Family->People[i]) {
								_Family->People[i] = NULL;
								break;
							}
						break;
					}
				}
				_Itr = _Itr->Next;
			}
		}
	}
	ActorThink((struct Actor*) _Person);
	for(int i = 0; i < _Family->Goods->Size; ++i) {
		_Good = (struct Good*) _Family->Goods->Table[i];
		if(_Good->Base->Category == EFOOD) {
			_Nutrition += PersonEat(_Person, (struct Food*) _Good);
			if(_Nutrition >= NUTRITION_DAILY)
				goto sufficient_food;
		}
	}
	EventPush(CreateEventStarvingFamily(_Family));
	sufficient_food:
	if(_Person->Nutrition <= 0)
		PersonDeath(_Person);
	return 1;
}

int PersonEat(struct Person* _Person, struct Food* _Food) {
	/*struct Good** _Tbl = (struct Good**)_Person->Family->Goods->Table;
	int _NutReq = NUTRITION_LOSS;*/
	struct Food* _FoodPtr = NULL;
	int _Nut = 0;
	int _FoodAmt = 0;
	struct Family* _Family = _Person->Family;
	int _GoodSz = _Family->Goods->Size;

		for(int i = 0; i < _GoodSz; ++i) {
			if(_Nut >= NUTRITION_DAILY)
				break;
			if(((struct Good*)_Family->Goods->Table[i])->Base->Category == EFOOD) {
				_FoodPtr = (struct Food*)_Family->Goods->Table[i];
				while(_FoodAmt < _FoodPtr->Quantity && _Nut < NUTRITION_DAILY) {
					_Nut += _FoodPtr->Base->Nutrition;
					++_FoodAmt;
				}
				if(_FoodPtr->Quantity - _FoodAmt <= 0) {
					free(_FoodPtr);
					ArrayRemove(_Family->Goods, i);
					--_GoodSz;
					i = i - 1;
				} else {
					_FoodPtr->Quantity = _FoodPtr->Quantity - _FoodAmt;
				}
				_FoodAmt = 0;
			}
		}
	if(_Nut == 0)
		Log(ELOG_WARNING, "Day %i: %i has no food to eat.", DateToDays(g_GameWorld.Date), _Person->Id);
	ActorFeed((struct Actor*)_Person, _Nut);
	return _Nut;
}

void PersonDeath(struct Person* _Person) {
	int i;
	struct Family* _Family = _Person->Family;

	if(_Person->Pregnancy != NULL)
		DestroyPregnancy(_Person->Pregnancy);
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
	PushEvent(EVENT_DEATH, _Person, NULL);
	++_Family->HomeLoc->YearDeaths;
	--_Family->HomeLoc->NumPeople;
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
