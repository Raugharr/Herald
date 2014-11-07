/*
 * File: LuaHelper.h
 * Author: David Brotz
 */

#ifndef __LUAHELPER_H
#define __LUAHELPER_H

struct Building;
typedef struct lua_State lua_State;
typedef int (*lua_CFunction) (lua_State *L);
struct LinkedList;
struct Constraint;
typedef struct lua_State lua_State;

#define LuaAbsPos(_State, _Index) ((_Index > 0) ? (_Index) : (((_Index < -(lua_gettop(_State))) ? (_Index) : lua_gettop(_State) + (_Index) + 1)))

#define ConstraintToLua(_State, _Constraint)			\
	lua_createtable((_State), 0, 2);					\
	lua_pushstring((_State), "Min");					\
	lua_pushinteger((_State), (_Constraint)->Min);		\
	lua_rawset((_State), -3);							\
	lua_pushstring((_State), "Max");					\
	lua_pushinteger((_State), (_Constraint)->Max);		\
	lua_rawset((_State), -3)

#define LUA_TYPERROR(_State, _Arg, _Type, _Func) "bad argument %d to '%s' (%s expected got %s)", (_Arg), (_Func), (_Type), lua_typename((_State), lua_type((_State), (_Arg)))

extern lua_State* g_LuaState;

void RegisterLuaFuncs(lua_State* _State);
int RegisterIterator(lua_State* _State);
int RegisterArrayItr(lua_State* _State);
int RegisterGood(lua_State* _State);
int RegisterFamily(lua_State* _State);
int RegisterField(lua_State* _State);
int RegisterArray(lua_State* _State);

int LuaGoodGetId(lua_State* _State);
int LuaGoodGetQuantity(lua_State* _State);
int LuaGoodGetBase(lua_State* _State);

int LuaFamilyGetId(lua_State* _State);
int LuaFamilyChildrenCt(lua_State* _State);
int LuaFamilyGetName(lua_State* _State);
int LuaFamilyGetPeople(lua_State* _State);
int LuaFamilyGetFields(lua_State* _State);
int LuaFamilyGetBuildings(lua_State* _State);
int LuaFamilyGetGoods(lua_State* _State);
int LuaFamilyGetAnimals(lua_State* _State);

int LuaFieldGetId(lua_State* _State);
int LuaFieldGetCrop(lua_State* _State);
int LuaFieldGetYield(lua_State* _State);
int LuaFieldGetAcres(lua_State* _State);

int LuaArrayCreate(lua_State* _State);
int LuaArrayItrNext(lua_State* _State);
int LuaArrayItrPrev(lua_State* _State);

int LuaArrayItr(lua_State* _State);

struct Good* LuaCheckGood(lua_State* _State, int _Index);
struct Family* LuaCheckFamily(lua_State* _State, int _Index);
struct Field* LuaCheckField(lua_State* _State, int _Index);
/**
 * Takes three arguments from the stack and returns a light user data containing a struct Constraint**.
 * The three arguments are in order, min, max, and interval.
 */
int LuaConstraint(lua_State* _State);
/**
 * A Lua variant of CreateConstrntVaBnds.
 */
int LuaConstraintBnds(lua_State* _State);
void ConstraintBndToLua(lua_State* _State, struct Constraint** _Constraints);
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

int LuaMonth(lua_State* _State);
int LuaPrintDate(lua_State* _State);
int LuaPrintYears(lua_State* _State);
int LuaHook(lua_State* _State);

int LuaLoadFile(lua_State* _State, const char* _File);
int LuaCallFunc(lua_State* _State, int _Args, int _Results, int _ErrFunc);
void LuaLoadList(lua_State* _State, const char* _File, const char* _Global, void*(*_Callback)(lua_State*, int), void(*_Insert)(struct LinkedList*, void*), struct LinkedList* _Return);
//TODO: Rename Add* to Lua*.
int AddInteger(lua_State* _State, int _Index, int* _Number);
int AddString(lua_State* _State, int _Index, const char** _String);
int AddNumber(lua_State* _State, int _Index, double* _Number);
int LuaLudata(lua_State* _State, int _Index, void** _Data);
int LuaFunction(lua_State* _State, int _Index, lua_CFunction* _Function);

/**
 * Functions that create an object type.
 */

int LuaCreateGood(lua_State* _State);
int LuaCreateBuilding(lua_State* _State);
int LuaCreateAnimal(lua_State* _State);

/**
 * _Table must be big enough to contain at least lua_gettop(_State) elements.
 */
void LuaStackToTable(lua_State* _State, int* _Table);
/**
 * Copies every element of the table at _Index into the table at the top of the stack.
 * The table at _Index will be removed.
 */
void LuaCopyTable(lua_State* _State, int _Index);
/**
 * Returns a pointer to C data that is contained inside a Lua class.
 */
void* LuaToClass(lua_State* _State, int _Index);
/**
 * Checks if the element at _Index in the Lua stack is of type _Class.
 * Returns a pointer to it's C data or NULL if the element at _Index is not
 * of type _Class.
 */
void* LuaTestClass(lua_State* _State, int _Index, const char* _Class);
void* LuaCheckClass(lua_State* _State, int _Index, const char* _Class);
/*
 * These functions are for retrieving data from simple tables.
 */

int LuaIntPair(lua_State* _State, int _Index, int* _One, int* _Two);
int LuaKeyValue(lua_State* _State, int _Index, const char** _Value, int* _Pair);

#endif
