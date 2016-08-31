/*
 * File: Herald.c
 * Author: David Brotz
 */

#include "Herald.h"

#include "Person.h"
#include "Family.h"
#include "Crop.h"
#include "Building.h"
#include "BigGuy.h"
#include "World.h"
#include "Good.h"
#include "Population.h"
#include "Mission.h"

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
#include "sys/StackAllocator.h"

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
struct LifoAllocator g_StackAllocator;

struct ObjectList {
	struct RBTree SearchTree;
	struct LinkedList ThinkList;
};

struct MissionEngine g_MissionEngine;

//TODO: SearchTree does nothing but be inserted to and removed from. ObjectList should be removed and have a LinkedList for storing thinks replace it.`
static struct ObjectList g_Objects = {
	{NULL, 0, (int(*)(const void*, const void*))ObjectCmp, (int(*)(const void*, const void*))ObjectCmp},
	{0, NULL, NULL}
};

int g_Id = 0;
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
	for(int i = 0; i < _Size; ++i) {
		Log(ELOG_ERROR, _Symbols[i]);
		free(_Symbols[i]);
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
	g_OpinionMods = CreateConstrntBnds(5, -BIGGUY_RELMAX, -76, -26, 25, 75, BIGGUY_RELMAX);
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

void* PowerSet_Aux(void* _Tbl, int _Size, int _ArraySize, struct StackNode* _Stack) {
	struct StackNode _Node;
	void** _Return = NULL;
	int i;

	if(_Size == 0) {
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

void CreateObject(struct Object* _Obj, int _Type, ObjectThink _Think) {
	_Obj->Id = NextId();
	_Obj->Type = _Type;
	_Obj->Think = _Think;
	_Obj->LastThink = 1;
	RBInsert(&g_Objects.SearchTree, _Obj);
	if(_Think != NULL) {
		LnkLstPushBack(&g_Objects.ThinkList, _Obj);
		_Obj->ThinkObj = g_Objects.ThinkList.Back;
	}
}

void DestroyObject(struct Object* _Object) {
	RBDelete(&g_Objects.SearchTree, _Object);
	if(_Object->Think != NULL)
		LnkLstRemove(&g_Objects.ThinkList, _Object->ThinkObj);
}

void ObjectsThink() {
	struct LnkLst_Node* _Itr = g_Objects.ThinkList.Front;
	struct LnkLst_Node* _Next = NULL;
	struct Object* _Object = NULL;

	while(_Itr != NULL) {
		_Object = ((struct Object*)_Itr->Data);
		_Next = _Itr->Next;
		_Object->Think(_Object);
		_Itr = _Next;
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

void* SAlloc(size_t _SizeOf) {
	return LifoAlloc(&g_StackAllocator, _SizeOf);
}
void SFree(void* _Ptr) {
	LifoFree(&g_StackAllocator, 1);
}
