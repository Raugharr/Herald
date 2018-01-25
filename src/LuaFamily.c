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
#include "Profession.h"

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
	{"IsBigGuy", LuaPersonIsBigGuy},
	{NULL, NULL}
};

static const luaL_Reg g_LuaFuncsGood[] = {
	{"GetId", LuaGoodGetId},
	{"GetQuantity", LuaGoodGetQuantity},
	{"GetBase", LuaGoodGetBase},
	{NULL, NULL}
};

static const luaL_Reg g_LuaFuncsProfession[] = {
	{"GetName", LuaProfGetName},
	{NULL, NULL}
};

static const struct LuaEnum g_LuaGoodCatEnum[] = {
	{"Food", GOOD_FOOD},
	{"Ingredient", GOOD_INGREDIENT},
	{"Animal", GOOD_ANIMAL},
	{"Seed", GOOD_SEED},
	{"Tool", GOOD_TOOL},
	{"Material", GOOD_MATERIAL},
	{"Weapon", GOOD_WEAPON},
	{"Armor", GOOD_ARMOR},
	{"Other", GOOD_OTHER},
	{"Flour", GOOD_FLOUR},
	{"Wood", GOOD_WOOD},
	{"Clothing", GOOD_CLOTHING},
	{NULL, 0}	
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
	{"GetSettlement", LuaFamilyGetSettlement},
	{"SetSettlement", LuaFamilySetSettlement},
	{"GetCasteName", LuaFamilyGetCasteName},
	{"Job", LuaFamilyJob},
	{NULL, NULL}
};

static const struct LuaEnum g_LuaPersonEnum[] = {
	{"DailyNut", NUTRITION_DAILY},
	{"Male", MALE},
	{"Female", FEMALE},
	{NULL, 0}
};

static const luaL_Reg g_LuaFuncsField[] = {
	{"GetId", LuaFieldGetId},
	{"GetCrop", LuaFieldGetCrop},
	{"GetYield", LuaFieldGetYield},
	{"GetAcres", LuaFieldGetAcres},
	{"GetUnusedAcres", LuaFieldGetUnusedAcres},
	{"GetStatus", LuaFieldGetStatus},
	{"GetStatusTime", LuaFieldGetStatusTime},
	{"StatusCompletion", LuaFieldStatusCompletion},
	{NULL, NULL}
};


static const luaL_Reg g_LuaFuncsAnimal[] = {
	{"IsMale", LuaAnimalIsMale},
	{"GetGender", LuaAnimalGetGender},
	{"GetNutrition", LuaAnimalGetNutrition},
	{"GetAge", LuaAnimalGetAge},
	{"GetBase", LuaAnimalGetBase},
	{NULL, NULL}
};

static const struct LuaEnum g_LuaAnimalEnum[] = {
	{"Male", MALE},
	{"Female", FEMALE},
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
	{LOBJ_PROFESSION, "Profession", LUA_REFNIL, g_LuaFuncsProfession},
	{LUA_REFNIL, NULL, LUA_REFNIL, NULL}
};

const struct LuaEnumReg g_LuaFamilyEnums[] = {
	{"Person", NULL,  g_LuaPersonEnum},
	{"Animal", NULL, g_LuaAnimalEnum},
	{"GoodCat", NULL, g_LuaGoodCatEnum},
	{NULL, NULL}
};

void InitLuaFamily(lua_State* State) {
	RegisterLuaObjects(State, g_LuaFamilyObjects);
	LuaRegisterFunctions(State, g_LuaFamilyFuncs);
	RegisterLuaEnums(State, g_LuaFamilyEnums);
}

int LuaPersonGetId(lua_State* State) {
	struct Person* Person = (struct Person*) LuaToObject(State, 1, LOBJ_PERSON);

	lua_pushinteger(State, Person->Object.Id);
	return 1;
}

int LuaPersonGetX(lua_State* State) {
	struct Person* Person = (struct Person*) LuaToObject(State, 1, LOBJ_PERSON);
	uint32_t x = 0;
	uint32_t y = 0;

	PersonGetPos(Person, &x, &y);
	lua_pushinteger(State, x);
	return 1;
}

int LuaPersonGetY(lua_State* State) {
	struct Person* Person = (struct Person*) LuaToObject(State, 1, LOBJ_PERSON);
	uint32_t x = 0;
	uint32_t y = 0;

	PersonGetPos(Person, &x, &y);
	lua_pushinteger(State, y);
	return 1;
}

