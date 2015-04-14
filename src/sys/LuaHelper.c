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
#include "Rule.h"
#include "../Herald.h"
#include "../Good.h"
#include "../Crop.h"
#include "../Building.h"
#include "../Population.h"
#include "../Person.h"
#include "../Family.h"
#include "../Zone.h"
#include "../Location.h"

#include <stdlib.h>
#include <string.h>
#include <lua/lauxlib.h>
#include <malloc.h>

lua_State* g_LuaState = NULL;

static const luaL_Reg g_LuaFuncsSettlement[] = {
		{"GetLeader", LuaSettlementGetLeader},
		{NULL, NULL}
};

static const luaL_Reg g_LuaFuncsRule[] = {
		{"LuaCall", LuaRuleLuaCall},
		{"GreaterThan", LuaRuleGreaterThan},
		{"LessThan", LuaRuleLessThan},
		{"True", LuaRuleTrue},
		{"False", LuaRuleFalse},
		{NULL, NULL}
};

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
		{"GetBuildMat", LuaBuildMat},
		{"Random", LuaRandom},
		{NULL, NULL}
};

static const luaL_Reg g_LuaFuncsIterator[] = {
		{"Itr", NULL},
		{"Next", NULL},
		{"Prev", NULL},
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
		{"GetStatus", LuaFieldGetStatus},
		{"GetStatusTime", LuaFieldGetStatusTime},
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

static const luaL_Reg g_LuaFuncsBuilding[] = {
		{"GetId", LuaBuildingGetId},
		{"GetWidth", LuaBuildingGetWidth},
		{"GetLength", LuaBuildingGetLength},
		{"ConstructionTime", LuaBuildingConstructionTime},
		{NULL, NULL}
};
static const luaL_Reg g_LuaFuncsArray[] = {
		{"Create", LuaArrayCreate},
		{NULL, NULL}
};

static struct LuaObjectReg g_ObjectRegs[] = {
		{"Settlement", g_LuaFuncsSettlement},
		{"Rule", g_LuaFuncsRule},
		{"Iterator", g_LuaFuncsIterator},
		{"Person", g_LuaFuncsPerson},
		{"Good", g_LuaFuncsGood},
		{"Family", g_LuaFuncsFamily},
		{"Field", g_LuaFuncsField},
		{"Animal", g_LuaFuncsField},
		{"Building", g_LuaFuncsBuilding},
		{"Array", g_LuaFuncsArray},
		{"ArrayItr", NULL},
		{"BuildMat", NULL},
		{NULL, NULL}
};

void RegisterLuaFuncs(lua_State* _State) {
	int i = 0;

	for(i = 0; (g_LuaFuncs[i].name != NULL && g_LuaFuncs[i].func != NULL); ++i)
		lua_register(_State, g_LuaFuncs[i].name, g_LuaFuncs[i].func);

	i = 0;
	while(g_ObjectRegs[i].Name != NULL) {
		if(LuaRegisterObject(_State, g_ObjectRegs[i].Name, g_ObjectRegs[i].Funcs) == 0)
			return (void) luaL_error(_State, "Loading Lua functions has failed.");
		++i;
	}
}

int LuaRegisterObject(lua_State* _State, const char* _Name, const luaL_Reg* _Funcs) {
	if(luaL_newmetatable(_State, _Name) == 0)
		return 0;
	lua_pushliteral(_State, "__index");
	lua_pushvalue(_State, -2);
	lua_rawset(_State, -3);
	lua_pushstring(_State, "__class");
	lua_pushstring(_State, _Name);
	lua_rawset(_State, -3);

	lua_pushliteral(_State, "__newindex");
	lua_pushnil(_State);
	lua_rawset(_State, -3);
	if(_Funcs != NULL)
		luaL_setfuncs(_State, _Funcs, 0);
	lua_setglobal(_State, _Name);
	return 1;
}

int LuaPersonGetId(lua_State* _State) {
	struct Person* _Person = (struct Person*) LuaToObject(_State, 1, "Person");

	lua_pushinteger(_State, _Person->Id);
	return 1;
}

int LuaPersonGetX(lua_State* _State) {
	struct Person* _Person = (struct Person*) LuaToObject(_State, 1, "Person");

	lua_pushinteger(_State, _Person->X);
	return 1;
}

int LuaPersonGetY(lua_State* _State) {
	struct Person* _Person = (struct Person*) LuaToObject(_State, 1, "Person");

	lua_pushinteger(_State, _Person->Y);
	return 1;
}

int LuaPersonGetGender(lua_State* _State) {
	struct Person* _Person = (struct Person*) LuaToObject(_State, 1, "Person");

	lua_pushinteger(_State, _Person->Gender);
	return 1;
}

int LuaPersonGetNutrition(lua_State* _State) {
	struct Person* _Person = (struct Person*) LuaToObject(_State, 1, "Person");

	lua_pushinteger(_State, _Person->Nutrition);
	return 1;
}

int LuaPersonGetAge(lua_State* _State) {
	struct Person* _Person = (struct Person*) LuaToObject(_State, 1, "Person");

	lua_pushinteger(_State, _Person->Age);
	return 1;
}

int LuaPersonGetName(lua_State* _State) {
	struct Person* _Person = (struct Person*) LuaToObject(_State, 1, "Person");

	lua_pushstring(_State, _Person->Name);
	return 1;
}

int LuaPersonGetFamily(lua_State* _State) {
	struct Person* _Person = (struct Person*) LuaToObject(_State, 1, "Person");

	lua_newtable(_State);
	lua_getglobal(_State, "Family");
	lua_setmetatable(_State, -2);

	lua_pushstring(_State, "__self");
	lua_pushlightuserdata(_State, _Person->Family);
	lua_rawset(_State, -3);
	return 1;
}

int LuaPersonGetParentFamily(lua_State* _State) {
	struct Person* _Person = (struct Person*) LuaToObject(_State, 1, "Person");

	lua_newtable(_State);
	lua_getglobal(_State, "Family");
	lua_setmetatable(_State, -2);

	lua_pushstring(_State, "__self");
	lua_pushlightuserdata(_State, _Person->Parent);
	lua_rawset(_State, -3);
	return 1;
}

int LuaGoodGetId(lua_State* _State) {
	struct Good* _Good = (struct Good*) LuaToObject(_State, 1, "Good");

	lua_pushinteger(_State, _Good->Id);
	return 1;
}

int LuaGoodGetQuantity(lua_State* _State) {
	struct Good* _Good = (struct Good*) LuaToObject(_State, 1, "Good");

	lua_pushinteger(_State, _Good->Quantity);
	return 1;
}

int LuaGoodGetBase(lua_State* _State) {
	struct Good* _Good = (struct Good*) LuaToObject(_State, 1, "Good");

	lua_pushstring(_State, _Good->Base->Name);
	lua_remove(_State, -2);
	LuaGoodBase(_State);
	return 1;
}

int LuaFamilyGetId(lua_State* _State) {
	struct Family* _Family = (struct Family*) LuaToObject(_State, 1, "Family");

	lua_pushinteger(_State, _Family->Id);
	return 1;
}

int LuaFamilyChildrenCt(lua_State* _State) {
	struct Family* _Family = (struct Family*) LuaToObject(_State, 1, "Family");

	lua_pushinteger(_State, _Family->NumChildren);
	return 1;
}

int LuaFamilyGetName(lua_State* _State) {
	struct Family* _Family = (struct Family*) LuaToObject(_State, 1, "Family");

	lua_pushstring(_State, _Family->Name);
	return 1;
}

int LuaFamilyGetPeople(lua_State* _State) {
	struct Family* _Family = (struct Family*) LuaToObject(_State, 1, "Family");
		int i;

		lua_createtable(_State, 10, 0);
		for(i = 0; i < FAMILY_PEOPLESZ; ++i) {
			lua_pushlightuserdata(_State, _Family->People[i]);
			lua_rawseti(_State, -2, i + 1);
		}
		return 1;
}

int LuaFamilyGetFields(lua_State* _State) {
	struct Family* _Family = (struct Family*) LuaToObject(_State, 1, "Family");

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
	struct Family* _Family = (struct Family*) LuaToObject(_State, 1, "Family");

	lua_newtable(_State);
	lua_getglobal(_State, "ArrayIterator");
	lua_setmetatable(_State, -2);

	lua_pushstring(_State, "__self");
	lua_pushlightuserdata(_State, _Family->Buildings);
	lua_rawset(_State, -3);

	lua_pushstring(_State, "__classtype");
	lua_pushstring(_State, "Building");
	lua_rawset(_State, -3);
	return 1;
}

int LuaFamilyGetBulidingCt(lua_State* _State) {
	struct Family* _Family = (struct Family*) LuaToObject(_State, 1, "Family");

	lua_pushinteger(_State, _Family->Buildings->Size);
	return 1;
}

int LuaFamilyGetGoods(lua_State* _State) {
	struct Family* _Family = (struct Family*) LuaToObject(_State, 1, "Family");

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
	struct Family* _Family = (struct Family*) LuaToObject(_State, 1, "Family");

	lua_pushinteger(_State, _Family->Goods->Size);
	return 1;
}

int LuaFamilyGetAnimals(lua_State* _State) {
	struct Family* _Family = (struct Family*) LuaToObject(_State, 1, "Family");

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
	struct Family* _Family = (struct Family*) LuaToObject(_State, 1, "Family");

	lua_pushinteger(_State, _Family->Animals->Size);
	return 1;
}

int LuaFieldGetId(lua_State* _State) {
	struct Field* _Field = (struct Field*) LuaToObject(_State, 1, "Field");

	lua_pushinteger(_State, _Field->Id);
	return 1;
}

int LuaFieldGetCrop(lua_State* _State) {
	struct Field* _Field = (struct Field*) LuaToObject(_State, 1, "Field");

	if(_Field == NULL || _Field->Crop == NULL) {
		lua_pushnil(_State);
		return 1;
	}
	lua_pushstring(_State, _Field->Crop->Name);
	lua_remove(_State, -2);
	LuaCrop(_State);
	return 1;
}

int LuaFieldGetYield(lua_State* _State) {
	struct Field* _Field = (struct Field*) LuaToObject(_State, 1, "Field");

	lua_pushinteger(_State, _Field->YieldTotal);
	return 1;
}

int LuaFieldGetAcres(lua_State* _State) {
	struct Field* _Field = (struct Field*) LuaToObject(_State, 1, "Field");

	lua_pushinteger(_State, _Field->Acres);
	return 1;
}

int LuaFieldGetStatus(lua_State* _State) {
	struct Field* _Field = (struct Field*) LuaToObject(_State, 1, "Field");

	lua_pushinteger(_State, _Field->Status);
	return 1;
}

int LuaFieldGetStatusTime(lua_State* _State) {
	struct Field* _Field = (struct Field*) LuaToObject(_State, 1, "Field");

	lua_pushinteger(_State, _Field->StatusTime);
	return 1;
}

int LuaAnimalGetId(lua_State* _State) {
	struct Animal* _Animal = (struct Animal*) LuaToObject(_State, 1, "Animal");

	lua_pushinteger(_State, _Animal->Id);
	return 1;
}

int LuaAnimalIsMale(lua_State* _State) {
	struct Animal* _Animal = (struct Animal*) LuaToObject(_State, 1, "Animal");

	lua_pushboolean(_State, (_Animal->Gender == EMALE));
	return 1;
}

int LuaAnimalGetNutrition(lua_State* _State) {
	struct Animal* _Animal = (struct Animal*) LuaToObject(_State, 1, "Animal");

	lua_pushinteger(_State, _Animal->Nutrition);
	return 1;
}

int LuaAnimalGetAge(lua_State* _State) {
	struct Animal* _Animal = (struct Animal*) LuaToObject(_State, 1, "Animal");

	lua_pushinteger(_State, _Animal->Age);
	return 1;
}

int LuaAnimalGetBase(lua_State* _State) {
	struct Animal* _Animal = (struct Animal*) LuaToObject(_State, 1, "Animal");

	if(_Animal == NULL) {
		lua_pushnil(_State);
		return 1;
	}
	lua_pushstring(_State, _Animal->PopType->Name);
	lua_remove(_State, -2);
	LuaPopulation(_State);
	return 1;
}

int LuaBuildingGetId(lua_State* _State) {
	struct Building* _Building = (struct Building*) LuaToObject(_State, 1, "Building");

	lua_pushinteger(_State, _Building->Id);
	return 1;
}

int LuaBuildingGetWidth(lua_State* _State) {
	struct Building* _Building = (struct Building*) LuaToObject(_State, 1, "Building");

	lua_pushinteger(_State, _Building->Id);
	return 1;
}

int LuaBuildingGetLength(lua_State* _State) {
	struct Building* _Building = (struct Building*) LuaToObject(_State, 1, "Building");

	lua_pushinteger(_State, _Building->Id);
	return 1;
}

int LuaBuildingConstructionTime(lua_State* _State) {
	struct BuildMat* _Floor = (struct BuildMat*) LuaToObject(_State, 1, "BuildMat");
	struct BuildMat* _Walls = (struct BuildMat*) LuaToObject(_State, 1, "BuildMat");
	struct BuildMat* _Roof = (struct BuildMat*) LuaToObject(_State, 1, "BuildMat");
	int _Area = luaL_checkinteger(_State, 4);

	lua_pushinteger(_State, ConstructionTime(_Floor, _Walls, _Roof, _Area));
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

int LuaSettlementGetLeader(lua_State* _State) {
	return 0;
}

int LuaArrayItr(lua_State* _State) {
	lua_pushlightuserdata(_State, LuaCheckClass(_State, 1, "Iterator"));
	return 1;
}

void* LuaToObject(lua_State* _State, int _Index, const char* _Name) {
	void* _Obj = NULL;

	if((_Obj = LuaTestClass(_State, _Index, _Name)) == NULL)
		return LuaCheckClass(_State, _Index, _Name);
	return _Obj;
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
	lua_pushinteger(_State, _Crop->GrowingDegree);
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
	int _Pos = LuaAbsPos(_State, _Index);

	if(lua_type(_State, _Pos) != LUA_TLIGHTUSERDATA)
		luaL_error(_State, LUA_TYPERROR(_State, 1, "Person", "LuaPushPerson"));
	LuaCtor(_State, "Person", lua_touserdata(_State, _Pos));
	return 1;
}

int LuaPerson(lua_State* _State) {
	LuaPushPerson(_State, 1);
	return 1;
}

int LuaBuildMat(lua_State* _State) {
	const char* _Name = NULL;
	struct BuildMat* _BuildMat = NULL;

	if(lua_isstring(_State, 1))
		_Name = luaL_checkstring(_State, 1);
	else {
		luaL_error(_State, LUA_TYPERROR(_State, 1, "string", "BuildMat"));
		return 0;
	}
	if((_BuildMat = HashSearch(&g_BuildMats, _Name)) == NULL)
		return luaL_error(_State, "BuildMat not given a valid build mat.");
	lua_newtable(_State);
	lua_getglobal(_State, "BuildMat");
	lua_setmetatable(_State, -2);
	lua_pushstring(_State, "__self");
	lua_pushlightuserdata(_State, _BuildMat);
	lua_rawset(_State, -3);
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

int LuaRandom(lua_State* _State) {
	Random(luaL_checkinteger(_State, 1), luaL_checkinteger(_State, 2));
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

int LuaLoadList(lua_State* _State, const char* _File, const char* _Global, void*(*_Callback)(lua_State*, int), void(*_Insert)(struct LinkedList*, void*), struct LinkedList* _Return) {
	void* _CallRet = NULL;

	if(LuaLoadFile(_State, _File) != LUA_OK)
		return 0;
	lua_getglobal(_State, _Global);
	if(!lua_istable(_State, -1))
		return 0;
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
	return 1;
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
	int _X = 0;
	int _Y = 0;
	int i = 0;
	int _Ct = 0;
	int _ResType = 0;
	int _Width = 0;
	int _Length = 0;
	const char* _Type = NULL;
	const char* _Str = NULL;
	struct ZoneBase* _ZoneBase = NULL;
	struct Zone* _Zone = NULL;
	struct Zone** _ZoneTbl = NULL;
	struct BuildMat* _FloorMat = NULL;
	struct BuildMat* _WallMat = NULL;
	struct BuildMat* _RoofMat = NULL;
	struct Location* _Location = NULL;

	luaL_checktype(_State, 1, LUA_TTABLE);
	_Location = lua_touserdata(_State, 5);
	if(_Location->Type != ELOC_SETTLEMENT)
		luaL_error(_State, "Location is not a settlement.");
	_ZoneTbl = alloca((lua_rawlen(_State, 1) + 1) * sizeof(struct Zone*));
	lua_pushvalue(_State, 1);
	lua_pushnil(_State);
	while(lua_next(_State, -2) != 0) {
		if(lua_type(_State, -1) != LUA_TTABLE)
			goto loop_end;
		lua_rawgeti(_State, -1, 1);
		if(lua_type(_State, -1) != LUA_TSTRING) {
			lua_pop(_State, 1);
			goto loop_end;
		}
		_Str = lua_tostring(_State, -1);
		for(i = 0; i < g_ZoneCt; ++i) {
			if(strcmp(_Str, g_Zones[i].Name) == 0) {
				_ZoneBase = &g_Zones[i];
				break;
			}
		}
		lua_pop(_State, 1);
		if(_ZoneBase == NULL)
			goto loop_end;
		lua_rawgeti(_State, -1, 2);
		if(lua_type(_State, -1) != LUA_TNUMBER) {
			lua_pop(_State, 1);
			goto loop_end;
		}
		_Width = lua_tointeger(_State, -1);
		lua_pop(_State, 1);
		lua_rawgeti(_State, -1, 3);
		if(lua_type(_State, -1) != LUA_TNUMBER) {
			lua_pop(_State, 1);
			goto loop_end;
		}
		_Length = lua_tointeger(_State, -1);
		lua_pop(_State, 1);
		_Zone = alloca(sizeof(struct Zone));
		_Zone->Id = NextZoneId();
		_Zone->X = 0;
		_Zone->Y = 0;
		_Zone->Width = _Width;
		_Zone->Length = _Length;
		_Zone->Owner = NULL;
		_Zone->Base = _ZoneBase;
		_ZoneTbl[_Ct++] = _Zone;
		loop_end:
		lua_pop(_State, 1);
	}
	_ZoneTbl[_Ct] = NULL;
	lua_pop(_State, 1);
	_FloorMat = (struct BuildMat*) LuaToObject(_State, 2, "BuildMat");
	_WallMat = (struct BuildMat*) LuaToObject(_State, 3, "BuildMat");
	_RoofMat = (struct BuildMat*) LuaToObject(_State, 4, "BuildMat");
	_Type = luaL_optstring(_State, 6, "Human");
	if(strcmp(_Type, "Human") == 0)
		_ResType = ERES_HUMAN;
	else if(strcmp(_Type, "Animal") == 0)
		_ResType = ERES_ANIMAL;
	else if(strcmp(_Type, "All") == 0)
		_ResType = ERES_HUMAN | ERES_ANIMAL;
	else
		return luaL_error(_State, "%s is not a valid house type.", _Type);
	_Width = 0;
	_Length = 0;
	BuildingPlanSize((const struct Zone**)_ZoneTbl, &_Width, &_Length);
	PlaceBuilding((struct CityLocation*) _Location, _Width, _Length, &_X, &_Y);
	_ZoneTbl[0]->X = _X;
	_ZoneTbl[0]->Y = _Y;
	HORIZONTAL_WALLS(_ZoneTbl[0]->X - 1, _ZoneTbl[0]->Y - 1, _ZoneTbl[0]->Width);
	HORIZONTAL_WALLS(_ZoneTbl[0]->X - 1, _ZoneTbl[0]->Y + _ZoneTbl[0]->Width + 1, _ZoneTbl[0]->Width);
	VERTICAL_WALLS(_ZoneTbl[0]->X - 1, _ZoneTbl[0]->Y, _ZoneTbl[0]->Length);
	VERTICAL_WALLS(_ZoneTbl[0]->X - 1, _ZoneTbl[0]->Y + _ZoneTbl[0]->Length, _ZoneTbl[0]->Length);
	LuaCtor(_State, "Building", CreateBuilding(_ResType, _WallMat, _FloorMat, _RoofMat, _ZoneTbl))
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
