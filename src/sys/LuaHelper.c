/*
 * File: LuaHelper.c
 * Author: David Brotz
 */

#include "LuaHelper.h"

#include "HashTable.h"
#include "LinkedList.h"
#include "Constraint.h"
#include "Log.h"
#include "Array.h"
#include "Event.h"
#include "Random.h"
#include "../Herald.h"
#include "../Good.h"
#include "../Crop.h"
#include "../Building.h"
#include "../Population.h"
#include "../Person.h"
#include "../Family.h"

#include <stdlib.h>
#include <string.h>
#include <lua/lauxlib.h>

lua_State* g_LuaState = NULL;

static const luaL_Reg g_LuaFuncs[] = {
		{"CreateConstraintBounds", LuaConstraintBnds},
		{"Crop", LuaCrop},
		{"Good", LuaGoodBase},
		{"Food", LuaFoodBase},
		{"GetAnimal", LuaPopulation},
		{"Person", LuaPerson},
		{"ToYears", LuaYears},
		{"ToMonth", LuaMonth},
		{"PrintDate", LuaPrintDate},
		{"PrintYears", LuaPrintYears},
		{"Hook", LuaHook},
		{"CreateGood", LuaCreateGood},
		{"CreateBuilding", LuaCreateBuilding},
		{"CreateAnimal", LuaCreateAnimal},
		{NULL, NULL}
};

static const luaL_Reg g_LuaFuncsPerson[] = {
		{"GetId", LuaPersonGetId},
		{"GetX", LuaPersonGetX},
		{"GetY", LuaPersonGetY},
		{"GetGender", LuaPersonGetGender},
		{"GetNutrition", LuaPersonGetNutrition},
		{"GetAge", LuaPersonGetAge},
		{"GetName", LuaPersonGetName},
		{"GetFamily", LuaPersonGetFamily},
		{"GetParentFamily", LuaPersonGetParentFamily},
		{NULL, NULL}
};

static const luaL_Reg g_LuaFuncsGood[] = {
		{"GetId", LuaGoodGetId},
		{"GetQuantity", LuaGoodGetQuantity},
		{"GetBase", LuaGoodGetBase},
		{NULL, NULL}
};

static const luaL_Reg g_LuaFuncsFamily[] = {
		{"GetId", LuaFamilyGetId},
		{"LuaFamilyChildrenCt", LuaFamilyChildrenCt},
		{"GetName", LuaFamilyGetName},
		{"LuaFamilyGetPeople", LuaFamilyGetPeople},
		{"GetFields", LuaFamilyGetFields},
		{"GetBuildings", LuaFamilyGetBuildings},
		{"GetBuildingCt", LuaFamilyGetBulidingCt},
		{"GetGoods", LuaFamilyGetGoods},
		{"GetGoodCt", LuaFamilyGetGoodCt},
		{"GetAnimals", LuaFamilyGetAnimals},
		{"GetAnimalCt", LuaFamilyGetAnimalCt},
		{NULL, NULL}
};

static const luaL_Reg g_LuaFuncsField[] = {
		{"GetId", LuaFieldGetId},
		{"GetCrop", LuaFieldGetCrop},
		{"GetYield", LuaFieldGetYield},
		{"GetAcres", LuaFieldGetAcres},
		{NULL, NULL}
};

static const luaL_Reg g_LuaFuncsAnimal[] = {
		{"GetId", LuaAnimalGetId},
		{"IsMale", LuaAnimalIsMale},
		{"GetNutrition", LuaAnimalGetNutrition},
		{"GetAge", LuaAnimalGetAge},
		{"GetBase", LuaAnimalGetBase},
		{NULL, NULL}
};

static const luaL_Reg g_LuaFuncsArray[] = {
		{"Create", LuaArrayCreate},
		{NULL, NULL}
};

void RegisterLuaFuncs(lua_State* _State) {
	int i;

	for(i = 0; (g_LuaFuncs[i].name != NULL && g_LuaFuncs[i].func != NULL); ++i)
		lua_register(_State, g_LuaFuncs[i].name, g_LuaFuncs[i].func);
	if(
	RegisterIterator(_State) == 0 ||
	RegisterPerson(_State) == 0  ||
	RegisterGood(_State) == 0  ||
	RegisterFamily(_State) == 0  ||
	RegisterField(_State) == 0  ||
	RegisterAnimal(_State) == 0  ||
	RegisterArray(_State) == 0  ||
	RegisterArrayItr(_State) == 0 )
		luaL_error(_State, "Loading Lua functions has failed.");
}

int RegisterIterator(lua_State* _State) {
	if(luaL_newmetatable(_State, "Iterator") == 0)
			return 0;
	lua_pushliteral(_State, "__index");
	lua_pushvalue(_State, -2);
	lua_rawset(_State, -3);

	lua_pushstring(_State, "Itr");
	lua_pushcfunction(_State, NULL);
	lua_rawset(_State, -3);

	lua_pushstring(_State, "Next");
	lua_pushcfunction(_State, NULL);
	lua_rawset(_State, -3);

	lua_pushstring(_State, "Prev");
	lua_pushcfunction(_State, NULL);
	lua_rawset(_State, -3);

	lua_pushstring(_State, "__class");
	lua_pushstring(_State, "Iterator");
	lua_rawset(_State, -3);

	lua_pushliteral(_State, "__newindex");
	lua_pushnil(_State);
	lua_rawset(_State, -3);
	lua_setglobal(_State, "Iterator");
	return 1;
}