int LuaPersonGetGender(lua_State* State) {
	struct Person* Person = (struct Person*) LuaToObject(State, 1, LOBJ_PERSON);

	lua_pushinteger(State, Person->Flags & MALE);
	return 1;
}

int LuaPersonGetNutrition(lua_State* State) {
	struct Person* Person = (struct Person*) LuaToObject(State, 1, LOBJ_PERSON);

	lua_pushinteger(State, Person->Nutrition);
	return 1;
}

int LuaPersonGetAge(lua_State* State) {
	struct Person* Person = (struct Person*) LuaToObject(State, 1, LOBJ_PERSON);

	lua_pushinteger(State, Person->Age.Years);
	return 1;
}

int LuaPersonGetName(lua_State* State) {
	struct Person* Person = (struct Person*) LuaToObject(State, 1, LOBJ_PERSON);

	if(Person == NULL)
		return LuaClassError(State, 1, LOBJ_PERSON);
	lua_pushstring(State, Person->Name);
	return 1;
}

int LuaPersonGetFamily(lua_State* State) {
	struct Person* Person = (struct Person*) LuaToObject(State, 1, LOBJ_PERSON);

	LuaCtor(State, Person->Family, LOBJ_FAMILY);
	return 1;
}

int LuaPersonBanish(lua_State* State) {
	struct Person* Person = LuaCheckClass(State, 1, LOBJ_PERSON);

	PersonDeath(Person);
	return 0;
}

int LuaPersonInRetinue(lua_State* State) {
	struct Person* Leader = LuaCheckClass(State, 1, LOBJ_PERSON);

	lua_pushboolean(State, IntSearch(&g_GameWorld.PersonRetinue, Leader->Object.Id) != NULL);
	return 1;
}

int LuaPersonRetinue(lua_State* State) {
	struct Person* Person = LuaCheckClass(State, 1, LOBJ_PERSON);

	LuaCtor(State, IntSearch(&g_GameWorld.PersonRetinue, Person->Object.Id), LOBJ_RETINUE);
	return 1;
}

int LuaPersonIsBigGuy(lua_State* State) {
	struct Person* Person = LuaCheckClass(State, 1, LOBJ_PERSON);

	lua_pushboolean(State, IsBigGuy(Person));
	return 1;
}

int LuaGoodGetId(lua_State* State) {
	struct Good* Good = (struct Good*) LuaToObject(State, 1, LOBJ_GOOD);

	lua_pushinteger(State, Good->Id);
	return 1;
}

int LuaGoodGetQuantity(lua_State* State) {
	struct Good* Good = (struct Good*) LuaToObject(State, 1, LOBJ_GOOD);

	lua_pushinteger(State, Good->Quantity);
	return 1;
}

int LuaGoodGetBase(lua_State* State) {
	struct Good* Good = (struct Good*) LuaToObject(State, 1, LOBJ_GOOD);

	lua_pushstring(State, Good->Base->Name);
	lua_remove(State, -2);
	LuaGoodBase(State);
	return 1;
}

int LuaProfGetName(lua_State* State) {
	struct Profession* Profession = LuaCheckClass(State, 1, LOBJ_PROFESSION);

	lua_pushstring(State, Profession->Name);
	return 1;
}

int LuaFamilyGetId(lua_State* State) {
	struct Family* Family = (struct Family*) LuaToObject(State, 1, LOBJ_FAMILY);

	lua_pushinteger(State, Family->Object.Id);
	return 1;
}

int LuaFamilyChildrenCt(lua_State* State) {
	struct Family* Family = (struct Family*) LuaToObject(State, 1, LOBJ_FAMILY);

	if(Family == NULL)
		return LuaClassError(State, 1, LOBJ_FAMILY);
	lua_pushinteger(State, Family->NumChildren);
	return 1;
}

int LuaFamilyGetName(lua_State* State) {
	struct Family* Family = (struct Family*) LuaToObject(State, 1, LOBJ_FAMILY);

	if(Family == NULL)
		return LuaClassError(State, 1, LOBJ_FAMILY);
	lua_pushstring(State, Family->Name);
	return 1;
}

