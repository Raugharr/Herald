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
#define IsMarried(__Person) (__Person->Family->Wife != NULL)
#define PersonMature(_Person) (YEAR(_Person->Age) > 13)
#define PersonDead(__Person) (__Person->Nutrition == 0)
#define MAX_NUTRITION (5000)
#define NUTRITION_REQ (3000)
#define NUTRITON_CHLDREQ (NUTRITION_REQ / 2)
#define NUTRITION_DAILY (3000 / YEAR_DAYS)
#define NUTRITION_CHILDDAILY (NUTRITION_DAILY / 2);
#define ADULT_AGE (15 << YEAR_BITSHIFT)
#define PERSON_CASTE(_Person) ((_Person)->Family->Caste->Type)

struct HashTable;
struct Object;
struct Settlement;
struct Food;

extern struct MemoryPool* g_PersonPool;

struct Person {
	int Id;
	int Type;
	ObjectThink Think;
	int LastThink; //In game ticks.
	struct LnkLst_Node* ThinkObj;
	SDL_Point Pos;
	int Gender;
	int Nutrition;
//	int Caste;
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
void PersonMarry(struct Person* _Father, struct Person* _Mother, struct Family* _Family);
double PersonEat(struct Person* _Person, struct Food* _Food);
void PersonDeath(struct Person* _Person);

/**
 * Returns 1 if _Person is a grown male who's family owns a weapon.
 * Otherwise returns 0.
 */
int PersonIsWarrior(const struct Person* _Person);
struct Person* GetFather(struct Person* _Person);
#endif

