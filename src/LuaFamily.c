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
	{"Banish", LuaPersonBanish},
	{"InRetinue", LuaPersonInRetinue},
	{"Retinue", LuaPersonRetinue},
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
	{"ChildrenCt", LuaFamilyChildrenCt},
	{"GetName", LuaFamilyGetName},
	{"GetPeople", LuaFamilyGetPeople},
	{"GetFields", LuaFamilyGetFields},
	{"GetBuildings", LuaFamilyGetBuildings},
	{"GetBuildingCt", LuaFamilyGetBulidingCt},
	{"GetGoods", LuaFamilyGetGoods},
	{"GetGoodCt", LuaFamilyGetGoodCt},
	{"GetAnimals", LuaFamilyGetAnimals},
	{"GetAnimalCt", LuaFamilyGetAnimalCt},
	{"ChangeNutrition", LuaFamilyChangeNutrition},
	{"CountAnimal", LuaFamilyCountAnimal},
	{"TakeAnimal", LuaFamilyTakeAnimal},
	{"GetSize", LuaFamilyGetSize},
	{"GetNutrition", LuaFamilyGetNutrition},
	{"GetNutritionReq", LuaFamilyGetNutritionReq},
	{"GetWealth", LuaFamilyGetWealth},
	{NULL, NULL}
};

static const struct LuaEnum g_LuaPersonEnum[] = {
	{"DailyNut", NUTRITION_DAILY},
	{"Male", EMALE},
	{"Female", EFEMALE},
	{NULL, 0}
};

static const luaL_Reg g_LuaFuncsField[] = {
	{"GetId", LuaFieldGetId},
	{"GetCrop", LuaFieldGetCrop},
	{"GetYield", LuaFieldGetYield},
	{"GetAcres", LuaFieldGetAcres},
	{"GetStatus", LuaFieldGetStatus},
	{"GetStatusTime", LuaFieldGetStatusTime},
	{"StatusCompletion", LuaFieldStatusCompletion},
	{NULL, NULL}
};

static const luaL_Reg g_LuaFuncsAnimal[] = {
	{"GetId", LuaAnimalGetId},
	{"IsMale", LuaAnimalIsMale},
	{"GetGender", LuaAnimalGetGender},
	{"GetNutrition", LuaAnimalGetNutrition},
	{"GetAge", LuaAnimalGetAge},
	{"GetBase", LuaAnimalGetBase},
	{NULL, NULL}
};

static const struct LuaEnum g_LuaAnimalEnum[] = {
	{"Male", EMALE},
	{"Female", EFEMALE},
	{NULL, 0}
};

static const luaL_Reg g_LuaFuncsBuilding[] = {
	{"GetId", LuaBuildingGetId},
	{"ConstructionTime", LuaBuildingConstructionTime},
	{NULL, NULL}
};

static const struct LuaObjectReg g_LuaFamilyObjects[] = {
	{LOBJ_PERSON, "Person", LUA_REFNIL, g_LuaFuncsPerson},
	{LOBJ_GOOD, "Good", LUA_REFNIL, g_LuaFuncsGood},
	{LOBJ_FAMILY, "Family", LUA_REFNIL, g_LuaFuncsFamily},
	{LOBJ_FIELD, "Field", LUA_REFNIL, g_LuaFuncsField},
	{LOBJ_ANIMAL, "Animal", LUA_REFNIL, g_LuaFuncsAnimal},
	{LOBJ_BUILDING, "Building", LUA_REFNIL, g_LuaFuncsBuilding},
	{LUA_REFNIL, NULL, LUA_REFNIL, NULL}
};

const struct LuaEnumReg g_LuaFamilyEnums[] = {
	{"Person", NULL,  g_LuaPersonEnum},
	{"Animal", NULL, g_LuaAnimalEnum},
	{NULL, NULL}
};

void InitLuaFamily(lua_State* _State) {
	RegisterLuaObjects(_State, g_LuaFamilyObjects);
	LuaRegisterFunctions(_State, g_LuaFamilyFuncs);
	RegisterLuaEnums(_State, g_LuaFamilyEnums);
}

int LuaPersonGetId(lua_State* _State) {
	struct Person* _Person = (struct Person*) LuaToObject(_State, 1, LOBJ_PERSON);

	lua_pushinteger(_State, _Person->Object.Id);
	return 1;
}

