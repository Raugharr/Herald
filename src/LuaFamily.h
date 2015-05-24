/*
 * File: LuaFamily.h
 * Author: David Brotz
 */
#ifndef __LUAFAMILY_H
#define __LUAFAMILY_H

#include "sys/LuaCore.h"

typedef struct lua_State lua_State;

void InitLuaFamily();

int LuaPersonGetId(lua_State* _State);
int LuaPersonGetX(lua_State* _State);
int LuaPersonGetY(lua_State* _State);
int LuaPersonGetGender(lua_State* _State);
int LuaPersonGetNutrition(lua_State* _State);
int LuaPersonGetAge(lua_State* _State);
int LuaPersonGetName(lua_State* _State);
int LuaPersonGetFamily(lua_State* _State);
int LuaPersonGetParentFamily(lua_State* _State);

int LuaGoodGetId(lua_State* _State);
int LuaGoodGetQuantity(lua_State* _State);
int LuaGoodGetBase(lua_State* _State);

int LuaFamilyGetId(lua_State* _State);
int LuaFamilyChildrenCt(lua_State* _State);
int LuaFamilyGetName(lua_State* _State);
int LuaFamilyGetPeople(lua_State* _State);
int LuaFamilyGetFields(lua_State* _State);
int LuaFamilyGetBuildings(lua_State* _State);
int LuaFamilyGetBulidingCt(lua_State* _State);
int LuaFamilyGetGoods(lua_State* _State);
int LuaFamilyGetGoodCt(lua_State* _State);
int LuaFamilyGetAnimals(lua_State* _State);
int LuaFamilyGetAnimalCt(lua_State* _State);

int LuaFieldGetId(lua_State* _State);
int LuaFieldGetCrop(lua_State* _State);
int LuaFieldGetYield(lua_State* _State);
int LuaFieldGetAcres(lua_State* _State);
int LuaFieldGetStatus(lua_State* _State);
int LuaFieldGetStatusTime(lua_State* _State);

int LuaAnimalGetId(lua_State* _State);
int LuaAnimalIsMale(lua_State* _State);
int LuaAnimalGetNutrition(lua_State* _State);
int LuaAnimalGetAge(lua_State* _State);
int LuaAnimalGetBase(lua_State* _State);

int LuaBuildingGetId(lua_State* _State);
int LuaBuildingGetWidth(lua_State* _State);
int LuaBuildingGetLength(lua_State* _State);
int LuaBuildingConstructionTime(lua_State* _State);

/*!
 * Creates a table containing information about a crop.
 * Requires one parameter that is a string equaling the key of a crop in g_Crops.
 */
int LuaCrop(lua_State* _State);
/*!
 * Creates a table containing information about a GoodBase.
 * Requires one parameter that is a string equaling the key of a GoodBase in g_Goods.
 */
int LuaGoodBase(lua_State* _State);
/*!
 * Creates a table containing information about a FoodBase.
 * Requires one parameter that is a string equaling the key of a FoodBase in g_Goods.
 */
int LuaFoodBase(lua_State* _State);
/*!
 * Creates a table containing information about a Population.
 * Requires one parameter that is a string equaling the key of a crop in g_Populations.
 */
int LuaPopulation(lua_State* _State);
int LuaPushPerson(lua_State* _State, int _Index);
/*!
 * Creates a table containing information about a Person.
 * Requires one parameter that is a light user data that contains a pointer to a Person.
 */
int LuaPerson(lua_State* _State);
int LuaBuildMat(lua_State* _State);

/**
 * Functions that create an object type.
 */

int LuaCreateGood(lua_State* _State);
int LuaCreateBuilding(lua_State* _State);
int LuaCreateAnimal(lua_State* _State);


#endif
