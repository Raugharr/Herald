/*
 * Author: David Brotz
 * File: Person.c
 */

#include "Person.h"

#include "Herald.h"
#include "Family.h"
#include "Crop.h"
#include "Good.h"
#include "sys/LinkedList.h"
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
#include <assert.h>

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

	_Person = (struct Person*) MemPool_Alloc(g_PersonPool);
	_Person->Name = _Name;
	CreateObject((struct Object*)_Person, _X, _Y, (int(*)(struct Object*))PersonThink);
	_Person->Age = _Age;
	_Person->Gender = _Gender;
	_Person->Nutrition = _Nutrition;
	_Person->Family = NULL;
	_Person->Parent = NULL;
	_Person->Occupation = NULL;

	ILL_CREATE(g_PersonList, _Person);
	_Person->Behavior = NULL;
	g_PersonList = _Person;
	return _Person;
}

void DestroyPerson(struct Person* _Person) {
	ILL_DESTROY(g_PersonList, _Person);
	MemPool_Free(g_PersonPool, _Person);
}

struct Person* CreateChild(struct Family* _Family) {
	struct Person* _Mother = _Family->People[WIFE];
	struct Person* _Child = CreatePerson("Foo", 0, Random(1, 2), _Mother->Nutrition, _Mother->X, _Mother->Y);
	
	_Child->Family = _Family;
	return _Child;
}

int PersonThink(struct Person* _Person) {
	if(_Person->Gender == EFEMALE) {
		if(_Person->Family != NULL
				&& ATimerSearch(&g_ATimer, (struct Object*)_Person, ATT_PREGANCY) == NULL
				&& _Person == _Person->Family->People[WIFE]
				&& _Person->Family->NumChildren < CHILDREN_SIZE
				&& Random(0, 999) < 20)
			ATimerInsert(&g_ATimer, CreatePregancy(_Person));
	}
	_Person->Nutrition -= NUTRITION_LOSS;
	/*if(Random(0, 999) < (MAX_NUTRITION - _Person->Nutrition) / 500) {
		++g_Deaths;
		PersonDeath(_Person);
		return 1;
	}*/
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
	EventPush(CreateEventDeath(_Person));
}

int PersonWorkMult(struct Person* _Person) {
	return (_Person->Nutrition / 15);
}

void BodyStrToBody(const char* _BodyStr, int* _Locations) {
	if(strcmp(_BodyStr, "Head") == 0)
		_Locations[EBODY_HEAD] = 1;
	else if(strcmp(_BodyStr, "Neck") == 0)
		_Locations[EBODY_NECK] = 1;
	else if(strcmp(_BodyStr, "Upper Chest") == 0) {
		_Locations[EBODY_UCHEST] = 1;
		_Locations[EBODY_BUCHEST] = 1;
	} else if(strcmp(_BodyStr, "Upper Arms") == 0) {
		_Locations[EBODY_URARM] = 1;
		_Locations[EBODY_BURARM] = 1;
		_Locations[EBODY_ULARM] = 1;
		_Locations[EBODY_BULARM] = 1;
	} else if(strcmp(_BodyStr, "Lower Arms") == 0) {
		_Locations[EBODY_LRARM] = 1;
		_Locations[EBODY_BLRARM] = 1;
		_Locations[EBODY_LLARM] = 1;
		_Locations[EBODY_BLLARM] = 1;
	} else if(strcmp(_BodyStr, "Wrists") == 0) {
		_Locations[EBODY_RWRIST] = 1;
		_Locations[EBODY_LWRIST] = 1;
	} else if(strcmp(_BodyStr, "Hands") == 0) {
		_Locations[EBODY_RHAND] = 1;
		_Locations[EBODY_LHAND] = 1;
	} else if(strcmp(_BodyStr, "Lower Chest") == 0) {
		_Locations[EBODY_LCHEST] = 1;
		_Locations[EBODY_BLCHEST] = 1;
	} else if(strcmp(_BodyStr, "Pelvis") == 0) {
		_Locations[EBODY_PELVIS] = 1;
		_Locations[EBODY_BPELVIS] = 1;
	} else if(strcmp(_BodyStr, "Upper Legs") == 0) {
		_Locations[EBODY_URLEG] = 1;
		_Locations[EBODY_BURLEG] = 1;
		_Locations[EBODY_ULLEG] = 1;
		_Locations[EBODY_BULLEG] = 1;
	} else if(strcmp(_BodyStr, "Lower Legs") == 0) {
		_Locations[EBODY_LRLEG] = 1;
		_Locations[EBODY_BLRLEG] = 1;
		_Locations[EBODY_LLLEG] = 1;
		_Locations[EBODY_BLLLEG] = 1;
	} else if(strcmp(_BodyStr, "Ankles") == 0) {
		_Locations[EBODY_RANKLE] = 1;
		_Locations[EBODY_LANKLE] = 1;
	}  else if(strcmp(_BodyStr, "Feet") == 0) {
		_Locations[EBODY_RFOOT] = 1;
		_Locations[EBODY_LFOOT] = 1;
	}
}

