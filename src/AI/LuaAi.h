/*
 * File: LuaAi.h
 * Author: David Brotz
 */
#ifndef __LUAAI_H
#define __LUAAI_H

#include "../sys/LuaCore.h"

typedef struct lua_State lua_State;
typedef struct luaL_Reg luaL_Reg;
typedef int (*lua_CFunction) (lua_State *L);

extern const struct LuaObjectReg g_LuaAIObjects[];

int LuaAgentGetAction(lua_State* _State);

#endif