int RegisterArrayItr(lua_State* _State) {
	if(luaL_newmetatable(_State, "ArrayIterator") == 0)
		return 0;

	lua_pushstring(_State, "__index");
	lua_pushvalue(_State, -2);
	lua_rawset(_State, -3);

	lua_pushstring(_State, "__newindex");
	lua_pushnil(_State);
	lua_rawset(_State, -3);

	lua_pushstring(_State, "Itr");
	lua_pushcfunction(_State, LuaArrayItr);
	lua_rawset(_State, -3);

	lua_pushstring(_State, "Next");
	lua_pushcfunction(_State, LuaArrayItrNext);
	lua_rawset(_State, -3);

	lua_pushstring(_State, "Prev");
	lua_pushcfunction(_State, LuaArrayItrPrev);
	lua_rawset(_State, -3);

	lua_pushstring(_State, "__classtype"); /* Not to be confused with __class. Takes a string that is equal to a metatable containing a class. */
	lua_pushnil(_State);
	lua_rawset(_State, -3);

	lua_pushstring(_State, "__class");
	lua_pushstring(_State, "Iterator");
	lua_rawset(_State, -3);

	lua_pushliteral(_State, "__newindex");
	lua_pushnil(_State);
	lua_rawset(_State, -3);

	lua_pushstring(_State, "__baseclass");
	lua_getglobal(_State, "Iterator");
	lua_rawset(_State, -3);
	lua_setglobal(_State, "ArrayIterator");
	return 1;
}

int RegisterPerson(lua_State* _State) {
	if(luaL_newmetatable(_State, "Person") == 0)
		return 0;
	lua_pushliteral(_State, "__index");
	lua_pushvalue(_State, -2);
	lua_rawset(_State, -3);

	lua_pushstring(_State, "__class");
	lua_pushstring(_State, "Person");
	lua_rawset(_State, -3);

	lua_pushliteral(_State, "__newindex");
	lua_pushnil(_State);
	lua_rawset(_State, -3);
	luaL_setfuncs(_State, g_LuaFuncsPerson, 0);
	lua_setglobal(_State, "Person");
	return 1;
}

int RegisterGood(lua_State* _State) {
	if(luaL_newmetatable(_State, "Good") == 0)
		return 0;
	lua_pushliteral(_State, "__index");
	lua_pushvalue(_State, -2);
	lua_rawset(_State, -3);

	lua_pushstring(_State, "__class");
	lua_pushstring(_State, "Good");
	lua_rawset(_State, -3);

	lua_pushliteral(_State, "__newindex");
	lua_pushnil(_State);
	lua_rawset(_State, -3);
	luaL_setfuncs(_State, g_LuaFuncsGood, 0);
	lua_setglobal(_State, "Good");
	return 1;
}

int RegisterFamily(lua_State* _State) {
	if(luaL_newmetatable(_State, "Family") == 0)
		return 0;
	lua_pushliteral(_State, "__index");
	lua_pushvalue(_State, -2);
	lua_rawset(_State, -3);

	lua_pushstring(_State, "__class");
	lua_pushstring(_State, "Family");
	lua_rawset(_State, -3);

	lua_pushliteral(_State, "__newindex");
	lua_pushnil(_State);
	lua_rawset(_State, -3);
	luaL_setfuncs(_State, g_LuaFuncsFamily, 0);
	lua_setglobal(_State, "Family");
	return 1;
}

int RegisterField(lua_State* _State) {
	if(luaL_newmetatable(_State, "Field") == 0)
		return 0;
	lua_pushliteral(_State, "__index");
	lua_pushvalue(_State, -2);
	lua_rawset(_State, -3);

	lua_pushstring(_State, "__class");
	lua_pushstring(_State, "Field");
	lua_rawset(_State, -3);

	lua_pushliteral(_State, "__newindex");
	lua_pushnil(_State);
	lua_rawset(_State, -3);
	luaL_setfuncs(_State, g_LuaFuncsField, 0);
	lua_setglobal(_State, "Field");
	return 1;
}

int RegisterAnimal(lua_State* _State) {
	if(luaL_newmetatable(_State, "Animal") == 0)
		return 0;
	lua_pushliteral(_State, "__index");
	lua_pushvalue(_State, -2);
	lua_rawset(_State, -3);

	lua_pushstring(_State, "__class");
	lua_pushstring(_State, "Animal");
	lua_rawset(_State, -3);

	lua_pushliteral(_State, "__newindex");
	lua_pushnil(_State);
	lua_rawset(_State, -3);
	luaL_setfuncs(_State, g_LuaFuncsAnimal, 0);
	lua_setglobal(_State, "Animal");
	return 1;
}

int RegisterArray(lua_State* _State) {
	if(luaL_newmetatable(_State, "Array") == 0)
		return 0;
	lua_pushliteral(_State, "__index");
	lua_pushvalue(_State, -2);
	lua_rawset(_State, -3);

	lua_pushstring(_State, "__class");
	lua_pushstring(_State, "Array");
	lua_rawset(_State, -3);

	lua_pushliteral(_State, "__newindex");
	lua_pushnil(_State);
	lua_rawset(_State, -3);
	luaL_setfuncs(_State, g_LuaFuncsArray, 0);
	lua_setglobal(_State, "Array");
	return 1;
}

int LuaPersonGetId(lua_State* _State) {
	struct Person* _Person = LuaCheckPerson(_State, 1);

	lua_pushinteger(_State, _Person->Id);
	return 1;
}

int LuaPersonGetX(lua_State* _State) {
	struct Person* _Person = LuaCheckPerson(_State, 1);

	lua_pushinteger(_State, _Person->X);
	return 1;
}

int LuaPersonGetY(lua_State* _State) {
	struct Person* _Person = LuaCheckPerson(_State, 1);

	lua_pushinteger(_State, _Person->Y);
	return 1;
}

int LuaPersonGetGender(lua_State* _State) {
	struct Person* _Person = LuaCheckPerson(_State, 1);

	lua_pushinteger(_State, _Person->Gender);
	return 1;
}

