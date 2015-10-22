/*
 * File: LuaFamily.c
 * Author: David Brotz
 */

#include "LuaFamily.h"

#include "Good.h"
#include "Crop.h"
#include "Building.h"
#include "Population.h"
#include "Person.h"
#include "Family.h"
#include "Location.h"

#include "sys/LuaCore.h"
#include "sys/Log.h"
#include "sys/Array.h"
#include "sys/Constraint.h"
#include "sys/Math.h"

#include <lua/lauxlib.h>
#include <lua/lualib.h>
#include <string.h>
#include <malloc.h>

static const luaL_Reg g_LuaFamilyFuncs[] = {
		{"Crop", LuaCrop},
		{"Good", LuaGoodBase},
		{"Food", LuaFoodBase},
		{"GetAnimal", LuaPopulation},
		{"CountAnimal", LuaFamilyCountAnimal},
		{"KillAnimal", LuaFamillyKillAnimal},
		{"GetPerson", LuaPerson},
		{"CreateGood", LuaCreateGood},
		{"CreateBuilding", LuaCreateBuilding},
		{"CreateAnimal", LuaCreateAnimal},
		{"GetBuildMat", LuaBuildMat},
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

static const struct LuaObjectReg g_LuaFamilyObjects[] = {
		{"Person", NULL, g_LuaFuncsPerson},
		{"Good", NULL, g_LuaFuncsGood},
		{"Family", NULL, g_LuaFuncsFamily},
		{"Field", NULL, g_LuaFuncsField},
		{"Animal", NULL, g_LuaFuncsAnimal},
		{"Building", NULL, g_LuaFuncsBuilding},
		{NULL, NULL, NULL}
};

void InitLuaFamily() {
	RegisterLuaObjects(g_LuaState, g_LuaFamilyObjects);
	LuaRegisterFunctions(g_LuaState, g_LuaFamilyFuncs);
}

int LuaPersonGetId(lua_State* _State) {
	struct Person* _Person = (struct Person*) LuaToObject(_State, 1, "Person");

	lua_pushinteger(_State, _Person->Id);
	return 1;
}

int LuaPersonGetX(lua_State* _State) {
	struct Person* _Person = (struct Person*) LuaToObject(_State, 1, "Person");

	lua_pushinteger(_State, _Person->Pos.x);
	return 1;
}

int LuaPersonGetY(lua_State* _State) {
	struct Person* _Person = (struct Person*) LuaToObject(_State, 1, "Person");

	lua_pushinteger(_State, _Person->Pos.y);
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

	LuaCtor(_State, "Family", _Person->Family);
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

	LuaCtor(_State, "ArrayIterator", _Family->Fields);
	lua_pushstring(_State, "__classtype");
	lua_pushstring(_State, "Field");
	lua_rawset(_State, -3);
	return 1;
}

int LuaFamilyGetBuildings(lua_State* _State) {
	struct Family* _Family = (struct Family*) LuaToObject(_State, 1, "Family");

	LuaCtor(_State, "ArrayIterator", _Family->Buildings);
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

	LuaCtor(_State, "ArrayIterator", _Family->Goods);
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

	LuaCtor(_State, "ArrayIterator", _Family->Animals);
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
	int _Pos = lua_absindex(_State, _Index);

	if(lua_type(_State, _Pos) != LUA_TLIGHTUSERDATA)
		luaL_error(_State, LUA_TYPERROR(_State, 1, "Person", "LuaPushPerson"));
	LuaCtor(_State, "Person", lua_touserdata(_State, _Pos));
	return 1;
}

int LuaFamilyCountAnimal(lua_State* _State) {
	struct Family* _Family = LuaToObject(_State, 1, "Family");
	const char* _Animal = luaL_checkstring(_State, 2);
	struct Population* _AnimalType = NULL;
	int _AnimalCt = 0;

	if((_AnimalType = HashSearch(&g_Populations, _Animal)) == NULL)
		goto end;
	for(int i = 0; i < _Family->Animals->Size; ++i) {
		if(_AnimalType->Id == ((struct Animal*)_Family->Animals->Table[i])->PopType->Id)
			++_AnimalCt;
	}
	end:
	lua_pushinteger(_State, _AnimalCt);
	return 1;
}

int LuaFamillyKillAnimal(lua_State* _State) {
	struct Family* _Family = LuaToObject(_State, 1, "Family");
	const char* _Animal = luaL_checkstring(_State, 2);
	struct Population* _AnimalType = NULL;
	int _KillAmount = luaL_checkinteger(_State, 3);
	int _KillCt = 0;

	if((_AnimalType = HashSearch(&g_Populations, _Animal)) == NULL)
		return 0;
	for(int i = 0; i < _Family->Animals->Size && _KillCt < _KillAmount; ++i) {
		if(_AnimalType->Id == ((struct Animal*)_Family->Animals->Table[i])->PopType->Id) {
			DestroyAnimal(_Family->Animals->Table[i]);
			ArrayRemove(_Family->Animals, i);
			--i;
			++_KillCt;
		}
	}
	return 0;
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
	LuaCtor(_State, "BuildMat", _BuildMat);
	return 1;
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
	LuaCtor(_State, "Good", _Good);
	return 1;
}

int LuaCreateBuilding(lua_State* _State) {
	int _ResType = 0;
	const char* _Type = NULL;
	struct BuildMat* _FloorMat = NULL;
	struct BuildMat* _WallMat = NULL;
	struct BuildMat* _RoofMat = NULL;
	struct Location* _Location = NULL;

	//luaL_checktype(_State, 1, LUA_TTABLE);
	_Location = lua_touserdata(_State, 4);
	if(_Location->LocType != ELOC_SETTLEMENT)
		luaL_error(_State, "Location is not a settlement.");
	/*lua_pushvalue(_State, 1);
	lua_pushnil(_State);
	while(lua_next(_State, -2) != 0) {
		if(lua_type(_State, -1) != LUA_TTABLE)
			goto loop_end;
		lua_rawgeti(_State, -1, 1);
		if(lua_type(_State, -1) != LUA_TSTRING) {
			lua_pop(_State, 1);
			goto loop_end;
		}
		lua_pop(_State, 1);
		lua_rawgeti(_State, -1, 2);
		if(lua_type(_State, -1) != LUA_TNUMBER) {
			lua_pop(_State, 1);
			goto loop_end;
		}
		lua_pop(_State, 1);
		lua_rawgeti(_State, -1, 3);
		if(lua_type(_State, -1) != LUA_TNUMBER) {
			lua_pop(_State, 1);
			goto loop_end;
		}
		lua_pop(_State, 1);
		loop_end:
		lua_pop(_State, 1);
	}
	lua_pop(_State, 1);*/
	_FloorMat = (struct BuildMat*) LuaToObject(_State, 1, "BuildMat");
	_WallMat = (struct BuildMat*) LuaToObject(_State, 2, "BuildMat");
	_RoofMat = (struct BuildMat*) LuaToObject(_State, 3, "BuildMat");
	_Type = luaL_optstring(_State, 5, "Human");
	if(strcmp(_Type, "Human") == 0)
		_ResType = ERES_HUMAN;
	else if(strcmp(_Type, "Animal") == 0)
		_ResType = ERES_ANIMAL;
	else if(strcmp(_Type, "All") == 0)
		_ResType = ERES_HUMAN | ERES_ANIMAL;
	else
		return luaL_error(_State, "%s is not a valid house type.", _Type);
	LuaCtor(_State, "Building", CreateBuilding(_ResType, _WallMat, _FloorMat, _RoofMat));
	return 1;
}

int LuaCreateAnimal(lua_State* _State) {
	const char* _Name = NULL;
	struct Population* _Population = NULL;

	_Name = luaL_checkstring(_State, 1);
	if((_Population = HashSearch(&g_Populations, _Name)) == NULL)
		return luaL_error(_State, "Cannot find Population %s.", _Name);
	LuaCtor(_State, "Animal", CreateAnimal(_Population, Random(0, _Population->Ages[AGE_DEATH]->Max), 1500, -1, -1));
	return 1;
}

