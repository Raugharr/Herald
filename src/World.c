/*
 * File: World.c
 * Author: David Brotz
 */

#include "World.h"

#include "Herald.h"
#include "Occupation.h"
#include "Person.h"
#include "Family.h"
#include "Crop.h"
#include "Building.h"
#include "Good.h"
#include "Population.h"
#include "sys/Array.h"
#include "sys/Constraint.h"
#include "sys/HashTable.h"
#include "sys/KDTree.h"
#include "sys/LinkedList.h"
#include "sys/Log.h"
#include "sys/MemoryPool.h"
#include "sys/Random.h"
#include "sys/RBTree.h"
#include "sys/LuaHelper.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <lua/lua.h>
#include <lua/lauxlib.h>
#include <lua/lualib.h>

#ifndef NULL
#define NULL ((void*)0)
#endif

int g_Date = 0;
char g_DataFld[] = "data/";
char* g_Months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dev"};
struct Array* g_World = NULL;
struct LinkedList* g_ManorList = NULL;
struct RBTree* g_GoodDeps = NULL;
struct Array* g_AnFoodDep = NULL;
struct RBTree g_Families;
struct KDTree g_ObjPos;

int FamilyICallback(const struct Family* _One, const struct Family* _Two) {
	return _One->Id - _Two->Id;
}

int FamilySCallback(const int* _One, const struct Family* _Two) {
	return (*_One) - _Two->Id;
}

int FamilyTypeCmp(const void* _One, const void* _Two) {
	return (((struct FamilyType*)_One)->Percent * 1000) - (((struct FamilyType*)_Two)->Percent * 1000);
}

void PopulateManor(int _Population, struct FamilyType** _FamilyTypes, int _X, int _Y) {
	int _FamilySize = -1;
	struct Family* _Family = NULL;
	struct Family* _Parent = NULL;
	struct Constraint** _AgeGroups = NULL;
	struct Constraint** _BabyAvg = NULL;

	lua_getglobal(g_LuaState, "AgeGroups");
	LuaConstraintBnds(g_LuaState);
	if((_AgeGroups = lua_touserdata(g_LuaState, -1)) == NULL) {
		Log(ELOG_ERROR, "AgeGroups is not defined.");
	}

	lua_getglobal(g_LuaState, "BabyAvg");
	LuaConstraintBnds(g_LuaState);
	if((_BabyAvg = lua_touserdata(g_LuaState, -1)) == NULL) {
		DestroyConstrntBnds(_AgeGroups);
		Log(ELOG_ERROR, "BabyAvg is not defined.");
		return;
	}
	while(_Population > 0) {
		_FamilySize = Fuzify(g_FamilySize, Random(1, 100));
		_Parent = CreateRandFamily("Bar", Fuzify(_BabyAvg, Random(0, 9999)) + 2, _AgeGroups, _BabyAvg, _X, _Y);
		FamilyAddGoods(_Parent, g_LuaState, _FamilyTypes, _X, _Y);
		RBInsert(&g_Families, _Parent);
		while(_FamilySize > 0) {
			_Family = CreateRandFamily("Bar", Fuzify(_BabyAvg, Random(0, 9999)) + 2, _AgeGroups, _BabyAvg, _X, _Y);
			FamilyAddGoods(_Family, g_LuaState, _FamilyTypes, _X, _Y);
			RBInsert(&g_Families, _Family);
			_FamilySize -= FamilySize(_Family);
		}
		lua_pop(g_LuaState, 4);
		_Population -= FamilySize(_Parent);
	}
	DestroyConstrntBnds(_AgeGroups);
	DestroyConstrntBnds(_BabyAvg);
}

