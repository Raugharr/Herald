/*
 * Author: David Brotz
 * File: Family.c
 */

#include "Family.h"

#include "Person.h"
#include "Herald.h"
#include "Crop.h"
#include "Good.h"
#include "Population.h"
#include "sys/Array.h"
#include "sys/Constraint.h"
#include "sys/Random.h"
#include "sys/LuaHelper.h"
#include "sys/Log.h"
#include "sys/RBTree.h"
#include "AI/LuaLib.h"
#include "AI/Setup.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <lua/lauxlib.h>

struct Constraint** g_AgeDistr = NULL;
static struct Array* g_FirstNames = NULL;

void Family_Init(struct Array* _Array) {
	g_AgeDistr = CreateConstrntBnds(19, 0, 737, 1464, 2157, 2867, 3632, 4489, 5368, 6162, 6870, 7472, 7885, 8317, 8744, 9150, 9471, 9717, 9875, 9999);
	g_FirstNames = _Array;
}

void Family_Quit() {
	DestroyConstrntBnds(g_AgeDistr);
	DestroyArray(g_FirstNames);
}

struct Family* CreateFamily(const char* _Name, struct Person* _Husband, struct Person* _Wife, struct Person** _Children, int _ChildrenSize) {
	struct Family* _Family = (struct Family*) malloc(sizeof(struct Family));
	int i;

	if(_ChildrenSize >= CHILDREN_SIZE) {
		DestroyFamily(_Family);
		return NULL;
	}

	_Family->Name = _Name;
	_Family->Id = NextId();
	memset(_Family->People, 0, sizeof(struct Person*) * (CHILDREN_SIZE + 2));
	_Family->People[HUSBAND] = _Husband;
	_Husband->Family = _Family;
	_Family->People[WIFE] = _Wife;
	_Wife->Family = _Family;//If _Female is last of its family the family's name will be a memory leak.
	_Family->NumChildren = 0;
	for(i = 0; i < _ChildrenSize; ++i)
		_Family->People[CHILDREN + i] = _Children[i];
	_Family->Field = NULL;
	_Family->Buildings = CreateArray(2);
	_Family->Goods = CreateArray(4);
	_Family->Animals = CreateArray(4);
	return _Family;
}

struct Family* CreateRandFamily(const char* _Name, int _Size, struct Constraint** _AgeGroups, struct Constraint** _BabyAvg, int _X, int _Y) {
	struct Family* _Family = NULL;

	if(_Size > CHILDREN_SIZE + 2)
		return NULL;

	if(_Size >= 2) {
		struct Person* _Husband = CreatePerson(g_FirstNames->Table[Random(0, g_FirstNames->Size)], Random(_AgeGroups[TEENAGER]->Min, _AgeGroups[ADULT]->Max), EMALE, 1500, 0, 0);
		struct Person* _Wife = CreatePerson(g_FirstNames->Table[Random(0, g_FirstNames->Size)], Random(_AgeGroups[TEENAGER]->Min, _AgeGroups[ADULT]->Max), EFEMALE, 1500, 0, 0);
		_Family = CreateFamily(_Name, _Husband, _Wife, NULL, 0);
		_Size -= 2;

		while(_Size-- > 0) {
			int _Child = CHILDREN + _Family->NumChildren;
			_Family->People[_Child] = CreatePerson(g_FirstNames->Table[Random(0, g_FirstNames->Size)], Fuzify(g_AgeDistr, Random(0, 9999)), Random(1, 2), 1500, 0, 0);
			_Family->People[_Child]->Family = _Family;
			++_Family->NumChildren;
		}
	}
	return _Family;
}

void DestroyFamily(struct Family* _Family) {
	int _Max = _Family->NumChildren + 1;
	int i;
	struct Array* _Array = _Family->Goods;

	for(i = 0; i < _Array->Size; ++i) {
		DestroyGood(_Array->Table[i]);
	}
	while(_Max >= 0) {
		if(_Family->People[_Max] != NULL) {
			DestroyPerson(_Family->People[_Max]);
			_Family->People[_Max] = NULL;
		}
		--_Max;
	}
	DestroyField(_Family->Field);
	DestroyArray(_Family->Buildings);
	DestroyArray(_Family->Goods);
	//DestroyArray(_Family->Animals);
	free(_Family);
}

int FamilySize(struct Family* _Family) {
	int _Size = 0;
	int i;

	for(i = 0; i < CHILDREN_SIZE + 2; ++i) {
		if(_Family->People[i] == NULL)
			continue;
		++_Size;
	}
	return _Size;
}

