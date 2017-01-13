/*
 * File: LuaWorld.h
 * Author: David Brotz
 */
#ifndef __LUAWORLD_H
#define __LUAWORLD_H

typedef struct lua_State lua_State;

void LuaWorldInit(lua_State* State);
int LuaWorldGetPlayer(lua_State* State);
int LuaWorldGetSettlement(lua_State* State);;
int LuaWorldGetDate(lua_State* State);
int LuaWorldPause(lua_State* State);
int LuaWorldIsPaused(lua_State* State);
int LuaWorldRenderMap(lua_State* State);
int LuaWorldIsRendering(lua_State* State);
int LuaWorldSetOnClick(lua_State* State);
int LuaWorldGetBigGuy(lua_State* State);
int LuaWorldGetPlot(lua_State* State);
int LuaWorldPolicies(lua_State* State);

#endif
