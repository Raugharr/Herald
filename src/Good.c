/*
 * File: Good.c
 * Author: David Brotz
 */

#include "Good.h"

#include "Herald.h"
#include "Log.h"
#include "Crop.h"
#include "sys/LuaHelper.h"
#include "sys/Array.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <lua/lua.h>

int GoodDepICallback(struct GoodDep* _One, struct GoodDep* _Two) {
	return _One->Good->Id - _Two->Good->Id;
}

int GoodDepSCallback(struct Good* _Good, struct GoodDep* _Pair) {
	return _Good->Id - _Pair->Good->Id;
}

struct Good* CreateGood(const char* _Name, int _Category) {
	struct Good* _Good = (struct Good*) malloc(sizeof(struct Good));

	_Good->Name = (char*) malloc(sizeof(char) * strlen(_Name) + 1);
	_Good->Category = _Category;
	_Good->Quantity = 0;
	_Good->Id = NextId();
	_Good->InputGoods.Size = 0;
	_Good->InputGoods.Front = NULL;
	_Good->InputGoods.Back = NULL;
	strcpy(_Good->Name, _Name);
	return _Good;
}

struct Good* CopyGood(const struct Good* _Good) {
	struct Good* _NewGood = (struct Good*) malloc(sizeof(struct Good));
	struct LnkLst_Node* _Itr = _Good->InputGoods.Front;

	_NewGood->Name = (char*) malloc(sizeof(char) * strlen(_Good->Name) + 1);
	_NewGood->Category = _Good->Category;
	_NewGood->Quantity = _Good->Quantity;
	_NewGood->Id = _Good->Id;
	strcpy(_NewGood->Name, _Good->Name);
	while(_Itr != NULL) {
		struct InputReq* _Req = CreateInputReq();
		_Req->Req = ((struct InputReq*)_Itr->Data)->Req;
		_Req->Quantity = ((struct InputReq*)_Itr->Data)->Quantity;
		LnkLst_PushBack(&_NewGood->InputGoods, _Req);
		_Itr = _Itr->Next;
	}
	return _NewGood;
}

void DestroyGood(struct Good* _Good) {
	struct LnkLst_Node* _Itr = _Good->InputGoods.Front;

	while(_Itr != NULL) {
		DestroyInputReq(_Itr->Data);
		_Itr = _Itr->Next;
	}
	free(_Good->Name);
	free(_Good);
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

struct GoodDep* CreateGoodDep(const struct Good* _Good) {
	struct GoodDep* _GoodDep = (struct GoodDep*) malloc(sizeof(struct GoodDep));

	_GoodDep->DepTbl = CreateArray(5);
	_GoodDep->Reachable = 0;
	_GoodDep->Good = _Good;
	return _GoodDep;
}
void DestroyGoodDep(struct GoodDep* _GoodDep) {
	DestroyArray(_GoodDep->DepTbl);
	free(_GoodDep);
}

struct RBTree* GoodBuildDep(const struct LinkedList* _CropList, const struct HashTable* _GoodList) {
	struct LnkLst_Node* _Itr = NULL;
	struct GoodDep* _Pair = NULL;
	struct RBTree* _Prereq = CreateRBTree((int(*)(void*, void*))&GoodDepICallback, (int(*)(void*, void*))&GoodDepSCallback);
	struct Good* _Good = NULL;

	_Itr = _CropList->Front;
	while(_Itr != NULL) {
		Hash_Find(_GoodList, ((struct Crop*)_Itr->Data)->Name, (void**)&_Good);
		if(_Good == NULL) {
			return NULL;
		}
		_Pair = CreateGoodDep(_Good);
		_Pair->Reachable = 1;
		RBInsert(_Prereq, _Pair);
		_Itr = _Itr->Next;
	}
	return _Prereq;
}

struct GoodDep* GoodDependencies(struct RBTree* _Tree, struct Good* _Good) {
	struct GoodDep* _Pair = NULL;
	struct GoodDep* _PairItr = NULL;
	struct LnkLst_Node* _Itr = NULL;

	if(_Good->Category != EFOOD && _Good->Category != EINGREDIENT)
		return NULL;
	if((_Pair = RBSearch(_Tree, _Good)) == NULL) {
		_Pair = CreateGoodDep(_Good);
		_Itr = _Good->InputGoods.Front;

		_Pair = CreateGoodDep(_Good);
		while(_Itr != NULL) {
			if((_PairItr = RBSearch(_Tree, (struct Good*)_Itr->Data)) == NULL)
				_PairItr = GoodDependencies(_Tree, (struct Good*)_Itr->Data);
			if(ArrayInsert(_Pair->DepTbl, _PairItr) == 0)
				ArrayResize(_Pair->DepTbl);
			ArrayInsert(_Pair->DepTbl, _PairItr);
		}
	}
	return _Pair;
}
