/*
 * File: Herald.c
 * Author: David Brotz
 */

#include "Herald.h"

#include "Person.h"
#include "Family.h"
#include "Crop.h"
#include "Building.h"
#include "Government.h"
#include "BigGuy.h"
#include "World.h"
#include "Good.h"
#include "Population.h"
#include "Mission.h"
#include "Warband.h"

#include "AI/BehaviorTree.h"
#include "AI/Pathfind.h"

#include "sys/Stack.h"
#include "sys/RBTree.h"
#include "sys/Math.h"
#include "sys/LinkedList.h"
#include "sys/TaskPool.h"
#include "sys/MemoryPool.h"
#include "sys/Array.h"
#include "sys/LuaCore.h"
#include "sys/Constraint.h"
#include "sys/Log.h"
#include "sys/Rule.h"
#include "sys/Event.h"
#include "sys/GenIterator.h"

#include "video/Animation.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <execinfo.h>
#include <lua/lua.h>
#include <lua/lauxlib.h>

#define AGEDIST_SIZE (17)
#define STACKALLOC_SZ (2048)

struct LinkedList g_GoodCats[GOOD_SIZE];
struct HashTable g_Crops;
struct HashTable g_Goods;
struct HashTable g_BuildMats;
struct HashTable g_Populations;
struct HashTable g_Animations;
struct HashTable g_Traits;
struct HashTable g_Professions;
struct HashTable g_BhvVars;
struct Constraint** g_FamilySize;
struct Constraint** g_AgeConstraints;
struct LifoAllocator g_StackAllocator = {0};

struct ObjectList {
	struct RBTree SearchTree;
	struct {
		struct Object* Front;
		ObjectThink Think;
	} ObjectList[OBJECT_SIZE];
};

struct MissionEngine g_MissionEngine;

//TODO: SearchTree does nothing but be inserted to and removed from. ObjectList should be removed and have a LinkedList for storing thinks replace it.`
static struct ObjectList g_Objects = {
	{NULL, 0, (int(*)(const void*, const void*))ObjectCmp, (int(*)(const void*, const void*))ObjectCmp},
	{{NULL, PersonObjThink},
	{NULL, AnimalObjThink},
	{NULL, NULL},
	{NULL, NULL},
	{NULL, NULL},
	{NULL, NULL},
	{NULL, NULL},
	{NULL, NULL},
	{NULL, FamilyObjThink},
	{NULL, NULL}
	}
};

static ObjectThink g_ObjectThinks[OBJECT_SIZE] = {
	PersonThink,
	AnimalThink,
	(ObjectThink)FieldUpdate,
	(ObjectThink)ArmyThink, 
	NULL,
	NULL,
	(ObjectThink)SettlementThink,
	(ObjectThink)BigGuyThink,
	FamilyThink,
	(ObjectThink)GovernmentThink
};

static uint32_t g_Id = 0;
struct Constraint** g_OpinionMods = NULL;

int IdISCallback(const int* _One, const int* _Two) {
	return *(_One) - *(_Two);
}

void SigSev(int _Sig) {
	void* _Array[16];
	size_t _Size = 0;
	char** _Symbols = NULL;

	_Size = backtrace(_Array, 16);
	_Symbols = backtrace_symbols(_Array, _Size);
	for(int i = 2; i < _Size; ++i) {
		Log(ELOG_ERROR, _Symbols[i]);
		//free(_Symbols[i]);
	}
	free(_Symbols);
}

int HeraldInit() {
	signal(SIGSEGV, SigSev);
	/*FIXME: Is g_Crops, g_GOods, g_BuildMats, and g_Populations not free their memory when HeraldDestroy is called or at al?
	 */
	g_Crops.TblSize = 0;
	g_Crops.Table = NULL;
	g_Crops.Size = 0;

	g_Goods.TblSize = 0;
	g_Goods.Table = NULL;
	g_Goods.Size = 0;

	g_BuildMats.TblSize = 0;
	g_BuildMats.Table = NULL;
	g_BuildMats.Size = 0;

	g_Populations.TblSize = 0;
	g_Populations.Table = NULL;
	g_Populations.Size = 0;

	g_Animations.TblSize = 0;
	g_Animations.Table = NULL;
	g_Animations.Size = 0;

	g_BhvVars.TblSize = 0;
	g_BhvVars.Table = calloc(sizeof(void*), 64);
	g_BhvVars.Size = 64;

	g_TaskPool = CreateTaskPool();

	g_FamilySize = CreateConstrntBnds(FAMILYSIZE, 2, 10, 20, 40, 75, 100);
	g_AgeConstraints = CreateConstrntLst(NULL, 0, 1068, 60);
	g_OpinionMods = CreateConstrntBnds(5, -REL_MAX, -76, -26, 25, 75, REL_MAX);
	EventInit();
	PathfindInit();
	MathInit();

	ConstructMissionEngine(&g_MissionEngine);

	g_StackAllocator.ArenaSize = STACKALLOC_SZ;
	g_StackAllocator.ArenaBot = malloc(g_StackAllocator.ArenaSize);
	g_StackAllocator.ArenaTop = g_StackAllocator.ArenaBot;
	return 1;
}

