/*
 * File: LuaHelper.h
 * Author: David Brotz
 */

#ifndef __LUAHELPER_H
#define __LUAHELPER_H

#include <lua/lua.h>

int AddInteger(lua_State* _State, int _Index, int* _Number);
int AddString(lua_State* _State, int _Index, const char** _String);
int AddNumber(lua_State* _State, int _Index, double* _Number);

void LuaStackToTable(lua_State* _State, int* _Table);
/*
 * These functions are for retrieving data from simple tables.
 */
int LuaIntPair(lua_State* _State, int _Index, int* _One, int* _Two);
int LuaKeyValue(lua_State* _State, int _Index, const char** _Value, int* _Pair);

#endif
