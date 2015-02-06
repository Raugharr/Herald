/*
 * Author: David Brotz
 * File: Person.h
 */

#ifndef __PERSON_H
#define __PERSON_H

#include "Herald.h"

#include "World.h"

#define PERSON_CLOTHMAX (16)
#define EMALE (1)
#define EFEMALE (2)
#define AVKID (3)
#define MAX_NUTRITION (5000)
#define IsMarried(__Person) (__Person->Family->Wife != NULL)
#define PersonMature(__Person) (TO_YEARS(__Person->Age) > 13)
#define PersonDead(__Person) (__Person->Nutrition == 0)
#define NUTRITION_REQ (3000)
#define NUTRITON_CHLDREQ (NUTRITION_REQ / 2)
#define ADULT_AGE (15 * YEAR_DAYS)

struct HashTable;
struct Object;

extern struct MemoryPool* g_PersonPool;
extern struct Person* g_PersonList;

enum {
	EBODYPOS_NONE,
	EBODYPOS_FRONT,
	EBODYPOS_BACK
};

enum {
	EBODY_HEAD,
	EBODY_NECK,
	EBODY_RWRIST,
	EBODY_LWRIST,
	EBODY_RHAND,
	EBODY_LHAND,
	EBODY_RANKLE,
	EBODY_LANKLE,
	EBODY_RFOOT,
	EBODY_LFOOT,
	EBODY_NOPOS = EBODY_LFOOT,
	EBODY_UCHEST,
	EBODY_LCHEST,
	EBODY_URARM,
	EBODY_ULARM,
	EBODY_LRARM,
	EBODY_LLARM,
	EBODY_PELVIS,
	EBODY_URLEG,
	EBODY_ULLEG,
	EBODY_LRLEG,
	EBODY_LLLEG,
	EBODY_BUCHEST,
	EBODY_BLCHEST,
	EBODY_BURARM,
	EBODY_BULARM,
	EBODY_BLRARM,
	EBODY_BLLARM,
	EBODY_BPELVIS,
	EBODY_BURLEG,
	EBODY_BULLEG,
	EBODY_BLRLEG,
	EBODY_BLLLEG,
	EBODY_SIZE
};

struct Person {
	int Id;
	int X;
	int Y;
	int(*Think)(struct Object*);
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
	struct Good* Clothing[PERSON_CLOTHMAX];
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
int PersonThink(struct Person* _Person);
void PersonWork(struct Person* _Person);
void PersonDeath(struct Person* _Person);
/*
 * _Locations should be the size of g_PersonBodyStr.
 */
void BodyStrToBody(const char* _BodyStr, int* _Locations);
void WearClothing(struct Person* _Person, struct Good* _Clothing);
/*
 * int* _Body should be 4 32 integers that are adjacent to each other.
 * There are 32 unique body locations each uses 4 bits to decide the status
 * it has. The body locations are ordered by their position in the
 * enumeration. For example the first 4 bits of the first integer would
 * contain the status of EBODY_HEAD, EBODY_LLANKLE would contain the first 
 * 4 bits of the second integer.
 */

/*
 * Returns the life left in a specific body location.
 * _Location should be a EBODY_*.
 * _Position should be a EBODYPOS_*.
 */
int BodyLocation(int* _Body, int _Location, int _Position);
/*
 * Sets the life in a specific body location.
 * _Location should be a EBODY_*.
 * _Position should be a EBODYPOS_*.
 */
void SetBodyLocationStatus(int* _Body, int _Location, int _Position, int _Health);
#endif

