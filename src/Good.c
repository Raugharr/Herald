/*
 * File: Good.c
 * Author: David Brotz
 */

#include "Good.h"

#include "Herald.h"
#include "sys/LuaHelper.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <lua/lua.h>

static int g_GoodId = 0;

struct Good* CreateGood(const char* _Name, int _Category) {
	struct Good* _Good = (struct Good*) malloc(sizeof(struct Good));

	_Good->Name = (char*) malloc(sizeof(char) * strlen(_Name) + 1);
	_Good->Category = _Category;
	_Good->Quantity = 0;
	_Good->Id = ++g_GoodId;
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

