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
#include "sys/Constraint.h"
#include "sys/Random.h"
#include "sys/LinkedList.h"
#include "sys/HashTable.h"
#include "sys/Array.h"

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <lua/lua.h>
#include <lua/lauxlib.h>

#ifndef NULL
#define NULL ((void*)0)
#endif

int g_Date = 0;
char g_DataFld[] = "data/";
char* g_Months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dev"};
struct Array* g_World = NULL;
lua_State* g_LuaState = NULL;

int LoadLuaFile(lua_State* _State, const char* _File) {
	int _Error = luaL_loadfile(_State, _File);

	switch(_Error) {
		case LUA_ERRSYNTAX:
			printf("%s", lua_tostring(_State, -1));
			return _Error;
		case LUA_ERRFILE:
			printf("Cannot load file: %s", _File);
			return _Error ;
	}
	lua_pcall(_State, 0, 0, 0);
	return 1;
}

void LoadLuaToList(lua_State* _State, const char* _File, const char* _Global, void*(*_Callback)(lua_State*, int), struct LinkedList* _Return) {
	void* _CallRet = NULL;

	if(LoadLuaFile(_State, _File) != 1)
		return;
	lua_getglobal(_State, _Global);
	if(!lua_istable(_State, -1))
		return;
	lua_pushnil(_State);
	while(lua_next(_State, -2) != 0) {
		if(!lua_istable(_State, -1)) {
			printf("Warning: index is not a table.");
			lua_pop(_State, 1);
			continue;
		}
		if((_CallRet = _Callback(_State, -1)) != NULL)
				LnkLst_PushBack(_Return, _CallRet);
		lua_pop(_State, 1);
	}
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
	g_ManorList = (struct LinkedList*) CreateLinkedList();
	Hash_Insert(&g_Occupations, "Farmer", CreateOccupationSpecial("Farmer", EFARMER));
	chdir(g_DataFld);
	_Array = FileLoad("FirstNames.txt", '\n');
	Person_Init();
	Family_Init(_Array);
	LoadLuaToList(g_LuaState, "crops.lua", "Crops", (void*(*)(lua_State*, int))&CropLoad, _CropList);
	LoadLuaToList(g_LuaState, "goods.lua", "Goods", (void*(*)(lua_State*, int))&GoodLoad, _GoodList);
	LISTTOHASH(_GoodList, _Itr, &g_Goods, ((struct Good*)_Itr->Data)->Name);
	LoadLuaFile(g_LuaState, "goods.lua");
	_Itr = _GoodList->Front;
	lua_getglobal(g_LuaState, "Goods");
	lua_pushnil(g_LuaState);
	while(lua_next(g_LuaState, -2) != 0) {
		if(!lua_istable(g_LuaState, -1)) {
			lua_pop(g_LuaState, 1);
			continue;
		}
		GoodLoadInput(g_LuaState, -1, _Itr->Data);
		_Itr = _Itr->Next;
		lua_pop(g_LuaState, 1);
	}
	LoadLuaToList(g_LuaState, "buildings.lua", "Buildings", (void*(*)(lua_State*, int))&BuildingLoad, _BuildList);
	LoadLuaToList(g_LuaState, "populations.lua", "Populations", (void*(*)(lua_State*, int))&PopulationLoad, _PopList);
	LoadLuaToList(g_LuaState, "occupations.lua", "Occupations", (void*(*)(lua_State*, int))&OccupationLoad, _OccupationList);

	LISTTOHASH(_CropList, _Itr, &g_Crops, ((struct Crop*)_Itr->Data)->Name);
	LISTTOHASH(_BuildList, _Itr, &g_Buildings, ((struct Building*)_Itr->Data)->Name);
	LISTTOHASH(_PopList, _Itr, &g_Populations, ((struct Population*)_Itr->Data)->Name);
	LISTTOHASH(_OccupationList, _Itr, &g_Occupations, ((struct Occupation*)_Itr->Data)->Name);

	LnkLst_PushBack(g_ManorList, CreateManor("Test", (Fuzify(g_ManorSize, Random(MANORSZ_MIN, MANORSZ_MAX)) * MANORSZ_INTRVL) + MANORSZ_INTRVL));
	DestroyLinkedList(_CropList);
	DestroyLinkedList(_OccupationList);
}

void World_Quit() {
	struct LnkLst_Node* _Itr = g_ManorList->Front;

	while(_Itr != NULL) {
		DestroyManor(_Itr->Data);
		_Itr = _Itr->Next;
	}
	DestroyLinkedList(g_ManorList);
	DestroyArray(g_World);
	Person_Quit();
	Family_Quit();
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
