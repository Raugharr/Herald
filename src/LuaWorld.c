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
#include "Warband.h"
#include "Grammar.h"
#include "Profession.h"

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
	{"RenderMode", LuaRenderMode},
	{"Profession", LuaProfession},
	{NULL, NULL}
};

static const luaL_Reg g_LuaWorldGrammerFuncs[] = {
	{"Load", LuaLoadGramSet},
	{NULL, NULL}
};

const struct LuaEnum g_LuaWorldActionEnum[] = {
	{"Default", WORLDACT_DEFAULT},
	{"RaiseArmy", WORLDACT_RAISEARMY},
	{NULL, 0}
};

const struct LuaEnum g_LuaArmyGoalEnum[] = {
	{"None", ARMYGOAL_NONE},
	{"Slaves", ARMYGOAL_SLAVES},
	{"Pillage", ARMYGOAL_PILLAAGE},
	{"Slaughter", ARMYGOAL_SLAUGHTER},
	{NULL, 0}
};

const struct LuaEnumReg g_LuaMapRenModeEnum[] = {
	{"State", MAPRENMODE_STATE},
	{"Diplomatic", MAPRENMODE_DIPLOMATIC},
	{NULL, NULL}
};

const struct LuaEnumReg g_LuaWorldEnums[] = {
	{"World", "Action",  g_LuaWorldActionEnum},
	{"Army", "Goal", g_LuaArmyGoalEnum},
	{"World", "RenderMode", g_LuaMapRenModeEnum},
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
	RegisterLuaEnums(State, g_LuaWorldEnums);

	luaL_newlib(State, g_LuaWorldGrammerFuncs);
	lua_setglobal(State, "Grammar");
}

int LuaWorldGetPlayer(lua_State* State) {
	LuaCtor(State, g_GameWorld.Player, LOBJ_BIGGUY);
	return 1;
}

int LuaWorldGetSettlement(lua_State* State) {
	LuaCtor(State, FamilyGetSettlement(g_GameWorld.Player->Person->Family)->Government, LOBJ_GOVERNMENT); 
	return 1;
}

int LuaWorldGetDate(lua_State* State) {
	LuaCtor(State, &g_GameWorld.Date, LOBJ_DATE);
	//lua_pushinteger(State, g_GameWorld.Date);
	return 1;
}

int LuaWorldPause(lua_State* State) {
	luaL_checktype(State, 1, LUA_TBOOLEAN);

	g_GameWorld.IsPaused = lua_toboolean(State, 1);
	return 0;
}

int LuaWorldIsPaused(lua_State* State) {
	lua_pushboolean(State, g_GameWorld.IsPaused);
	return 1;
}

int LuaWorldRenderMap(lua_State* State) {
	luaL_checktype(State, 1, LUA_TBOOLEAN);

	g_GameWorld.MapRenderer->IsRendering = lua_toboolean(State, 1);
	return 0;
}

int LuaWorldIsRendering(lua_State* State) {
	lua_pushboolean(State, g_GameWorld.MapRenderer->IsRendering);
	return 1;
}

int LuaWorldSetOnClick(lua_State* State) {
	luaL_checktype(State, 1, LUA_TNUMBER);
	luaL_checktype(State, 2, LUA_TLIGHTUSERDATA);

	int ClickState = lua_tointeger(State, 1);
	//FIXME: Create a Lua binding for Objects.
	struct Object* Obj = lua_touserdata(State, 2);

	SetClickState(Obj, ClickState, luaL_optint(State, 3, 0));
	return 0;
}

int LuaWorldGetBigGuy(lua_State* State) {
	struct Person* Person = LuaCheckClass(State, 1, LOBJ_PERSON);
	struct BigGuy* Guy = NULL;

	if((Guy = RBSearch(&g_GameWorld.BigGuys, Person)) == NULL) {
		lua_pushnil(State);
		return 1;
	}
	LuaCtor(State, Guy, LOBJ_BIGGUY);
	return 1;
}

int LuaWorldGetPlot(lua_State* State) {
	struct BigGuy* Guy = LuaCheckClass(State, 1, LOBJ_BIGGUY);
	struct Plot* Plot = RBSearch(&g_GameWorld.PlotList, Guy);

	if(Plot == NULL) {
		lua_pushnil(State);
		return 1;
	}
	LuaCtor(State, Plot, LOBJ_PLOT);
	return 1;					
}

//FIXME: Should use a static Lua table constructed at program beginning instead.
int LuaWorldPolicies(lua_State* State) {
	lua_createtable(State, g_GameWorld.Policies.Size, 0);
	for(int i = 0; i < g_GameWorld.Policies.Size; ++i) {
		LuaCtor(State, g_GameWorld.Policies.Table[i], LOBJ_POLICY);
		lua_rawseti(State, -2, i + 1);
	}
	return 1;
}

int LuaRenderMode(lua_State* State) {
	int Mode = luaL_checkinteger(State, 1);

	if(Mode < 0 || Mode >= MAPRENMODE_SIZE) return 0;
	g_GameWorld.MapRenderer->RenderMode = Mode;
	return 0;
}

int LuaProfession(lua_State* State) {
	int Id = luaL_checkinteger(State, 1);

//	lua_pushstring(State, ((struct Profession*) HashSearch(&g_Professions, "Farmer"))->Name);
	lua_pushstring(State, ProfessionName(Id));
	return 1;
}