int LuaPersonGetNutrition(lua_State* _State) {
	struct Person* _Person = LuaCheckPerson(_State, 1);

	lua_pushinteger(_State, _Person->Nutrition);
	return 1;
}

int LuaPersonGetAge(lua_State* _State) {
	struct Person* _Person = LuaCheckPerson(_State, 1);

	lua_pushinteger(_State, _Person->Age);
	return 1;
}

int LuaPersonGetName(lua_State* _State) {
	struct Person* _Person = LuaCheckPerson(_State, 1);

	lua_pushstring(_State, _Person->Name);
	return 1;
}

int LuaPersonGetFamily(lua_State* _State) {
	struct Person* _Person = LuaCheckPerson(_State, 1);

	lua_newtable(_State);
	lua_getglobal(_State, "Family");
	lua_setmetatable(_State, -2);

	lua_pushstring(_State, "__self");
	lua_pushlightuserdata(_State, _Person->Family);
	lua_rawset(_State, -3);
	return 1;
}

int LuaPersonGetParentFamily(lua_State* _State) {
	struct Person* _Person = LuaCheckPerson(_State, 1);

	lua_newtable(_State);
	lua_getglobal(_State, "Family");
	lua_setmetatable(_State, -2);

	lua_pushstring(_State, "__self");
	lua_pushlightuserdata(_State, _Person->Parent);
	lua_rawset(_State, -3);
	return 1;
}

int LuaGoodGetId(lua_State* _State) {
	struct Good* _Good = LuaCheckGood(_State, 1);

	lua_pushinteger(_State, _Good->Id);
	return 1;
}

int LuaGoodGetQuantity(lua_State* _State) {
	struct Good* _Good = LuaCheckGood(_State, 1);

	lua_pushinteger(_State, _Good->Quantity);
	return 1;
}

int LuaGoodGetBase(lua_State* _State) {
	struct Good* _Good = LuaCheckGood(_State, 1);

	lua_pushstring(_State, _Good->Base->Name);
	lua_remove(_State, -2);
	LuaGoodBase(_State);
	return 1;
}

int LuaFamilyGetId(lua_State* _State) {
	struct Family* _Family = LuaCheckFamily(_State, 1);

	lua_pushinteger(_State, _Family->Id);
	return 1;
}

int LuaFamilyChildrenCt(lua_State* _State) {
	struct Family* _Family = LuaCheckFamily(_State, 1);

	lua_pushinteger(_State, _Family->NumChildren);
	return 1;
}

int LuaFamilyGetName(lua_State* _State) {
	struct Family* _Family = LuaCheckFamily(_State, 1);

	lua_pushstring(_State, _Family->Name);
	return 1;
}

int LuaFamilyGetPeople(lua_State* _State) {
	struct Family* _Family = LuaCheckFamily(_State, 1);
		int i;

		lua_createtable(_State, 10, 0);
		for(i = 0; i < FAMILY_PEOPLESZ; ++i) {
			lua_pushlightuserdata(_State, _Family->People[i]);
			lua_rawseti(_State, -2, i + 1);
		}
		return 1;
}

int LuaFamilyGetFields(lua_State* _State) {
	struct Family* _Family = LuaCheckFamily(_State, 1);

	lua_newtable(_State);
	lua_getglobal(_State, "ArrayIterator");
	lua_setmetatable(_State, -2);

	lua_pushstring(_State, "__self");
	lua_pushlightuserdata(_State, _Family->Fields);
	lua_rawset(_State, -3);

	lua_pushstring(_State, "__classtype");
	lua_pushstring(_State, "Field");
	lua_rawset(_State, -3);
	return 1;
}

int LuaFamilyGetBuildings(lua_State* _State) {
	struct Family* _Family = LuaCheckFamily(_State, 1);

	lua_newtable(_State);
	lua_getglobal(_State, "Array");
	lua_setmetatable(_State, -2);

	lua_pushstring(_State, "__self");
	lua_pushlightuserdata(_State, _Family->Buildings);
	lua_rawset(_State, -3);
	return 1;
}

int LuaFamilyGetBulidingCt(lua_State* _State) {
	struct Family* _Family = LuaCheckFamily(_State, 1);

	lua_pushinteger(_State, _Family->Buildings->Size);
	return 1;
}

int LuaFamilyGetGoods(lua_State* _State) {
	struct Family* _Family = LuaCheckFamily(_State, 1);

	lua_newtable(_State);
	lua_getglobal(_State, "ArrayIterator");
	lua_setmetatable(_State, -2);

	lua_pushstring(_State, "__self");
	lua_pushlightuserdata(_State, _Family->Goods);
	lua_rawset(_State, -3);

	lua_pushstring(_State, "__classtype");
	lua_pushstring(_State, "Good");
	lua_rawset(_State, -3);
	return 1;
}

int LuaFamilyGetGoodCt(lua_State* _State) {
	struct Family* _Family = LuaCheckFamily(_State, 1);

	lua_pushinteger(_State, _Family->Goods->Size);
	return 1;
}

int LuaFamilyGetAnimals(lua_State* _State) {
	struct Family* _Family = LuaCheckFamily(_State, 1);

	lua_newtable(_State);
	lua_getglobal(_State, "ArrayIterator");
	lua_setmetatable(_State, -2);

	lua_pushstring(_State, "__self");
	lua_pushlightuserdata(_State, _Family->Animals);
	lua_rawset(_State, -3);

	lua_pushstring(_State, "__classtype");
	lua_pushstring(_State, "Animal");
	lua_rawset(_State, -3);
	return 1;
}

int LuaFamilyGetAnimalCt(lua_State* _State) {
	struct Family* _Family = LuaCheckFamily(_State, 1);

	lua_pushinteger(_State, _Family->Animals->Size);
	return 1;
}