int LuaFamilyGetPeople(lua_State* State) {
	struct Family* Family = (struct Family*) LuaToObject(State, 1, LOBJ_FAMILY);
	int Idx = 0;

	if(Family == NULL)
		return LuaClassError(State, 1, LOBJ_FAMILY);
	lua_createtable(State, Family->NumChildren + 2, 0);
	for(int i = 0; i < Family->NumChildren + 2; ++i) {
		if(Family->People[i] == NULL)
			continue;
		LuaCtor(State, Family->People[i], LOBJ_PERSON);
		lua_rawseti(State, -2, ++Idx);
	}
	return 1;
}

int LuaFamilyGetFields(lua_State* State) {
	struct Family* Family = (struct Family*) LuaToObject(State, 1, LOBJ_FAMILY);
	int Idx = 0;

	if(Family->Prof != PROF_FARMER) {
		lua_createtable(State, 0, 0);
		return 1;
	}
	lua_createtable(State, Family->Farmer.FieldCt, 0);
	for(int i = 0; i < Family->Farmer.FieldCt; ++i) {
		LuaCtor(State, Family->Farmer.Fields[i], LOBJ_FIELD);
		lua_rawseti(State, -2, ++Idx);
	}
	return 1;
}

int LuaFamilyGetBuildings(lua_State* State) {
	//struct Family* Family = (struct Family*) LuaToObject(State, 1, LOBJ_FAMILY);

	/*lua_createtable(State, Family->BuildingCt, 0);
	for(int i = 0; i < Family->BuildingCt; ++i) {
		LuaCtor(State, Family->Buildings[i], LOBJ_BUILDING);
		lua_rawseti(State, -2, i + 1);
	}
	return 1;*/
	return 0;
}

int LuaFamilyGetBulidingCt(lua_State* State) {
//	struct Family* Family = (struct Family*) LuaToObject(State, 1, LOBJ_FAMILY);

//	lua_pushinteger(State, Family->BuildingCt);
//	return 1;
	return 0;
}

int LuaFamilyGetGoods(lua_State* State) {
	struct Family* Family = (struct Family*) LuaToObject(State, 1, LOBJ_FAMILY);

	CreateLuaArrayItr(State, &Family->Goods, LOBJ_GOOD);
	return 1;
}

int LuaFamilyGetGoodCt(lua_State* State) {
	struct Family* Family = (struct Family*) LuaToObject(State, 1, LOBJ_FAMILY);

	lua_pushinteger(State, Family->Goods.Size);
	return 1;
}

int LuaFamilyGetAnimals(lua_State* State) {
	struct Family* Family = (struct Family*) LuaToObject(State, 1, LOBJ_FAMILY);

	CreateLuaArrayItr(State, &Family->Animals, LOBJ_ANIMAL);
	return 1;
}

int LuaFamilyGetAnimalCt(lua_State* State) {
	struct Family* Family = (struct Family*) LuaToObject(State, 1, LOBJ_FAMILY);

	lua_pushinteger(State, Family->Animals.Size);
	return 1;
}

int LuaFamilyChangeNutrition(lua_State* State) {
	struct Family* Family = (struct Family*) LuaToObject(State, 1, LOBJ_FAMILY);
	int Change = luaL_checkinteger(State, 2);

	if(Family->Food.SlowSpoiled - Change < 0)
		Family->Food.SlowSpoiled = 0;
	else
		Family->Food.SlowSpoiled -= Change;
	return 0;
}

int LuaFamilyJob(lua_State* State) {
	struct Family* Family = LuaCheckClass(State, 1, LOBJ_FAMILY);
	struct Profession* Profession = GetProf(Family->Prof);
	
	LuaCtor(State, Profession, LOBJ_PROFESSION);
	return 1;
}

int LuaFieldGetId(lua_State* State) {
	struct Field* Field = (struct Field*) LuaToObject(State, 1, LOBJ_FIELD);

	lua_pushinteger(State, Field->Object.Id);
	return 1;
}

int LuaFieldGetCrop(lua_State* State) {
	struct Field* Field = (struct Field*) LuaToObject(State, 1, LOBJ_FIELD);

	if(Field == NULL || Field->Crop == NULL) {
		lua_pushnil(State);
		return 1;
	}
	lua_pushvalue(State, LUA_REGISTRYINDEX);
	lua_rawgeti(State, -1, Field->Crop->LuaRef);
	return 1;
}

int LuaFieldGetYield(lua_State* State) {
	struct Field* Field = (struct Field*) LuaToObject(State, 1, LOBJ_FIELD);

	lua_pushinteger(State, Field->YieldTotal);
	return 1;
}

