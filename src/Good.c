/*
 * File: Good.c
 * Author: David Brotz
 */

#include "Good.h"

#include "Herald.h"
#include "World.h"
#include "Crop.h"
#include "sys/Log.h"
#include "sys/LuaHelper.h"
#include "sys/Array.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <lua/lua.h>

int GoodDepCmp(const struct GoodDep* _One, const struct GoodDep* _Two) {
	return _One->Good->Id - _Two->Good->Id;
}

int GoodBaseDepCmp(const struct GoodBase* _Good, const struct GoodDep* _Pair) {
	return _Good->Id - _Pair->Good->Id;
}

int InputReqGoodCmp(const struct InputReq* _One, const struct Good* _Two) {
	return ((struct GoodBase*)_One->Req)->Id - _Two->Base->Id;
}

struct GoodBase* InitGoodBase(struct GoodBase* _Good, const char* _Name, int _Category) {
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

int GoodBaseCmp(const void* _One, const void* _Two) {
	return ((struct GoodBase*)_One)->Id - ((struct GoodBase*)_Two)->Id;
}

void DestroyGoodBase(struct GoodBase* _Good) {
	int i;

	for(i = 0; i < _Good->IGSize; ++i)
		DestroyInputReq(_Good->InputGoods[i]);
	free(_Good->Name);
	free(_Good->InputGoods);
	free(_Good);
}

int GoodInpGdCmp(const void* _One, const void* _Two) {
	return ((struct GoodBase*)((struct InputReq*)_One)->Req)->Id - ((struct GoodBase*)((struct InputReq*)_Two)->Req)->Id;
}

int GoodThink(struct Good* _Good) {
	return 1;
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
	if(_Category == ETOOL) {
		int _Function = 0;

		lua_getfield(_State, -1, "Function");
		if(AddString(_State, -1, &_Temp) == 0) {
			Log(ELOG_WARNING, "Tool requires Function field.");
			goto fail;
		}
		if(!strcmp(_Temp, "Plow"))
			_Function = ETOOL_PLOW;
		else if(!strcmp(_Temp, "Reap"))
			_Function = ETOOL_REAP;
		else if(!strcmp(_Temp, "Cut"))
			_Function = ETOOL_CUT;
		else if(!strcmp(_Temp, "Logging"))
			_Function = ETOOL_LOGGING;
		else {
			Log(ELOG_WARNING, "Tool contains an invalid Function field: %s", _Temp);
			goto fail;
		}
		lua_pop(_State, 1);
		_Good = (struct GoodBase*) CreateToolBase(_Name, _Category, _Function);
	} else if(_Category == EFOOD || _Category == EINGREDIENT || _Category == ESEED) {
		_Good = (struct GoodBase*) CreateFoodBase(_Name, _Category, 0);
	} else
		_Good = InitGoodBase((struct GoodBase*)malloc(sizeof(struct GoodBase)), _Name, _Category);
	if(_Return > 0)
		return _Good;
	fail:
	if(_Good != NULL)
		DestroyGoodBase(_Good);
	lua_settop(_State, _Top);
	return NULL;
}

int GoodLoadInput(lua_State* _State, struct GoodBase* _Good) {
	const char* _Name = NULL;
	int _Top = lua_gettop(_State);
	int i;
	struct InputReq* _Req = NULL;
	struct LinkedList* _List = CreateLinkedList();
	struct LnkLst_Node* _Itr = NULL;

	if(_Good == NULL)
		return 0;
	if(HashSearch(&g_Crops, _Good->Name) != NULL || _Good->InputGoods != NULL || _Good->IGSize > 0)
		return 1;

	lua_getglobal(_State, "Goods");
	lua_pushstring(_State, _Good->Name);
	lua_rawget(_State, -2);
	lua_remove(_State, -2);
	lua_pushstring(_State, "InputGoods");
	lua_rawget(_State, -2);
	lua_pushnil(_State);
	while(lua_next(_State, -2) != 0) {
		lua_pushnil(_State);
		if(lua_next(_State, -2) == 0)
			goto fail;
		if(lua_isstring(_State, -1) == 1) {
			_Req = CreateInputReq();
			_Name = lua_tostring(_State, -1);
			if((_Req->Req = HashSearch(&g_Goods, _Name)) != NULL) {
				lua_pop(_State, 1);
				if(lua_next(_State, -2) == 0)
					goto fail;
				if(AddNumber(_State, -1, &_Req->Quantity) == -1) {
					goto fail;
				}
				LnkLst_PushBack(_List, _Req);
			} else {
				Log(ELOG_WARNING, "Good %s cannot add %s as an input good: %s does not exist.", _Name, _Good->Name, _Name);
				goto fail;
			}
			lua_pop(_State, 2);
		} else
			lua_pop(_State, 2);
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
	if(_Good->Category == EFOOD || _Good->Category == EINGREDIENT || _Good->Category == ESEED) {
		int _Nutrition = 0;

		if(_Good->IGSize == 0) {
			lua_getfield(_State, -1, "Nutrition");
			if(lua_tointeger(_State, -1) == 0) {
				Log(ELOG_WARNING, "Warning: Good %s cannot be a food because it contains no InputGoods or a field named Nutrition containing an integer.");
				goto fail;
			}
			_Nutrition = lua_tointeger(_State, -1);
			lua_pop(_State, 1);
		} else {
			for(i = 0; i < _List->Size; ++i) {
				GoodLoadInput(_State, ((struct GoodBase*)_Good->InputGoods[i]->Req));
				_Nutrition += GoodNutVal((struct GoodBase*)_Good);
			}
		}
		((struct FoodBase*)_Good)->Nutrition = _Nutrition;
	}
	lua_pop(_State, 1);
	InsertionSort(_Good->InputGoods, _Good->IGSize, GoodInpGdCmp);
	DestroyLinkedList(_List);
	return 1;
	fail:
	DestroyInputReq(_Req);
	lua_settop(_State, _Top);
	DestroyLinkedList(_List);
	HashDelete(&g_Goods, _Good->Name);
	DestroyGoodBase(_Good);
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

struct Good* CreateGood(const struct GoodBase* _Base, int _X, int _Y) {
	struct Good* _Good = (struct Good*) malloc(sizeof(struct Good));

	CreateObject((struct Object*)_Good, _X, _Y, (int(*)(struct Object*))GoodThink);
	_Good->Base = _Base;
	_Good->Quantity = 0;
	return _Good;
}

int GoodCmp(const void* _One, const void* _Two) {
	return ((struct Good*)_One)->Id - ((struct Good*)_Two)->Id;
}

void DestroyGood(struct Good* _Good) {
	free(_Good);
}

int GoodGBaseCmp(const struct Good* _One, const struct GoodBase* _Two) {
	return _One->Base->Id - _Two->Id;
}

int GoodBaseGoodCmp(const struct GoodBase* _One, const struct Good* _Two) {
	return _One->Id - _Two->Base->Id;
}

struct ToolBase* CreateToolBase(const char* _Name, int _Category, int _Function) {
	struct ToolBase* _Tool = (struct ToolBase*) InitGoodBase((struct GoodBase*)malloc(sizeof(struct ToolBase)), _Name, _Category);

	_Tool->Function = _Function;
	return _Tool;
}

void DestroyToolBase(struct ToolBase* _Tool) {
	free(_Tool);
}

struct FoodBase* CreateFoodBase(const char* _Name, int _Category, int _Nutrition) {
	struct FoodBase* _Food = (struct FoodBase*) InitGoodBase((struct GoodBase*) malloc(sizeof(struct FoodBase)), _Name, _Category);

	_Food->Nutrition = _Nutrition;
	return _Food;
}

void DestroyFoodBase(struct FoodBase* _Food) {
	free(_Food);
}

struct Food* CreateFood(const struct FoodBase* _Base, int _X, int _Y) {
	struct Food* _Food = (struct Food*) malloc(sizeof(struct Food));
	
	CreateObject((struct Object*)_Food, _X, _Y, (int(*)(struct Object*))GoodThink);
	_Food->Base = _Base;
	_Food->Quantity = 0;
	_Food->Parts = FOOD_MAXPARTS;
	return _Food;
}

void DestroyFood(struct Food* _Food) {
	free(_Food);
}

struct RBTree* GoodBuildDep(const struct HashTable* _GoodList) {
	struct HashItrCons* _Itr = NULL;
	struct RBTree* _Prereq = CreateRBTree((int(*)(const void*, const void*))&GoodDepCmp, (int(*)(const void*, const void*))&GoodBaseDepCmp);
	struct GoodDep* _Dep = NULL;

	_Itr = HashCreateItrCons(_GoodList);
	while(_Itr != NULL) {
		_Dep = GoodDependencies(_Prereq, ((const struct GoodBase*)_Itr->Node->Pair)); //Fails when _Itr->Key == "Shear". _Itr->Pair is invalid.
		if(RBSearch(_Prereq, _Dep->Good) == NULL)
			RBInsert(_Prereq, _Dep);
		_Itr = HashNextCons(_GoodList, _Itr);
	}
	HashDeleteItrCons(_Itr);
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
			RBInsert(_Tree, _PairItr);
		}
	}
	return _Pair;
}

int GoodNutVal(struct GoodBase* _Base) {
	struct Crop* _Crop = NULL;
	int _Nut = 0;
	int i;

	for(i = 0; i < _Base->IGSize; ++i) {
		if(((struct GoodBase*)_Base->InputGoods[i]->Req)->Category == ESEED) {
			if((_Crop = HashSearch(&g_Crops, ((struct GoodBase*)_Base->InputGoods[i]->Req)->Name)) == NULL) {
				Log(ELOG_WARNING, "Crop %s not found.", ((struct GoodBase*)_Base->InputGoods[i]->Req)->Name);
				continue;
			}
			_Nut += _Crop->NutVal / OUNCE * _Base->InputGoods[i]->Quantity;
		} else
			_Nut += ((struct FoodBase*)_Base->InputGoods[i]->Req)->Nutrition * _Base->InputGoods[i]->Quantity;
	}
	return _Nut;
}

struct InputReq** GoodBuildList(const struct Array* _Goods, int* _Size, int _Categories) {
	int i;
	int j;
	int _TblSize = _Goods->Size;
	int _Amount = 0;
	void** _Tbl = _Goods->Table;
	int _OutputSize = 0;
	struct GoodDep* _Dep = NULL;
	struct InputReq** _Outputs = (struct InputReq**)calloc(_TblSize, sizeof(struct InputReq*));
	const struct GoodBase* _Output = NULL;
	struct Good* _Good = NULL;