int LuaFieldGetId(lua_State* _State) {
	struct Field* _Field = LuaCheckField(_State, 1);

	lua_pushinteger(_State, _Field->Id);
	return 1;
}

int LuaFieldGetCrop(lua_State* _State) {
	struct Field* _Field = LuaCheckField(_State, 1);

	if(_Field == NULL) {
		lua_pushnil(_State);
		return 1;
	}
	lua_pushstring(_State, _Field->Crop->Name);
	lua_remove(_State, -2);
	LuaCrop(_State);
	return 1;
}

int LuaFieldGetYield(lua_State* _State) {
	struct Field* _Field = LuaCheckField(_State, 1);

	lua_pushnumber(_State, _Field->YieldTotal);
	return 1;
}

int LuaFieldGetAcres(lua_State* _State) {
	struct Field* _Field = LuaCheckField(_State, 1);

	lua_pushinteger(_State, _Field->Acres);
	return 1;
}

int LuaAnimalGetId(lua_State* _State) {
	struct Animal* _Animal = LuaCheckAnimal(_State, 1);

	lua_pushinteger(_State, _Animal->Id);
	return 1;
}

int LuaAnimalIsMale(lua_State* _State) {
	struct Animal* _Animal = LuaCheckAnimal(_State, 1);

	lua_pushboolean(_State, (_Animal->Gender == EMALE));
	return 1;
}

int LuaAnimalGetNutrition(lua_State* _State) {
	struct Animal* _Animal = LuaCheckAnimal(_State, 1);

	lua_pushinteger(_State, _Animal->Nutrition);
	return 1;
}

int LuaAnimalGetAge(lua_State* _State) {
	struct Animal* _Animal = LuaCheckAnimal(_State, 1);

	lua_pushinteger(_State, _Animal->Age);
	return 1;
}

int LuaAnimalGetBase(lua_State* _State) {
	struct Animal* _Animal = LuaCheckAnimal(_State, 1);

	if(_Animal == NULL) {
		lua_pushnil(_State);
		return 1;
	}
	lua_pushstring(_State, _Animal->PopType->Name);
	lua_remove(_State, -2);
	LuaPopulation(_State);
	return 1;
}

int LuaArrayCreate(lua_State* _State) {
	int _Size = luaL_checkinteger(_State, 1);
	struct Array* _Array = CreateArray(_Size);

	lua_newtable(_State);
	lua_getglobal(_State, "Array");
	lua_setmetatable(_State, -2);

	lua_pushstring(_State, "__self");
	lua_pushlightuserdata(_State, _Array);
	lua_rawset(_State, -3);
	return 1;
}

int LuaArrayItr_Aux(lua_State* _State) {
	struct Array* _Array = LuaCheckClass(_State, lua_upvalueindex(1), "Iterator");
	int _Index = lua_tointeger(_State, lua_upvalueindex(2));
	int _Change = lua_tointeger(_State, lua_upvalueindex(3));
	int i;

	for(i = _Index; i < _Array->TblSize; i += _Change) {
		if(_Array->Table[_Index] != NULL)
			break;
		_Index += _Change;
	}

	if(_Index < 0 || _Index >= _Array->TblSize) {
		lua_pushnil(_State);
		lua_pushvalue(_State, 1);
		lua_pushnil(_State);
		return 3;
	}
	lua_pushvalue(_State, lua_upvalueindex(1));
	lua_pushstring(_State, "__classtype");
	lua_rawget(_State, -2);
	if(lua_type(_State, -1) != LUA_TSTRING)
		luaL_error(_State, "Iterator does not have a __classtype.");

	lua_newtable(_State);
	lua_getglobal(_State, lua_tostring(_State, -2));
	lua_setmetatable(_State, -2);
	lua_remove(_State, -2);

	lua_pushstring(_State, "__self");
	lua_pushlightuserdata(_State, _Array->Table[_Index]);
	lua_rawset(_State, -3);
	lua_pushinteger(_State, _Index + _Change);
	lua_replace(_State, lua_upvalueindex(2));
	return 1;
}

int LuaArrayItrNext(lua_State* _State) {
	LuaTestClass(_State, 1, "Iterator");
	lua_pushinteger(_State, 0);
	lua_pushinteger(_State, 1);
	lua_pushcclosure(_State, LuaArrayItr_Aux, 3);
	return 1;
}

int LuaArrayItrPrev(lua_State* _State) {
	LuaTestClass(_State, 1, "Iterator");
	lua_pushinteger(_State, 0);
	lua_pushinteger(_State, -1);
	lua_pushcclosure(_State, LuaArrayItr_Aux, 3);
	return 1;
}

int LuaArrayItr(lua_State* _State) {
	lua_pushlightuserdata(_State, LuaCheckClass(_State, 1, "Iterator"));
	return 1;
}

struct Person* LuaCheckPerson(lua_State* _State, int _Index) {
	struct Person* _Person = NULL;

	if((_Person = LuaTestClass(_State, _Index, "Person")) == NULL)
		return (struct Person*) LuaCheckClass(_State, _Index, "Person");
	return _Person;
}

struct Good* LuaCheckGood(lua_State* _State, int _Index) {
	struct Good* _Good = NULL;

	if((_Good = LuaTestClass(_State, _Index, "Good")) == NULL)
		return (struct Good*) LuaCheckClass(_State, _Index, "Good");
	return _Good;
}

struct Family* LuaCheckFamily(lua_State* _State, int _Index) {
	struct Family* _Family = NULL;

	if((_Family = LuaTestClass(_State, _Index, "Family")) == NULL)
		return (struct Family*) LuaCheckClass(_State, _Index, "Family");
	return _Family;
}

struct Field* LuaCheckField(lua_State* _State, int _Index) {
	struct Field* _Field = NULL;

