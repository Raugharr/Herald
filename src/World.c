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
#include "AI/Setup.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <lua/lua.h>
#include <lua/lauxlib.h>

#ifndef NULL
#define NULL ((void*)0)
#endif

int g_Date = 0;
struct Array* g_World = NULL;
struct RBTree* g_GoodDeps = NULL;
struct Array* g_AnFoodDep = NULL;
struct RBTree g_Families;
struct KDTree g_ObjPos;
struct Person* g_Player = NULL;
struct HashTable* g_AIHash = NULL;

static const luaL_Reg g_LuaWorldFuncs[] = {
		{"GetPlayer", LuaWorldGetPlayer},
		{"GetDate", LuaWorldGetDate},
		{"GetPersons", LuaWorldGetPersons},
		{"Tick", LuaWorldTick},
		{NULL, NULL}
};

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
		_Population -= FamilySize(_Parent);
	}
	DestroyConstrntBnds(_AgeGroups);
	DestroyConstrntBnds(_BabyAvg);
	lua_pop(g_LuaState, 4);
}

int PopulateWorld() {
	struct FamilyType** _FamilyTypes = NULL;
	struct Constraint** _ManorSize = NULL;
	const char* _Temp = NULL;
	int _ManorMin = 0;
	int _ManorMax = 0;
	int _ManorInterval = 0;
	int i = 0;

	if(LuaLoadFile(g_LuaState, "std.lua") != LUA_OK) {
		goto end;
	}
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
	_FamilyTypes = calloc(lua_rawlen(g_LuaState, -1) + 1, sizeof(struct FamilyType*));
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
	g_Player = PickPlayer();
	DestroyConstrntBnds(_ManorSize);
	return 1;
	end:
	DestroyConstrntBnds(_ManorSize);
	return 0;
}

int LuaPersonItrItr(lua_State* _State) {
	lua_pushlightuserdata(_State, LuaCheckClass(_State, 1, "Iterator"));
	return 1;
}

int LuaPersonItrNext_Aux(lua_State* _State) {
	int _UpValue = lua_upvalueindex(1);
	struct Person* _Person = lua_touserdata(_State, _UpValue);

	if(_Person == NULL)
		goto end;
	if(_Person->Next == NULL) {
		lua_pushnil(_State);
		lua_pushvalue(_State, 1);
		lua_pushnil(_State);
		return 3;
	}
	_Person = _Person->Next;
	end:
	lua_pushlightuserdata(_State, _Person);
	lua_pushvalue(_State, -1);
	lua_replace(_State, _UpValue);
	return 1;
}

int LuaPersonItrNext(lua_State* _State) {
	lua_pushlightuserdata(_State, LuaCheckClass(_State, 1, "Iterator"));
	lua_pushcclosure(_State, LuaPersonItrNext_Aux, 1);
	return 1;
}

int LuaPersonItrPrev_Aux(lua_State* _State) {
	int _UpValue = lua_upvalueindex(1);
	struct Person* _Person = lua_touserdata(_State, _UpValue);

	if(_Person == NULL)
		goto end;
	if(_Person->Prev == NULL) {
		lua_pushnil(_State);
		lua_pushvalue(_State, 1);
		lua_pushnil(_State);
		return 3;
	}
	_Person = _Person->Prev;
	end:
	lua_pushlightuserdata(_State, _Person);
	lua_pushvalue(_State, -1);
	lua_replace(_State, _UpValue);
	return 1;
}

int LuaPersonItrPrev(lua_State* _State) {
	lua_pushlightuserdata(_State, LuaCheckClass(_State, 1, "Iterator"));
	lua_pushcclosure(_State, LuaPersonItrPrev_Aux, 1);
	return 1;
}

struct Person* PickPlayer() {
	struct Person* _Person = g_PersonList;

	while(_Person != NULL) {
		if(_Person->Gender == EMALE && _Person->Age > (15 * YEAR_DAYS))
			break;
		_Person = _Person->Next;
	}
	return _Person;
}

int LuaRegisterPersonItr(lua_State* _State) {
	if(luaL_newmetatable(_State, "PersonIterator") == 0)
		return 0;

	lua_pushliteral(_State, "__index");
	lua_pushvalue(_State, -2);
	lua_rawset(_State, -3);

	lua_pushliteral(_State, "__newindex");
	lua_pushnil(_State);
	lua_rawset(_State, -3);

	lua_pushliteral(_State, "Itr");
	lua_pushcfunction(_State, LuaPersonItrItr);
	lua_rawset(_State, -3);

	lua_pushliteral(_State, "Next");
	lua_pushcfunction(_State, LuaPersonItrNext);
	lua_rawset(_State, -3);

	lua_pushliteral(_State, "Prev");
	lua_pushcfunction(_State, LuaPersonItrPrev);
	lua_rawset(_State, -3);

	lua_pushstring(_State, "__baseclass");
	lua_getglobal(_State, "Iterator");
	lua_rawset(_State, -3);
	lua_setglobal(_State, "PersonIterator");
	return 1;
}

int LuaWorldGetPlayer(lua_State* _State) {
	lua_pushlightuserdata(_State, g_Player);
	return 1;
}

int LuaWorldGetPersons(lua_State* _State) {
	lua_newtable(_State);

	lua_getglobal(_State, "PersonIterator");
	lua_setmetatable(_State, -2);
	lua_pushstring(_State, "__self");
	lua_pushlightuserdata(_State, g_PersonList);
	lua_rawset(_State, -3);
	return 1;
}

int LuaWorldGetDate(lua_State* _State) {
	lua_pushinteger(_State, g_Date);
	return 1;
}

