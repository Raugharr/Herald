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
#include "Policy.h"

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
		{"SetOnClick", LuaWorldSetOnClick},
		{"GetBigGuy", LuaWorldGetBigGuy},
		{"GetPlot", LuaWorldGetPlot},
		{"Policies", LuaWorldPolicies},
		{"LoadPolicy", LuaPolicyLoad},
		{NULL, NULL}
};

void LuaWorldInit(lua_State* State) {
	luaL_newlib(State, g_LuaWorldFuncs);
	lua_pushstring(State, "Castes"); 
	lua_createtable(State, CASTE_SIZE, 0);
	for(int i = 0; i < CASTE_SIZE; ++i) {
		lua_pushstring(State, g_CasteNames[i]);
		lua_rawseti(State, -2, i + 1);
	}
	lua_rawset(State, -3);

	lua_pushstring(State, "CasteNum");
	lua_pushinteger(State, CASTE_SIZE);
	lua_rawset(State, -3);
	lua_setglobal(State, "World");
}

int LuaWorldGetPlayer(lua_State* _State) {
	LuaCtor(_State, g_GameWorld.Player, LOBJ_BIGGUY);
	return 1;
}

int LuaWorldGetSettlement(lua_State* _State) {
	LuaCtor(_State, FamilyGetSettlement(g_GameWorld.Player->Person->Family)->Government,LOBJ_GOVERNMENT); 
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

int LuaWorldSetOnClick(lua_State* _State) {
	luaL_checktype(_State, 1, LUA_TNUMBER);
	luaL_checktype(_State, 2, LUA_TLIGHTUSERDATA);

	int _ClickState = lua_tointeger(_State, 1);
	//FIXME: Create a Lua binding for Objects.
	struct Object* _Obj = lua_touserdata(_State, 2);

	SetClickState(_Obj, _ClickState);
	return 0;
}

int LuaWorldGetBigGuy(lua_State* _State) {
	struct Person* _Person = LuaCheckClass(_State, 1, LOBJ_PERSON);
	struct BigGuy* _Guy = NULL;

	if((_Guy = RBSearch(&g_GameWorld.BigGuys, _Person)) == NULL) {
		lua_pushnil(_State);
		return 1;
	}
	LuaCtor(_State, _Guy, LOBJ_BIGGUY);
	return 1;
}

int LuaWorldGetPlot(lua_State* _State) {
	struct BigGuy* _Guy = LuaCheckClass(_State, 1, LOBJ_BIGGUY);
	struct Plot* _Plot = RBSearch(&g_GameWorld.PlotList, _Guy);

	if(_Plot == NULL) {
		lua_pushnil(_State);
		return 1;
	}
	LuaCtor(_State, _Plot, LOBJ_PLOT);
	return 1;					
}

//FIXME: Should use a static Lua table constructed at program beginning instead.
int LuaWorldPolicies(lua_State* _State) {
	lua_createtable(_State, g_GameWorld.Policies.Size, 0);
	for(int i = 0; i < g_GameWorld.Policies.Size; ++i) {
		LuaCtor(_State, g_GameWorld.Policies.Table[i], LOBJ_POLICY);
		lua_rawseti(_State, -2, i + 1);
	}
	return 1;
}