	if((_Field = LuaTestClass(_State, _Index, "Field")) == NULL)
		return (struct Field*) LuaCheckClass(_State, _Index, "Field");
	return _Field;
}

struct Animal* LuaCheckAnimal(lua_State* _State, int _Index) {
	struct Animal* _Animal = NULL;

	if((_Animal = LuaTestClass(_State, _Index, "Animal")) == NULL)
		return (struct Animal*) LuaCheckClass(_State, _Index, "Animal");
	return _Animal;
}

int LuaConstraint(lua_State* _State) {
	lua_pushlightuserdata(_State, CreateConstrntLst(NULL, luaL_checkint(_State, 1),  luaL_checkint(_State, 2),  luaL_checkint(_State, 3)));
	return 1;
}

int LuaConstraintBnds(lua_State* _State) {
	luaL_checktype(_State, -1, LUA_TTABLE);

	int _Size = lua_rawlen(_State, -1);
	int _CurrMin = -1;
	int _CurrMax = -1;
	int i = 0;
	struct Constraint** _Constrnt = (struct Constraint**) malloc(sizeof(struct Constraint) * (_Size));

	lua_pushnil(_State);
	if(lua_next(_State, -2) != 0) {
		_CurrMin = luaL_checkint(_State, -1);
		lua_pop(_State, 1);
		if(lua_next(_State, -2) != 0) {
			_CurrMax = luaL_checkint(_State, -1);
			_Constrnt[i++] = CreateConstraint(_CurrMin, _CurrMax);
		} else
			goto error;
		lua_pop(_State, 1);
	} else
		goto error;
	while(lua_next(_State, -2) != 0) {
		_CurrMin = _CurrMax + 1;
		_CurrMax = luaL_checkint(_State, -1);
		_Constrnt[i++] = CreateConstraint(_CurrMin, _CurrMax);
		lua_pop(_State, 1);
	}
	_Constrnt[_Size - 1] = NULL;
	lua_pushlightuserdata(_State, _Constrnt);
	return 1;
	error:
	luaL_error(_State, "Cannot create constraint.");
	free(_Constrnt);
	return 0;
}

void ConstraintBndToLua(lua_State* _State, struct Constraint** _Constraints) {
	int i = 0;

	lua_newtable(_State);
	while(*_Constraints != NULL) {
		ConstraintToLua(_State, *_Constraints);
		lua_rawseti(_State, -2, i++);
		_Constraints += sizeof(struct Constraint*);
	}
}

int LuaGoodBase(lua_State* _State) {
	const struct GoodBase* _Good = NULL;
	const char* _Name = NULL;
	int i;

	_Name = luaL_checklstring(_State, 1, NULL);
	if((_Good = HashSearch(&g_Goods, _Name)) == NULL) {
		luaL_error(_State, "%s is not a valid GoodBase.", _Name);
		return 0;
	}
	lua_createtable(_State, 0, 4);

	lua_pushstring(_State, "Id");
	lua_pushinteger(_State, _Good->Id);
	lua_rawset(_State, -3);

	lua_pushstring(_State, "Name");
	lua_pushstring(_State, _Good->Name);
	lua_rawset(_State, -3);

	lua_pushstring(_State, "Category");
	lua_pushinteger(_State, _Good->Category);
	lua_rawset(_State, -3);

	lua_createtable(_State, _Good->IGSize, 0);
	lua_pushstring(_State, "InputGoods");
	lua_pushvalue(_State, -2);
	lua_rawset(_State, -4);
	for(i = 0; i < _Good->IGSize; ++i) {
		lua_pushinteger(_State, i + 1);
		lua_pushstring(_State, ((struct GoodBase*)_Good->InputGoods[i]->Req)->Name);
		lua_rawset(_State, -3);
	}
	lua_pop(_State, 1);
	//lua_rawset(_State, -3);
	return 1;
}

int LuaFoodBase(lua_State* _State) {
	const struct FoodBase* _Good = NULL;
	const char* _Name = NULL;

	if(LuaGoodBase(_State) == 0)
		return 0;
	if((_Good = HashSearch(&g_Goods, _Name)) == NULL)
		luaL_error(_State, "%s is not a valid FoodBase.");
	lua_pushstring(_State, "Nutrition");
	lua_pushinteger(_State, _Good->Nutrition);
	lua_rawget(_State, -3);
	return 1;
}

int LuaCrop(lua_State* _State) {
	const char* _Name = NULL;
	const struct Crop* _Crop = NULL;

	_Name = luaL_checklstring(_State, 1, NULL);
	if((_Crop = HashSearch(&g_Crops, _Name)) == NULL)
		luaL_error(_State, "%s is not a valid crop.", _Name);
	lua_createtable(_State, 0, 7);

	lua_pushstring(_State, "Id");
	lua_pushinteger(_State, _Crop->Id);
	lua_rawset(_State, -3);

	lua_pushstring(_State, "Name");
	lua_pushstring(_State, _Crop->Name);
	lua_rawset(_State, -3);

	lua_pushstring(_State, "Type");
	lua_pushinteger(_State, _Crop->Type);
	lua_rawset(_State, -3);

	lua_pushstring(_State, "PerAcre");
	lua_pushinteger(_State, _Crop->PerAcre);
	lua_rawset(_State, -3);

	lua_pushstring(_State, "NutVal");
	lua_pushinteger(_State, _Crop->NutVal);
	lua_rawset(_State, -3);

	lua_pushstring(_State, "GrowDays");
	lua_pushinteger(_State, _Crop->GrowDays);
	lua_rawset(_State, -3);

	lua_pushstring(_State, "YieldMult");
	lua_pushnumber(_State, _Crop->YieldMult);
	lua_rawset(_State, -3);
	return 1;
}

