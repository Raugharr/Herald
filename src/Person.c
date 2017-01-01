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

struct Pregnancy* CreatePregnancy(struct Person* Person) {
	struct Pregnancy* Pregancy = (struct Pregnancy*) malloc(sizeof(struct Pregnancy));

	CreateObject(&Pregancy->Object, OBJECT_PREGANCY, (void(*)(struct Object*))PregnancyThink);
	Pregancy->TTP = TO_DAYS(BIRTH_TIME) - 15 + Random(0, 29) + 1;
	Pregancy->Mother = Person;
	return Pregancy;
};

void DestroyPregnancy(struct Pregnancy* Pregnancy) {
	Pregnancy->Mother->Pregnancy = NULL;
	DestroyObject((struct Object*)Pregnancy);
	free(Pregnancy);
}

void PregnancyThink(struct Pregnancy* Pregnancy) {
	struct Person* Child = NULL;

	if(--Pregnancy->TTP <= 0) {
		if(Pregnancy->Mother->Family->NumChildren >= CHILDREN_SIZE) {
			DestroyPregnancy(Pregnancy);
			return;
		}
		Child = CreateChild(Pregnancy->Mother->Family);
		Pregnancy->Mother->Family->People[2 + Pregnancy->Mother->Family->NumChildren++] = Child;
		++Pregnancy->Mother->Family->HomeLoc->YearBirths;
		PushEvent(EVENT_BIRTH, Pregnancy->Mother, Child);
		DestroyPregnancy(Pregnancy);
	}
}

struct Person* CreatePerson(const char* Name, int Age, int Gender, int Nutrition, int X, int Y, struct Family* Family) {
	struct Person* Person = NULL;
	if(Name == NULL || Age < 0 || (Gender != EMALE && Gender != EFEMALE) || Family == NULL || X < 0 || Y < 0) {
		Log(ELOG_WARNING, "Cannot create person, invalid attributes.");
		return NULL;
	}

	Person = (struct Person*) MemPoolAlloc(g_PersonPool);
	CreateObject(&Person->Object, OBJECT_PERSON, PersonThink);
	Person->Name = Name;
	Person->Age = Age;
	Person->Gender = Gender;
	Person->Nutrition = Nutrition;
	Person->Family = Family;
	Person->Pregnancy = NULL;

	SettlementAddPerson(Person->Family->HomeLoc, Person);
	return Person;
}

void DestroyPerson(struct Person* Person) {
	//struct Family* Family = Person->Family;

	DestroyObject(&Person->Object);
	SettlementRemovePerson(Person->Family->HomeLoc, Person);
	if(FamilySize(Person->Family) == 0)
		Person->Family->IsAlive = false;
	//	DestroyFamily(Family);
	MemPoolFree(g_PersonPool, Person);
}

struct Person* CreateChild(struct Family* Family) {
	struct Person* Mother = Family->People[WIFE];
	struct Person* Child = CreatePerson("Foo", 0, Random(1, 2), Mother->Nutrition, Mother->Pos.x, Mother->Pos.y, Mother->Family);
	
	Child->Family = Family;
	++Family->HomeLoc->YearBirths;
	return Child;
}

void PersonThink(struct Object* Obj) {
	struct Person* Person = (struct Person*) Obj;

	if(Person->Gender == EFEMALE) {
		if(Person->Family != NULL
				&& Person->Pregnancy == NULL
				&& Person == Person->Family->People[WIFE]
				&& Person->Family->NumChildren < CHILDREN_SIZE
				&& Random(0, 1499) < 1) {
			Person->Pregnancy = CreatePregnancy(Person);
		}
	} 	
	Person->Nutrition -= (PersonMature(Person) == true) ? (NUTRITION_DAILY) : (NUTRITION_CHILDDAILY);
	if(Person->Nutrition < 0)
		Person->Nutrition = 0;
	NextDay(&Person->Age);
	if(Person->Nutrition > MAX_NUTRITION)
		Person->Nutrition = MAX_NUTRITION;
}

void PersonMarry(struct Person* Father, struct Person* Mother, struct Family* Family) {
	struct Family* NewFam = CreateFamily(Family->Name, FamilyGetSettlement(Family), Family);

	NewFam->People[HUSBAND] = Father;
	NewFam->People[WIFE] = Mother;
	Father->Family = NewFam;
	Mother->Family = NewFam;
	NewFam->Fields[NewFam->FieldCt++] = CreateField(NULL, 30, NewFam);
}

void PersonDeath(struct Person* Person) {
	struct Family* Family = Person->Family;

	if(Person->Pregnancy != NULL)
		DestroyPregnancy(Person->Pregnancy);
	for(int i = 0; i < Family->NumChildren + CHILDREN; ++i) {
		if(Family->People[i] == Person) {
			Family->People[i] = NULL;
			if(i >= CHILDREN) {
				--Family->NumChildren;
				Family->People[i] = Family->People[CHILDREN + Family->NumChildren];
				Family->People[CHILDREN + Family->NumChildren] = NULL;
			}
			break;
		}
	}
	PushEvent(EVENT_DEATH, Person, NULL);
	++Family->HomeLoc->YearDeaths;
	if(PersonMature(Person) == 1) {
		if(Person->Gender == EMALE)
			--Family->HomeLoc->AdultMen;
		else
			--Family->HomeLoc->AdultWomen;
	}
}

int PersonIsWarrior(const struct Person* Person) {
	if(Person->Gender != EMALE || YEAR(Person->Age) < 15)
		return 0;
	return 1;
}

struct Person* GetFather(struct Person* Person) {
	struct Family* Family = Person->Family;

	if(Family->People[HUSBAND] != Person)
		return Family->People[HUSBAND];
	if(Family->Parent != NULL)
		return Family->Parent->People[HUSBAND];
	return NULL;
}

uint16_t PersonWorkMult(const struct Person* Person) {
	double Modifier = Person->Nutrition / ((double) MAX_NUTRITION);
	int WorkRate = MAX_WORKRATE;

	if(YEAR(Person->Age) < ADULT_AGE) {
		WorkRate = MAX_WORKRATE / 2;
	}
	return (Modifier >= 1.0f) ? (WorkRate) : (WorkRate * Modifier);
}
