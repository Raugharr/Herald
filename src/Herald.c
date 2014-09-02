/*
 * File: Herald.c
 * Author: David Brotz
 */

#include "Herald.h"

#include "Person.h"
#include "Family.h"
#include "Crop.h"
#include "Building.h"
#include "World.h"
#include "Good.h"
#include "Occupation.h"
#include "Population.h"
#include "sys/Stack.h"
#include "sys/RBTree.h"
#include "sys/Random.h"
#include "sys/LinkedList.h"
#include "sys/MemoryPool.h"
#include "sys/Array.h"
#include "sys/LuaHelper.h"
#include "sys/Constraint.h"
#include "sys/KDTree.h"
#include "sys/Event.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <lua/lua.h>
#include <lua/lauxlib.h>

#define AGEDIST_SIZE (17)
#define CROPS_TBLSZ (64)
#define GOODS_TBLSZ (512)
#define BUILDINGS_TBLSZ (64)
#define OCCUPATIONS_TBLSZ (128)

struct HashTable g_Crops;
struct HashTable g_Goods;
struct HashTable g_BuildMats;
struct HashTable g_Occupations;
struct HashTable g_Populations;
struct ATimer g_ATimer;
int g_ObjPosBal = 2;
struct Constraint** g_FamilySize;
struct Constraint** g_AgeConstraints;

int g_Id = 0;
char* g_Months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dev"};

int IdISCallback(const int* _One, const int* _Two) {
	return *(_One) - *(_Two);
}

void HeraldInit() {
	g_Crops.TblSize = CROPS_TBLSZ;
	g_Crops.Table = (struct HashNode**) malloc(sizeof(struct HashNode*) * g_Crops.TblSize);
	g_Crops.Size = 0;
	memset(g_Crops.Table, 0, g_Crops.TblSize * sizeof(struct HashNode*));

	g_Goods.TblSize = GOODS_TBLSZ;
	g_Goods.Table = (struct HashNode**) malloc(sizeof(struct HashNode*) * g_Goods.TblSize);
	g_Goods.Size = 0;
	memset(g_Goods.Table, 0, g_Goods.TblSize * sizeof(struct HashNode*));

	g_BuildMats.TblSize = BUILDINGS_TBLSZ;
	g_BuildMats.Table = (struct HashNode**) malloc(sizeof(struct HashNode*) * g_BuildMats.TblSize);
	g_BuildMats.Size = 0;
	memset(g_BuildMats.Table, 0, g_BuildMats.TblSize * sizeof(struct HashNode*));

	g_Occupations.TblSize = OCCUPATIONS_TBLSZ;
	g_Occupations.Table = (struct HashNode**) malloc(sizeof(struct HashNode*) * g_Occupations.TblSize);
	g_Occupations.Size = 0;
	memset(g_Occupations.Table, 0, g_Occupations.TblSize * sizeof(struct HashNode*));

	g_Populations.TblSize = OCCUPATIONS_TBLSZ;
	g_Populations.Table = (struct HashNode**) malloc(sizeof(struct HashNode*) * g_Populations.TblSize);
	g_Populations.Size = 0;
	memset(g_Populations.Table, 0, g_Populations.TblSize * sizeof(struct HashNode*));

	g_ATimer.Tree = CreateRBTree((int(*)(const void*, const void*))ATimerICallback, (int(*)(const void*, const void*))ATimerSCallback);
	g_ATimer.ATypes = calloc(ATT_SIZE, sizeof(struct ATType*));
	ATimerAddType(&g_ATimer, CreateATType(ATT_PREGANCY, (int(*)(void*))PregancyUpdate, (void(*)(void*))DestroyPregancy));
	ATimerAddType(&g_ATimer, CreateATType(ATT_CONSTRUCTION, (int(*)(void*))ConstructUpdate, (void(*)(void*))DestroyConstruct));

	g_FamilySize = CreateConstrntBnds(5, 1, 5, 15, 40, 75, 100);
	g_AgeConstraints = CreateConstrntLst(NULL, 0, 1068, 60);
	EventInit();
}

void HeraldDestroy() {
	DestroyConstrntBnds(g_FamilySize);
	DestroyConstrntBnds(g_AgeConstraints);
	ATTimerRmAll(&g_ATimer);
	EventQuit();
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

void CreateObject(struct Object* _Obj, int _X, int _Y) {
	struct Array* _Table = NULL;
	int _Pos[2];

	_Obj->Id = NextId();
	_Obj->X = _X;
	_Obj->Y = _Y;

	_Pos[0] = _X;
	_Pos[1] = _Y;
	if((_Table = KDSearch(&g_ObjPos, _Pos)) == NULL) {
		_Table = CreateArray(128);
		ArrayInsert(_Table, _Obj);
		KDInsert(&g_ObjPos, _Table, _X, _Y);
	} else
		ArrayInsertSort_S(_Table, _Obj, (int(*)(const void*, const void*)) IdISCallback);
	if(g_ObjPos.Size >= g_ObjPosBal) {
		g_ObjPosBal *= 2;
		KDBalance(&g_ObjPos);
	}
}

int NextId() {return g_Id++;}

int ObjCmp(const void* _One, const void* _Two) {
	return *((int*)_One) - *((int*)_Two);
}

DATE MonthToInt(const char* _Month) {
	int i;

	for(i = 0; i < MONTHS; ++i)
		if(strcmp(_Month, g_Months[i]) == 0)
			return i;
	return -1;
}

DATE DaysBetween(int _DateOne, int _DateTwo) {
	if(_DateTwo < _DateOne)
		return 0;
	return DateToDays(_DateTwo) - DateToDays(_DateOne);
}

DATE DateToDays(int _Date) {
	int _Total = 0;
	int _Years = YEAR(_Date);
	int _Months = MONTH(_Date);
	int _Result = 0;

	_Total = _Years + (_Years / 4);
	_Result = _Months / 2;
	_Total += (_Result + 1) * 31;
	_Total += _Result * 30;
	if(_Months >= 1) {
		if(_Years % 4 == 0)
			_Total += 28;
		else
			_Total += 29;
	}
	if(_Months >= 8)
		++_Total;
	return _Total;
}

