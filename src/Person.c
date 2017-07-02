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

const uint8_t g_AdultRations[RATION_SIZE] = {
	NUTRITION_DAILY / 4,
	NUTRITION_DAILY / 2,
	(NUTRITION_DAILY / 4) * 3,
	NUTRITION_DAILY,
	NUTRITION_DAILY + (NUTRITION_DAILY / 4),
	NUTRITION_DAILY + (NUTRITION_DAILY / 2),
	NUTRITION_DAILY + ((NUTRITION_DAILY  / 4) * 3),
	NUTRITION_DAILY * 2
};

const uint8_t g_ChildRations[RATION_SIZE] = {
	NUTRITION_CHILDDAILY / 4,
	NUTRITION_CHILDDAILY / 2,
	(NUTRITION_CHILDDAILY / 4) * 3,
	NUTRITION_CHILDDAILY,
	NUTRITION_CHILDDAILY + (NUTRITION_CHILDDAILY / 4),
	NUTRITION_CHILDDAILY + (NUTRITION_CHILDDAILY / 2),
	NUTRITION_CHILDDAILY + ((NUTRITION_CHILDDAILY  / 4) * 3),
	NUTRITION_CHILDDAILY * 2
};

void PregOnDeath(const struct EventData* Data, void* Extra1, void* Extra2) {
	struct Pregnancy* Pregnancy = Data->One;
	
	Assert(Pregnancy->Mother == Data->OwnerObj);
	DestroyPregnancy(Pregnancy);
}

struct Pregnancy* CreatePregnancy(struct Person* Person, uint16_t BirthDay, struct GameWorld* World) {
	struct Pregnancy* Pregnancy = (struct Pregnancy*) malloc(sizeof(struct Pregnancy));
	uint16_t TableEnd = 0;

	Pregnancy->Mother = Person;
	Pregnancy->Next = NULL;
	Pregnancy->Prev = NULL;
	Person->Flags |= PREGNANT;
	//Set TableEnd to the last element in World->Pregnancies.
	TableEnd = World->Pregnancies.End;//& PREGTABLE_SZ;
	EventHook(EVENT_DEATH, PregOnDeath, Person, Pregnancy, NULL);
	//Look for the entry in World->Pregnancies that has a BirthDay equal to BirthDay. Each BirthDay in World->Pregnancies should be ordered in ascending order.
	while(TableEnd != World->Pregnancies.Start && World->Pregnancies.Table[TableEnd - 1]->BirthDay > BirthDay) {
		//If the below code is executed that must mean BirthDay is not the biggest BirthDay and does not exist.
		if(BirthDay < World->Pregnancies.Table[TableEnd - 1]->BirthDay) {
			World->Pregnancies.Table[World->Pregnancies.End] = &World->Pregnancies.AllocTable[(World->Pregnancies.AllocIdx++) & PREGTABLE_SZ];
			Pregnancy->Front = World->Pregnancies.Table[World->Pregnancies.End];
			World->Pregnancies.Table[World->Pregnancies.End]->Data = Pregnancy;
			World->Pregnancies.Table[World->Pregnancies.End]->BirthDay = BirthDay;//DateAddInt(World->Date, BirthDay);
			//ILL_CREATE(World->Pregnancies.Table[World->Pregnancies.End]->Data, Pregnancy);
			goto insert_entry;
		}
		TableEnd = (TableEnd - 1);
	}
	if(World->Pregnancies.End == TableEnd) {
		World->Pregnancies.Table[TableEnd] = &World->Pregnancies.AllocTable[(World->Pregnancies.AllocIdx++) & PREGTABLE_SZ];
		World->Pregnancies.Table[TableEnd]->Data = Pregnancy;
		World->Pregnancies.Table[TableEnd]->BirthDay = BirthDay;
		++World->Pregnancies.End;
		if(World->Pregnancies.End > PREGTABLE_SZ) World->Pregnancies.End = 0;
	} else {
		ILL_CREATE(World->Pregnancies.Table[TableEnd]->Data, Pregnancy);
	}
	Pregnancy->Front = World->Pregnancies.Table[TableEnd];
	Assert(Pregnancy->Front != NULL);
	return Pregnancy;
	insert_entry:
	//Move everything from EndTable to End one space over and fill in the gap with BirthDay and Pregnancy.	
	--TableEnd;
	for(uint16_t Idx = World->Pregnancies.End - 1;; Idx = (Idx - 1)) {
		uint16_t NextIdx = Idx + 1;
		struct PregElem* Temp = World->Pregnancies.Table[NextIdx];

		World->Pregnancies.Table[NextIdx] = World->Pregnancies.Table[Idx];
		World->Pregnancies.Table[Idx] = Temp;
		if(Idx == TableEnd)
			break;
	}
	World->Pregnancies.End = World->Pregnancies.End + 1;
	if(World->Pregnancies.End > PREGTABLE_SZ) World->Pregnancies.End = 0;
	Assert(Pregnancy->Front != NULL);
	return Pregnancy;
}

