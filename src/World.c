/*
 * File: World.c
 * Author: David Brotz
 */

#include "World.h"

#include "Herald.h"
#include "Occupation.h"
#include "Manor.h"
#include "Person.h"
#include "Family.h"
#include "Crop.h"
#include "Building.h"
#include "Good.h"
#include "sys/LinkedList.h"
#include "sys/RBTree.h"
#include "sys/Constraint.h"
#include "sys/Random.h"
#include "sys/LinkedList.h"
#include "sys/HashTable.h"
#include "sys/Array.h"
#include "sys/MemoryPool.h"
#include "sys/Log.h"

#include "sys/LuaHelper.h"

#include <stdio.h>
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

void World_Init(int _Area) {
	int _ManorMin = 0;
	int _ManorMax = 0;
	int _ManorInterval = 0;
	struct Constraint** _ManorSize = NULL;
	struct Array* _Array = NULL;
	struct LinkedList* _CropList = CreateLinkedList();
	struct LinkedList* _GoodList = CreateLinkedList();
	struct LinkedList* _BuildList = CreateLinkedList();
	struct LinkedList* _PopList = CreateLinkedList();
	struct LinkedList* _OccupationList = CreateLinkedList();
	struct LnkLst_Node* _Itr = NULL;
	int _LuaArray[20];
	memset(_LuaArray, 0, sizeof(int) * 20);

	g_World = CreateArray(_Area * _Area);
	g_LuaState = luaL_newstate();
	luaL_openlibs(g_LuaState);
	lua_register(g_LuaState, "CreateConstraint", LuaConstraint);
	lua_register(g_LuaState, "CreateConstraintBounds", LuaConstraintBnds);

	g_ManorList = (struct LinkedList*) CreateLinkedList();
	HashInsert(&g_Occupations, "Farmer", CreateOccupationSpecial("Farmer", EFARMER));
	chdir(g_DataFld);
	_Array = FileLoad("FirstNames.txt", '\n');
	g_PersonPool = (struct MemoryPool*) CreateMemoryPool(sizeof(struct Person), 10000);
	Family_Init(_Array);
	LuaLoadToList(g_LuaState, "crops.lua", "Crops", (void*(*)(lua_State*, int))&CropLoad, _CropList);
	LuaLoadToList(g_LuaState, "goods.lua", "Goods", (void*(*)(lua_State*, int))&GoodLoad, _GoodList);
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
	LuaLoadToList(g_LuaState, "buildings.lua", "Buildings", (void*(*)(lua_State*, int))&BuildingLoad, _BuildList);
	LuaLoadToList(g_LuaState, "populations.lua", "Populations", (void*(*)(lua_State*, int))&PopulationLoad, _PopList);
	LuaLoadToList(g_LuaState, "occupations.lua", "Occupations", (void*(*)(lua_State*, int))&OccupationLoad, _OccupationList);

	LISTTOHASH(_BuildList, _Itr, &g_Buildings, ((struct Building*)_Itr->Data)->Name);
	LISTTOHASH(_PopList, _Itr, &g_Populations, ((struct Population*)_Itr->Data)->Name);
	LISTTOHASH(_OccupationList, _Itr, &g_Occupations, ((struct Occupation*)_Itr->Data)->Name);

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
	LnkLst_PushBack(g_ManorList, CreateManor("Test", (Fuzify(_ManorSize, Random(_ManorMin, _ManorMax)) * _ManorInterval) + _ManorInterval));

	g_GoodDeps = GoodBuildDep(&g_Goods);
	g_AnFoodDep = AnimalFoodDep(&g_Populations);
	end:
	DestroyLinkedList(_CropList);
	DestroyLinkedList(_GoodList);
	DestroyLinkedList(_BuildList);
	DestroyLinkedList(_PopList);
	DestroyLinkedList(_OccupationList);
	DestroyConstrntBnds(_ManorSize);
}

void World_Quit() {
	struct LnkLst_Node* _Itr = g_ManorList->Front;

	while(_Itr != NULL) {
		DestroyManor(_Itr->Data);
		_Itr = _Itr->Next;
	}
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
