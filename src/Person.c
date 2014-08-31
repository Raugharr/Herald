/*
 * Author: David Brotz
 * File: Person.c
 */

#include "Person.h"

#include "Herald.h"
#include "Family.h"
#include "Crop.h"
#include "Good.h"
#include "sys/LuaHelper.h"
#include "sys/RBTree.h"
#include "sys/Constraint.h"
#include "sys/Random.h"
#include "sys/Array.h"
#include "sys/MemoryPool.h"
#include "sys/KDTree.h"
#include "sys/Event.h"
#include "sys/Log.h"
#include "AI/LuaLib.h"
#include "AI/Setup.h"

#include <stdlib.h>
#include <string.h>
#include <lua/lua.h>
#include <lua/lauxlib.h>

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
		EventPush(CreateEventBirth(_Pregancy->Mother, _Child));
		return 1;
	}
	return 0;
}

struct Person* CreatePerson(const char* _Name, int _Age, int _Gender, int _Nutrition, int _X, int _Y) {
	struct Person* _Person = NULL;
	struct LuaBehavior* _Bhv = NULL;

	lua_getglobal(g_LuaState, "AI");
	lua_getfield(g_LuaState, -1, "SetAI");
	LuaCallFunc(g_LuaState, 0, 1, 0);
	if(lua_isstring(g_LuaState, -1) == 0) {
		Log(ELOG_WARNING, "AI.SetAI did not return a string.");
		return NULL;
	}
	if((_Bhv = BinarySearch(lua_tostring(g_LuaState, -1), g_BhvList.Table, g_BhvList.TblSize, luaStrLuaBhvCmp)) == NULL) {
		Log(ELOG_WARNING, "AI.SetAI did not return a valid Behavior.");
		return NULL;
	}

	_Person = (struct Person*) MemPool_Alloc(g_PersonPool);
	_Person->Name = _Name;
	CreateObject((struct Object*)_Person, _X, _Y);
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
	_Person->Behavior = _Bhv->Behavior;
	g_PersonList = _Person;
	lua_pop(g_LuaState, 2);
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
	struct Person* _Mother = _Family->People[WIFE];
	struct Person* _Child = CreatePerson("Foo", 0, Random(1, 2), _Mother->Nutrition, _Mother->X, _Mother->Y);
	
	_Child->Family = _Family;
	return _Child;
}

int PersonUpdate(struct Person* _Person, struct HashTable* _Table) {
	if(_Person->Gender == EFEMALE) {
		if(_Person->Family != NULL
				&& ATimerSearch(&g_ATimer, (struct Object*)_Person, ATT_PREGANCY) == NULL
				&& _Person == _Person->Family->People[WIFE]
				&& _Person->Family->NumChildren < CHILDREN_SIZE
				&& Random(0, 999) < 20)
			ATimerInsert(&g_ATimer, CreatePregancy(_Person));
	}
	_Person->Nutrition = NUTRITION_LOSS;
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
				--_Family->NumChildren;
				_Family->People[i] = _Family->People[CHILDREN + _Family->NumChildren];
				_Family->People[CHILDREN + _Family->NumChildren] = NULL;
			}
			break;
		}
	DestroyPerson(_Person);
	//Event_Push(CreateEventDeath(_Person));
}

int PersonWorkMult(struct Person* _Person) {
	return (_Person->Nutrition / 15);
}