int LuaPopulation(lua_State* _State) {
	int i;
	const char* _Name = NULL;
	const struct Population* _Pop = NULL;

	_Name = luaL_checklstring(_State, 1, NULL);
	if((_Pop = HashSearch(&g_Populations, _Name)) == NULL) {
		Log(ELOG_WARNING, "%s is not a valid population.", _Name);
		return 0;
	}
	lua_createtable(_State, 0, 10);

	lua_pushstring(_State, "Id");
	lua_pushinteger(_State, _Pop->Id);
	lua_rawset(_State, -3);

	lua_pushstring(_State, "Name");
	lua_pushstring(_State, _Pop->Name);
	lua_rawset(_State, -3);

	lua_pushstring(_State, "Nutrition");
	lua_pushinteger(_State, _Pop->Nutrition);
	lua_rawset(_State, -3);

	lua_pushstring(_State, "Meat");
	lua_pushinteger(_State, _Pop->Meat);
	lua_rawset(_State, -3);

	lua_pushstring(_State, "Milk");
	lua_pushinteger(_State, _Pop->Milk);
	lua_rawset(_State, -3);

	lua_pushstring(_State, "MaleRatio");
	lua_pushnumber(_State, _Pop->MaleRatio);
	lua_rawset(_State, -3);
	lua_pushstring(_State, "Ages");
	ConstraintBndToLua(_State, _Pop->Ages);
	lua_rawset(_State, -3);

	lua_pushstring(_State, "Outputs");
	lua_createtable(_State, ArrayLen(_Pop->Outputs), 0);
	for(i = 0; _Pop->Outputs[i] != NULL; ++i) {
		lua_pushstring(_State, ((struct Good*)_Pop->Outputs[i])->Base->Name);
		lua_rawseti(_State, -2, i);
	}
	lua_rawset(_State, -3);

	lua_pushstring(_State, "Eats");
	lua_createtable(_State, _Pop->EatsSize, 0);
	for(i = 0; i < _Pop->EatsSize; ++i) {
		lua_pushstring(_State, ((struct FoodBase*)_Pop->Eats[i])->Name);
		lua_rawseti(_State, -2, i);
	}
	lua_rawset(_State, -3);
	return 1;
}

int LuaPushPerson(lua_State* _State, int _Index) {
	struct Person* _Person = NULL;

	_Person = lua_touserdata(_State, _Index);
	lua_createtable(_State, 0, 10);

	lua_pushstring(_State, "X");
	lua_pushinteger(_State, _Person->X);
	lua_rawset(_State, -3);

	lua_pushstring(_State, "Y");
	lua_pushinteger(_State, _Person->Y);
	lua_rawset(_State, -3);

	lua_pushstring(_State, "Male");
	lua_pushboolean(_State, (_Person->Gender == EMALE) ? (1) : (0));
	lua_rawset(_State, -3);

	lua_pushstring(_State, "Nutrition");
	lua_pushinteger(_State, _Person->Nutrition);
	lua_rawset(_State, -3);

	lua_pushstring(_State, "Age");
	lua_pushinteger(_State, _Person->Age);
	lua_rawset(_State, -3);

	lua_pushstring(_State, "Name");
	lua_pushstring(_State, _Person->Name);
	lua_rawset(_State, -3);

	lua_pushstring(_State, "Family");
	lua_newtable(_State);
	lua_getglobal(_State, "Family");
	lua_setmetatable(_State, -2);
	lua_pushstring(_State, "__self");
	lua_pushlightuserdata(_State, _Person->Family);
	lua_rawset(_State, -3);
	lua_rawset(_State, -3);

	lua_pushstring(_State, "Parent");
	lua_pushstring(_State, (_Person->Parent != NULL) ? (_Person->Parent->Name) : ("NULL"));
	lua_rawset(_State, -3);
	return 1;
}

int LuaPerson(lua_State* _State) {
	luaL_checktype(_State, 1, LUA_TLIGHTUSERDATA);

	lua_newtable(_State);
	lua_getglobal(_State, "Person");
	lua_setmetatable(_State, -2);
	lua_pushstring(_State, "__self");
	lua_pushlightuserdata(_State, lua_touserdata(_State, 1));
	lua_rawset(_State, -3);
	//LuaPushPerson(_State, 1);
	return 1;
}

int LuaYears(lua_State* _State) {
	lua_pushinteger(_State, luaL_checkinteger(_State, 1) * YEAR_DAYS);
	return 1;
}

int LuaMonth(lua_State* _State) {
	const char* _Type = NULL;

	_Type = luaL_checkstring(_State, 1);
	luaL_checkinteger(_State, 2);
	if(!strcmp(_Type, "Years"))
		lua_pushinteger(_State, TO_YEARS(lua_tointeger(_State, 2)));
	else if(!strcmp(_Type, "Days"))
		lua_pushinteger(_State, TO_DAYS(lua_tointeger(_State, 2)));
	else {
		return luaL_argerror(_State, 2, "Must be either \"Years\" or \"Days\".");

	}
	return 1;
}

int LuaPrintDate(lua_State* _State) {
	DATE _Date = luaL_checkinteger(_State, 1);
	DATE _Days = DAY(_Date);
	DATE _Months = MONTH(_Date);
	DATE _Years = YEAR(_Date);

	if(_Months > 11)
		_Months = 11;
	lua_pushfstring(_State, "%s %d, %d", g_ShortMonths[_Months], _Days, _Years);
	return 1;
}

int LuaPrintYears(lua_State* _State) {
	DATE _Date = luaL_checkinteger(_State, 1);
	DATE _Years = YEAR(_Date);

	lua_pushfstring(_State, "%d Years", _Years);
	return 1;
}