int LuaPersonGetX(lua_State* _State) {
	struct Person* _Person = (struct Person*) LuaToObject(_State, 1, LOBJ_PERSON);

	lua_pushinteger(_State, _Person->Pos.x);
	return 1;
}

int LuaPersonGetY(lua_State* _State) {
	struct Person* _Person = (struct Person*) LuaToObject(_State, 1, LOBJ_PERSON);

	lua_pushinteger(_State, _Person->Pos.y);
	return 1;
}

int LuaPersonGetGender(lua_State* _State) {
	struct Person* _Person = (struct Person*) LuaToObject(_State, 1, LOBJ_PERSON);

	lua_pushinteger(_State, _Person->Gender);
	return 1;
}

int LuaPersonGetNutrition(lua_State* _State) {
	struct Person* _Person = (struct Person*) LuaToObject(_State, 1, LOBJ_PERSON);

	lua_pushinteger(_State, _Person->Nutrition);
	return 1;
}

int LuaPersonGetAge(lua_State* _State) {
	struct Person* _Person = (struct Person*) LuaToObject(_State, 1, LOBJ_PERSON);

	lua_pushinteger(_State, _Person->Age);
	return 1;
}

int LuaPersonGetName(lua_State* _State) {
	struct Person* _Person = (struct Person*) LuaToObject(_State, 1, LOBJ_PERSON);

	if(_Person == NULL)
		return LuaClassError(_State, 1, LOBJ_PERSON);
	lua_pushstring(_State, _Person->Name);
	return 1;
}

int LuaPersonGetFamily(lua_State* _State) {
	struct Person* _Person = (struct Person*) LuaToObject(_State, 1, LOBJ_PERSON);

	LuaCtor(_State, _Person->Family, LOBJ_FAMILY);
	return 1;
}

int LuaPersonBanish(lua_State* _State) {
	struct Person* _Person = LuaCheckClass(_State, 1, LOBJ_PERSON);

	PersonDeath(_Person);
	return 0;
}

int LuaPersonInRetinue(lua_State* _State) {
	struct Person* _Leader = LuaCheckClass(_State, 1, LOBJ_PERSON);

	lua_pushboolean(_State, IntSearch(&g_GameWorld.PersonRetinue, _Leader->Object.Id) != NULL);
	return 1;
}

int LuaPersonRetinue(lua_State* _State) {
	struct Person* _Person = LuaCheckClass(_State, 1, LOBJ_PERSON);

	LuaCtor(_State, IntSearch(&g_GameWorld.PersonRetinue, _Person->Object.Id), LOBJ_RETINUE);
	return 1;
}

int LuaGoodGetId(lua_State* _State) {
	struct Good* _Good = (struct Good*) LuaToObject(_State, 1, LOBJ_GOOD);

	lua_pushinteger(_State, _Good->Id);
	return 1;
}

int LuaGoodGetQuantity(lua_State* _State) {
	struct Good* _Good = (struct Good*) LuaToObject(_State, 1, LOBJ_GOOD);

	lua_pushinteger(_State, _Good->Quantity);
	return 1;
}

int LuaGoodGetBase(lua_State* _State) {
	struct Good* _Good = (struct Good*) LuaToObject(_State, 1, LOBJ_GOOD);

	lua_pushstring(_State, _Good->Base->Name);
	lua_remove(_State, -2);
	LuaGoodBase(_State);
	return 1;
}

int LuaFamilyGetId(lua_State* _State) {
	struct Family* _Family = (struct Family*) LuaToObject(_State, 1, LOBJ_FAMILY);

	lua_pushinteger(_State, _Family->Object.Id);
	return 1;
}

int LuaFamilyChildrenCt(lua_State* _State) {
	struct Family* _Family = (struct Family*) LuaToObject(_State, 1, LOBJ_FAMILY);

	if(_Family == NULL)
		return LuaClassError(_State, 1, LOBJ_FAMILY);
	lua_pushinteger(_State, _Family->NumChildren);
	return 1;
}

int LuaFamilyGetName(lua_State* _State) {
	struct Family* _Family = (struct Family*) LuaToObject(_State, 1, LOBJ_FAMILY);

	if(_Family == NULL)
		return LuaClassError(_State, 1, LOBJ_FAMILY);
	lua_pushstring(_State, _Family->Name);
	return 1;
}

