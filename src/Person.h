/*
 * Author: David Brotz
 * File: Person.h
 */

#ifndef __PERSON_H
#define __PERSON_H

#include "Herald.h"

#include "World.h"
#include "Actor.h"

#define EMALE (1)
#define EFEMALE (2)
#define AVKID (3) //Average kids.
#define IsMarried(__Person) (__Person->Family->Wife != NULL)
#define PersonMature(__Person) (TO_YEARS(__Person->Age) > 13)
#define PersonDead(__Person) (__Person->Nutrition == 0)
#define MAX_NUTRITION (5000)
#define NUTRITION_REQ (3000)
#define NUTRITON_CHLDREQ (NUTRITION_REQ / 2)
#define NUTRITION_DAILY (3000 / YEAR_DAYS)
#define NUTRITION_CHILDDAILY (NUTRITION_DAILY / 2);
#define ADULT_AGE (15 * YEAR_DAYS)

struct HashTable;
struct Object;
struct Settlement;
struct Food;

extern struct MemoryPool* g_PersonPool;

enum {
	CASTE_SERF,
	CASTE_FARMER,
	CASTE_CRAFTER,
	CASTE_WARRIOR
};

struct Person {
	int Id;
	int Type;
	void (*Think)(struct Object*);
	int LastThink; //In game ticks.
	struct LnkLst_Node* ThinkObj;
	SDL_Point Pos;
	int Gender;
	int Nutrition;
	int Caste;
	DATE Age;
	struct ActorJob* Action;
	const char* Name;
	struct Family* Family;
	struct Person* Next;
	struct Person* Prev;
	struct Behavior* Behavior;
	//FIXME: Should be placed in a different spot most people will no be pregnant.
	struct Pregnancy* Pregnancy;
};

struct Pregnancy {
	int Id;
	int Type;
	void (*Think)(struct Object*);
	int LastThink; //In game ticks.
	struct LnkLst_Node* ThinkObj;
	struct Person* Mother;
	int TTP;//Time to pregancy
};

struct Pregnancy* CreatePregnancy(struct Person* _Person);
void DestroyPregnancy(struct Pregnancy* _Pregnancy);
void PregnancyThink(struct Pregnancy* _Pregnancy);

struct Person* CreatePerson(const char* _Name, int _Age, int _Gender, int _Nutrition, int _X, int _Y, struct Family* _Family);
//FIXME: DestroyFamily should be called by DestroyPerson if _Person is the last Person in its family.
void DestroyPerson(struct Person* _Person);
struct Person* CreateChild(struct Family* _Family);
int PersonThink(struct Person* _Person);
int PersonEat(struct Person* _Person, struct Food* _Food);
void PersonWork(struct Person* _Person);
void PersonDeath(struct Person* _Person);

int PersonIsWarrior(const struct Person* _Person);
struct Person* GetFather(struct Person* _Person);
#endif