int LuaFieldGetAcres(lua_State* State) {
	struct Field* Field = (struct Field*) LuaToObject(State, 1, LOBJ_FIELD);

	lua_pushinteger(State, Field->Acres);
	return 1;
}
int LuaFieldGetUnusedAcres(lua_State* State) {
	struct Field* Field = (struct Field*) LuaToObject(State, 1, LOBJ_FIELD);

	lua_pushinteger(State, Field->UnusedAcres);
	return 1;
}

int LuaFieldGetStatus(lua_State* State) {
	struct Field* Field = (struct Field*) LuaToObject(State, 1, LOBJ_FIELD);
	
	lua_pushstring(State, FieldStatusName(Field));
	return 1;
}

int LuaFieldGetStatusTime(lua_State* State) {
	struct Field* Field = (struct Field*) LuaToObject(State, 1, LOBJ_FIELD);

	lua_pushinteger(State, Field->StatusTime);
	return 1;
}

int LuaFieldStatusCompletion(lua_State* State) {
	struct Field* Field = (struct Field*) LuaToObject(State, 1, LOBJ_FIELD);

	lua_pushinteger(State, FieldStatusDays(Field));
	return 1;
}


int LuaAnimalIsMale(lua_State* State) {
	struct Animal* Animal = (struct Animal*) LuaToObject(State, 1, LOBJ_ANIMAL);

	lua_pushboolean(State, (Animal->Gender == MALE));
	return 1;
}

int LuaAnimalGetGender(lua_State* State) {
	struct Animal* Animal = (struct Animal*) LuaToObject(State, 1, LOBJ_ANIMAL);

	lua_pushinteger(State, Animal->Gender);
	return 1;
}

int LuaAnimalGetNutrition(lua_State* State) {
	//struct Animal* Animal = (struct Animal*) LuaToObject(State, 1, LOBJ_ANIMAL);

	//lua_pushinteger(State, Animal->Nutrition);
	lua_pushinteger(State, 0);
	return 1;
}

int LuaAnimalGetAge(lua_State* State) {
	struct Animal* Animal = (struct Animal*) LuaToObject(State, 1, LOBJ_ANIMAL);

	lua_pushinteger(State, Animal->Age.Years);
	return 1;
}

int LuaAnimalGetBase(lua_State* State) {
	struct Animal* Animal = (struct Animal*) LuaToObject(State, 1, LOBJ_ANIMAL);

	if(Animal == NULL) {
		lua_pushnil(State);
		return 1;
	}
	lua_pushstring(State, Animal->PopType->Name);
	lua_remove(State, -2);
	LuaPopulation(State);
	return 1;
}

int LuaBuildingGetId(lua_State* State) {
	struct Building* Building = (struct Building*) LuaToObject(State, 1, LOBJ_BUILDING);

	lua_pushinteger(State, Building->Object.Id);
	return 1;
}

int LuaBuildingConstructionTime(lua_State* State) {
	struct BuildMat* Floor = (struct BuildMat*) LuaToObject(State, 1, LOBJ_BUILDMAT);
	struct BuildMat* Walls = (struct BuildMat*) LuaToObject(State, 1, LOBJ_BUILDMAT);
	struct BuildMat* Roof = (struct BuildMat*) LuaToObject(State, 1, LOBJ_BUILDMAT);
	int Area = luaL_checkinteger(State, 4);

	lua_pushinteger(State, ConstructionTime(Floor, Walls, Roof, Area));
	return 1;
}


int LuaGoodBase(lua_State* State) {
	const struct GoodBase* Good = NULL;
	const char* Name = NULL;
	int i;

	Name = luaL_checklstring(State, 1, NULL);
	if((Good = HashSearch(&g_Goods, Name)) == NULL) {
		luaL_error(State, "%s is not a valid GoodBase.", Name);
		return 0;
	}
	lua_createtable(State, 0, 4);

	lua_pushstring(State, "Id");
	lua_pushinteger(State, Good->Id);
	lua_rawset(State, -3);

	lua_pushstring(State, "Name");
	lua_pushstring(State, Good->Name);
	lua_rawset(State, -3);

	lua_pushstring(State, "Category");
	lua_pushinteger(State, Good->Category);
	lua_rawset(State, -3);

	lua_createtable(State, Good->IGSize, 0);
	lua_pushstring(State, "InputGoods");
	lua_pushvalue(State, -2);
	lua_rawset(State, -4);
	for(i = 0; i < Good->IGSize; ++i) {
		lua_pushinteger(State, i + 1);
		lua_pushstring(State, ((struct GoodBase*)Good->InputGoods[i]->Req)->Name);
		lua_rawset(State, -3);
	}
	lua_pop(State, 1);
	//lua_rawset(State, -3);
	return 1;
}

