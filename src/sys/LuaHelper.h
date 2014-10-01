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

#define LuaAbsPos(_State, _Index) ((_Index > 0) ? (_Index) : (lua_gettop(_State) + (_Index) + 1))

#define ConstraintToLua(_State, _Constraint)			\
	lua_createtable((_State), 0, 2);					\
	lua_pushstring((_State), "Min");					\
	lua_pushinteger((_State), (_Constraint)->Min);		\
	lua_rawset((_State), -3);							\
	lua_pushstring((_State), "Max");					\
	lua_pushinteger((_State), (_Constraint)->Max);		\
	lua_rawset((_State), -3)

#define LUA_TYPERROR(_State, _Arg, _Type, _Func) "bad argument %i to '%s' (%s expected got %s)", (_Arg), (_Func),  (_Type), lua_typename((_State), (_Arg))
#define LUA_BADARG(_Arg, _Extra) Log(ELOG_WARNING, "Bad argument #%i (%s)", (_Arg), (_Extra))
#define LUA_BADARG_V(_Arg, _Extra, ...) Log(ELOG_WARNING, "Bad argument #%i (%s)", (_Arg), (_Extra), __VA_ARGS__)
#define LuaLoadCFuncs(_State)														\
	lua_register((_State), "CreateConstraint", LuaConstraint);						\
	lua_register((_State), "CreateConstraintBounds", LuaConstraintBnds);			\
	lua_register((_State), "Crop", LuaCrop);										\
	lua_register((_State), "Good", LuaGoodBase);									\
	lua_register((_State), "Food", LuaFoodBase);									\
	lua_register((_State), "Animal", LuaPopulation);								\
	lua_register((_State), "Person", LuaPerson);									\
	lua_register((_State), "ToMonth", LuaMonth);									\
	lua_register((_State), "Hook", LuaHook);										\
	lua_register((_State), "CreateGood", LuaCreateGood);							\
	lua_register((_State), "CreateBuilding", LuaCreateBuilding);					\
	lua_register((_State), "CreateAnimal", LuaCreateAnimal)

extern lua_State* g_LuaState;

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
/*!
 * Creates a table containing information about a Person.
 * Requires one parameter that is a light user data that contains a pointer to a Person.
 */
int LuaPerson(lua_State* _State);

int LuaMonth(lua_State* _State);
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

/*
 * Functions that create an object type.
 */

int LuaCreateGood(lua_State* _State);
int LuaCreateBuilding(lua_State* _State);
int LuaCreateAnimal(lua_State* _State);

void LuaStackToTable(lua_State* _State, int* _Table);
void LuaCopyTable(lua_State* _State, int _Index);
void* LuaToClass(lua_State* _State, int _Index);
void* LuaTestClass(lua_State* _State, int _Index, const char* _Class);
void* LuaCheckClass(lua_State* _State, int _Index, const char* _Class);
/*
 * These functions are for retrieving data from simple tables.
 */

int LuaIntPair(lua_State* _State, int _Index, int* _One, int* _Two);
int LuaKeyValue(lua_State* _State, int _Index, const char** _Value, int* _Pair);

#endif
