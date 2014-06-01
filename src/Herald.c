/*
 * File: Herald.c
 * Author: David Brotz
 */

#include "Herald.h"

#include "Person.h"
#include "Family.h"
#include "Crop.h"
#include "Manor.h"
#include "World.h"
#include "sys/RBTree.h"
#include "sys/Random.h"
#include "sys/LinkedList.h"
#include "sys/MemoryPool.h"
#include "sys/Array.h"
#include "sys/LuaHelper.h"
#include "sys/Constraint.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <lua/lua.h>

#define AGEDIST_SIZE (17)
#define CROPS_TBLSZ (512)
#define GOODS_TBLSZ (512)
#define BUILDINGS_TBLSZ (512)
#define OCCUPATIONS_TBLSZ (512)

struct HashTable g_Crops;
struct HashTable g_Goods;
struct HashTable g_Buildings;
struct HashTable g_Occupations;
struct RBTree g_Strings;
struct Constraint** g_FamilySize;
struct Constraint** g_AgeGroups;
struct Constraint** g_AgeConstraints;
struct Constraint** g_BabyAvg;
struct Constraint** g_ManorSize;
struct LinkedList* g_ManorList;

void HeraldInit() {
	g_Crops.TblSize = CROPS_TBLSZ;
	g_Crops.Table = (struct HashNode**) malloc(sizeof(struct HashNode*) * g_Crops.TblSize);
	SetArray((void***)&g_Crops.Table, g_Crops.TblSize, NULL);
	g_Goods.TblSize = GOODS_TBLSZ;
	g_Goods.Table = (struct HashNode**) malloc(sizeof(struct HashNode*) * g_Goods.TblSize);
	SetArray((void***)&g_Goods.Table, g_Goods.TblSize, NULL);
	g_Buildings.TblSize = BUILDINGS_TBLSZ;
	g_Buildings.Table = (struct HashNode**) malloc(sizeof(struct HashNode*) * g_Buildings.TblSize);
	SetArray((void***)&g_Buildings.Table, g_Buildings.TblSize, NULL);
	g_Occupations.TblSize = OCCUPATIONS_TBLSZ;
	g_Occupations.Table = (struct HashNode**) malloc(sizeof(struct HashNode*) * g_Occupations.TblSize);
	SetArray((void***)&g_Occupations.Table, g_Occupations.TblSize, NULL);

	g_FamilySize = CreateConstrntBnds(5, 1, 5, 15, 40, 75, 100);
	g_AgeGroups = CreateConstrntBnds(5, 0, 71, 155, 191, 719, 1200);
	g_AgeConstraints = CreateConstrntLst(NULL, 0, 1068, 60);
	g_BabyAvg = CreateConstrntBnds(8, 0, 624, 1349, 2599, 4999, 6249, 7499, 8749, 9999);
	g_ManorSize = CreateConstrntLst(NULL, MANORSZ_MIN, MANORSZ_MAX, MANORSZ_INTRVL);
}

void HeraldDestroy() {
	DestroyConstrntBnds(g_FamilySize);
	DestroyConstrntBnds(g_AgeGroups);
	DestroyConstrntBnds(g_AgeConstraints);
	DestroyConstrntBnds(g_BabyAvg);
	DestroyConstrntBnds(g_ManorSize);
}

struct Array* LoadFile(const char* _File, char _Delimiter) {
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
				Array_Insert(_Array, _Name);
				_Pos = 0;
			} else {
				if(_Pos >= 256)
					return _Array;
				_Buffer[_Pos++] = _Char;
			}
		}
	return _Array;
}

struct Crop* LoadCrop(lua_State* _State, int _Index) {
	const char* _Name = NULL;
	const char* _Key = NULL;
	const char* _Temp = NULL;
	int _PerAcre = 0;
	double _YieldMult = 0;
	int _NutValue = 0;
	int _StartMonth = 0;
	int _EndMonth = 0;
	int _Return = -2;

	lua_getmetatable(_State, _Index);
	lua_pushnil(_State);
	while(lua_next(_State, -2) != 0) {
		if(lua_isstring(_State, -2))
			_Key = lua_tostring(_State, -2);
		else
			continue;
		if(!strcmp("PoundsPerAcre", _Key))
			_Return = AddInteger(_State, -1, &_PerAcre);
		else if (!strcmp("YieldPerSeed", _Key))
			_Return = AddNumber(_State, -1, &_YieldMult);
		else if (!strcmp("NutritionalValue", _Key))
			_Return = AddInteger(_State, -1, &_NutValue);
		else if (!strcmp("Name", _Key))
			_Return = AddString(_State, -1, &_Name);
		else if (!strcmp("StartMonth", _Key)) {
			_Return = AddString(_State, -1, &_Temp);
			_StartMonth = MonthToInt(_Temp);
			if(_StartMonth == -1)
				_Return = 0;
		} else if (!strcmp("EndMonth", _Key)) {
			_Return = AddString(_State, -1, &_Temp);
			_EndMonth = MonthToInt(_Temp);
			if(_EndMonth == -1)
				_Return = 0;
		}
	}
	if(_Return > 0)
		return CreateCrop(_Name, _PerAcre, _NutValue, _YieldMult, _StartMonth, _EndMonth);
	return NULL;
}

int Tick() {
	struct LnkLst_Node* _Itr = g_ManorList->Front;

	while(_Itr != NULL) {
		if(Manor_Tick(_Itr->Data) == 0)
			return 0;
		_Itr = _Itr->Next;
	}
	if(World_Tick() == 0)
		return 0;
	return 1;
}
