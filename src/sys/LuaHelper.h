/*
 * File: LuaHelper.h
 * Author: David Brotz
 */

#ifndef __LUAHELPER_H
#define __LUAHELPER_H

#include <lua/lua.h>

struct LinkedList;

extern lua_State* g_LuaState;

struct Constraint;
typedef struct lua_State lua_State;

int LuaConstraint(lua_State* _State);
int LuaConstraintBnds(lua_State* _State);

int LuaLoadFile(lua_State* _State, const char* _File);
void LuaLoadToList(lua_State* _State, const char* _File, const char* _Global, void*(*_Callback)(lua_State*, int), struct LinkedList* _Return);
//TODO: Rename Add* to Lua*.
int AddInteger(lua_State* _State, int _Index, int* _Number);
int AddString(lua_State* _State, int _Index, const char** _String);
int AddNumber(lua_State* _State, int _Index, double* _Number);
int LuaLudata(lua_State* _State, int _Index, void** _Data);

void LuaStackToTable(lua_State* _State, int* _Table);
/*
 * These functions are for retrieving data from simple tables.
 */
int LuaIntPair(lua_State* _State, int _Index, int* _One, int* _Two);
int LuaKeyValue(lua_State* _State, int _Index, const char** _Value, int* _Pair);

#endif
