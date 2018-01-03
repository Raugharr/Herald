/*
 * File: Herald.h
 * Author: David Brotz
 */

#ifndef __HERALD_H
#define __HERALD_H

#include "Good.h"

#include "sys/RBTree.h"
#include "sys/HashTable.h"
#include "sys/LinkedList.h"
#include "sys/FrameAllocator.h"

#include <math.h>

#define ACRE_WIDTH (66)
#define ACRE_LENGTH (660)
#define ACRE_SQFEET (43560)
#define MILE_FEET (5280)
#define LISTTOHASH(List, Itr, Hash, Key)				\
	(Itr) = (List)->Front;								\
	while((Itr) != NULL) {									\
		HashInsert((Hash), (Key), (Itr)->Data);			\
		(Itr) = (Itr)->Next;								\
	}
#define PowerSet(Array, Count) PowerSet_Aux(Array, Count, 0, NULL, NULL)
#define DATAFLD "data/"
#define Distance(XOne, YOne, XTwo, YTwo) ((int)(sqrt((pow(((YOne) - (YTwo)), 2) + pow((XOne) - (XTwo), 2)))))
#define FAMILYSIZE (5)
#define FindPopulation(Str) HashSearch(&g_Populations, (Str))
#define ObjectAlive(Obj) (((Obj)->Flags & OBJFLAG_DEAD) == 0)

typedef struct lua_State lua_State;
typedef struct SDL_Color SDL_Color;
struct StackNode;
struct LinkedList;
struct TaskPool;
struct Behavior;

extern const char* g_ShortMonths[];
extern struct Constraint** g_OpinionMods;
//Use SAlloc and SFree instead of using this directly.
extern struct LifoAllocator g_StackAllocator;
/*
 * TODO: Object should be given its own file as well as DATE.
 */

enum {
	BABY = 0,
	CHILD,
	TEENAGER,
	ADULT,
	SENIOR
};

enum {
	OBJECT_PERSON,
	OBJECT_CROP,
	OBJECT_ARMY,
	OBJECT_BATTLE,
	OBJECT_PREGANCY,
	OBJECT_LOCATION,
	OBJECT_BIGGUY,
	OBJECT_FAMILY,
	OBJECT_GOVERNMENT,
	OBJECT_RETINUE,
	OBJECT_SIZE
};

enum {
	OBJFLAG_DEAD = (1 << 0),
	OBJFLAG_SIZE
};

struct InputReq {
	const void* Req;
	double Quantity;
};

struct System {
	const char* Name;
	int(*Init)(void);
	void(*Quit)(void);
};

struct Object;
typedef void (*ObjectThink)(struct Object*);
typedef uint32_t ObjectId;
typedef uint8_t ObjectType;
typedef uint8_t ObjectFlags;
/*
 * TODO: Have objects think in order of their type.
 */
struct Object {
	const ObjectId Id;
	ObjectType Type;
	ObjectFlags Flags;
	IMPLICIT_LINKEDLIST(struct Object);
};

extern struct LinkedList g_GoodCats[GOOD_SIZE];
extern struct HashTable g_Crops;
extern struct HashTable g_Goods;
extern struct HashTable g_Profs;
extern struct HashTable g_BuildMats;
extern struct HashTable g_Populations;
extern struct HashTable g_Animations;
extern struct HashTable g_Traits;
extern struct HashTable g_Professions;
extern struct TaskPool* g_TaskPool;
extern struct MissionEngine g_MissionEngine;


extern struct Constraint** g_AgeConstraints;
//TODO: g_FamilySize and g_AgeAvg are only used for generation of manor's and should only be exposed to the function that does this.
extern struct Constraint** g_FamilySize;//Average number of families sharing the same last name.

int IdISCallback(const int* One, const int* Two);

#define StackTrace() SigSev(0)
void SigSev(int Sig);
int HeraldInit(void);
void HeraldDestroy(void);
void ClearObjects();
struct InputReq* CreateInputReq();
void DestroyInputReq(struct InputReq* Mat);
int InputReqQtyCmp(const void* One, const void* Two);
int InputReqCropCmp(const void* One, const void* Two);
int ObjectCmp(const void* One, const void* Two);

struct Array* FileLoad(const char* File, char Delimiter);
struct Array* ListToArray(const struct LinkedList* List);
void* PowerSet_Aux(void* Tbl, int Size, int ArraySize, uint32_t* BrSz, struct StackNode* Stack);

void* CreateObject(uint8_t Type);
void DestroyObject(struct Object* Obj);
void ObjectsThink();
int NextId();

void BehaviorRun(const struct Behavior* Tree, struct Family* Family); 
static inline void* SAlloc(size_t SizeOf) {
	return FrameAlloc(SizeOf);
}

static inline void SFree(size_t SizeOf) {
	FrameReduce(SizeOf);
}

struct Object* FindObject(ObjectId Id);
void ObjectDie(struct Object* Obj);
#endif