int LuaFamilyGetPeople(lua_State* _State) {
	struct Family* _Family = (struct Family*) LuaToObject(_State, 1, LOBJ_FAMILY);

	if(_Family == NULL)
		return LuaClassError(_State, 1, LOBJ_FAMILY);
	lua_createtable(_State, FAMILY_PEOPLESZ, 0);
	for(int i = 0; i < FAMILY_PEOPLESZ; ++i) {
		LuaCtor(_State, _Family->People[i], LOBJ_PERSON);
		lua_rawseti(_State, -2, i + 1);
	}
	return 1;
}

int LuaFamilyGetFields(lua_State* _State) {
	struct Family* _Family = (struct Family*) LuaToObject(_State, 1, LOBJ_FAMILY);

	lua_createtable(_State, _Family->FieldCt, 0);
	for(int i = 0; i < _Family->FieldCt; ++i) {
		LuaCtor(_State, _Family->Fields[i], LOBJ_FIELD);
		lua_rawseti(_State, -2, i + 1);
	}
	return 1;
}

int LuaFamilyGetBuildings(lua_State* _State) {
	struct Family* _Family = (struct Family*) LuaToObject(_State, 1, LOBJ_FAMILY);

	lua_createtable(_State, _Family->BuildingCt, 0);
	for(int i = 0; i < _Family->BuildingCt; ++i) {
		LuaCtor(_State, _Family->Buildings[i], LOBJ_BUILDING);
		lua_rawseti(_State, -2, i + 1);
	}
	return 1;
}

int LuaFamilyGetBulidingCt(lua_State* _State) {
	struct Family* _Family = (struct Family*) LuaToObject(_State, 1, LOBJ_FAMILY);

	lua_pushinteger(_State, _Family->BuildingCt);
	return 1;
}

int LuaFamilyGetGoods(lua_State* _State) {
	struct Family* _Family = (struct Family*) LuaToObject(_State, 1, LOBJ_FAMILY);

	CreateLuaArrayItr(_State, &_Family->Goods, LOBJ_GOOD);
	return 1;
}

int LuaFamilyGetGoodCt(lua_State* _State) {
	struct Family* _Family = (struct Family*) LuaToObject(_State, 1, LOBJ_FAMILY);

	lua_pushinteger(_State, _Family->Goods.Size);
	return 1;
}

int LuaFamilyGetAnimals(lua_State* _State) {
	struct Family* _Family = (struct Family*) LuaToObject(_State, 1, LOBJ_FAMILY);

	CreateLuaArrayItr(_State, &_Family->Animals, LOBJ_ANIMAL);
	return 1;
}

int LuaFamilyGetAnimalCt(lua_State* _State) {
	struct Family* _Family = (struct Family*) LuaToObject(_State, 1, LOBJ_FAMILY);

	lua_pushinteger(_State, _Family->Animals.Size);
	return 1;
}

int LuaFamilyChangeNutrition(lua_State* _State) {
	struct Family* _Family = (struct Family*) LuaToObject(_State, 1, LOBJ_FAMILY);
	int _Change = luaL_checkinteger(_State, 2);

	if(_Family->Food.SlowSpoiled - _Change < 0)
		_Family->Food.SlowSpoiled = 0;
	else
		_Family->Food.SlowSpoiled -= _Change;
	return 0;
}

int LuaFieldGetId(lua_State* _State) {
	struct Field* _Field = (struct Field*) LuaToObject(_State, 1, LOBJ_FIELD);

	lua_pushinteger(_State, _Field->Object.Id);
	return 1;
}

int LuaFieldGetCrop(lua_State* _State) {
	struct Field* _Field = (struct Field*) LuaToObject(_State, 1, LOBJ_FIELD);

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
	struct Field* _Field = (struct Field*) LuaToObject(_State, 1, LOBJ_FIELD);

	lua_pushinteger(_State, _Field->YieldTotal);
	return 1;
}

int LuaFieldGetAcres(lua_State* _State) {
	struct Field* _Field = (struct Field*) LuaToObject(_State, 1, LOBJ_FIELD);

	lua_pushinteger(_State, _Field->Acres);
	return 1;
}

int LuaFieldGetStatus(lua_State* _State) {
	struct Field* _Field = (struct Field*) LuaToObject(_State, 1, LOBJ_FIELD);
	
	lua_pushstring(_State, FieldStatusName(_Field));
	return 1;
}

