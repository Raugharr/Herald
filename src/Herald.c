/*
 * File: Herald.c
 * Author: David Brotz
 */

#include "Herald.h"

#include "Person.h"
#include "Family.h"
#include "Crop.h"
#include "Manor.h"
#include "Building.h"
#include "World.h"
#include "Good.h"
#include "LuaWrappers.h"
#include "Occupation.h"
#include "Population.h"
#include "sys/RBTree.h"
#include "sys/Random.h"
#include "sys/LinkedList.h"
#include "sys/MemoryPool.h"
#include "sys/Array.h"
#include "sys/LuaHelper.h"
#include "sys/Constraint.h"
#include "events/Event.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <lua/lua.h>
#include <lua/lauxlib.h>

#define AGEDIST_SIZE (17)
#define CROPS_TBLSZ (512)
#define GOODS_TBLSZ (512)
#define BUILDINGS_TBLSZ (512)
#define OCCUPATIONS_TBLSZ (512)

struct HashTable g_Crops;
struct HashTable g_Goods;
struct HashTable g_Buildings;
struct HashTable g_Occupations;
struct HashTable g_Populations;
struct RBTree g_Strings;
struct RBTree g_PregTree;
struct Constraint** g_FamilySize;
struct Constraint** g_AgeGroups;
struct Constraint** g_AgeConstraints;
struct Constraint** g_BabyAvg;
struct Constraint** g_ManorSize;
struct LinkedList* g_ManorList;

int PregancyICallback(struct Pregancy* _PregOne, struct Pregancy* _PregTwo) {
	return _PregOne->Mother->Id - _PregTwo->Mother->Id;
}

int PregancySCallback(struct Person* _Mother, struct Pregancy* _Preg) {
	return _Mother->Id - _Preg->Mother->Id;
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

	g_Buildings.TblSize = BUILDINGS_TBLSZ;
	g_Buildings.Table = (struct HashNode**) malloc(sizeof(struct HashNode*) * g_Buildings.TblSize);
	g_Buildings.Size = 0;
	memset(g_Buildings.Table, 0, g_Buildings.TblSize * sizeof(struct HashNode*));

	g_Occupations.TblSize = OCCUPATIONS_TBLSZ;
	g_Occupations.Table = (struct HashNode**) malloc(sizeof(struct HashNode*) * g_Occupations.TblSize);
	g_Occupations.Size = 0;
	memset(g_Occupations.Table, 0, g_Occupations.TblSize * sizeof(struct HashNode*));

	g_Populations.TblSize = OCCUPATIONS_TBLSZ;
	g_Populations.Table = (struct HashNode**) malloc(sizeof(struct HashNode*) * g_Populations.TblSize);
	g_Populations.Size = 0;
	memset(g_Populations.Table, 0, g_Populations.TblSize * sizeof(struct HashNode*));

	g_PregTree.Table = NULL;
	g_PregTree.Size = 0;
	g_PregTree.ICallback = (int (*)(void*, void*))&PregancyICallback;
	g_PregTree.SCallback = (int (*)(void*, void*))&PregancySCallback;

	g_FamilySize = CreateConstrntBnds(5, 1, 5, 15, 40, 75, 100);
	g_AgeGroups = CreateConstrntBnds(5, 0, 71, 155, 191, 719, 1200);
	g_AgeConstraints = CreateConstrntLst(NULL, 0, 1068, 60);
	g_BabyAvg = CreateConstrntBnds(8, 0, 624, 1349, 2599, 4999, 6249, 7499, 8749, 9999);
	g_ManorSize = CreateConstrntLst(NULL, MANORSZ_MIN, MANORSZ_MAX, MANORSZ_INTRVL);
	Event_Init();
}

void HeraldDestroy() {
	DestroyConstrntBnds(g_FamilySize);
	DestroyConstrntBnds(g_AgeGroups);
	DestroyConstrntBnds(g_AgeConstraints);
	DestroyConstrntBnds(g_BabyAvg);
	DestroyConstrntBnds(g_ManorSize);
	Event_Quit();
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

int Tick() {
	struct LnkLst_Node* _Itr = g_ManorList->Front;

	RBIterate(&g_PregTree, (int(*)(void*))PregancyUpdate);
	while(_Itr != NULL) {
		if(Manor_Update(_Itr->Data) == 0)
			return 0;
		_Itr = _Itr->Next;
	}
	if(World_Tick() == 0)
		return 0;
	return 1;
}
