/*
 * File: LuaWorld.c
 * Author: David Brotz
 */

#include "LuaWorld.h"

#include "World.h"
#include "Person.h"
#include "Family.h"
#include "Location.h"
#include "Government.h"
#include "BigGuy.h"
#include "Herald.h"

#include "sys/LuaCore.h"
#include "sys/Log.h"

#include <lua/lua.h>
#include <lua/lauxlib.h>

static const luaL_Reg g_LuaWorldFuncs[] = {
		{"GetPlayer", LuaWorldGetPlayer},
		{"GetSettlement", LuaWorldGetSettlement},
		{"GetDate", LuaWorldGetDate},
		{"Pause", LuaWorldPause},
		{"IsPaused", LuaWorldIsPaused},
		{"Render", LuaWorldRenderMap},
		{"IsRendering", LuaWorldIsRendering},
		{NULL, NULL}
};

void LuaWorldInit() {
	luaL_newlib(g_LuaState, g_LuaWorldFuncs);
	lua_setglobal(g_LuaState, "World");
}

int LuaWorldGetPlayer(lua_State* _State) {
	LuaCtor(_State, "BigGuy", g_GameWorld.Player);
	return 1;
}

int LuaWorldGetSettlement(lua_State* _State) {
	LuaCtor(_State, "Government", g_GameWorld.Player->Person->Family->HomeLoc->Government);
	return 1;
}

int LuaWorldGetDate(lua_State* _State) {
	lua_pushinteger(_State, g_GameWorld.Date);
	return 1;
}

int LuaWorldPause(lua_State* _State) {
	luaL_checktype(_State, 1, LUA_TBOOLEAN);

	g_GameWorld.IsPaused = lua_toboolean(_State, 1);
	return 0;
}

int LuaWorldIsPaused(lua_State* _State) {
	lua_pushboolean(_State, g_GameWorld.IsPaused);
	return 1;
}

int LuaWorldRenderMap(lua_State* _State) {
	luaL_checktype(_State, 1, LUA_TBOOLEAN);

	g_GameWorld.MapRenderer->IsRendering = lua_toboolean(_State, 1);
	return 0;
}

int LuaWorldIsRendering(lua_State* _State) {
	lua_pushboolean(_State, g_GameWorld.MapRenderer->IsRendering);
	return 1;
}
