/**
 * Author: David Brotz
 * File: Person.h
 */

#ifndef __PERSON_H
#define __PERSON_H

#include "Herald.h"

#include "Family.h"
#include "Location.h"
#include "World.h"

#include <inttypes.h>
#include <stdbool.h>

#define EMALE (1)
#define EFEMALE (2)
#define IsMarried(__Person) (__Person->Family->Wife != NULL)
#define PersonDead(__Person) (__Person->Nutrition == 0)
#define MAX_NUTRITION (250)
#define MAX_WORKRATE (100)
#define NUTRITION_REQ (2920)
#define DAYSWORK (100)
#define ADULT_AGE (13)
#define NUTRITION_CHILDREQ (NUTRITION_REQ / 2)
#define NUTRITION_DAILY (NUTRITION_REQ / YEAR_DAYS)
#define NUTRITION_CHILDDAILY (NUTRITION_DAILY / 2)
#define PERSON_CASTE(_Person) ((_Person)->Family->Caste->Type)
#define PersonGetGovernment(_Person) ((_Person)->Family->HomeLoc->Government)

struct HashTable;
struct Object;
struct Settlement;
struct Food;

extern struct MemoryPool* g_PersonPool;

struct Person {
	struct Object Object;
	SDL_Point Pos;
	int16_t Nutrition;
	DATE Age;
	const char* Name;
	struct Family* Family;
	struct Person* Next;
	struct Person* Prev;
	//FIXME: Should be placed in a different spot most people will no be pregnant.
	struct Pregnancy* Pregnancy;
	uint8_t Gender;
};

struct Pregnancy {
	struct Object Object;
	struct Person* Mother;
	uint32_t TTP;//Time to pregancy
};

struct Pregnancy* CreatePregnancy(struct Person* _Person);
void DestroyPregnancy(struct Pregnancy* _Pregnancy);
void PregnancyThink(struct Pregnancy* _Pregnancy);

struct Person* CreatePerson(const char* _Name, int _Age, int _Gender, int _Nutrition, int _X, int _Y, struct Family* _Family);
//FIXME: DestroyFamily should be called by DestroyPerson if _Person is the last Person in its family.
void DestroyPerson(struct Person* _Person);
struct Person* CreateChild(struct Family* _Family);
void PersonThink(struct Object* _Obj);
void PersonMarry(struct Person* _Father, struct Person* _Mother, struct Family* _Family);
double PersonEat(struct Person* _Person, struct Food* _Food);
void PersonDeath(struct Person* _Person);
struct Retinue* PersonRetinue(const struct Person* _Person);
static inline struct Settlement* PersonHome(const struct Person* _Person) {
	return _Person->Family->HomeLoc;
}

/**
 * Returns 1 if _Person is a grown male who's family owns a weapon.
 * Otherwise returns 0.
 */
int PersonIsWarrior(const struct Person* _Person);
struct Person* GetFather(struct Person* _Person);
static inline bool IsChild(const struct Person* _Person) {
	return (YEAR(_Person->Age) < ADULT_AGE);
}

static inline bool PersonMature(const struct Person* _Person) {
	return YEAR(_Person->Age) >= 15;
}
uint16_t PersonWorkMult(const struct Person* _Person);
#endif