void DestroyPregnancy(struct Pregnancy* Pregnancy) {
	Pregnancy->Mother->Flags = (Pregnancy->Mother->Flags & (~PREGNANT));
	EventHookRemove(EVENT_DEATH, Pregnancy->Mother, Pregnancy, NULL);
	ILL_DESTROY(Pregnancy->Front->Data, Pregnancy);
	free(Pregnancy);
}

struct Person* BirthChild(struct Pregnancy* Pregnancy) {
	struct Person* Child = NULL;

	//if(Pregnancy->Mother->Family->NumChildren >= CHILDREN_SIZE) {
	//	DestroyPregnancy(Pregnancy);
	//	return NULL;
	//}
	Child = CreateChild(Pregnancy->Mother->Family);
	//Pregnancy->Mother->Family->People[2 + Pregnancy->Mother->Family->NumChildren++] = Child;
	//++Pregnancy->Mother->Family->HomeLoc->YearBirths;
	PushEvent(EVENT_BIRTH, Pregnancy->Mother, Child);
	DestroyPregnancy(Pregnancy);
	return Child;
}

struct Person* CreatePerson(const char* Name, int Age, int Gender, int Nutrition, struct Family* Family) {
	struct Person* Person = NULL;

	Assert(Name != NULL && Age >= 0 && (Gender == MALE || Gender == FEMALE) && Family != NULL);

	Person = (struct Person*) MemPoolAlloc(g_PersonPool);
	CreateObject(&Person->Object, OBJECT_PERSON);
	Person->Name = Name;
	Person->Age.Years = Age;
	Person->Age.Months = 0;
	Person->Flags = (MALE & Gender);
	Person->Nutrition = Nutrition;
	Person->Family = Family;
	Person->Location = Family->HomeLoc->Object.Id;
	Person->NutRate = PersonRation(Person);
	//Person->NutRate = (PersonMature(Person) == true) ? (Person->Nutrition / NUTRITION_DIV) : (Person->Nutrition / (NUTRITION_DIV * 2));
	SettlementAddPerson(Person->Family->HomeLoc, Person);
	return Person;
}

void DestroyPerson(struct Person* Person) {
	struct Family* Family = Person->Family;

	FamilyRemovePerson(Person->Family, Person);
	//for(int i = 0; i < FAMILY_PEOPLESZ; ++i) {
	//	if(Family->People[i] != NULL) goto end;
	//}
	//DestroyFamily(Family);
	//end:
	MemPoolFree(g_PersonPool, Person);
}

struct Person* CreateChild(struct Family* Family) {
	struct Person* Mother = Family->People[WIFE];
	struct Person* Child = NULL;
	
	Child = CreatePerson("Foo", 0, ((RandByte() & 0x80) != 0) + 1, Mother->Nutrition, Mother->Family);
	Child->Location = Mother->Location;
	++Family->HomeLoc->YearBirths;
	++Family->NumChildren;
	return Child;
}