int LuaHook(lua_State* _State) {
	const char* _Name = NULL;

	_Name = luaL_checkstring(_State, 1);
	if(!strcmp(_Name, "Age")) {
		lua_pushlightuserdata(_State, CreateEventTime(NULL, luaL_checkinteger(_State, 2)));
	} else {
		luaL_error(_State, "Must be a valid hook type.");
		return 0;
	}
	return 1;
}

int LuaLoadFile(lua_State* _State, const char* _File) {
	int _Error = luaL_loadfile(_State, _File);

	if(_Error != 0)
		goto error;
	if((_Error = lua_pcall(_State, 0, LUA_MULTRET, 0)) != 0)
		goto error;
	return LUA_OK;

	error:
	switch(_Error) {
		case LUA_ERRSYNTAX:
			Log(ELOG_ERROR, "%s", lua_tostring(_State, -1));
			return _Error;
		case LUA_ERRFILE:
			Log(ELOG_ERROR, "Cannot load file: %s", _File);
			return _Error;
		case LUA_ERRRUN:
			Log(ELOG_ERROR, "Cannot run file: %s", lua_tostring(_State, -1));
			return _Error;
	}
	return LUA_ERRERR;
}

int LuaCallFunc(lua_State* _State, int _Args, int _Results, int _ErrFunc) {
	//TODO: If in debug mode the stack should be checked to ensure its balanced.
	int _Error = lua_pcall(_State, _Args, _Results, _ErrFunc);

	if(_Error != 0)
		goto error;
	return 1;

	error:
	Log(ELOG_ERROR, "%s", lua_tostring(_State, -1));
	return 0;
}

void LuaLoadList(lua_State* _State, const char* _File, const char* _Global, void*(*_Callback)(lua_State*, int), void(*_Insert)(struct LinkedList*, void*), struct LinkedList* _Return) {
	void* _CallRet = NULL;

	if(LuaLoadFile(_State, _File) != LUA_OK)
		return;
	lua_getglobal(_State, _Global);
	if(!lua_istable(_State, -1))
		return;
	lua_pushnil(_State);
	while(lua_next(_State, -2) != 0) {
		if(!lua_istable(_State, -1)) {
			Log(ELOG_WARNING, "Warning: index is not a table.");
			lua_pop(_State, 1);
			continue;
		}
		if((_CallRet = _Callback(_State, -1)) != NULL)
			_Insert(_Return, _CallRet);
		lua_pop(_State, 1);
	}
	lua_pop(_State, 1);
}

int AddInteger(lua_State* _State, int _Index, int* _Number) {
	if(lua_isnumber(_State, _Index)) {
		*_Number = lua_tointeger(_State, _Index);
		return 1;
	}
	Log(ELOG_ERROR, "metafield is not a integer");
	return 0;
}

int AddString(lua_State* _State, int _Index, const char** _String) {
	if(lua_isstring(_State, _Index)) {
		*_String = lua_tostring(_State, _Index);
		return 1;
	}
	Log(ELOG_ERROR, "metafield is not a string");
	return 0;
}

int AddNumber(lua_State* _State, int _Index, double* _Number) {
	if(lua_isnumber(_State, _Index)) {
		*_Number = lua_tonumber(_State, _Index);
		return 1;
	}
	Log(ELOG_ERROR, "metafield is not a number");
	return 0;
}

int LuaLudata(lua_State* _State, int _Index, void** _Data) {
	if(lua_islightuserdata(_State, _Index)) {
		*_Data = lua_touserdata(_State, _Index);
		return 1;
	}
	Log(ELOG_ERROR, "metafield is not user data");
	return 0;
}

int LuaFunction(lua_State* _State, int _Index, lua_CFunction* _Function) {
	if(lua_iscfunction(_State, _Index)) {
		*_Function = lua_tocfunction(_State, _Index);
		return 1;
	}
	Log(ELOG_ERROR, "metafield is not a c function.");
	return 0;
}

int LuaCreateGood(lua_State* _State) {
	const struct GoodBase* _GoodBase = NULL;
	struct Good* _Good = NULL;
	const char* _Name = NULL;
	int _Quantity;

	_Name = luaL_checkstring(_State, 1);
	_Quantity = luaL_checkinteger(_State, 2);
	if((_GoodBase = HashSearch(&g_Goods, _Name)) == NULL)
		luaL_error(_State, "Cannot find GoodBase %s.", _Name);
	_Good = CreateGood(_GoodBase, -1, -1);
	_Good->Quantity = _Quantity;
	lua_pushlightuserdata(_State, _Good);
	return 1;
}

int LuaCreateBuilding(lua_State* _State) {
	int _Width = 0;
	int _Length = 0;
	int _ResType = 0;
	const char* _Floor = NULL;
	const char* _Walls = NULL;
	const char* _Roof = NULL;
	const char* _Type = NULL;
	struct BuildMat* _FloorMat = NULL;
	struct BuildMat* _WallMat = NULL;
	struct BuildMat* _RoofMat = NULL;

	_Width = luaL_optint(_State, 1, 10);
	_Length = luaL_optint(_State, 2, 10);
	_Floor = luaL_checkstring(_State, 3);
	if((_FloorMat = HashSearch(&g_BuildMats, _Floor)) == NULL)
		return luaL_error(_State, "%s is not a BuildMat.", _Floor);
	_Walls = luaL_checkstring(_State, 4);
	if((_WallMat = HashSearch(&g_BuildMats, _Walls)) == NULL)
		return luaL_error(_State, "%s is not a BuildMat.", _Walls);
	_Roof = luaL_checkstring(_State, 5);
	if((_RoofMat = HashSearch(&g_BuildMats, _Roof)) == NULL)
		return luaL_error(_State, "%s is not a BuildMat.", _Roof);
	_Type = luaL_optstring(_State, 6, "Human");
	if(strcmp(_Type, "Human") == 0)
		_ResType = ERES_HUMAN;
	else if(strcmp(_Type, "Animal") == 0)
		_ResType = ERES_ANIMAL;
	else if(strcmp(_Type, "All") == 0)
		_ResType = ERES_HUMAN | ERES_ANIMAL;
	else
		return luaL_error(_State, "%s is not a valid house type.", _Type);
	lua_pushlightuserdata(_State, CreateBuilding(_ResType, _Width, _Length, _WallMat, _FloorMat, _RoofMat));
	return 1;
}

