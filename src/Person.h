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

#define IsMarried(_Person) (_Person->Family->Wife != NULL)
#define PersonDead(_Person) (_Person->Nutrition == 0)
#define MAX_WORKRATE (100)
#define NUTRITION_REQ (2920)
#define NUTRITION_MAX (112)
#define NUTRITION_DIV (14)
#define DAYSWORK (100)
#define ADULT_AGE (13)
#define NUTRITION_CHILDREQ (NUTRITION_REQ / 2)
#define NUTRITION_DAILY (NUTRITION_REQ / YEAR_DAYS)
#define NUTRITION_CHILDDAILY (NUTRITION_DAILY / 2)
#define PERSON_CASTE(Person) ((Person)->Family->Caste)
#define PersonGetGovernment(Person) ((Person)->Family->HomeLoc->Government)
#define IsPregnant(Person) (((Person)->Flags & PREGNANT) == PREGNANT)
#define IsAlive(Person) (((Person)->Flags & DEAD) != DEAD)
#define PREG_MINDAY (38 * 7)
#define PREG_MAXDAY (42 * 7)

struct HashTable;
struct Object;
struct Settlement;
struct Food;

extern struct MemoryPool* g_PersonPool;

enum PersonFlags {
	MALE = (1 << 0),
	FEMALE = (1 << 1),
	PRISONER = (1 << 2),
	PREGNANT = (1 << 3),
	DEAD = (1 << 4),
	BIGGUY = (1 << 5)
};

enum {
	DEATH_ILLNESS,
	DEATH_DUEL,
	DEATH_BATTLE
};

struct Person {
	struct Object Object;
	//FIXME: Should be replaced with an integer that indexes a name that is stored in an array.
	const char* Name;
	struct Family* Family;
	//FIXME: Should be placed in a different spot most people will no be pregnant.
	struct Person* Next;
	struct Person* Prev;
	uint32_t Location; //Id of Object that this person is located at. If they are at home this should be equal to the Id of the person's home settlement.
	int16_t Nutrition;
	uint8_t NutRate;
	uint8_t Flags;
	struct {
		uint8_t Years;
		uint8_t Months;
	} Age;
};

struct Pregnancy {
	struct Person* Mother;
	IMPLICIT_LINKEDLIST(struct Pregnancy);
	struct PregElem* Front;
	//uint16_t Table; //Index this pregnancy is in for World.Pregnancies.Table.
// use as value in IntRBuint32_t TTP;//Time to pregancy
};

struct Pregnancy* CreatePregnancy(struct Person* Person, uint16_t BirthDay, struct GameWorld* World);
void DestroyPregnancy(struct Pregnancy* Pregnancy);
struct Person* BirthChild(struct Pregnancy* Pregnancy);

struct Person* CreatePerson(const char* Name, DATE Age, int Gender, int Nutrition, int X, int Y, struct Family* Family);
//FIXME: DestroyFamily should be called by DestroyPerson if Person is the last Person in its family.
void DestroyPerson(struct Person* Person);
struct Person* CreateChild(struct Family* Family);
void PersonThink(struct Object* Obj);
void PersonObjThink(struct Object* Obj);
void PersonMarry(struct Person* Father, struct Person* Mother, struct Family* Family);
double PersonEat(struct Person* Person, struct Food* Food);
void PersonDeath(struct Person* Person);
/**
 *\precondition Every Person in List must be from the same settlement.
 */
void PersonDeathArr(struct Person** List, uint32_t Size);

struct Retinue* PersonRetinue(const struct Person* Person);
static inline struct Settlement* PersonHome(const struct Person* Person) {
	return Person->Family->HomeLoc;
}
void PersonGetPos(const struct Person* Person, uint32_t* x, uint32_t* y);

/**
 * Returns 1 if Person is a grown male who's family owns a weapon.
 * Otherwise returns 0.
 */
int PersonIsWarrior(const struct Person* Person);
struct Person* GetFather(struct Person* Person);
static inline bool IsChild(const struct Person* Person) {return Person->Age.Years < ADULT_AGE;}
static inline bool PersonMature(const struct Person* Person) {return Person->Age.Years >= ADULT_AGE;}
static inline uint8_t Gender(const struct Person* Person) {return Person->Flags & (MALE | FEMALE);}
static inline void Prisoner(struct Person* Person, bool Prisoner) {Person->Flags = (Person->Flags | (Prisoner << 1));}
static inline bool IsPrisoner(struct Person* Person) {return (Person->Flags & PRISONER);}
uint16_t PersonWorkMult(const struct Person* Person);
#endif