	memset(_Outputs, 0, sizeof(struct InputReq*) * _TblSize);
	for(i = 0; i < _TblSize; ++i) {
		_Good = (struct Good*)_Tbl[i];
		if((_Categories &_Good->Base->Category) != _Good->Base->Category && _Good->Quantity > 0)
			continue;
		_Dep = RBSearch(g_GoodDeps, _Good->Base);
		for(j = 0; j < _Dep->DepTbl->Size; ++j) {
			_Output = ((struct GoodDep*)_Dep->DepTbl->Table[j])->Good;
			if((_Amount = GoodCanMake(_Output, _Goods)) != 0) {
				struct InputReq* _Temp = CreateInputReq();

				_Temp->Req = (void*)_Output;
				_Temp->Quantity = _Amount;
				_Outputs[_OutputSize++] = _Temp;
			}
		}
	}

	if(_Size)
		*_Size = _OutputSize;
	_Outputs = realloc(_Outputs, sizeof(struct InputReq*) * _OutputSize);
	return _Outputs;
}

int GoodCanMake(const struct GoodBase* _Good, const struct Array* _Goods) {
	int i;
	int _Max = INT_MAX;
	int _Quantity = 0;
	struct Good** _Tbl = (struct Good**) _Goods->Table;
	struct Good* _Temp = NULL;
	
	for(i = 0; i < _Good->IGSize; ++i) {
		if((_Temp = LinearSearch(_Good->InputGoods[i], _Tbl, _Goods->Size, (int(*)(const void*, const void*))InputReqGoodCmp)) == NULL
				|| (_Temp->Quantity < _Good->InputGoods[i]->Quantity))
			return 0;
		_Quantity = _Temp->Quantity / _Good->InputGoods[i]->Quantity;
		if(_Quantity < _Max)
			_Max = _Quantity;
	}
	return _Max;
}
