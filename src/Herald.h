/*
 * File: Herald.h
 * Author: David Brotz
 */

#ifndef __HERALD_H
#define __HERALD_H

#include "sys/ATimer.h"
#include "sys/RBTree.h"
#include "sys/HashTable.h"

#define TO_YEARS(__Months) (__Months * 12)
#define TO_MONTHS(__Days) ((int)(_Days / 30))
#define TO_DAYS(__Months) (__Months * 30)
#define MANORSZ_MIN (50)
#define MANORSZ_INTRVL (50)
#define MANORSZ_MAX (800)
#define LISTTOHASH(__List, __Itr, __Hash, __Key)			\
	(__Itr) = (__List)->Front;								\
	while((__Itr) != NULL) {								\
		HashInsert((__Hash), (__Key), (__Itr)->Data);		\
		(__Itr) = (__Itr)->Next;							\
	}
#define WORKMULT (1000)

typedef struct lua_State lua_State;

enum {
	BABY = 0,
	CHILD,
	TEENAGER,
	ADULT,
	SENIOR
};

struct InputReq {
	void* Req;
	int Quantity;
};

struct Object {
	int Id;
};

extern struct HashTable g_Crops;
extern struct HashTable g_Goods;
extern struct HashTable g_Buildings;
extern struct HashTable g_Occupations;
extern struct HashTable g_Populations;
extern struct ATimer g_ATimer;
extern struct RBTree g_Families;
extern struct RBTree* g_GoodDeps;

extern struct Constraint** g_AgeConstraints;
extern struct Constraint** g_AgeGroups;
//TODO: g_FamilySize and g_AgeAvg are only used for generation of manor's and should only be exposed to the function that does this.
extern struct Constraint** g_FamilySize;//Average number of families sharing the same last name.
extern struct Constraint** g_AgeAvg;//Distribution of ages.
//! Percentages of how the probability of the number of children a family will have 3.125 will have 0 babies, etc.
extern struct Constraint** g_BabyAvg;
extern struct Constraint** g_ManorSize;

extern struct LinkedList* g_ManorList;

int IdISCallback(const int* _One, const int* _Two);

void HeraldInit();
void HeraldDestroy();
struct InputReq* CreateInputReq();
void DestroyInputReq(struct InputReq* _Mat);
int NextId();

struct Array* FileLoad(const char* _File, char _Delimiter);

#endif
