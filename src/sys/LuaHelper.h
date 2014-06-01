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

#endif