void Marry(struct Person* _Male, struct Person* _Female) {
	assert(_Male->Gender == EMALE && _Female->Gender == EFEMALE);
	CreateFamily(_Male->Family->Name, _Male, _Female, NULL, 0);
}

void FamilyAddGoods(struct Family* _Family, lua_State* _State, struct FamilyType** _FamilyTypes, int _X, int _Y) {
	int i;
	int j;
	int _FamType = Random(0, 9999);
	int _Quantity = 0;
	int _Percent = 0;
	const char* _Name = NULL;
	const struct Population* _Population = NULL;
	const struct LuaBehavior* _Behavior = NULL;
	struct LuaBehavior _Cmp;
	char* _Error = NULL;
	void* _Obj = NULL;

	for(i = 0; _FamilyTypes[i] != NULL; ++i) {
		if(_FamilyTypes[i]->Percent * 10000 > _FamType + _Percent) {
			lua_getglobal(_State, _FamilyTypes[i]->LuaFunc);
			lua_pushinteger(_State, FamilySize(_Family));
			if(LuaCallFunc(_State, 1, 1, 0) == 0)
				return;
			lua_getfield(_State, -1, "Goods");
			lua_pushnil(_State);
			while(lua_next(_State, -2) != 0) {
				luaL_checktype(_State, -1, LUA_TLIGHTUSERDATA);
				_Obj = lua_touserdata(_State, -1);
				((struct Good*)_Obj)->X = _X;
				((struct Good*)_Obj)->Y = _Y;
				ArrayInsertSort_S(_Family->Goods, _Obj, GoodCmp);
				lua_pop(_State, 1);
			}
			lua_pop(_State, 1);

			lua_getfield(_State, -1, "Buildings");
			lua_pushnil(_State);
			while(lua_next(_State, -2) != 0) {
				luaL_checktype(_State, -1, LUA_TLIGHTUSERDATA);
				_Obj = lua_touserdata(_State, -1);
				ArrayInsertSort_S(_Family->Buildings, _Obj, ObjCmp);
				lua_pop(_State, 1);
			}
			lua_pop(_State, 1);

			lua_getfield(_State, -1, "Animals");
			lua_pushnil(_State);
			while(lua_next(_State, -2) != 0) {
				lua_pushnil(_State);
				if(lua_next(_State, -2) == 0) {
					_Error = "Animals";
					goto LuaError;
				}
				AddString(_State, -1, &_Name);
				lua_pop(_State, 1);
				if(lua_next(_State, -2) == 0) {
					_Error = "Animals";
					goto LuaError;
				}
				AddInteger(_State, -1, &_Quantity);
				if((_Population = HashSearch(&g_Populations, _Name)) == NULL) {
					Log(ELOG_WARNING, "Cannot find Population %s.", _Name);
					lua_pop(_State, 3);
					continue;
				}
				for(j = 0; j < _Quantity; ++j)
					ArrayInsertSort_S(_Family->Animals, CreateAnimal(_Population, Random(0, _Population->Ages[AGE_DEATH]->Max), _X, _Y), AnimalCmp);
				lua_pop(_State, 3);
			}
			lua_pop(_State, 1);
			lua_getfield(_State, -1, "AI");
			luaL_checktype(_State, -1, LUA_TFUNCTION);
			lua_pop(_State, 1);
			for(j = 0; j < _Family->NumChildren + 2; ++i) {
				if(_Family->People[j] == NULL)
					continue;
				lua_getfield(_State, -1, "AI");
				lua_getglobal(_State, "Person");
				lua_pushlightuserdata(_State, _Family->People[j]);
				if(LuaCallFunc(_State, 1, 1, 0) == 0)
					return;
				if(LuaCallFunc(_State, 1, 1, 0) == 0)
					return;
				_Cmp.Name = luaL_checkstring(_State, -1);
				if((_Behavior = BinarySearch(&_Cmp, g_BhvList.Table, g_BhvList.Size, LuaBhvCmp)) == NULL) {
					Log(ELOG_WARNING, "%s is not a behavior", lua_tostring(_State, -1));
					return;
				}
				_Family->People[j]->Behavior = _Behavior->Behavior;
			}
			lua_pop(_State, 2);
			break;
		}
		_Percent += _FamilyTypes[i]->Percent * 10000;
	}
	return;
	LuaError:
	luaL_error(_State, "In function %s the %s table does not contain a valid element.", _FamilyTypes[i]->LuaFunc, _Error);
}