int PopulateWorld() {
	struct FamilyType** _FamilyTypes = NULL;
	struct Constraint** _ManorSize = NULL;
	const char* _Temp = NULL;
	int _ManorMin = 0;
	int _ManorMax = 0;
	int _ManorInterval = 0;
	int i = 0;

	LuaLoadFile(g_LuaState, "std.lua");
	lua_getglobal(g_LuaState, "ManorConstraints");
	if(lua_type(g_LuaState, -1) != LUA_TTABLE) {
		Log(ELOG_ERROR, "ManorConstraints is not defined.");
		goto end;
	}
	lua_getfield(g_LuaState, -1, "Min");
	AddInteger(g_LuaState, -1, &_ManorMin);
	lua_pop(g_LuaState, 1);
	lua_getfield(g_LuaState, -1, "Max");
	AddInteger(g_LuaState, -1, &_ManorMax);
	lua_pop(g_LuaState, 1);
	lua_getfield(g_LuaState, -1, "Interval");
	AddInteger(g_LuaState, -1, &_ManorInterval);
	lua_pop(g_LuaState, 2);
	_ManorSize = CreateConstrntLst(NULL, _ManorMin, _ManorMax, _ManorInterval);

	lua_getglobal(g_LuaState, "FamilyTypes");
	if(lua_type(g_LuaState, -1) != LUA_TTABLE) {
		Log(ELOG_ERROR, "FamilyTypes is not defined.");
		goto end;
	}
	_FamilyTypes = calloc(lua_rawlen(g_LuaState, -1)+ 1, sizeof(struct FamilyType*));
	lua_pushnil(g_LuaState);
	while(lua_next(g_LuaState, -2) != 0) {
		_FamilyTypes[i] = malloc(sizeof(struct FamilyType));
		lua_pushnil(g_LuaState);
		lua_next(g_LuaState, -2);
		AddNumber(g_LuaState, -1, &_FamilyTypes[i]->Percent);
		lua_pop(g_LuaState, 1);
		lua_next(g_LuaState, -2);
		AddString(g_LuaState, -1, &_Temp);
		_FamilyTypes[i]->LuaFunc = calloc(strlen(_Temp) + 1, sizeof(char));
		strcpy(_FamilyTypes[i]->LuaFunc, _Temp);
		++i;
		lua_pop(g_LuaState, 3);
	}
	lua_pop(g_LuaState, 1);
	InsertionSort(_FamilyTypes, i, FamilyTypeCmp);
	_FamilyTypes[i] = NULL;
	PopulateManor((Fuzify(_ManorSize, Random(_ManorMin, _ManorMax)) * _ManorInterval) + _ManorInterval, _FamilyTypes, Random(0, g_World->TblSize - 1),  Random(0, g_World->TblSize - 1));
	DestroyConstrntBnds(_ManorSize);
	return 1;
	end:
	DestroyConstrntBnds(_ManorSize);
	return 0;
}

