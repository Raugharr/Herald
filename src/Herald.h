/*
 * File: Herald.h
 * Author: David Brotz
 */

#ifndef __HERALD_H
#define __HERALD_H

#include "sys/RBTree.h"
#include "sys/HashTable.h"
#include "sys/LinkedList.h"

#include <math.h>

#define ACRE_WIDTH (66)
#define ACRE_LENGTH (660)
#define ACRE_SQFEET (43560)
#define MILE_FEET (5280)
#define YEAR_DAYS (365)
#define MONTH_DAYS (30)
#define TO_YEARS(_Months) ((_Months) * 12)
#define TO_MONTHS(_Days) ((int)((_Days) / 30))
#define TO_DAYS(_Months) ((_Months) * 30)
#define YEAR(__Date) ((__Date) >> 9)
#define MONTH(__Date) (((__Date) >> 5) & 15)
#define MONTHS (12)
#define DAY(__Date) (__Date & 31)
#define TO_DATE(__Year, __Month, __Day) (DAY(__Day) | (_Month << 5) | (_Year << 9))
#define LISTTOHASH(_List, _Itr, _Hash, _Key)				\
	(_Itr) = (_List)->Front;								\
	while((_Itr) != NULL) {									\
		HashInsert((_Hash), (_Key), (_Itr)->Data);			\
		(_Itr) = (_Itr)->Next;								\
	}
#define WORKMULT (1000)
#define PowerSet(_Array, _Count) PowerSet_Aux(_Array, _Count, 0, NULL)
#define DATAFLD "data/"
#define DATE int
#define Distance(_XOne, _YOne, _XTwo, _YTwo) ((int)(sqrt((pow(((_YOne) - (_YTwo)), 2) + pow((_XOne) - (_XTwo), 2)))))
#define FAMILYSIZE (5)

typedef struct lua_State lua_State;
typedef struct SDL_Color SDL_Color;
struct StackNode;
struct LinkedList;
struct TaskPool;

extern const char* g_ShortMonths[];
extern struct Constraint** g_OpinionMods;

enum {
	BABY = 0,
	CHILD,
	TEENAGER,
	ADULT,
	SENIOR
};

enum {
	OBJECT_GOOD,
	OBJECT_PERSON,
	OBJECT_ANIMAL,
	OBJECT_BUILDING,
	OBJECT_CROP,
	OBJECT_ACTOR,
	OBJECT_ARMY,
	OBJECT_BATTLE,
	OBJECT_PREGANCY,
	OBJECT_CONSTRUCT,
	OBJECT_LOCATION,
	OBJECT_BIGGUY
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

/*
 * TODO: Have objects think in order of their type.
 */
struct Object {
	int Id;
	int Type;
	void (*Think)(struct Object*);
	int LastThink; //In game ticks.
	struct LnkLst_Node* ThinkObj;
};

extern struct HashTable g_Crops;
extern struct HashTable g_Goods;
extern struct HashTable g_BuildMats;
extern struct HashTable g_Populations;
extern struct RBTree g_MissionTree;
extern struct RBTree g_MissionList;
extern struct TaskPool* g_TaskPool;
extern int g_ObjPosBal;


extern struct Constraint** g_AgeConstraints;
//TODO: g_FamilySize and g_AgeAvg are only used for generation of manor's and should only be exposed to the function that does this.
extern struct Constraint** g_FamilySize;//Average number of families sharing the same last name.

int IdISCallback(const int* _One, const int* _Two);

int HeraldInit(void);
void HeraldDestroy(void);
struct InputReq* CreateInputReq();
void DestroyInputReq(struct InputReq* _Mat);
int InputReqQtyCmp(const void* _One, const void* _Two);
int InputReqCropCmp(const void* _One, const void* _Two);
int ObjectCmp(const void* _One, const void* _Two);

struct Array* FileLoad(const char* _File, char _Delimiter);
struct Array* ListToArray(const struct LinkedList* _List);
void* PowerSet_Aux(void* _Tbl, int _Size, int _ArraySize, struct StackNode* _Stack);

void CreateObject(struct Object* _Obj, int _Type, void (*_Think)(struct Object*));
void DestroyObject(struct Object* _Object);
void ObjectsThink();
int NextId();

DATE MonthToInt(const char* _Month);
DATE DaysBetween(int _DateOne, int _DateTwo);
DATE DateToDays(int _Date);
DATE DaysToDate(int _Days);
int IsNewMonth(int _Date);
void NextDay(int* _Date);

void* DownscaleImage(void* _Image, int _Width, int _Height, int _ScaleArea);
void NewZoneColor(SDL_Color* _Color);

#endif
