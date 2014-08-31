/*
 * Author: David Brotz
 * File: Person.h
 */

#ifndef __PERSON_H
#define __PERSON_H

#include "Herald.h"

#include "World.h"

#define EMALE (1)
#define EFEMALE (2)
#define AVKID (3)
#define MAX_NUTRITION (4000)
#define IsMarried(__Person) (__Person->Family->Wife != NULL)
#define PersonMature(__Person) (TO_YEARS(__Person->Age) > 13)
#define PersonDead(__Person) (__Person->Nutrition == 0)
#define NUTRITION_LOSS (16)

struct HashTable;

extern struct MemoryPool* g_PersonPool;
extern struct Person* g_PersonList;

struct Person {
	int Id;
	int X;
	int Y;
	int Gender;
	int Nutrition;
	DATE Age;
	const char* Name;
	struct Family* Family;
	struct Family* Parent;
	struct Person* Next;
	struct Person* Prev;
	struct Occupation* Occupation;
	struct Behavior* Behavior;
};

struct Pregancy {
	void* Prev;
	void* Next;
	int Type;
	struct Person* Mother;
	int TTP;//Time to pregancy
};

struct Pregancy* CreatePregancy(struct Person* _Person);
void DestroyPregancy(struct Pregancy* _Pregancy);
int PregancyUpdate(struct Pregancy* _Pregancy);

struct Person* CreatePerson(const char* _Name, int _Age, int _Gender, int _Nutrition, int _X, int _Y);
void DestroyPerson(struct Person* _Person);
struct Person* CreateChild(struct Family* _Family);
int PersonUpdate(struct Person* _Person, struct HashTable* _Table);
void PersonWork(struct Person* _Person);
void PersonDeath(struct Person* _Person);
int PersonWorkMult(struct Person* _Person);
#endif