void HeraldDestroy() {
	struct HashItr* _Itr = HashCreateItr(&g_Animations);

	while(_Itr != NULL) {
		DestroyAnimation(_Itr->Node->Pair);
		_Itr = HashNext(&g_Animations, _Itr);
	}
	HashDeleteItr(_Itr);
	free(g_BhvVars.Table);
	DestroyTaskPool(g_TaskPool);
	DestroyConstrntBnds(g_FamilySize);
	DestroyConstrntBnds(g_AgeConstraints);
	DestroyConstrntBnds(g_OpinionMods);
	EventQuit();
	PathfindQuit();
	DestroyMissionEngine(&g_MissionEngine);
	free(g_StackAllocator.ArenaBot);
}

void ClearObjects() {
	for(int i = 0; i < OBJECT_SIZE; ++i) {
		while(g_Objects.ObjectList[i].Front != NULL) 
			DestroyObject(g_Objects.ObjectList[i].Front);
	}
}

struct InputReq* CreateInputReq() {
	struct InputReq* _Mat = (struct InputReq*) malloc(sizeof(struct InputReq));

	_Mat->Req = NULL;
	_Mat->Quantity = 0;
	return _Mat;
}

void DestroyInputReq(struct InputReq* _Mat) {
	free(_Mat);
}

int InputReqQtyCmp(const void* _One, const void* _Two) {
	return (int) ((struct InputReq*)_One)->Quantity - ((struct InputReq*)_Two)->Quantity;
}

int InputReqCropCmp(const void* _One, const void* _Two) {
	return (int) ((struct Crop*)((struct InputReq*)_One)->Req)->Id - ((struct Crop*)((struct InputReq*)_Two)->Req)->Id;
}

struct Array* FileLoad(const char* _File, char _Delimiter) {
	int _Pos = 0;
	int _Size = 1;
	char* _Name = NULL;
	char _Char = 0;
	char _Buffer[256];
	FILE* _FilePtr = fopen(_File, "r");
	struct Array* _Array = NULL;

	if(_FilePtr == NULL)
		return NULL;

	while((_Char = fgetc(_FilePtr)) != EOF) {
		if(_Char == _Delimiter)
			++_Size;
	}
	rewind(_FilePtr);
	_Array = CreateArray(_Size);
	while((_Char = fgetc(_FilePtr)) != EOF) {
			if(_Char == _Delimiter && _Pos > 0) {
				_Buffer[_Pos] = 0;
				_Name = (char*) malloc(sizeof(char) * _Pos + 1);
				_Name[0] = 0;
				strncat(_Name, _Buffer, _Pos);
				ArrayInsert(_Array, _Name);
				_Pos = 0;
			} else {
				if(_Pos >= 256)
					return _Array;
				_Buffer[_Pos++] = _Char;
			}
		}
	return _Array;
}

struct Array* ListToArray(const struct LinkedList* _List) {
	struct Array* _Array = NULL;
	struct LnkLst_Node* _Itr = _List->Front;

	if(_List->Size < 1)
		return NULL;
	_Array = CreateArray(_List->Size);
	while(_Itr != NULL) {
		ArrayInsert(_Array, _Itr->Data);
		_Itr = _Itr->Next;
	}
	return _Array;
}