int LuaFieldGetStatusTime(lua_State* _State) {
	struct Field* _Field = (struct Field*) LuaToObject(_State, 1, LOBJ_FIELD);

	lua_pushinteger(_State, _Field->StatusTime);
	return 1;
}

int LuaFieldStatusCompletion(lua_State* _State) {
	struct Field* _Field = (struct Field*) LuaToObject(_State, 1, LOBJ_FIELD);

	lua_pushinteger(_State, FieldStatusDays(_Field));
	return 1;
}


int LuaAnimalGetId(lua_State* _State) {
	struct Animal* _Animal = (struct Animal*) LuaToObject(_State, 1, LOBJ_ANIMAL);

	lua_pushinteger(_State, _Animal->Object.Id);
	return 1;
}

int LuaAnimalIsMale(lua_State* _State) {
	struct Animal* _Animal = (struct Animal*) LuaToObject(_State, 1, LOBJ_ANIMAL);

	lua_pushboolean(_State, (_Animal->Gender == EMALE));
	return 1;
}

int LuaAnimalGetGender(lua_State* _State) {
	struct Animal* _Animal = (struct Animal*) LuaToObject(_State, 1, LOBJ_ANIMAL);

	lua_pushinteger(_State, _Animal->Gender);
	return 1;
}

int LuaAnimalGetNutrition(lua_State* _State) {
	struct Animal* _Animal = (struct Animal*) LuaToObject(_State, 1, LOBJ_ANIMAL);

	lua_pushinteger(_State, _Animal->Nutrition);
	return 1;
}

int LuaAnimalGetAge(lua_State* _State) {
	struct Animal* _Animal = (struct Animal*) LuaToObject(_State, 1, LOBJ_ANIMAL);

	lua_pushinteger(_State, _Animal->Age);
	return 1;
}

int LuaAnimalGetBase(lua_State* _State) {
	struct Animal* _Animal = (struct Animal*) LuaToObject(_State, 1, LOBJ_ANIMAL);

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
	struct Building* _Building = (struct Building*) LuaToObject(_State, 1, LOBJ_BUILDING);

	lua_pushinteger(_State, _Building->Object.Id);
	return 1;
}

int LuaBuildingConstructionTime(lua_State* _State) {
	struct BuildMat* _Floor = (struct BuildMat*) LuaToObject(_State, 1, LOBJ_BUILDMAT);
	struct BuildMat* _Walls = (struct BuildMat*) LuaToObject(_State, 1, LOBJ_BUILDMAT);
	struct BuildMat* _Roof = (struct BuildMat*) LuaToObject(_State, 1, LOBJ_BUILDMAT);
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
	lua_pushinteger(_State, _Crop->SeedsPerAcre);
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
		luaL_error(_State, LUA_TYPERROR(_State, 1, LOBJ_PERSON, "LuaPushPerson"));
	LuaCtor(_State, lua_touserdata(_State, _Pos), LOBJ_PERSON);
	return 1;
}

int LuaFamilyCountAnimal(lua_State* _State) {
	struct Family* _Family = LuaToObject(_State, 1, LOBJ_FAMILY);
	const char* _Animal = luaL_checkstring(_State, 2);
	struct Population* _AnimalType = NULL;
	int _AnimalCt = 0;

	if((_AnimalType = HashSearch(&g_Populations, _Animal)) == NULL)
		return luaL_error(_State, "CountAnimal: %s is not an animal type.", _Animal);
	for(int i = 0; i < _Family->Animals.Size; ++i) {
		if(_AnimalType->Id == ((struct Animal*)_Family->Animals.Table[i])->PopType->Id)
			++_AnimalCt;
	}
	lua_pushinteger(_State, _AnimalCt);
	return 1;
}

