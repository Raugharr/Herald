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

	//CreateObject(&Pregancy->Object, OBJECT_PREGANCY);
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

	Assert(Name != NULL && Age >= 0 && (Gender == MALE || Gender == FEMALE) && Family != NULL && X >= 0 && Y >= 0);

	Person = (struct Person*) MemPoolAlloc(g_PersonPool);
	CreateObject(&Person->Object, OBJECT_PERSON);
	Person->Name = Name;
	Person->Age = Age;
	Person->Flags = (MALE & Gender);
	Person->Nutrition = Nutrition;
	Person->Family = Family;
	Person->Pregnancy = NULL;

	SettlementAddPerson(Person->Family->HomeLoc, Person);
	return Person;
}

void DestroyPerson(struct Person* Person) {
	//struct Family* Family = Person->Family;

	DestroyObject(&Person->Object);
	if(FamilySize(Person->Family) == 0)
		Person->Family->IsAlive = false;
	FamilyRemovePerson(Person->Family, Person);
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

	if(Gender(Person) == FEMALE) {
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
	//Removed as PersonMarry code hasnt been touched in a year or so and Families are no longer all farmers(2/2/2017).
	//NewFam->Fields[NewFam->FieldCt++] = CreateField(NULL, 30, NewFam);
}

void PersonDeath(struct Person* Person) {
	struct Family* Family = Person->Family;
	struct BigGuy* Guy = NULL;

	if(Person->Pregnancy != NULL)
		DestroyPregnancy(Person->Pregnancy);
	++Family->HomeLoc->YearDeaths;
	SettlementRemovePerson(Person->Family->HomeLoc, Person);
	if((Guy = RBSearch(&g_GameWorld.BigGuys, Person)) != NULL)
		BigGuyDeath(Guy);
	DestroyPerson(Person);
	FamilyRemovePerson(Family, Person);
	PushEvent(EVENT_DEATH, Person, NULL);
}

/**
 *\precondition Every Person in List must be from the same settlement.
 */
void PersonDeathArr(struct Person** List, uint32_t Size) {
	struct BigGuy* Guy = NULL;

	Assert(Size > 0);
	Assert(List != NULL);

	struct Settlement* Settlement = List[0]->Family->HomeLoc;

	for(int i = 0; i < Size; ++i) {
		struct Family* Family = List[i]->Family;

		if(List[i]->Pregnancy != NULL)
			DestroyPregnancy(List[i]->Pregnancy);
		SettlementRemovePerson(Settlement, List[i]);
		if((Guy = RBSearch(&g_GameWorld.BigGuys, List[i])) != NULL)
			BigGuyDeath(Guy);
		DestroyPerson(List[i]);
		FamilyRemovePerson(Family, List[i]);
		PushEvent(EVENT_DEATH, List[i], NULL);
	}
	Settlement->YearDeaths += Size;
}

int PersonIsWarrior(const struct Person* Person) {
	if(Gender(Person) != MALE || !PersonMature(Person))
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
