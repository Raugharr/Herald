/*
 * File: LuaWorld.h
 * Author: David Brotz
 */
#ifndef __LUAWORLD_H
#define __LUAWORLD_H

typedef struct lua_State lua_State;

void LuaWorldInit();
int LuaWorldGetPlayer(lua_State* _State);
int LuaWorldGetSettlement(lua_State* _State);;
int LuaWorldGetDate(lua_State* _State);
int LuaWorldPause(lua_State* _State);
int LuaWorldIsPaused(lua_State* _State);
int LuaWorldRenderMap(lua_State* _State);
int LuaWorldIsRendering(lua_State* _State);

#endif