int LuaFoodBase(lua_State* State) {
	const struct FoodBase* Good = NULL;
	const char* Name = NULL;

	if(LuaGoodBase(State) == 0)
		return 0;
	if((Good = HashSearch(&g_Goods, Name)) == NULL)
		luaL_error(State, "%s is not a valid FoodBase.");
	lua_pushstring(State, "Nutrition");
	lua_pushinteger(State, Good->Nutrition);
	lua_rawget(State, -3);
	return 1;
}

int LuaCrop(lua_State* State) {
	const char* Name = NULL;
	const struct Crop* Crop = NULL;

	Name = luaL_checklstring(State, 1, NULL);
	if((Crop = HashSearch(&g_Crops, Name)) == NULL)
		luaL_error(State, "%s is not a valid crop.", Name);
	lua_createtable(State, 0, 7);

	lua_pushstring(State, "Id");
	lua_pushinteger(State, Crop->Id);
	lua_rawset(State, -3);

	lua_pushstring(State, "Name");
	lua_pushstring(State, Crop->Name);
	lua_rawset(State, -3);

	lua_pushstring(State, "Type");
	lua_pushinteger(State, Crop->Type);
	lua_rawset(State, -3);

	lua_pushstring(State, "PerAcre");
	lua_pushinteger(State, Crop->SeedsPerAcre);
	lua_rawset(State, -3);

	lua_pushstring(State, "NutVal");
	lua_pushinteger(State, Crop->NutVal);
	lua_rawset(State, -3);

	lua_pushstring(State, "GrowDays");
	lua_pushinteger(State, Crop->GrowingDegree);
	lua_rawset(State, -3);

	lua_pushstring(State, "YieldMult");
	lua_pushnumber(State, Crop->YieldMult);
	lua_rawset(State, -3);
	return 1;
}

int LuaPopulation(lua_State* State) {
	int i;
	const char* Name = NULL;
	const struct Population* Pop = NULL;

	Name = luaL_checklstring(State, 1, NULL);
	if((Pop = HashSearch(&g_Populations, Name)) == NULL) {
		Log(ELOG_WARNING, "%s is not a valid population.", Name);
		return 0;
	}
	lua_createtable(State, 0, 10);

	lua_pushstring(State, "Id");
	lua_pushinteger(State, Pop->Id);
	lua_rawset(State, -3);

	lua_pushstring(State, "Name");
	lua_pushstring(State, Pop->Name);
	lua_rawset(State, -3);

	lua_pushstring(State, "Nutrition");
	lua_pushinteger(State, Pop->Nutrition);
	lua_rawset(State, -3);

	lua_pushstring(State, "Meat");
	lua_pushinteger(State, Pop->Meat);
	lua_rawset(State, -3);

	lua_pushstring(State, "Milk");
	lua_pushinteger(State, Pop->Milk);
	lua_rawset(State, -3);

	lua_pushstring(State, "MaleRatio");
	lua_pushnumber(State, Pop->MaleRatio);
	lua_rawset(State, -3);
	lua_pushstring(State, "Ages");
	ConstraintBndToLua(State, Pop->Ages);
	lua_rawset(State, -3);

	lua_pushstring(State, "Outputs");
	lua_createtable(State, ArrayLen(Pop->Outputs), 0);
	for(i = 0; Pop->Outputs[i] != NULL; ++i) {
		lua_pushstring(State, ((struct Good*)Pop->Outputs[i])->Base->Name);
		lua_rawseti(State, -2, i);
	}
	lua_rawset(State, -3);

	lua_pushstring(State, "Eats");
	lua_createtable(State, Pop->EatsSize, 0);
	for(i = 0; i < Pop->EatsSize; ++i) {
		lua_pushstring(State, ((struct FoodBase*)Pop->Eats[i])->Base.Name);
		lua_rawseti(State, -2, i);
	}
	lua_rawset(State, -3);
	return 1;
}

