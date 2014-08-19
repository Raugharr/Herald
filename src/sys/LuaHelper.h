/*
 * File: LuaHelper.h
 * Author: David Brotz
 */

#ifndef __LUAHELPER_H
#define __LUAHELPER_H

#include <lua/lua.h>

#define ConstraintToLua(_State, _Constraint)			\
	lua_createtable((_State), 0, 2);					\
	lua_pushstring((_State), "Min");					\
	lua_pushinteger((_State), (_Constraint)->Min);		\
	lua_rawset((_State), -3);							\
	lua_pushstring((_State), "Max");					\
	lua_pushinteger((_State), (_Constraint)->Max);		\
	lua_rawset((_State), -3)

#define LUA_BADARG(_Arg, _Extra) Log(ELOG_WARNING, "Bad argument #%i (%s)", (_Arg), (_Extra))
#define LuaLoadCFuncs(_State)														\
	lua_register((_State), "CreateConstraint", LuaConstraint);						\
	lua_register((_State), "CreateConstraintBounds", LuaConstraintBnds);			\
	lua_register((_State), "Crop", LuaCrop);										\
	lua_register((_State), "Good", LuaGoodBase);									\
	lua_register((_State), "Food", LuaFoodBase);									\
	lua_register((_State), "Animal", LuaPopulation);								\
	lua_register((_State), "ToMonth", LuaMonth)

struct LinkedList;
struct Constraint;
typedef struct lua_State lua_State;

extern lua_State* g_LuaState;

int LuaConstraint(lua_State* _State);
int LuaConstraintBnds(lua_State* _State);
void ConstraintBndToLua(lua_State* _State, struct Constraint** _Constraints);
/*!
 * Creates a table containing information about a crop.
 * Requires one parameter that is a string equaling the name of a crop in g_Crops.
 */
int LuaCrop(lua_State* _State);
int LuaGoodBase(lua_State* _State);
int LuaFoodBase(lua_State* _State);
/*!
 * Creates a table containing information about a Population.
 * Requires one parameter that is a string equaling the name of a crop in g_Populations.
 */
int LuaPopulation(lua_State* _State);

int LuaMonth(lua_State* _State);

int LuaLoadFile(lua_State* _State, const char* _File);
int LuaCallFunc(lua_State* _State, int _Args, int _Results, int _ErrFunc);
void LuaLoadList(lua_State* _State, const char* _File, const char* _Global, void*(*_Callback)(lua_State*, int), void(*_Insert)(struct LinkedList*, void*), struct LinkedList* _Return);
//TODO: Rename Add* to Lua*.
int AddInteger(lua_State* _State, int _Index, int* _Number);
int AddString(lua_State* _State, int _Index, const char** _String);
int AddNumber(lua_State* _State, int _Index, double* _Number);
int LuaLudata(lua_State* _State, int _Index, void** _Data);
int LuaFunction(lua_State* _State, int _Index, lua_CFunction* _Function);

void LuaStackToTable(lua_State* _State, int* _Table);
/*
 * These functions are for retrieving data from simple tables.
 */
int LuaIntPair(lua_State* _State, int _Index, int* _One, int* _Two);
int LuaKeyValue(lua_State* _State, int _Index, const char** _Value, int* _Pair);

#endif