int LuaFamillyKillAnimal(lua_State* _State) {
	struct Family* _Family = LuaToObject(_State, 1, LOBJ_FAMILY);
	const char* _Animal = luaL_checkstring(_State, 2);
	struct Population* _AnimalType = NULL;
	int _KillAmount = luaL_checkinteger(_State, 3);
	int _KillCt = 0;

	if((_AnimalType = HashSearch(&g_Populations, _Animal)) == NULL)
		return 0;
	for(int i = 0; i < _Family->Animals.Size && _KillCt < _KillAmount; ++i) {
		if(_AnimalType->Id == ((struct Animal*)_Family->Animals.Table[i])->PopType->Id) {
			DestroyAnimal(_Family->Animals.Table[i]);
			ArrayRemove(&_Family->Animals, i);
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
		luaL_error(_State, LUA_TYPERROR(_State, 1, "string", LOBJ_BUILDMAT));
		return 0;
	}
	if((_BuildMat = HashSearch(&g_BuildMats, _Name)) == NULL)
		return luaL_error(_State, "BuildMat not given a valid build mat.");
	LuaCtor(_State, _BuildMat, LOBJ_BUILDMAT);
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
	LuaCtor(_State, _Good, LOBJ_GOOD);
	return 1;
}

int LuaCreateBuilding(lua_State* _State) {
	int _ResType = 0;
	const char* _Type = NULL;
	struct BuildMat* _FloorMat = NULL;
	struct BuildMat* _WallMat = NULL;
	struct BuildMat* _RoofMat = NULL;
	int _SquareFeet = luaL_checkinteger(_State, 5);
	//struct Settlement* _Location = NULL;

	_FloorMat = (struct BuildMat*) LuaToObject(_State, 1, LOBJ_BUILDMAT);
	_WallMat = (struct BuildMat*) LuaToObject(_State, 2, LOBJ_BUILDMAT);
	_RoofMat = (struct BuildMat*) LuaToObject(_State, 3, LOBJ_BUILDMAT);
	//_Location = (struct Settlement*) LuaToObject(_State, 4, "Settlement");
	_Type = luaL_optstring(_State, 6, "Human");
	if(strcmp(_Type, "Human") == 0)
		_ResType = ERES_HUMAN;
	else if(strcmp(_Type, "Animal") == 0)
		_ResType = ERES_ANIMAL;
	else if(strcmp(_Type, "All") == 0)
		_ResType = ERES_HUMAN | ERES_ANIMAL;
	else
		return luaL_error(_State, "%s is not a valid house type.", _Type);
	LuaCtor(_State, CreateBuilding(_ResType, _WallMat, _FloorMat, _RoofMat, _SquareFeet), LOBJ_BUILDING);
	return 1;
}

int LuaCreateAnimal(lua_State* _State) {
	const char* _Name = NULL;
	struct Population* _Population = NULL;

	_Name = luaL_checkstring(_State, 1);
	if((_Population = HashSearch(&g_Populations, _Name)) == NULL)
		return luaL_error(_State, "Cannot find Population %s.", _Name);
	LuaCtor(_State, CreateAnimal(_Population, Random(0, _Population->Ages[AGE_DEATH]->Max), 1500, -1, -1), LOBJ_ANIMAL);
	return 1;
}

int LuaFamilyTakeAnimal(lua_State* _State) {
	struct Family* _From = LuaCheckClass(_State, 1, LOBJ_FAMILY);
	struct Family* _To = LuaCheckClass(_State, 2, LOBJ_FAMILY);
	const char* _Animal = luaL_checkstring(_State, 3);
	struct Animal* _Temp = NULL;
	int _AnCount = luaL_checkinteger(_State, 4);

	for(int i = 0; i < _From->Animals.Size; ++i) {
		if(strcmp(((struct Animal*)_From->Animals.Table[i])->PopType->Name, _Animal) == 0) {
			if(_AnCount <= 0)
				return 0;
			--_AnCount;
			_Temp = FamilyTakeAnimal(_From, i);
			ArrayInsert_S(&_To->Animals, _Temp);
			goto found_animal;
		}
	}
	found_animal:
	return 0;
}
int LuaFamilyGetSize(lua_State* _State) {
	struct Family* _Family = LuaCheckClass(_State, 1, LOBJ_FAMILY);

	lua_pushinteger(_State, FamilySize(_Family));
	return 1;
}

int LuaFamilyGetNutrition(lua_State* _State) {
	struct Family* _Family = LuaCheckClass(_State, 1, LOBJ_FAMILY);

	lua_pushinteger(_State, FamilyGetNutrition(_Family));
	return 1;
}

int LuaFamilyGetNutritionReq(lua_State* _State) {
	struct Family* _Family = LuaCheckClass(_State, 1, LOBJ_FAMILY);

	lua_pushinteger(_State, FamilyNutReq(_Family));
	return 1;
}

int LuaFamilyGetWealth(lua_State* _State) {
	struct Family* _Family = LuaCheckClass(_State, 1, LOBJ_FAMILY);

	lua_pushinteger(_State, FamilyGetWealth(_Family));
	return 1;
}