int LuaPushPerson(lua_State* State, int Index) {
	int Pos = lua_absindex(State, Index);

	if(lua_type(State, Pos) != LUA_TLIGHTUSERDATA)
		luaL_error(State, LUA_TYPERROR(State, 1, LOBJ_PERSON, "LuaPushPerson"));
	LuaCtor(State, lua_touserdata(State, Pos), LOBJ_PERSON);
	return 1;
}

int LuaFamilyCountAnimal(lua_State* State) {
	struct Family* Family = LuaToObject(State, 1, LOBJ_FAMILY);
	const char* Animal = luaL_checkstring(State, 2);
	struct Population* AnimalType = NULL;
	int AnimalCt = 0;

	if((AnimalType = HashSearch(&g_Populations, Animal)) == NULL)
		return luaL_error(State, "CountAnimal: %s is not an animal type.", Animal);
	for(int i = 0; i < Family->Animals.Size; ++i) {
		if(AnimalType->Id == ((struct Animal*)Family->Animals.Table[i])->PopType->Id)
			++AnimalCt;
	}
	lua_pushinteger(State, AnimalCt);
	return 1;
}

int LuaFamillyKillAnimal(lua_State* State) {
	struct Family* Family = LuaToObject(State, 1, LOBJ_FAMILY);
	const char* Animal = luaL_checkstring(State, 2);
	struct Population* AnimalType = NULL;
	int KillAmount = luaL_checkinteger(State, 3);
	int KillCt = 0;

	if((AnimalType = HashSearch(&g_Populations, Animal)) == NULL)
		return 0;
	for(int i = 0; i < Family->Animals.Size && KillCt < KillAmount; ++i) {
		if(AnimalType->Id == ((struct Animal*)Family->Animals.Table[i])->PopType->Id) {
			DestroyAnimal(Family->Animals.Table[i]);
			ArrayRemove(&Family->Animals, i);
			--i;
			++KillCt;
		}
	}
	return 0;
}

int LuaPerson(lua_State* State) {
	LuaPushPerson(State, 1);
	return 1;
}

int LuaBuildMat(lua_State* State) {
	const char* Name = NULL;
	struct BuildMat* BuildMat = NULL;

	if(lua_isstring(State, 1))
		Name = luaL_checkstring(State, 1);
	else {
		luaL_error(State, LUA_TYPERROR(State, 1, "string", LOBJ_BUILDMAT));
		return 0;
	}
	if((BuildMat = HashSearch(&g_BuildMats, Name)) == NULL)
		return luaL_error(State, "BuildMat not given a valid build mat.");
	LuaCtor(State, BuildMat, LOBJ_BUILDMAT);
	return 1;
}

int LuaCreateGood(lua_State* State) {
	const struct GoodBase* GoodBase = NULL;
	struct Good* Good = NULL;
	const char* Name = NULL;
	int Quantity;

	Name = luaL_checkstring(State, 1);
	Quantity = luaL_checkinteger(State, 2);
	if((GoodBase = HashSearch(&g_Goods, Name)) == NULL)
		luaL_error(State, "Cannot find GoodBase %s.", Name);
	Good = CreateGood(GoodBase);
	Good->Quantity = Quantity;
	LuaCtor(State, Good, LOBJ_GOOD);
	return 1;
}

int LuaCreateBuilding(lua_State* State) {
	int ResType = 0;
	const char* Type = NULL;
	struct BuildMat* FloorMat = NULL;
	struct BuildMat* WallMat = NULL;
	struct BuildMat* RoofMat = NULL;
	int SquareFeet = luaL_checkinteger(State, 5);
	//struct Settlement* Location = NULL;

	FloorMat = (struct BuildMat*) LuaToObject(State, 1, LOBJ_BUILDMAT);
	WallMat = (struct BuildMat*) LuaToObject(State, 2, LOBJ_BUILDMAT);
	RoofMat = (struct BuildMat*) LuaToObject(State, 3, LOBJ_BUILDMAT);
	//_Location = (struct Settlement*) LuaToObject(State, 4, "Settlement");
	Type = luaL_optstring(State, 6, "Human");
	if(strcmp(Type, "Human") == 0)
		ResType = ERES_HUMAN;
	else if(strcmp(Type, "Animal") == 0)
		ResType = ERES_ANIMAL;
	else if(strcmp(Type, "All") == 0)
		ResType = ERES_HUMAN | ERES_ANIMAL;
	else
		return luaL_error(State, "%s is not a valid house type.", Type);
	LuaCtor(State, CreateBuilding(ResType, WallMat, FloorMat, RoofMat, SquareFeet), LOBJ_BUILDING);
	return 1;
}

