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
#define IsMarried(_Person) (_Person->Family->Wife != NULL)
#define PersonDead(_Person) (_Person->Nutrition == 0)
#define MAX_NUTRITION (250)
#define MAX_WORKRATE (100)
#define NUTRITION_REQ (2920)
#define DAYSWORK (100)
#define ADULT_AGE (13)
#define NUTRITION_CHILDREQ (NUTRITION_REQ / 2)
#define NUTRITION_DAILY (NUTRITION_REQ / YEAR_DAYS)
#define NUTRITION_CHILDDAILY (NUTRITION_DAILY / 2)
#define PERSON_CASTE(Person) ((Person)->Family->Caste)
#define PersonGetGovernment(Person) ((Person)->Family->HomeLoc->Government)

struct HashTable;
struct Object;
struct Settlement;
struct Food;

extern struct MemoryPool* g_PersonPool;

struct Person {
struct Object Object;
SDL_Point Pos;
DATE Age;
const char* Name;
struct Family* Family;
struct Person* Next;
struct Person* Prev;
//FIXME: Should be placed in a different spot most people will no be pregnant.
struct Pregnancy* Pregnancy;
int16_t Nutrition;
uint8_t Gender;
};

struct Pregnancy {
struct Object Object;
struct Person* Mother;
uint32_t TTP;//Time to pregancy
};

struct Pregnancy* CreatePregnancy(struct Person* Person);
void DestroyPregnancy(struct Pregnancy* Pregnancy);
void PregnancyThink(struct Pregnancy* Pregnancy);

struct Person* CreatePerson(const char* Name, int Age, int Gender, int Nutrition, int X, int Y, struct Family* Family);
//FIXME: DestroyFamily should be called by DestroyPerson if Person is the last Person in its family.
void DestroyPerson(struct Person* Person);
struct Person* CreateChild(struct Family* Family);
void PersonThink(struct Object* Obj);
void PersonMarry(struct Person* Father, struct Person* Mother, struct Family* Family);
double PersonEat(struct Person* Person, struct Food* Food);
void PersonDeath(struct Person* Person);
struct Retinue* PersonRetinue(const struct Person* Person);
static inline struct Settlement* PersonHome(const struct Person* Person) {
return Person->Family->HomeLoc;
}

/**
 * Returns 1 if Person is a grown male who's family owns a weapon.
 * Otherwise returns 0.
 */
int PersonIsWarrior(const struct Person* Person);
struct Person* GetFather(struct Person* Person);
static inline bool IsChild(const struct Person* Person) {
return (YEAR(Person->Age) < ADULT_AGE);
}

static inline bool PersonMature(const struct Person* Person) {
return YEAR(Person->Age) >= ADULT_AGE;
}
uint16_t PersonWorkMult(const struct Person* Person);
#endif

