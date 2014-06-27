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

int GoodDepSCallback(const struct GoodBase* _Good, const struct GoodDep* _Pair) {
	return _Good->Id - _Pair->Good->Id;
}

struct GoodBase* CreateGoodBase(const char* _Name, int _Category) {
	struct GoodBase* _Good = (struct GoodBase*) malloc(sizeof(struct GoodBase));

	_Good->Name = (char*) malloc(sizeof(char) * strlen(_Name) + 1);
	strcpy(_Good->Name, _Name);
	_Good->Category = _Category;
	_Good->Id = NextId();
	_Good->InputGoods = NULL;
	_Good->IGSize = 0;
	return _Good;
}

struct GoodBase* CopyGoodBase(const struct GoodBase* _Good) {
	int i;
	struct GoodBase* _NewGood = (struct GoodBase*) malloc(sizeof(struct GoodBase));

	_NewGood->Name = (char*) malloc(sizeof(char) * strlen(_Good->Name) + 1);
	_NewGood->Category = _Good->Category;
	_NewGood->Id = _Good->Id;
	strcpy(_NewGood->Name, _Good->Name);
	_NewGood->InputGoods = calloc(_Good->IGSize, sizeof(struct InputReq*));
	_NewGood->IGSize = _Good->IGSize;
	for(i = 0; i < _Good->IGSize; ++i) {
		struct InputReq* _Req = CreateInputReq();
		_Req->Req = ((struct InputReq*)_Good->InputGoods[i])->Req;
		_Req->Quantity = ((struct InputReq*)_Good->InputGoods[i])->Quantity;
		_NewGood->InputGoods[i] = _Req;
	}
	return _NewGood;
}

void DestroyGoodBase(struct GoodBase* _Good) {
	int i;

	for(i = 0; i < _Good->IGSize; ++i)
		DestroyInputReq(_Good->InputGoods[i]);
	free(_Good->Name);
	free(_Good->InputGoods);
	free(_Good);
}

struct GoodBase* GoodLoad(lua_State* _State, int _Index) {
	struct GoodBase* _Good = NULL;
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
					else if(!strcmp("Seed", _Temp))
						_Category = ESEED;
					else if(!strcmp("Tool", _Temp))
						_Category = ETOOL;
					else if(!strcmp("Material", _Temp))
						_Category = EMATERIAL;
					else if(!strcmp("Clothing", _Temp))
						_Category = ECLOTHING;
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
	_Good = CreateGoodBase(_Name, _Category);
	_Top = lua_gettop(_State);
	if(_Return > 0)
		return _Good;
	fail:
	if(_Good != NULL)
		DestroyGoodBase(_Good);
	lua_settop(_State, _Top);
	return NULL;
}

int GoodLoadInput(lua_State* _State, int _Index, struct GoodBase* _Good) {
	const char* _Name = NULL;
	int _Top = lua_gettop(_State);
	int i;
	struct InputReq* _Req = NULL;
	struct LinkedList* _List = CreateLinkedList();
	struct LnkLst_Node* _Itr = NULL;

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
				}
				LnkLst_PushBack(_List, _Req);
			} else {
				goto fail;
			}
			lua_pop(_State, 2);
		}
		lua_pop(_State, 1);
	}
	lua_pop(_State, 1);
	_Good->IGSize = _List->Size;
	_Good->InputGoods = calloc(_Good->IGSize, sizeof(struct InputReq*));
	_Itr = _List->Front;
	for(i = 0; i < _List->Size; ++i) {
		_Good->InputGoods[i] = (struct InputReq*)_Itr->Data;
		_Itr = _Itr->Next;
	}
	DestroyLinkedList(_List);
	return 1;
	fail:
	DestroyInputReq(_Req);
	DestroyGoodBase(_Good);
	lua_settop(_State, _Top);
	DestroyLinkedList(_List);
	return 0;
}

struct GoodDep* CreateGoodDep(const struct GoodBase* _Good) {
	struct GoodDep* _GoodDep = (struct GoodDep*) malloc(sizeof(struct GoodDep));

	_GoodDep->DepTbl = CreateArray(5);
	_GoodDep->Good = _Good;
	return _GoodDep;
}

void DestroyGoodDep(struct GoodDep* _GoodDep) {
	DestroyArray(_GoodDep->DepTbl);
	free(_GoodDep);
}

struct Good* CreateGood(struct GoodBase* _Base) {
	struct Good* _Good = (struct Good*) malloc(sizeof(struct Good));

	_Good->Base = _Base;
	_Good->Quantity = 0;
	return _Good;
}

void DestroyGood(struct Good* _Good) {
	free(_Good);
}

struct Tool* CreateTool(struct GoodBase* _Base, int _Function) {
	struct Tool* _Tool = (struct Tool*) malloc(sizeof(struct Tool));

	_Tool = (struct Tool*) CreateGood(_Base);
	_Tool->Function = _Function;
	return _Tool;
}

void DestroyTool(struct Tool* _Tool) {
	free(_Tool);
}

struct RBTree* GoodBuildDep(const struct HashTable* _GoodList) {
	struct HashItrCons* _Itr = NULL;
	struct RBTree* _Prereq = CreateRBTree((int(*)(const void*, const void*))&GoodDepICallback, (int(*)(const void*, const void*))&GoodDepSCallback);

	_Itr = HashCreateItrCons(_GoodList);
	while(_Itr != NULL) {
		RBInsert(_Prereq, GoodDependencies(_Prereq, ((const struct GoodBase*)_Itr->Node->Pair)));
		_Itr = HashNextCons(_GoodList, _Itr);
	}
	return _Prereq;
}

struct GoodDep* GoodDependencies(struct RBTree* _Tree, const struct GoodBase* _Good) {
	struct GoodDep* _Pair = NULL;
	struct GoodDep* _PairItr = NULL;
	int i;

	if((_Pair = RBSearch(_Tree, _Good)) == NULL) {
		_Pair = CreateGoodDep(_Good);
		for(i = 0; i < _Good->IGSize; ++i) {
			if((_PairItr = RBSearch(_Tree, ((struct GoodBase*)_Good->InputGoods[i]))) == NULL)
				_PairItr = GoodDependencies(_Tree, ((struct GoodBase*)_Good->InputGoods[i]->Req));
			if(ArrayInsert(_PairItr->DepTbl, _Pair) == 0) {
				ArrayResize(_PairItr->DepTbl);
				ArrayInsert(_PairItr->DepTbl, _PairItr);
			}
		}
	}
	return _Pair;
}
