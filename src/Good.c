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

int GoodDepICallback(const struct GoodDep* _One, const struct GoodDep* _Two) {
	return _One->Good->Id - _Two->Good->Id;
}

int GoodDepSCallback(const struct Good* _Good, const struct GoodDep* _Pair) {
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
	_NewGood->InputGoods.Size = 0;
	_NewGood->InputGoods.Front = NULL;
	_NewGood->InputGoods.Back = NULL;
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
	const char* _Name = NULL;
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
					else if(!strcmp("Material", _Temp))
						_Category = EMATERIAL;
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
	//lua_pop(_State, 1);
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
		if(lua_next(_State, -2) == 0)
			goto fail;
		if(lua_isstring(_State, -2) == 1) {
			_Req = CreateInputReq();
			_Name = lua_tostring(_State, -1);
			if((_Req->Req = HashSearch(&g_Goods, _Name)) != NULL) {
				lua_pop(_State, 1);
				if(lua_next(_State, -2) == 0)
					goto fail;
				if(AddInteger(_State, -1, &_Req->Quantity) == -1) {
					goto fail;
					DestroyInputReq(_Req);
					DestroyGood(_Good);
					return 0;
				}
				LnkLst_PushBack(&_Good->InputGoods, _Req);
			} else {
				goto fail;
			}
			lua_pop(_State, 2);
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
	_GoodDep->Good = _Good;
	return _GoodDep;
}
void DestroyGoodDep(struct GoodDep* _GoodDep) {
	DestroyArray(_GoodDep->DepTbl);
	free(_GoodDep);
}

struct RBTree* GoodBuildDep(const struct HashTable* _GoodList) {
	struct HashItrCons* _Itr = NULL;
	struct RBTree* _Prereq = CreateRBTree((int(*)(const void*, const void*))&GoodDepICallback, (int(*)(const void*, const void*))&GoodDepSCallback);

	_Itr = HashCreateItrCons(_GoodList);
	while(_Itr != NULL) {
		RBInsert(_Prereq, GoodDependencies(_Prereq, ((const struct Good*)_Itr->Node->Pair)));
		_Itr = HashNextCons(_GoodList, _Itr);
	}
	return _Prereq;
}

struct GoodDep* GoodDependencies(struct RBTree* _Tree, const struct Good* _Good) {
	struct GoodDep* _Pair = NULL;
	struct GoodDep* _PairItr = NULL;
	struct LnkLst_Node* _Itr = NULL;

	if((_Pair = RBSearch(_Tree, _Good)) == NULL) {
		_Pair = CreateGoodDep(_Good);
		_Itr = _Good->InputGoods.Front;

		while(_Itr != NULL) {
			if((_PairItr = RBSearch(_Tree, (struct Good*)((struct InputReq*)_Itr->Data)->Req)) == NULL)
				_PairItr = GoodDependencies(_Tree, (struct Good*)((struct InputReq*)_Itr->Data)->Req);
			if(ArrayInsert(_PairItr->DepTbl, _Pair) == 0) {
				ArrayResize(_PairItr->DepTbl);
				ArrayInsert(_PairItr->DepTbl, _PairItr);
			}
			_Itr = _Itr->Next;
		}
	}
	return _Pair;
}
