/*
 * File: LuaFamily.h
 * Author: David Brotz
 */
#ifndef __LUAFAMILY_H
#define __LUAFAMILY_H

#include "sys/LuaCore.h"

typedef struct lua_State lua_State;

void InitLuaFamily();

int LuaPersonGetId(lua_State* State);
int LuaPersonGetX(lua_State* State);
int LuaPersonGetY(lua_State* State);
int LuaPersonGetGender(lua_State* State);
int LuaPersonGetNutrition(lua_State* State);
int LuaPersonGetAge(lua_State* State);
int LuaPersonGetName(lua_State* State);
int LuaPersonGetFamily(lua_State* State);
int LuaPersonInRetinue(lua_State* State);
int LuaPersonRetinue(lua_State* State);
int LuaPersonBanish(lua_State* State);
int LuaPersonIsBigGuy(lua_State* State);

int LuaGoodGetId(lua_State* State);
int LuaGoodGetQuantity(lua_State* State);
int LuaGoodGetBase(lua_State* State);

int LuaFamilyGetId(lua_State* State);
int LuaFamilyChildrenCt(lua_State* State);
int LuaFamilyGetName(lua_State* State);
int LuaFamilyGetPeople(lua_State* State);
int LuaFamilyGetFields(lua_State* State);
int LuaFamilyGetBuildings(lua_State* State);
int LuaFamilyGetBulidingCt(lua_State* State);
int LuaFamilyGetGoods(lua_State* State);
int LuaFamilyGetGoodCt(lua_State* State);
int LuaFamilyGetAnimals(lua_State* State);
int LuaFamilyGetAnimalCt(lua_State* State);
int LuaFamilyChangeNutrition(lua_State* State);
int LuaFamilyGetWealth(lua_State* State);
int LuaFamilyGetSettlement(lua_State* State);
int LuaFamilySetSettlement(lua_State* State);
int LuaFamilyGetCasteName(lua_State* State);

int LuaFieldGetId(lua_State* State);
int LuaFieldGetCrop(lua_State* State);
int LuaFieldGetYield(lua_State* State);
int LuaFieldGetAcres(lua_State* State);
int LuaFieldGetUnusedAcres(lua_State* State);
int LuaFieldGetStatus(lua_State* State);
int LuaFieldGetStatusTime(lua_State* State);
int LuaFieldStatusCompletion(lua_State* State);

int LuaAnimalGetId(lua_State* State);
int LuaAnimalIsMale(lua_State* State);
int LuaAnimalGetGender(lua_State* State);
int LuaAnimalGetNutrition(lua_State* State);
int LuaAnimalGetAge(lua_State* State);
int LuaAnimalGetBase(lua_State* State);

int LuaBuildingGetId(lua_State* State);
int LuaBuildingConstructionTime(lua_State* State);

/*!
 * Creates a table containing information about a crop.
 * Requires one parameter that is a string equaling the key of a crop in g_Crops.
 */
int LuaCrop(lua_State* State);
/*!
 * Creates a table containing information about a GoodBase.
 * Requires one parameter that is a string equaling the key of a GoodBase in g_Goods.
 */
int LuaGoodBase(lua_State* State);
/*!
 * Creates a table containing information about a FoodBase.
 * Requires one parameter that is a string equaling the key of a FoodBase in g_Goods.
 */
int LuaFoodBase(lua_State* State);
/*!
 * Creates a table containing information about a Population.
 * Requires one parameter that is a string equaling the key of a crop in g_Populations.
 */
int LuaPopulation(lua_State* State);
int LuaPushPerson(lua_State* State, int Index);
int LuaFamilyCountAnimal(lua_State* State);
int LuaFamillyKillAnimal(lua_State* State);
/*!
 * Creates a table containing information about a Person.
 * Requires one parameter that is a light user data that contains a pointer to a Person.
 */
int LuaPerson(lua_State* State);
int LuaBuildMat(lua_State* State);

/**
 * Functions that create an object type.
 */

int LuaCreateGood(lua_State* State);
int LuaCreateBuilding(lua_State* State);
int LuaCreateAnimal(lua_State* State);
int LuaFamilyTakeAnimal(lua_State* State);
int LuaFamilyGetSize(lua_State* State);
int LuaFamilyGetNutrition(lua_State* State);
int LuaFamilyGetNutritionReq(lua_State* State);

#endif
