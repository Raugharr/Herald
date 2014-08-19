/*
 * File: Herald.h
 * Author: David Brotz
 */

#ifndef __HERALD_H
#define __HERALD_H

#include "sys/ATimer.h"
#include "sys/RBTree.h"
#include "sys/HashTable.h"

#define TO_YEARS(_Months) ((_Months) * 12)
#define TO_MONTHS(_Days) ((int)((_Days) / 30))
#define TO_DAYS(_Months) ((_Months) * 30)
#define LISTTOHASH(_List, _Itr, _Hash, _Key)				\
	(_Itr) = (_List)->Front;								\
	while((_Itr) != NULL) {									\
		HashInsert((_Hash), (_Key), (_Itr)->Data);			\
		(_Itr) = (_Itr)->Next;								\
	}
#define WORKMULT (1000)
#define PowerSet(_Array, _Count) PowerSet_Aux(_Array, _Count, 0, NULL)

typedef struct lua_State lua_State;
struct StackNode;
struct LinkedList;

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
	int X;
	int Y;
};

extern struct HashTable g_Crops;
extern struct HashTable g_Goods;
extern struct HashTable g_BuiltMats;
extern struct HashTable g_Occupations;
extern struct HashTable g_Populations;
extern struct ATimer g_ATimer;
extern int g_ObjPosBal;

extern struct Constraint** g_AgeConstraints;
//TODO: g_FamilySize and g_AgeAvg are only used for generation of manor's and should only be exposed to the function that does this.
extern struct Constraint** g_FamilySize;//Average number of families sharing the same last name.

int IdISCallback(const int* _One, const int* _Two);

void HeraldInit();
void HeraldDestroy();
struct InputReq* CreateInputReq();
void DestroyInputReq(struct InputReq* _Mat);
int NextId();

struct Array* FileLoad(const char* _File, char _Delimiter);
struct Array* ListToArray(const struct LinkedList* _List);
void* PowerSet_Aux(void* _Tbl, int _Size, int _ArraySize, struct StackNode* _Stack);
void CreateObject(struct Object* _Obj, int _X, int _Y);

#endif
