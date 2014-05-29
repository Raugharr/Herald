/*
 * Author: David Brotz
 * File: Family.c
 */

#include "Family.h"

#include "Person.h"
#include "Constraint.h"
#include "sys/Random.h"

#include <stdlib.h>
#include <assert.h>

struct Family* CreateFamily(const char* _Name, struct Person* _Husband, struct Person* _Wife, struct Person** _Children, int _ChildrenSize) {
	struct Family* _Family = (struct Family*) malloc(sizeof(struct Family));
	int i;

	if(_ChildrenSize >= CHILDREN_SIZE) {
		DestroyFamily(_Family);
		return NULL;
	}

	_Family->Name = _Name;
	_Family->Husband = _Husband;
	_Husband->Family = _Family;
	_Family->Wife = _Wife;
	_Wife->Family = _Family;//If _Female is last of its family the family's name will be a memory leak.
	_Family->Children = (struct Person**) malloc(sizeof(struct Person) * CHILDREN_SIZE);
	SetArray((void**)_Family->Children, CHILDREN_SIZE, NULL);
	_Family->NumChildren = 0;
	for(i = 0; i < _ChildrenSize; ++i)
		_Family->Children[i] = _Children[i];
	return _Family;
}

struct Family* CreateRandFamily(const char* _Name, int _Size) {
	struct Family* _Family = NULL;

	if(_Size >= 2) {
		struct Person* _Husband = CreatePerson("Foo", Random(g_AgeGroups[TEENAGER]->Min, g_AgeGroups[ADULT]->Max), EMALE, 100);
		struct Person* _Wife = CreatePerson("Foo", Random(g_AgeGroups[TEENAGER]->Min, g_AgeGroups[ADULT]->Max), EFEMALE, 100);
		_Family = CreateFamily(_Name, _Husband, _Wife, NULL, 0);
		_Size -= 2;

		while(_Size-- > 0)
			_Family->Children[_Family->NumChildren++] = CreatePerson("Foo", Fuzify(g_AgeDistr, Random(0, 9999)), Random(1, 2), 100);
	} else
		return NULL;
	return _Family;
}

void DestroyFamily(struct Family* _Family) {
	while(_Family->NumChildren > 0) {
		free(_Family->Children[_Family->NumChildren]);
		--_Family->NumChildren;
	}
	free(_Family->Children);
	free(_Family);
}

int Family_Size(struct Family* _Family) {
	int _Size = 0;

	if(_Family->Husband != NULL)
		++_Size;
	if(_Family->Wife != NULL)
		++_Size;
	_Size += _Family->NumChildren;
	return _Size;
}

//#define Marry(__Male, __Female) CreateFamily(__Male->Family->Name, __Male, __Female, NULL, 0);
void Marry(struct Person* _Male, struct Person* _Female) {
	assert(_Male->Gender == EMALE && _Female->Gender == EFEMALE);
	CreateFamily(_Male->Family->Name, _Male, _Female, NULL, 0);
}