void PersonThink(struct Object* Obj) {
	struct Person* Person = (struct Person*) Obj;

//	if(Rand() < 631737810745000) {
//		PersonDeath(Person);
//		return;
//	}
	Person->Nutrition -= Person->NutRate;
	if(Person->Nutrition > NUTRITION_MAX)
		Person->Nutrition = NUTRITION_MAX;
	if(Person->Nutrition < 0) {
		Person->Nutrition = 0;
		PersonDeath(Person);
	}
}

void PersonObjThink(struct Object* Obj) {
	if(DAY(g_GameWorld.Date) == 0) {
		for(struct Person* Person = (struct Person*)Obj; Person != NULL; Person = (struct Person*) Person->Object.Next) {
			++Person->Age.Months;
			if(Person->Age.Months >= MONTHS) {
				++Person->Age.Years;
				//FIXME: This shouldn't be placed here because it will be computed on every tick.
				//Instead it should be placed somewhere that will only be evaulated for people who are not adults.
				if(Person->Age.Years == ADULT_AGE) {
					if(Gender(Person) == MALE) {
						 ArrayInsert_S(&Person->Family->HomeLoc->Suitors, Person);
						++Person->Family->HomeLoc->AdultMen;
					} else {
						ArrayInsert_S(&Person->Family->HomeLoc->Brides, Person);
						++Person->Family->HomeLoc->AdultWomen;
					}
				}
				Person->Age.Months = 0;
			}
		}
	}
}

void PersonDeath(struct Person* Person) {
	struct Family* Family = Person->Family;
	struct BigGuy* Guy = NULL;

	++Person->Family->HomeLoc->YearDeaths;
	if((Guy = RBSearch(&g_GameWorld.BigGuys, Person)) != NULL)
		BigGuyDeath(Guy);
	SettlementRemovePerson(Person->Family->HomeLoc, Person);
	Person->Flags |= DEAD;
	if(FamilySize(Family) == 0)
		Family->IsAlive = false;
	DestroyObject(&Person->Object);
	ILL_CREATE(g_GameWorld.DeadPeople, &Person->Object);
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

		if((Guy = RBSearch(&g_GameWorld.BigGuys, List[i])) != NULL)
			BigGuyDeath(Guy);
		SettlementRemovePerson(List[i]->Family->HomeLoc, List[i]);
		List[i]->Flags |= DEAD;
		if(FamilySize(Family) == 0)
			Family->IsAlive = false;
		DestroyObject(&List[i]->Object);
		ILL_CREATE(g_GameWorld.DeadPeople, &List[i]->Object);
	}
	Settlement->YearDeaths += Size;
}

int PersonIsWarrior(const struct Person* Person) {
	if(Gender(Person) != MALE || !PersonMature(Person))
		return 0;
	return 1;
}

void PersonGetPos(const struct Person* Person, uint32_t* x, uint32_t* y) {
	struct Object* Object = FindObject(Person->Location);

	Assert(Object != NULL);
	*x = Person->Family->HomeLoc->Pos.x;
	*y = Person->Family->HomeLoc->Pos.y;
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
//	float Modifier = Person->Nutrition / ((float) NUTRITION_MAX);
	uint32_t Modifier = (Person->Nutrition * 100) / NUTRITION_MAX;
	int WorkRate = MAX_WORKRATE;

	if(Person->Age.Years < ADULT_AGE) {
		WorkRate = MAX_WORKRATE / 2;
	}
	//return (Modifier >= 1.0f) ? (WorkRate) : (WorkRate * Modifier);
	return (Modifier >= 100) ? (WorkRate) : ((WorkRate * Modifier) / MAX_WORKRATE);
}

uint16_t PersonRation(const struct Person* Person) {
		return (PersonMature(Person) == true) ? (g_AdultRations[Person->Family->Rations]) : (g_ChildRations[Person->Family->Rations]);
}
