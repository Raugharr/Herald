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
#include "AI/AIHelper.h"
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
#include "video/Animation.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <lua/lua.h>
#include <lua/lauxlib.h>

#define AGEDIST_SIZE (17)

struct HashTable g_Crops;
struct HashTable g_Goods;
struct HashTable g_BuildMats;
struct HashTable g_Populations;
struct HashTable g_Animations;
int g_ObjPosBal = 2;
struct Constraint** g_FamilySize;
struct Constraint** g_AgeConstraints;

struct ObjectList {
	struct RBTree SearchTree;
	struct LinkedList ThinkList;
};
struct MissionEngine g_MissionEngine;

static struct ObjectList g_Objects = {
	{NULL, 0, (int(*)(const void*, const void*))ObjectCmp, (int(*)(const void*, const void*))ObjectCmp},
	{0, NULL, NULL}
};

int g_Id = 0;
struct Constraint** g_OpinionMods = NULL;

int IdISCallback(const int* _One, const int* _Two) {
	return *(_One) - *(_Two);
}

int HeraldInit() {
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

	g_TaskPool = CreateTaskPool();

	g_FamilySize = CreateConstrntBnds(FAMILYSIZE, 2, 10, 20, 40, 75, 100);
	g_AgeConstraints = CreateConstrntLst(NULL, 0, 1068, 60);
	g_OpinionMods = CreateConstrntBnds(5, -BIGGUY_RELMAX, -76, -26, 25, 75, BIGGUY_RELMAX);
	EventInit();
	PathfindInit();
	MathInit();
	g_MissionEngine.MissionQueue.Size = 0;
	g_MissionEngine.MissionQueue.Compare = (int(*)(const void*, const void*))MissionHeapInsert;
	g_MissionEngine.MissionQueue.Table = calloc(1024, sizeof(void*));
	g_MissionEngine.MissionQueue.TblSz = 1024;
	g_MissionEngine.UsedMissionQueue.Size = 0;
	g_MissionEngine.UsedMissionQueue.Compare = (int(*)(const void*, const void*))UsedMissionHeapInsert;
	g_MissionEngine.UsedMissionQueue.Table = calloc(1024, sizeof(void*));
	g_MissionEngine.UsedMissionQueue.TblSz = 1024;

	g_MissionEngine.Missions.Table = NULL;
	g_MissionEngine.Missions.Size = 0;
	g_MissionEngine.Missions.ICallback = (int(*)(const void*, const void*))MissionTreeInsert;
	g_MissionEngine.Missions.SCallback = (int(*)(const void*, const void*))MissionTreeSearch;

	g_MissionEngine.UsedMissionTree.Table = NULL;
	g_MissionEngine.UsedMissionTree.Size = 0;
	g_MissionEngine.UsedMissionTree.ICallback = (int(*)(const void*, const void*))UsedMissionInsert;
	g_MissionEngine.UsedMissionTree.SCallback = (int(*)(const void*, const void*))UsedMissionSearch;

	g_MissionEngine.MissionId.Table = NULL;
	g_MissionEngine.MissionId.Size = 0;
	g_MissionEngine.MissionId.ICallback = (int(*)(const void*, const void*))MissionIdSearch;
	g_MissionEngine.MissionId.SCallback = (int(*)(const void*, const void*))MissionIdInsert;

	g_MissionEngine.Categories[0].GetObj = BigGuyGetCrisis;
	g_MissionEngine.Categories[0].StateStr = g_CrisisStateStr;
	g_MissionEngine.Categories[0].GetState = NULL;

	for(int i = 0; i < WORLDSTATE_ATOMSZ; ++i) {
		g_MissionEngine.MissionList[i].Size = 0;
		g_MissionEngine.MissionList[i].Front = NULL;
		g_MissionEngine.MissionList[i].Back = NULL;
	}
	InitMissions();
	return 1;
}

void HeraldDestroy() {
	struct HashItr* _Itr = HashCreateItr(&g_Animations);

	while(_Itr != NULL) {
		DestroyAnimation(_Itr->Node->Pair);
		HashNext(&g_Animations, _Itr);
	}
	DestroyTaskPool(g_TaskPool);
	DestroyConstrntBnds(g_FamilySize);
	DestroyConstrntBnds(g_AgeConstraints);
	DestroyConstrntBnds(g_OpinionMods);
	EventQuit();
	PathfindQuit();
	DestroyMissionEngine(&g_MissionEngine);
	QuitMissions();
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

void CreateObject(struct Object* _Obj, int _Type, void (*_Think)(struct Object*)) {
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

int ObjectCmp(const void* _One, const void* _Two) {
	return *((int*)_One) - *((int*)_Two);
}
