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
#define LISTTOHASH(_List, _Itr, _Hash, _Key)				\
	(_Itr) = (_List)->Front;								\
	while((_Itr) != NULL) {									\
		HashInsert((_Hash), (_Key), (_Itr)->Data);			\
		(_Itr) = (_Itr)->Next;								\
	}
#define PowerSet(_Array, _Count) PowerSet_Aux(_Array, _Count, 0, NULL)
#define DATAFLD "data/"
#define Distance(_XOne, _YOne, _XTwo, _YTwo) ((int)(sqrt((pow(((_YOne) - (_YTwo)), 2) + pow((_XOne) - (_XTwo), 2)))))
#define FAMILYSIZE (5)
#define FindPopulation(Str) HashSearch(&g_Populations, (Str))

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
	OBJECT_SIZE
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
/*
 * TODO: Have objects think in order of their type.
 */
struct Object {
	const uint32_t Id;
	IMPLICIT_LINKEDLIST(struct Object);
	uint8_t Type;
};

extern struct LinkedList g_GoodCats[GOOD_SIZE];
extern struct HashTable g_Crops;
extern struct HashTable g_Goods;
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

int IdISCallback(const int* _One, const int* _Two);

int HeraldInit(void);
void HeraldDestroy(void);
void ClearObjects();
struct InputReq* CreateInputReq();
void DestroyInputReq(struct InputReq* _Mat);
int InputReqQtyCmp(const void* _One, const void* _Two);
int InputReqCropCmp(const void* _One, const void* _Two);
int ObjectCmp(const void* _One, const void* _Two);

struct Array* FileLoad(const char* _File, char _Delimiter);
struct Array* ListToArray(const struct LinkedList* _List);
void* PowerSet_Aux(void* _Tbl, int _Size, int _ArraySize, struct StackNode* _Stack);

void CreateObject(struct Object* _Obj, uint8_t _Type);
void DestroyObject(struct Object* _Obj);
void ObjectsThink();
int NextId();

void BehaviorRun(const struct Behavior* _Tree, struct Family* _Family); 
static inline void* SAlloc(size_t _SizeOf) {
	return FrameAlloc(_SizeOf);
}

static inline void SFree(size_t _SizeOf) {
	FrameReduce(_SizeOf);
}

struct Object* FindObject(uint32_t Id);
#endif