int LuaCreateAnimal(lua_State* State) {
	const char* Name = NULL;
	struct Population* Population = NULL;

	Name = luaL_checkstring(State, 1);
	if((Population = HashSearch(&g_Populations, Name)) == NULL)
		return luaL_error(State, "Cannot find Population %s.", Name);
	LuaCtor(State, CreateAnimal(Population, Random(0, Population->Ages[AGE_DEATH]->Max), 0), LOBJ_ANIMAL);
	return 1;
}

int LuaFamilyTakeAnimal(lua_State* State) {
	struct Family* To = LuaCheckClass(State, 1, LOBJ_FAMILY);
	struct Family* From = LuaCheckClass(State, 2, LOBJ_FAMILY);
	int AnCount = luaL_optinteger(State, 3, 1);
	const char* AnStr = luaL_optstring(State, 4, NULL);
	const struct Population* AnType = NULL; 
	int ListSz = AnCount;
	struct Animal* InsertList[ListSz];
	int i = 0;

	if(AnStr != NULL) {
		if((AnType = FindPopulation(AnStr)) == NULL) {
			lua_pushinteger(State, 0);
			return 1;
		}
	} else {
		goto no_check;
	}
	for(i = 0; i < From->Animals.Size; ++i) {
		if(((struct Animal*)From->Animals.Table[i])->PopType == AnType) {
			--AnCount;
			InsertList[i] = FamilyRemoveAnimal(From, i);
			if(AnCount <= 0) goto end;
		}
	}
	goto end;
	no_check:
	for(i = 0; i < From->Animals.Size; ++i) {
		--AnCount;
		//NOTE: Inlined insertion sort? Would prevent function pointer calls.
		InsertList[i] = FamilyRemoveAnimal(From, Random(0, From->Animals.Size));
		if(AnCount <= 0) goto end;
	}
	end:
	++i;
//	InsertionSortPtr(InsertList, i, AnimalCmp);
	FamilyInsertAnimalArr(To, InsertList, i);
	lua_pushinteger(State, i);
	return 1;
}
int LuaFamilyGetSize(lua_State* State) {
	struct Family* Family = LuaCheckClass(State, 1, LOBJ_FAMILY);

	lua_pushinteger(State, FamilySize(Family));
	return 1;
}

int LuaFamilyGetNutrition(lua_State* State) {
	struct Family* Family = LuaCheckClass(State, 1, LOBJ_FAMILY);

	lua_pushinteger(State, FamilyGetNutrition(Family));
	return 1;
}

int LuaFamilyGetNutritionReq(lua_State* State) {
	struct Family* Family = LuaCheckClass(State, 1, LOBJ_FAMILY);

	lua_pushinteger(State, FamilyNutReq(Family));
	return 1;
}

int LuaFamilyGetWealth(lua_State* State) {
	struct Family* Family = LuaCheckClass(State, 1, LOBJ_FAMILY);

	lua_pushinteger(State, FamilyGetWealth(Family));
	return 1;
}

int LuaFamilyGetSettlement(lua_State* State) {
	struct Family* Family = LuaCheckClass(State, 1, LOBJ_FAMILY);

	LuaCtor(State, Family->HomeLoc, LOBJ_SETTLEMENT);
	return 1;
}

int LuaFamilySetSettlement(lua_State* State) {
	struct Family* Family = LuaCheckClass(State, 1, LOBJ_FAMILY);
	struct Settlement* Settlement = LuaCheckClass(State, 2, LOBJ_SETTLEMENT);

	if(Family->HomeLoc != NULL)
		SettlementRemoveFamily(Family->HomeLoc, Family);
	SettlementPlaceFamily(Settlement, Family);
	return 0;
}

int LuaFamilyGetCasteName(lua_State* State) {
	struct Family* Family = LuaCheckClass(State, 1, LOBJ_FAMILY);

	lua_pushstring(State, g_CasteNames[Family->Caste]);
	return 1;
}

int LuaFamillyJob(lua_State* State) {
	struct Family* Family = LuaCheckClass(State, 1, LOBJ_FAMILY);
	
	LuaCtor(State, (void*) GetProf(Family->Prof), LOBJ_PROFESSION);
	return 1;
}
