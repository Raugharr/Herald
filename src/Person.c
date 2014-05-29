/*
 * Author: David Brotz
 * File: Person.c
 */

#include "Person.h"

#include "Herald.h"
#include "Family.h"
#include "sys/Random.h"
#include "sys/LinkedList.h"

#include <stdlib.h>

#ifndef NULL
#define NULL (((void*)0))
#endif
#define BIRTH_TIME (10)

//struct Array g_BoyNames = LoadCSV("Boy.txt", &((char**)g_BoyNames.Table), &g_BoyNames.Size);
//struct Array g_GirlNames = LoadCSV("Girl.txt", (char***)g_GirlNames.Table, &g_GirlNames.Size);

struct Person* CreatePerson(const char* _Name, int _Age, int _Gender, int _Nutrition) {
	struct Person* _Person = (struct Person*) malloc(sizeof(struct Person));

	_Person->Name = _Name;
	_Person->Age = _Age;
	_Person->Gender = _Gender;
	_Person->Nutrition = _Nutrition;
	_Person->Family = NULL;
	_Person->Parent = NULL;
	_Person->Ocupation = NULL;
	return _Person;
}

struct Person* CreateChild(struct Family* _Family) {
	struct Person* _Child = (struct Person*) malloc(sizeof(struct Person));
	
	_Child->Gender = Random(1, 2);
	if(_Child->Gender == EMALE)
		_Child->Name = "Foo";
	else
		_Child->Name = "Foo";
	_Child->Age = 0;
	_Child->Nutrition = _Family->Wife->Nutrition;
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

void BirthCheck(struct Person* _Person) {
	if(_Person->Family->NumChildren < CHILDREN_SIZE && _Person->Gender == EFEMALE && Random(1, 100) > g_BabyAvg[_Person->Family->NumChildren] && Random(1, 100) > 10)
		CreatePregancy(_Person);
}

void Birth(struct Pregancy* _Pregancy) {
	if(--_Pregancy->TTP <= 0)
		_Pregancy->Mother->Family->Children[_Pregancy->Mother->Family->NumChildren++] = CreateChild(_Pregancy->Mother->Family);
}

void Person_DeleteNames() {
	/*
	 int i;

	for(i = 0; i < g_BoyNames.Size; ++i)
		free((char*)g_BoyNames.Table[i]);
	for(i = 0; i < g_GirlNames.Size; ++i)
		free((char*)g_GirlNames.Table[i]);
	free(g_GirlNames.Table);
	free(g_BoyNames.Table);
	*/
}
