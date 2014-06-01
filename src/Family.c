/*
 * Author: David Brotz
 * File: Family.c
 */

#include "Family.h"

#include "Person.h"
#include "Herald.h"
#include "sys/Constraint.h"
#include "sys/Array.h"
#include "sys/Random.h"

#include <stdlib.h>
#include <assert.h>

struct Constraint** g_AgeDistr = NULL;
static struct Array* g_FirstNames = NULL;

void Family_Init(struct Array* _Array) {
	g_AgeDistr = CreateConstrntBnds(19, 0, 737, 1464, 2157, 2867, 3632, 4489, 5368, 6162, 6870, 7472, 7885, 8317, 8744, 9150, 9471, 9717, 9875, 9999);
	g_FirstNames = _Array;
}

void Family_Quit() {
	DestroyConstrntBnds(g_AgeDistr);
	DestroyArray(g_FirstNames);
}

struct Family* CreateFamily(const char* _Name, struct Person* _Husband, struct Person* _Wife, struct Person** _Children, int _ChildrenSize) {
	struct Family* _Family = (struct Family*) malloc(sizeof(struct Family));
	int i;

	if(_ChildrenSize >= CHILDREN_SIZE) {
		DestroyFamily(_Family);
		return NULL;
	}

	_Family->Name = _Name;
	_Family->People[HUSBAND] = _Husband;
	_Husband->Family = _Family;
	_Family->People[WIFE] = _Wife;
	_Wife->Family = _Family;//If _Female is last of its family the family's name will be a memory leak.
	_Family->People = (struct Person**) malloc(sizeof(struct Person) * (CHILDREN_SIZE + 2));
	SetArray((void**)_Family->People[0] + (sizeof(struct Person) * 2), CHILDREN_SIZE, NULL);
	_Family->NumChildren = 0;
	for(i = 0; i < _ChildrenSize; ++i)
		_Family->People[CHILDREN + i] = _Children[i];
	return _Family;
}

struct Family* CreateRandFamily(const char* _Name, int _Size) {
	struct Family* _Family = NULL;

	if(_Size >= 2) {
		struct Person* _Husband = CreatePerson(g_FirstNames->Table[Random(0, g_FirstNames->Size)], Random(g_AgeGroups[TEENAGER]->Min, g_AgeGroups[ADULT]->Max), EMALE, 100);
		struct Person* _Wife = CreatePerson(g_FirstNames->Table[Random(0, g_FirstNames->Size)], Random(g_AgeGroups[TEENAGER]->Min, g_AgeGroups[ADULT]->Max), EFEMALE, 100);
		Hash_Find(&g_Occupations, "Farmer", _Husband->Occupation);
		_Family = CreateFamily(_Name, _Husband, _Wife, NULL, 0);
		_Size -= 2;

		while(_Size-- > 0)
			_Family->People[CHILDREN + _Family->NumChildren++] = CreatePerson(g_FirstNames->Table[Random(0, g_FirstNames->Size)], Fuzify(g_AgeDistr, Random(0, 9999)), Random(1, 2), 100);
	} else
		return NULL;
	return _Family;
}

void DestroyFamily(struct Family* _Family) {\
	int _Max = _Family->NumChildren + 2;
	while(_Max > 0) {
		DestroyPerson(_Family->People[_Max]);
		--_Max;
	}
	free(_Family);
}

int Family_Size(struct Family* _Family) {
	int _Size = 0;

	if(_Family->People[HUSBAND] != NULL)
		++_Size;
	if(_Family->People[WIFE] != NULL)
		++_Size;
	_Size += _Family->NumChildren;
	return _Size;
}

void Marry(struct Person* _Male, struct Person* _Female) {
	assert(_Male->Gender == EMALE && _Female->Gender == EFEMALE);
	CreateFamily(_Male->Family->Name, _Male, _Female, NULL, 0);
}

int Family_Work(const struct Family* _Family) {
	int _Total = _Family->People[HUSBAND]->Nutrition;
	int i;

	for(i = 0; i < _Family->NumChildren; ++i) {
		_Total += _Family->People[CHILDREN + i]->Nutrition;
	}
}

void Family_Update(struct Family* _Family) {
	int i = _Family->NumChildren + 2;

	while(i > 0) {
		if(_Family->People[i] == NULL)
			continue;
		Person_Update(_Family->People[i], 1500);
	}
}
