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

struct Constraint* LuaConstraint(lua_State* _State, int _Index);
int LuaConstraintBnds(lua_State* _State, int _Index);

//TODO: Rename LoadLua* to LuaLoad*.
int LoadLuaFile(lua_State* _State, const char* _File);
void LoadLuaToList(lua_State* _State, const char* _File, const char* _Global, void*(*_Callback)(lua_State*, int), struct LinkedList* _Return);
//TODO: Rename Add* to Lua*.
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