//FIXME: We should be able to easily calculate the size of the outgoing table, there should be no need to allocate memory here.
void* PowerSet_Aux(void* _Tbl, int _Size, int _ArraySize, struct StackNode* _Stack) {
	struct StackNode _Node;
	void** _Return = NULL;
	int i;

	if(_Size == 0) {
		//No Elements exist in Tbl return set containing NULL.
		if(_Stack == NULL) {
			_Return = malloc(sizeof(void*));
			*((int**)_Return) = 0;
			goto end;
		}
		_Return = calloc(_ArraySize + 1, sizeof(void*));
		for(i = _ArraySize - 1; i >= 0; --i) {
			_Return[i] = _Stack->Data;
			_Stack = _Stack->Prev;
		}
		_Return[_ArraySize] = NULL;
	} else {
		int _Len = 0;
		void** _Left = NULL;
		void** _Right = NULL;

		_Node.Prev = _Stack;
		_Node.Data = (void*)*(int*)_Tbl;
		_Left = PowerSet_Aux(_Tbl + sizeof(void*), _Size - 1, _ArraySize, _Stack);
		_Right = PowerSet_Aux(_Tbl + sizeof(void*), _Size - 1, _ArraySize + 1, &_Node);

		if(_Size == 1) {
			_Return = calloc(3, sizeof(void*));
			_Return[0] = _Left;
			_Return[1] = _Right;
			_Return[2] = NULL;
			goto end;
		}
		_Len = ArrayLen(_Left);
		_Return = calloc(_Len * 2 + 1, sizeof(void*));
		for(i = 0; i < _Len; ++i) {
			_Return[i] = _Left[i];
		}
		for(i = 0; i < _Len; ++i)
			_Return[i + _Len] = _Right[i];
		_Return[_Len * 2] = NULL;
	}
	end:
	return _Return;
}

/* Use a single array to store the power set.
 * To distinguish the sets and to allow them to be easy to use add an extra _Size to the front of the table such that
 * given {a, b, c} we return the array 
 * {addr(8), addr(9), addr(11), addr(13), addr(15), addr(18), addr(21), addr(24) 
 	NULL,
	a, NULL,
	b, NULL,
	c NULL,
	a, b, NULL,
	a c, NULL,
	b c, NULL,
	a, b, c, NULL}
	where addr(x) is the address of the xth elemenet of the array.
*/
/*void* PowerSet_Aux(void* _Tbl, int _Size, int _ArraySize, struct StackNode* _Stack) {
	void* _Array = calloc(_Size << 1, sizeof(void*));
	uint64_t _Mask = 1;
	uint32_t _Set = 1;

	_Array[0] = NULL;
	while(_Mask < (_Size << 1)) {
		for(int i = 0; i < Set; ++i) {
			if(((i << 0) & _Set) != (i << 0))
				continue;
		}
	}
}*/

void CreateObject(struct Object* _Obj, uint8_t _Type) {
	*(uint32_t*)&_Obj->Id = NextId();
	_Obj->Type = _Type;
	RBInsert(&g_Objects.SearchTree, _Obj);
	Assert(RBSearch(&g_Objects.SearchTree, _Obj));
	ILL_CREATE(g_Objects.ObjectList[_Type].Front, _Obj);
}

void DestroyObject(struct Object* _Obj) {
	RBDelete(&g_Objects.SearchTree, _Obj);
	ILL_DESTROY(g_Objects.ObjectList[_Obj->Type].Front, _Obj);
}

void ObjectsThink() {
	for(int i = 0; i < OBJECT_SIZE; ++i) {
		if(g_Objects.ObjectList[i].Think != NULL) {
			g_Objects.ObjectList[i].Think(g_Objects.ObjectList[i].Front);
		}
		struct Object* _Obj = g_Objects.ObjectList[i].Front;
		struct Object* _Next = NULL;

		while(_Obj != NULL) {
			_Next = _Obj->Next;
			g_ObjectThinks[i](_Obj);
			_Obj = _Next;
		}
	}
}

int NextId() {return g_Id++;}

void BhvHashFree(char* _Str) {
	free(_Str);
}

void BehaviorRun(const struct Behavior* _Tree, struct Family* _Family) {
	HashDeleteAll(&g_BhvVars, (void(*)(void*)) BhvHashFree);
	BHVRun(_Tree, _Family, &g_BhvVars);
}

int ObjectCmp(const void* _One, const void* _Two) {
	return *((int*)_One) - *((int*)_Two);
}

struct Object* FindObject(uint32_t Id) {
	return RBSearch(&g_Objects.SearchTree, &Id);
}
