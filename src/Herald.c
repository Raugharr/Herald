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
struct Constraint** g_FamilySize;
struct Constraint** g_AgeGroups;
struct Constraint** g_AgeConstraints;
struct Constraint** g_BabyAvg;
struct Constraint** g_ManorSize;
struct LinkedList* g_ManorList;

void HeraldInit() {
	g_Crops.TblSize = CROPS_TBLSZ;
	g_Crops.Table = (struct HashNode**) malloc(sizeof(struct HashNode*) * g_Crops.TblSize);
	SetArray((void**)g_Crops.Table, g_Crops.TblSize, NULL);

	g_Goods.TblSize = GOODS_TBLSZ;
	g_Goods.Table = (struct HashNode**) malloc(sizeof(struct HashNode*) * g_Goods.TblSize);
	SetArray((void**)g_Goods.Table, g_Goods.TblSize, NULL);

	g_Buildings.TblSize = BUILDINGS_TBLSZ;
	g_Buildings.Table = (struct HashNode**) malloc(sizeof(struct HashNode*) * g_Buildings.TblSize);
	SetArray((void**)g_Buildings.Table, g_Buildings.TblSize, NULL);

	g_Occupations.TblSize = OCCUPATIONS_TBLSZ;
	g_Occupations.Table = (struct HashNode**) malloc(sizeof(struct HashNode*) * g_Occupations.TblSize);
	SetArray((void**)g_Occupations.Table, g_Occupations.TblSize, NULL);

	g_Populations.TblSize = OCCUPATIONS_TBLSZ;
	g_Populations.Table = (struct HashNode**) malloc(sizeof(struct HashNode*) * g_Populations.TblSize);
	SetArray((void**)g_Populations.Table, g_Populations.TblSize, NULL);

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

struct InputReq* CreateInputReq() {
	struct InputReq* _Mat = (struct InputReq*) malloc(sizeof(struct InputReq));

	_Mat->Req = NULL;
	_Mat->Quantity = 0;
	return _Mat;
}

void DestroyInputReq(struct InputReq* _Mat) {
	free(_Mat);
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

struct Good* LoadGood(lua_State* _State, int _Index) {
	struct Good* _Good = NULL;
	const char* _Name = NULL;
	const char* _Temp = NULL;
	int _Categories = 0;
	int _Return = -2;

	lua_pushnil(_State);
	while(lua_next(_State, -2) != 0) {
		if(lua_isstring(_State, -2)) {
			if(!strcmp("Name", lua_tostring(_State, -2)))
				_Return = AddString(_State, -1, &_Name);
			else if(!strcmp("Categories", lua_tostring(_State, -2))) {

				lua_pushnil(_State);
				while(lua_next(_State, -2) != 0) {
					if(lua_isstring(_State, -1) == 1) {
						_Temp = lua_tostring(_State, -1);
					if(!strcmp("Food", _Temp))
						_Categories |= EFOOD;
					else if(!strcmp("Ingredient", _Temp))
						_Categories |= EINGREDIENT;
					else if(!strcmp("Animal", _Temp))
						_Categories |= EANIMAL;
					else if(!strcmp("Weapon", _Temp))
						_Categories |= EWEAPON;
					else if(!strcmp("Armor", _Temp))
						_Categories |= EARMOR;
					else if(!strcmp("Shield", _Temp))
						_Categories |= ESHIELD;
					else if(!strcmp("Seed", _Temp))
						_Categories |= ESEED;
					else if(!strcmp("Tool", _Temp))
						_Categories |= ETOOL;
					else if(!strcmp("Other", _Temp))
						_Categories |= EOTHER;
					}
					lua_pop(_State, 1);
				}
			}
		}
		lua_pop(_State, 1);
	}
	_Good = CreateGood(_Name, _Categories);
	lua_getfield(_State, -1, "InputGoods");
	lua_pushnil(_State);
	while(lua_next(_State, -2) != 0) {
		if(lua_isstring(_State, -2) == 1) {
			struct InputReq* _Req = CreateInputReq();
			_Temp = lua_tostring(_State, -2);
			if(Hash_Find(&g_Goods, _Temp, _Req->Req) == 1) {
				_Return = AddInteger(_State, -1, &_Req->Quantity);
				LnkLst_PushBack(&_Good->InputGoods, _Req);
			} else {
				DestroyInputReq(_Req);
				goto fail;
			}
		}
		lua_pop(_State, 1);
	}
	lua_pop(_State, 1);
	if(_Return > 0)
		return _Good;
	fail:
	DestroyGood(_Good);
	return NULL;
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
		lua_pop(_State, 1);
		if(!(_Return > 0))
			return NULL;
	}
	return CreateCrop(_Name, _PerAcre, _NutValue, _YieldMult, _StartMonth, _EndMonth);
}


struct Building* LoadBuilding(lua_State* _State, int _Index) {
	const char* _Key = NULL;
	const char* _Name = NULL;
	const char* _Temp = NULL;
	struct Good* _Output = NULL;
	int _Tax = 0;
	int _Throughput = 0;
	int _Return = 0;
	int _SquareFeet = 0;
	struct Building* _Building = NULL;
	struct LinkedList* _BuildMats = CreateLinkedList();
	struct LinkedList* _Animals = CreateLinkedList();
	struct LnkLst_Node* _Itr = NULL;

	lua_getmetatable(_State, _Index);
	lua_pushnil(_State);

	while(lua_next(_State, -2) != 0) {
		if(lua_isstring(_State, -2))
			_Key = lua_tostring(_State, -2);
		else
			continue;
		if(!strcmp("Output", _Key)) {
			_Return = AddString(_State, -1, &_Temp);
			if(Hash_Find(&g_Goods, _Temp, _Output) == 0)
				luaL_error(_State, "cannot find the good %s", _Temp);
		} else if (!strcmp("Tax", _Key))
			_Return = AddInteger(_State, -1, &_Tax);
		else if (!strcmp("Throughput", _Key))
			_Return = AddInteger(_State, -1, &_Throughput);
		else if (!strcmp("Name", _Key))
			_Return = AddString(_State, -1, &_Name);
		else if(!strcmp("SquareFeet", _Key))
			_Return = AddInteger(_State, -1, &_SquareFeet);
		else if(!strcmp("BuildMats", _Key)) {
			lua_pushnil(_State);
			while(lua_next(_State, -2) != 0) {
				if(lua_isstring(_State, -2)) {
					struct InputReq* _Good = CreateInputReq();
					int _Return = AddInteger(_State, -1, &_Good->Quantity);
					if(_Return == -1) {
						_Building = NULL;
						goto end;
					}
					if(Hash_Find(&g_Goods, lua_tostring(_State, -2), _Good->Req) == 0) {
						_Building = NULL;
						goto end;
					}
					LnkLst_PushBack(_BuildMats, _Good);
				}
				lua_pop(_State, 1);
			}
		} else if(!strcmp("Requires", _Key)) {
			lua_pushnil(_State);
			while(lua_next(_State, -2) != 0) {
				if(lua_isstring(_State, -2)) {
					struct InputReq* _Animal = CreateInputReq();
					struct Population* _Info = NULL;
					int _Return = AddInteger(_State, -1, &_Animal->Quantity);
					if(_Return == -1) {
						_Building = NULL;
						goto end;
					}
					if(Hash_Find(&g_Populations, lua_tostring(_State, -2), _Info) == 0) {
						_Building = NULL;
						goto end;
					}
					_Animal->Req = _Info;
					LnkLst_PushBack(_Animals, _Animal);
				}
				lua_pop(_State, 1);
			}
		}
	}
	if(_Return < 0) {
		_Building = NULL;
		goto end;
	}

	_Building = CreateBuilding(_Name, _Output, _Tax, _Throughput, _SquareFeet);
	_Itr = _BuildMats->Front;
	while(_Itr != NULL) {
		LnkLst_PushBack(&_Building->BuildMats, _Itr->Data);
		_Itr = _Itr->Next;
	}
	_Itr = _Animals->Front;
	while(_Itr != NULL) {
		LnkLst_PushBack(&_Building->Animals, _Itr->Data);
		_Itr = _Itr->Next;
	}
	end:
	DestroyLinkedList(_BuildMats);
	DestroyLinkedList(_Animals);
	return _Building;
}

struct Population* LoadPopulation(lua_State* _State, int _Index) {
	const char* _Key = 0;
	const char* _Name = 0;
	int _AdultAge = 0;
	int _AdultFood = 0;
	int _ChildFood = 0;
	int _Return = -2;

	lua_getmetatable(_State, _Index);
	lua_pushnil(_State);
	while(lua_next(_State, -2) != 0) {
		if(lua_isstring(_State, -2))
			_Key = lua_tostring(_State, -2);
		else
			continue;
		if(!strcmp("AdultAge", _Key))
			_Return = AddInteger(_State, -1, &_AdultAge);
		else if (!strcmp("AdultFood", _Key))
			_Return = AddInteger(_State, -1, &_AdultFood);
		else if (!strcmp("ChildFood", _Key))
			_Return = AddInteger(_State, -1, &_ChildFood);
		else if (!strcmp("Name", _Key))
			_Return = AddString(_State, -1, &_Name);
		if(!(_Return > 0))
			return NULL;
	}
	return CreatePopulation(_Name, _AdultFood, _ChildFood, _AdultAge);
}

struct Occupation* LoadOccupation(lua_State* _State, int _Index) {
	int _Return = 0;
	const char* _Key = NULL;
	const char* _Name = NULL;
	const char* _Temp = NULL;
	struct Good* _Output = NULL;
	struct Building* _Workplace = NULL;
	struct Constraint* _AgeConst = NULL;

	lua_getmetatable(_State, _Index);
	lua_pushnil(_State);
	while(lua_next(_State, -2) != 0) {
		if(lua_isstring(_State, -2))
			_Key = lua_tostring(_State, -2);
		else
			continue;
		if(!strcmp("Name", _Key))
			_Return = AddString(_State, -1, &_Name);
		else if(!strcmp("Output", _Key)) {
			_Return = AddString(_State, -1, &_Temp);
			if(Hash_Find(&g_Goods, _Temp, _Output) == 0)
				return NULL;
		} else if(!strcmp("Workplace", _Key)) {
			_Return = AddString(_State, -1, &_Temp);
			if(Hash_Find(&g_Buildings, _Temp, _Workplace) == 0)
				return NULL;
		} else if(!strcmp("AgeConst", _Key)) {
			if((_AgeConst = ConstraintFromLua(_State, -1)) == NULL)
				return NULL;
		}
		lua_pop(_State, 1);
		if(!(_Return > 0))
			return NULL;
	}
	return CreateOccupation(_Name, _Output, _Workplace, _AgeConst);
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
