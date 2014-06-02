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

void LoadLuaFile(lua_State* _State, const char* _File, const char* _Global, void*(*_Callback)(lua_State*, int), struct LinkedList* _Return) {
	int _Error = luaL_loadfile(_State, _File);

	switch(_Error) {
		case LUA_ERRSYNTAX:
			printf("%s", lua_tostring(_State, -1));
			return;
		case LUA_ERRFILE:
			printf("Cannot load file: %s", _File);
			return;
	}
	lua_pcall(_State, 0, 0, 0);
	lua_getglobal(_State, _Global);
	lua_pushnil(_State);
	while(lua_next(_State, -2) != 0) {
		if(!lua_istable(_State, -1)) {
			printf("Warning: index is not a table.");
			continue;
		}
		LnkLst_PushBack(_Return, _Callback(_State, -1));
		lua_pop(_State, 1);
	}
}

void World_Init(int _Area) {
	struct Array* _Array = NULL;
	struct LinkedList* _CropList = CreateLinkedList();
	struct LinkedList* _OccupationList = CreateLinkedList();
	struct LnkLst_Node* _Itr = NULL;

	g_World = CreateArray(_Area * _Area);
	g_LuaState = luaL_newstate();
	g_ManorList = (struct LinkedList*) CreateLinkedList();
	Hash_Insert(&g_Occupations, "Farmer", CreateOccupationSpecial("Farmer", EFARMER));
	chdir(g_DataFld);
	_Array = LoadFile("FirstNames.txt", '\n');
	Person_Init();
	Family_Init(_Array);
	LoadLuaFile(g_LuaState, "crops.lua", "Crops", (void*(*)(lua_State*, int))&LoadCrop, _CropList);
	LoadLuaFile(g_LuaState, "occupations.lua", "Occupations", (void*(*)(lua_State*, int))&LoadOccupation, _OccupationList);

	LISTTOHASH(_CropList, _Itr, &g_Crops, ((struct Crop*)_Itr->Data)->Name);
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

	if((_Month & 1) == 0) {
		if(_Day == 30) {
			_Day = 0;
			++_Month;
		}
	} else if(_Month == 1) {
		if(_Day == 28 || ((_Year % 4) == 0 && _Day == 29)) {
			_Day = 0;
			++_Month;
		}
	} else {
		_Day = 0;
		++_Month;
	}
	++_Day;
	if(_Month >= 12)
		++_Year;
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