int LuaCreateAnimal(lua_State* _State) {
	const char* _Name = NULL;
	struct Population* _Population = NULL;

	_Name = luaL_checkstring(_State, 1);
	if((_Population = HashSearch(&g_Populations, _Name)) == NULL)
		return luaL_error(_State, "Cannot find Population %s.", _Name);
	lua_pushlightuserdata(_State, CreateAnimal(_Population, Random(0, _Population->Ages[AGE_DEATH]->Max), 1500, -1, -1));
	return 1;
}

void LuaStackToTable(lua_State* _State, int* _Table) {
	int _Top = lua_gettop(_State);
	int i;

	for(i = 0; i < _Top; ++i)
		_Table[i] = lua_type(_State, i);
}

void LuaCopyTable(lua_State* _State, int _Index) {
	if(lua_type(_State, _Index) != LUA_TTABLE)
		return;
	if(lua_type(_State, -1) != LUA_TTABLE)
		return;
	lua_pushvalue(_State, _Index);
	lua_pushnil(_State);
	while(lua_next(_State, -2) != 0) {
		lua_pushvalue(_State, -2);
		lua_pushvalue(_State, -2);
		lua_rawset(_State, -6);
		lua_pop(_State, 1);
	}
	lua_pop(_State, 1);
	lua_remove(_State, _Index);
}

void* LuaToClass(lua_State* _State, int _Index) {
	void* _Pointer = NULL;
	int _Pos = LuaAbsPos(_State, _Index);

	if((_Pointer = lua_touserdata(_State, _Pos)) == NULL) {
		if(lua_type(_State, _Pos) == LUA_TNIL)
			return NULL;
		if(lua_type(_State, _Pos) != LUA_TTABLE)
			luaL_error(_State, "LuaToClass: index is not a class (expected table got %s).", lua_typename(_State, lua_type(_State, _Pos)));
		lua_pushstring(_State, "__self");
		lua_rawget(_State, _Pos);
		_Pointer = lua_touserdata(_State, -1);
		lua_pop(_State, 1);
	}
	return _Pointer;
}

void* LuaTestClass(lua_State* _State, int _Index, const char* _Class) {
	if(lua_getmetatable(_State, _Index) == 0)
		 luaL_error(_State, LUA_TYPERROR(_State, 1, _Class, "LuaTestClass"));
	lua_pushstring(_State, "__class");
	lua_rawget(_State, -2);
	if(!lua_isstring(_State, -1) || strcmp(_Class, lua_tostring(_State, -1)) != 0) {
		lua_pop(_State, 2);
		return NULL;
	}
	lua_pop(_State, 2);
	return LuaToClass(_State, _Index);
}

void* LuaCheckClass(lua_State* _State, int _Index, const char* _Class) {
	int _Pop = 4;
	lua_getglobal(_State, _Class);
	if(lua_getmetatable(_State, _Index) == 0) {
		lua_pop(_State, 1);
		return NULL;
	}
	if(lua_rawequal(_State, -1, -2)) {
		lua_pop(_State, 2);
		return LuaToClass(_State, _Index);
	}
	lua_pop(_State, 2);
	lua_pushvalue(_State, _Index);
	if(lua_getmetatable(_State, -1) == 0) {
		lua_pop(_State, 1);
		goto end;
	}
	top:
	lua_pushliteral(_State, "__baseclass");
	lua_rawget(_State, -2);
	if(lua_type(_State, -1) == LUA_TTABLE) {
		lua_getglobal(_State, _Class);
	if(!lua_rawequal(_State, -1, -2)) {
			lua_copy(_State, -2, -4);
			lua_pop(_State, 3);
			_Pop = 3;
			goto top;
		}
		lua_pop(_State, _Pop);
		return LuaToClass(_State, _Index);
	}
	end:
	lua_pop(_State, 1);
	return (void*) luaL_error(_State, LUA_TYPERROR(_State, 1, _Class, "LuaCheckClass"));
}

int LuaIntPair(lua_State* _State, int _Index, int* _One, int* _Two) {
	int _Top = lua_gettop(_State);

	lua_pushnil(_State);
	if(lua_next(_State, _Index - 1) == 0)
		goto fail;
	if(AddInteger(_State, -1, _One) == -1)
		goto fail;
	lua_pop(_State, 1);
	if(lua_next(_State, _Index - 1) == 0)
		goto fail;
	if(AddInteger(_State, -1, _Two) == -1)
		goto fail;
	lua_pop(_State, 2);
	return 1;
	fail:
	lua_settop(_State, _Top);
	return 0;
}

int LuaKeyValue(lua_State* _State, int _Index, const char** _Value, int* _Pair) {
	int _Top = lua_gettop(_State);

	lua_pushnil(_State);
	if(lua_next(_State, _Index - 1) == 0)
		goto fail;
	if(AddString(_State, -1, _Value) == -1)
		goto fail;
	lua_pop(_State, 1);
	if(lua_next(_State, _Index - 1) == 0)
		goto fail;
	if(AddInteger(_State, -1, _Pair) == -1)
		goto fail;
	lua_pop(_State, 2);
	return 1;
	fail:
	lua_settop(_State, _Top);
	return 0;
}