void World_Init(int _Area) {
	struct Array* _Array = NULL;
	struct LinkedList* _CropList = CreateLinkedList();
	struct LinkedList* _GoodList = CreateLinkedList();
	struct LinkedList* _BuildList = CreateLinkedList();
	struct LinkedList* _PopList = CreateLinkedList();
	struct LinkedList* _OccupationList = CreateLinkedList();
	struct LnkLst_Node* _Itr = NULL;

	g_World = CreateArray(_Area * _Area);
	g_LuaState = luaL_newstate();
	luaL_openlibs(g_LuaState);
	LuaLoadCFuncs(g_LuaState);

	g_ManorList = (struct LinkedList*) CreateLinkedList();
	HashInsert(&g_Occupations, "Farmer", CreateOccupationSpecial("Farmer", EFARMER));
	chdir(g_DataFld);
	_Array = FileLoad("FirstNames.txt", '\n');
	g_PersonPool = (struct MemoryPool*) CreateMemoryPool(sizeof(struct Person), 10000);
	Family_Init(_Array);
	LuaLoadList(g_LuaState, "crops.lua", "Crops", (void*(*)(lua_State*, int))&CropLoad, &LnkLst_PushBack, _CropList);
	LuaLoadList(g_LuaState, "goods.lua", "Goods", (void*(*)(lua_State*, int))&GoodLoad, &LnkLst_PushBack, _GoodList);
	LISTTOHASH(_CropList, _Itr, &g_Crops, ((struct Crop*)_Itr->Data)->Name);
	LISTTOHASH(_GoodList, _Itr, &g_Goods, ((struct GoodBase*)_Itr->Data)->Name);

	LuaLoadFile(g_LuaState, "goods.lua");
	_Itr = _GoodList->Front;
	lua_getglobal(g_LuaState, "Goods");
	lua_pushnil(g_LuaState);
	while(lua_next(g_LuaState, -2) != 0 && _Itr != NULL) {
		if(!lua_istable(g_LuaState, -1)) {
			lua_pop(g_LuaState, 1);
			continue;
		}
		lua_getfield(g_LuaState, -1, "Name");
		if(lua_isstring(g_LuaState, -1) && !strcmp(lua_tostring(g_LuaState, -1), ((struct GoodBase*)_Itr->Data)->Name)) {
			lua_pop(g_LuaState, 1);
			GoodLoadInput(g_LuaState, -1, _Itr->Data);
		} else {
			lua_pop(g_LuaState, 1);
		}
		_Itr = _Itr->Next;
		lua_pop(g_LuaState, 1);
	}
	lua_pop(g_LuaState, 1);
	LuaLoadList(g_LuaState, "populations.lua", "Populations", (void*(*)(lua_State*, int))&PopulationLoad, &LnkLst_PushBack,  _PopList);
	LuaLoadList(g_LuaState, "occupations.lua", "Occupations", (void*(*)(lua_State*, int))&OccupationLoad, &LnkLst_PushBack, _OccupationList);

	LISTTOHASH(_PopList, _Itr, &g_Populations, ((struct Population*)_Itr->Data)->Name);
	LISTTOHASH(_OccupationList, _Itr, &g_Occupations, ((struct Occupation*)_Itr->Data)->Name);

	g_Families.Table = NULL;
	g_Families.Size = 0;
	g_Families.ICallback = (int (*)(const void*, const void*))&FamilyICallback;
	g_Families.SCallback = (int (*)(const void*, const void*))&FamilySCallback;

	g_ObjPos.Root = NULL;
	g_ObjPos.Size = 0;

	if(PopulateWorld() == 0)
		goto end;
	g_GoodDeps = GoodBuildDep(&g_Goods);
	g_AnFoodDep = AnimalFoodDep(&g_Populations);
	end:
	DestroyLinkedList(_CropList);
	DestroyLinkedList(_GoodList);
	DestroyLinkedList(_BuildList);
	DestroyLinkedList(_PopList);
	DestroyLinkedList(_OccupationList);
}

void World_Quit() {
	RBRemoveAll(&g_Families, (void(*)(void*))DestroyFamily);
	DestroyLinkedList(g_ManorList);
	DestroyArray(g_World);
	DestroyMemoryPool(g_PersonPool);
	Family_Quit();
	DestroyRBTree(g_GoodDeps);
	DestroyArray(g_AnFoodDep);
	lua_close(g_LuaState);
}

void NextDay(int* _Date) {
	int _Day = DAY(*_Date);
	int _Month = MONTH(*_Date);
	int _Year = YEAR(*_Date);

	if((_Month & 1) == 0 || _Month == 7) {
		if(_Day == 31) {
			_Day = 0;
			++_Month;
		}
	} else if(_Month == 1) {
		if(_Day == 28 || ((_Year % 4) == 0 && _Day == 29)) {
			_Day = 0;
			++_Month;
		}
	} else {
		if(_Day == 30) {
			_Day = 0;
			++_Month;
		}
	}
	++_Day;
	if(_Month >= 12) {
		++_Year;
		_Month = 0;
	}
	*_Date = TO_DATE(_Year, _Month, _Day);
}

int World_Tick() {
	NextDay(&g_Date);
	return 1;
}

int MonthToInt(const char* _Month) {
	int i;

	for(i = 0; i < MONTHS; ++i)
		if(strcmp(_Month, g_Months[i]) == 0)
			return i;
	return -1;
}

int DaysBetween(int _DateOne, int _DateTwo) {
	if(_DateTwo < _DateOne)
		return 0;
	return DateToDays(_DateTwo) - DateToDays(_DateOne);
}

int DateToDays(int _Date) {
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