int LuaWorldTick(lua_State* _State) {
	World_Tick();
	return 0;
}

void WorldInit(int _Area) {
	struct Array* _Array = NULL;
	struct LinkedList* _CropList = CreateLinkedList();
	struct LinkedList* _GoodList = CreateLinkedList();
	struct LinkedList* _BuildList = CreateLinkedList();
	struct LinkedList* _PopList = CreateLinkedList();
	struct LinkedList* _OccupationList = CreateLinkedList();
	struct LnkLst_Node* _Itr = NULL;

	Log(ELOG_INFO, "Creating World.");
	++g_Log.Indents;
	g_AIHash = CreateHash(32);
	g_World = CreateArray(_Area * _Area);
	luaL_newlib(g_LuaState, g_LuaWorldFuncs);
	lua_setglobal(g_LuaState, "World");
	LuaRegisterPersonItr(g_LuaState);
	chdir(DATAFLD);
	AIInit(g_LuaState);
	_Array = FileLoad("FirstNames.txt", '\n');
	g_PersonPool = (struct MemoryPool*) CreateMemoryPool(sizeof(struct Person), 10000);
	Family_Init(_Array);
	LuaLoadList(g_LuaState, "goods.lua", "Goods", (void*(*)(lua_State*, int))&GoodLoad, &LnkLst_PushBack, _GoodList);
	g_Goods.TblSize = (_GoodList->Size * 5) / 4;
	g_Goods.Table = (struct HashNode**) malloc(sizeof(struct HashNode*) * g_Goods.TblSize);
	memset(g_Goods.Table, 0, g_Goods.TblSize * sizeof(struct HashNode*));;
	LISTTOHASH(_GoodList, _Itr, &g_Goods, ((struct GoodBase*)_Itr->Data)->Name);
	
	LuaLoadList(g_LuaState, "crops.lua", "Crops", (void*(*)(lua_State*, int))&CropLoad, &LnkLst_PushBack, _CropList);
	g_Crops.TblSize = (_CropList->Size * 5) / 4;
	g_Crops.Table = (struct HashNode**) malloc(sizeof(struct HashNode*) * _CropList->Size);
	memset(g_Crops.Table, 0, g_Crops.TblSize * sizeof(struct HashNode*));
	LISTTOHASH(_CropList, _Itr, &g_Crops, ((struct Crop*)_Itr->Data)->Name)


	if(_GoodList->Size == 0) {
		Log(ELOG_WARNING, "Failed to load goods.");
		goto GoodLoadEnd;
	}
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
	GoodLoadEnd:
	LuaLoadList(g_LuaState, "populations.lua", "Populations", (void*(*)(lua_State*, int))&PopulationLoad, &LnkLst_PushBack,  _PopList);
	g_Populations.TblSize = (_PopList->Size * 5) / 4;
	g_Populations.Table = (struct HashNode**) malloc(sizeof(struct HashNode*) * g_Populations.TblSize);
	memset(g_Populations.Table, 0, g_Populations.TblSize * sizeof(struct HashNode*));

	LuaLoadList(g_LuaState, "occupations.lua", "Occupations", (void*(*)(lua_State*, int))&OccupationLoad, &LnkLst_PushBack, _OccupationList);
	g_Occupations.TblSize = ((_OccupationList->Size + 1) * 5) / 4;
	g_Occupations.Table = (struct HashNode**) malloc(sizeof(struct HashNode*) * g_Occupations.TblSize);
	HashInsert(&g_Occupations, "Farmer", CreateOccupationSpecial("Farmer", EFARMER));
	memset(g_Occupations.Table, 0, g_Occupations.TblSize * sizeof(struct HashNode*));

	LuaLoadList(g_LuaState, "buildings.lua", "BuildMats", (void*(*)(lua_State*, int))&BuildingLoad, (void(*)(struct LinkedList*, void*))&LnkLst_CatNode, _BuildList);
	g_BuildMats.TblSize = (_BuildList->Size * 5) / 4;
	g_BuildMats.Table = (struct HashNode**) malloc(sizeof(struct HashNode*) * g_BuildMats.TblSize);
	memset(g_BuildMats.Table, 0, g_BuildMats.TblSize * sizeof(struct HashNode*));

	LISTTOHASH(_PopList, _Itr, &g_Populations, ((struct Population*)_Itr->Data)->Name);
	LISTTOHASH(_OccupationList, _Itr, &g_Occupations, ((struct Occupation*)_Itr->Data)->Name);
	LISTTOHASH(_BuildList, _Itr, &g_BuildMats, ((struct BuildMat*)_Itr->Data)->Good->Name);
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
	chdir("..");
	--g_Log.Indents;
}

void WorldQuit() {
	AIQuit();
	RBRemoveAll(&g_Families, (void(*)(void*))DestroyFamily);
	DestroyArray(g_World);
	DestroyMemoryPool(g_PersonPool);
	Family_Quit();
	DestroyRBTree(g_GoodDeps);
	DestroyArray(g_AnFoodDep);
	DestroyHash(g_AIHash);
}

int World_Tick() {
	struct Person* _Person = g_PersonList;
	int _Ct = 0;
	int _Size = 0;

	ATImerUpdate(&g_ATimer);
	while(_Person != NULL) {
		if(_Person->Age < TO_YEARS(13))
			_Ct++;
		else
			++_Size;
		HashClear(g_AIHash);
		BHVRun(_Person->Behavior, _Person, g_AIHash);
		_Person = _Person->Next;
	}
	NextDay(&g_Date);
	return 1;
}