void ClothingWear(struct Person* _Person, struct Good* _Clothing) {
	int i = 0;
	int j = 0;
	int _Parent = PERSON_CLOTHMAX + 1;
	struct Good* _ClothParent = NULL;

	/*
	 * Check if an open spot exists.
	 */
	for(i = 0; i < PERSON_CLOTHMAX; ++i)
		if(_Person->Clothing[i] == NULL)
			goto escape;
	return;
	escape:
	/*
	 * Find if there is a parent of this clothing.
	 */
	for(i = 0; i < PERSON_CLOTHMAX; ++i) {
		if(_Person->Clothing[i] == NULL)
			continue;
		for(j = 0; j < EBODY_SIZE; ++j)
			if(((struct ClothingBase*)_Clothing->Base)->Locations[j] != 0 && ((struct ClothingBase*)_Person->Clothing[i]->Base)->Locations[j] != 0) {
				_Parent = j;
				break;
			}
	}
	for(i = _Parent - 1; i > 0; --i)
		if(_Person->Clothing[i] == NULL) {
			_Person->Clothing[i] = _Clothing;
			goto end;
		}
	/*
	 * _Clothing could not be placed at a index less than _Parent's index, move things around to make room.
	 */
	_ClothParent = _Person->Clothing[_Parent];
	for(i = 1; i < PERSON_CLOTHMAX; ++i) {
		if(_Person->Clothing[i - 1] == NULL) {
			_Person->Clothing[i - 1] = _Person->Clothing[_Parent];
			_Person->Clothing[_Parent] = _Clothing;
			goto end;
		}
	}
	assert(_Person->Clothing[PERSON_CLOTHMAX - 2] == NULL);
	_Person->Clothing[PERSON_CLOTHMAX - 1] = _Clothing;
	_Person->Clothing[PERSON_CLOTHMAX - 2] = _ClothParent;
	end:
	ObjectRmPos((struct Object*)_Clothing);
}

void ClothingRemove(struct Person* _Person, struct Good* _Clothing) {
	int i = 0;
	
	for(i = 0; i < PERSON_CLOTHMAX; ++i)
		if(_Person->Clothing[i] != NULL && GoodCmp(_Clothing, _Person->Clothing[i]) == 0) {
			ObjectAddPos(_Person->X, _Person->Y, (struct Object*)_Clothing);
			_Person->Clothing[i] = NULL;
			return;	
		}
}

int BodyLocation(int* _Body, int _Location, int _Position) {
	int _Int = 0;

	if(_Position < EBODYPOS_NONE || _Position > EBODYPOS_BACK)
		return -1;
	if(_Location < EBODY_HEAD || _Location > EBODY_SIZE)
		return -1;
	if(_Location > EBODY_NOPOS && _Position == EBODYPOS_NONE) 
		return -1;
	_Int = _Location / 8;
	return _Body[_Int] & (0xF << (4 * (_Location - (_Int * 8)) - 4));
}

void SetBodyLoctionHealth(int* _Body, int _Location, int _Position, int _Health) {
	int _Int = 0;

	if(_Position < EBODYPOS_NONE || _Position > EBODYPOS_BACK)
		return;
	if(_Location < EBODY_HEAD || _Location > EBODY_SIZE)
		return;
	if(_Location > EBODY_NOPOS && _Position == EBODYPOS_NONE) 
		return;
	_Int = _Location / 8;
	_Body[_Int] = _Body[_Int] & ((_Health & 0xF) << (4 * (_Location - (_Int * 8)) - 4));
}
