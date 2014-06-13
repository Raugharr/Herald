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

struct Good* GoodLoad(lua_State* _State, int _Index) {
	struct Good* _Good = NULL;
	char* _Name = NULL;
	const char* _Temp = NULL;
	int _Category = 0;
	int _Return = -2;
	int _Top = lua_gettop(_State);

	lua_pushnil(_State);
	while(lua_next(_State, -2) != 0) {
		if(lua_isstring(_State, -2)) {
			if(!strcmp("Name", lua_tostring(_State, -2)))
				_Return = AddString(_State, -1, &_Name);
			else if(!strcmp("Category", lua_tostring(_State, -2))) {
					if(lua_isstring(_State, -1) == 1) {
						_Temp = lua_tostring(_State, -1);
					if(!strcmp("Food", _Temp))
						_Category = EFOOD;
					else if(!strcmp("Ingredient", _Temp))
						_Category = EINGREDIENT;
					else if(!strcmp("Animal", _Temp))
						_Category = EANIMAL;
					else if(!strcmp("Weapon", _Temp))
						_Category = EWEAPON;
					else if(!strcmp("Armor", _Temp))
						_Category = EARMOR;
					else if(!strcmp("Shield", _Temp))
						_Category = ESHIELD;
					else if(!strcmp("Seed", _Temp))
						_Category = ESEED;
					else if(!strcmp("Tool", _Temp))
						_Category = ETOOL;
					else if(!strcmp("Other", _Temp))
						_Category = EOTHER;
					else _Return = -1;
					}
				if(_Category <= 0 && _Return <= 0) {
					printf("%s is not a valid category.", _Temp);
					goto fail;
				}
			}
		}
		lua_pop(_State, 1);
	}
	if(_Name == NULL)
		goto fail;
	_Good = CreateGood(_Name, _Category);
	_Top = lua_gettop(_State);
	if(_Return > 0)
		return _Good;
	fail:
	if(_Good != NULL)
		DestroyGood(_Good);
	lua_settop(_State, _Top);
	return NULL;
}

int GoodLoadInput(lua_State* _State, int _Index, struct Good* _Good) {
	const char* _Name = NULL;
	int _Top = lua_gettop(_State);
	struct InputReq* _Req = NULL;

	if(_Good == NULL)
		return 0;

	lua_getfield(_State, -1, "InputGoods");
	lua_pushnil(_State);
	while(lua_next(_State, -2) != 0) {
		lua_pushnil(_State);
		while(lua_next(_State, -2) != 0) {
			if(lua_isstring(_State, -2) == 1) {
				_Req = CreateInputReq();
				_Name = lua_tostring(_State, -1);
				if(Hash_Find(&g_Goods, _Name, (void**)&_Req->Req) == 1) {
					if(AddInteger(_State, -2, &_Req->Quantity) == -1) {
						goto fail;
						DestroyInputReq(_Req);
						DestroyGood(_Good);
						return 0;
					}
					LnkLst_PushBack(&_Good->InputGoods, _Req);
				} else {
					goto fail;
				}
			}
			lua_pop(_State, 1);
		}
		lua_pop(_State, 1);
	}
	lua_pop(_State, 1);
	return 1;
	fail:
	DestroyInputReq(_Req);
	DestroyGood(_Good);
	lua_settop(_State, _Top);
	return 0;
}

struct Crop* CropLoad(lua_State* _State, int _Index) {
	char* _Name = NULL;
	const char* _Key = NULL;
	int _PerAcre = 0;
	double _YieldMult = 0;
	int _NutValue = 0;
	int _GrowTime = 0;
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
		else if (!strcmp("GrowTime", _Key)) {
			_Return = AddInteger(_State, -1, &_GrowTime);
		}
		lua_pop(_State, 1);
		if(!(_Return > 0))
			return NULL;
	}
	return CreateCrop(_Name, _PerAcre, _NutValue, _YieldMult, _GrowTime);
}

struct Building* BuildingLoad(lua_State* _State, int _Index) {
	const char* _Key = NULL;
	char* _Name = NULL;
	char* _Temp = NULL;
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
			if(Hash_Find(&g_Goods, _Temp, (void**)&_Output) == 0)
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
			if(lua_istable(_State, -1) == 0) {
				printf("Input for good is not a table");
				goto end;
			}
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
					if(Hash_Find(&g_Populations, lua_tostring(_State, -2), (void**)&_Info) == 0) {
						_Building = NULL;
						goto end;
					}
					_Animal->Req = _Info;
					LnkLst_PushBack(_Animals, _Animal);
				}
			}
		}
		lua_pop(_State, 1);
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

struct Population* PopulationLoad(lua_State* _State, int _Index) {
	const char* _Key = NULL;
	char* _Name = NULL;
	int _AdultAge = 0;
	int _AdultFood = 0;
	int _ChildFood = 0;
	int _Return = -2;
	int _Top = lua_gettop(_State);

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
		if(!(_Return > 0)) {
			lua_settop(_State, _Top);
			return NULL;
		}
		lua_pop(_State, 1);
	}
	return CreatePopulation(_Name, _AdultFood, _ChildFood, _AdultAge);
}

struct Occupation* OccupationLoad(lua_State* _State, int _Index) {
	int _Return = 0;
	const char* _Key = NULL;
	char* _Name = NULL;
	char* _Temp = NULL;
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
			if(Hash_Find(&g_Goods, _Temp, (void**)&_Output) == 0)
				return NULL;
		} else if(!strcmp("Workplace", _Key)) {
			_Return = AddString(_State, -1, &_Temp);
			if(Hash_Find(&g_Buildings, _Temp, (void**)&_Workplace) == 0)
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

	RBIterate(&g_PregTree, (int(*)(void*))Pregancy_Update);
	while(_Itr != NULL) {
		if(Manor_Update(_Itr->Data) == 0)
			return 0;
		_Itr = _Itr->Next;
	}
	if(World_Tick() == 0)
		return 0;
	return 1;
}
